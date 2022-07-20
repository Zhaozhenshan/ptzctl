#include "mqtt_interactor.h"
#include "libsn/sn.h"
#include "read_config.h"
MqttInteractor::MqttInteractor(std::string addr)
{
    mqtt_addr = std::move(addr);

    // 7-10-6 补丢掉的代码 增加一个topic
    event_sub_topic_group = "/ptz/control/all/" + ReadConfig::getInstance().config().group_id; // 1

    mqttActor.subscribe(mqtt_sub_topic);
    mqttActor.subscribe(event_sub_topic_group); // 2

    auto func = [=](std::shared_ptr<std::string> topic, std::shared_ptr<std::string> content) {
        LOG(INFO) << "recv " << *content;

        nlohmann::json recv_data;
        ControlCommand cmd;

        try {
            recv_data = nlohmann::json::parse(*content);
            cmd.focus_type = recv_data["focus_type"];
            cmd.focus = recv_data["focus"];
            cmd.timestamp = recv_data["ts"];
        } catch (const std::exception&) {
            LOG(ERROR) << "json parse error " << *content;
            return;
        }

        cmd_signal_.call(cmd);
    };

    mqttActor.registerHandler(func);
    mqttActor.init(mqtt_addr, "test", "admin", "public");

    char snbuf[256];
    int snlen = read_mec_sn(snbuf);
    auto sn = snlen > 0 ? std::string(snbuf, snlen) : "Unknown";
    event_pub_topic = "/mec/algoentry/result/event/" + sn;
}

void MqttInteractor::set_cmd_callback(MqttCommandCallback cb)
{
    slots_.push_back(cmd_signal_.connect(std::move(cb)));
}

void MqttInteractor::send_status(const BallCameraStatus& status)
{
    nlohmann::json j {
        { "device_serial", status.device_serial },
        { "focus_type", status.focus_type },
        { "focus", status.focus },
        { "tracking", status.tracking },
        { "p", status.p },
        { "t", status.t },
        { "z", status.z },
        { "ts", afl::Timestamp::now().milliSecondsSinceEpoch() }
    };

    std::string serialized = j.dump();
    mqttActor.sendData(mqtt_pub_topic + status.device_serial, serialized);
    VLOG(5) << " published the status of " << status.device_serial << ":" << serialized;
}

void MqttInteractor::send_event(const std::string& ev)
{
    mqttActor.sendData(event_pub_topic, ev);
    VLOG(5) << " published event：" << ev;
}
