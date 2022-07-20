#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "control_context.h"
#include "httplib.h"
#include "ptz_controller.h"
#include "read_config.h"

#include "base/Timestamp.h"
#include "jsonhelper/jsonpbhelper.h"
#include "mqtt_interactor.h"
#include "zmq_interactor.h"

#include "utils/base64.h"
#include <GeographicLib/UTMUPS.hpp>
#include <fstream>

ControlContext::ControlContext(std::shared_ptr<PtzController> ptz, std::shared_ptr<ZmqInteractor> zmq,
    std::shared_ptr<MqttInteractor> mqtt, std::shared_ptr<afl::net::EventLoop> loop)
    : ptz_(ptz)
    , zmq_(zmq)
    , mqtt_(mqtt)
    , loop_(loop)
{
    zmq->set_vehicles_callback([&](const v2x::ParticipantInfos& ptcs) { on_receive_vehicles(ptcs); });
    zmq->set_evnets_callback([&](const v2x::EventInfos& evs) { on_receive_events(evs); });

    if (nullptr == mqtt) {
        return;
    }

    mqtt->set_cmd_callback(std::bind(&ControlContext::on_receive_cmd, this, std::placeholders::_1));

    auto status_func = [&]() {
        BallCameraStatus status;
        status.device_serial = ptz_->get_config().device_serial;
        status.focus_type = focus_type_;
        status.focus = focus_;
        status.tracking = tracking_ ? 1 : 0;
        ptz_->get_current_ptz(status.p, status.t, status.z);
        mqtt_->send_status(status);
    };

    auto reset_func = [&]() {
        auto now = afl::Timestamp::now();
        if ((afl::timeDifference(now, last_ctrl_time_) > 20) && !is_on_preset_) {
            reset_tracking();
            focus_ = "null";
            focus_type_ = 0;
            ctrl_cnt_ = 0;
        }
    };

    loop_->runEvery(1, status_func);
    loop_->runEvery(2, reset_func);
}

void ControlContext::set_focus_method(int type, std::string focus)
{
    tracking_ = false;
    focus_type_ = type;
    focus_ = std::move(focus);

    // ptz_->reset_camera(ptz_->get_config().preset);
    ptz_->reset_camera_immediately(ptz_->get_config().preset);
    see_back_ = false;
    is_on_preset_ = true;
}

void ControlContext::calibrate(double x, double y, double& dp, double& dt)
{
    ptz_->calibrate(x, y, dp, dt);
}

void ControlContext::on_receive_cmd(const ControlCommand& cmd)
{

    auto now = afl::Timestamp::now().milliSecondsSinceEpoch();
    if (now - cmd.timestamp > 10 * 1000) {
        LOG(WARNING) << "Cmd from V2X-cloud expire!";
        return;
    }

    LOG(INFO) << "CMD:" << cmd.focus_type << " " << cmd.focus;
    set_focus_method(cmd.focus_type, cmd.focus);
}

void ControlContext::on_receive_vehicles(const v2x::ParticipantInfos& participantInfos)
{
    VLOG(1) << "Travers targets," << ptz_->get_config().name << " before focus";

    if (0 == focus_type_) {
        return;
    }

    VLOG(1) << "Travers targets," << ptz_->get_config().name << " after focus";

    for (int i = 0; i < participantInfos.participants().size(); ++i) {
        const auto& ptc = participantInfos.participants(i);

        int zone = 50;
        bool north = true;
        double x = 0, y = 0;
        GeographicLib::UTMUPS::Forward(ptc.latitude(), ptc.longitude(), zone, north, x, y);
        const double dist = std::hypot(ptz_->get_config().x - x, ptz_->get_config().y - y);

        // 7.2_新增代码 1: 如果目标距离近，则更新方向
        if (std::hypot(direction_.first, direction_.second) < 1e-4 && dist < 100.0) {
            direction_.first = ptc.speedx();
            direction_.second = ptc.speedy();
        }

        if (!ptc.plate().empty()) {
            VLOG(1) << "Travers targets," << ptz_->get_config().name << " track_id:" << ptc.ptcid()
                    << " timestamp:" << ptc.timestamp() / 1000
                    << " plate:" << ptc.plate() << " dist:" << dist;
        }

        if (!is_matched(ptc)) {
            continue;
        }

        if (0 != (ctrl_cnt_++ % ptz_->get_config().ctrn)) {
            VLOG(2) << ptz_->get_config().name << " skip control! ctrl cnt:" << ctrl_cnt_ << " ctrn:" << ptz_->get_config().ctrn;
            return;
        }

        auto now = afl::Timestamp::now();
        LOG(INFO) << "Vehicle Matched " << ptz_->get_config().name << " track_id:" << ptc.ptcid()
                  << " delta_x:" << x - ptz_->get_config().x << " delta_y:" << y - ptz_->get_config().y
                  << " tdiff:" << now.milliSecondsSinceEpoch() - ptc.timestamp()
                  << " plate: " << ptc.plate() << " dist:" << dist;

        if (dist < ptz_->get_config().ctrl_dist) {
            if (!tracking_) {
                ptz_->reset_camera(100);
                LOG(INFO) << "Move to first Preset";
            }

            tracking_ = true;

            const auto sign = (x - ptz_->get_config().x) * ptc.speedx() + (y - ptz_->get_config().y) * ptc.speedy();
            const bool move_away = (sign > 0);

            LOG(INFO) << "Matched Vehicle is coming? " << bool(sign < 0) << " " << move_away << " " << dist;

            if ((dist < 50) || (move_away && !see_back_)) {
                if (!see_back_) {
                    ptz_->reset_camera(101);
                    ptz_->reset_camera(102);
                    LOG(INFO) << "Move to second & third Presets";
                    see_back_ = true;
                    ctrl_cnt_ = 0;
                }
            } else {
                ptz_->on_vehicle_detected_adjust_zoom(x + 1.5 * ptc.speedx(), y + 1.5 * ptc.speedy(), 0,
                    ptc.speedx(), ptc.speedy());
            }

            is_on_preset_ = false;
            last_ctrl_time_ = now;
        } else {
            reset_tracking();
        }

        break;
    }
}

// 7.2_新增代码2 :根据事件位置返回应转向的预置位编号
int ControlContext::event_pos(const double& lon, const double& lat)
{
    double event_x = 0.0, event_y = 0.0;
    int zone = 50;
    bool north = true;
    GeographicLib::UTMUPS::Forward(lat, lon, zone, north, event_x, event_y);
    const auto sign = (event_x - ptz_->get_config().x) * direction_.first + (event_y - ptz_->get_config().y) * direction_.second;

    return sign < 1e-6 ? 7 : 102;
}

// 7.2_新增代码3 :把base64穿给云端，返回一个url
bool get_event_url(const std::string& image, std::string& output)
{
    // 7.10-4 补丢掉的代码 2行, 用于云端地址的验证
    const std::string cloud_conf = ReadConfig::getInstance().config().connConfig.mqtt_cloud_addr;
    const std::string addr = cloud_conf.size() == 0 ? "172.18.32.32" : cloud_conf;
    const std::string url = "/ihs/monitor/minioFile/upload";
    try {
        httplib::Client cli(addr, 80);

        httplib::MultipartFormData item;
        item.name = "file";
        item.content = image;
        item.content_type = "image/jpeg";
        item.filename = "event_image.jpg";
        httplib::MultipartFormDataItems items;
        items.emplace_back(std::move(item));

        auto receive = cli.Post(url.c_str(), items);

        if (receive && receive->status == 200) {
            LOG(INFO) << "Success : get the image url";
            std::string image_url = nlohmann::json::parse(receive->body)["data"]["access_url"];
            output = image_url;
            return true;
        } else {
            LOG(INFO) << "Failure : get the image url";
            return false;
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failure : get the image url" << e.what();
        return false;
    }
    return false;
}

void ControlContext::on_receive_events(const v2x::EventInfos& eventinfos)
{
    VLOG(5) << "Receive radar events";

    // 7.2_新增代码4 : 球机正在跟踪或者mqtt为空，直接过滤该事件。
    if (tracking_ == true || nullptr == mqtt_) {
        return;
    }

    v2x::EventInfos einfos;
    einfos.CopyFrom(eventinfos);

    if (einfos.ihstrafficeventlist().empty()) {
        VLOG(5) << "Not a jam event";
        return;
    }

    // 7.2_新增代码5: 判断此次收到事件的时间到上次球机拍摄的时间是否小于一分钟，如果是，直接过滤
    if (events_valid_.valid() && afl::timeDifference(afl::Timestamp::now(), events_valid_) < (1 * 60)) {
        VLOG(5) << "Ignore jam event, in 1*60 s";
        return;
    }

    auto now = afl::Timestamp::now();

    // 7.2_新增代码6: 过滤事件，只保留一个事件
    for (auto iter = einfos.ihstrafficeventlist().begin(); iter != einfos.ihstrafficeventlist().end();) {
        int zone = 50;
        bool north = true;
        double x = 0, y = 0;
        GeographicLib::UTMUPS::Forward(iter->latitude(), iter->longitude(), zone, north, x, y);

        const double dist = std::hypot(ptz_->get_config().x - x, ptz_->get_config().y - y);

        //         删掉距离大的
        if (dist > ptz_->get_config().ctrl_dist) {
            iter = einfos.mutable_ihstrafficeventlist()->erase(iter);
            continue;
        }

        if (events_last_time_.find(iter->eventtype()) == events_last_time_.end()) {
            events_last_time_[iter->eventtype()] = afl::Timestamp();
        }
        const auto ct = events_last_time_[iter->eventtype()];

        //         删掉5分钟之内的
        if (ct.valid() && afl::timeDifference(now, ct) < (5 * 60)) {
            iter = einfos.mutable_ihstrafficeventlist()->erase(iter);
            continue;
        }

        iter++;
    }

    //             列表为空直接return，不为空则保留第一个事件，删除其他
    if (einfos.ihstrafficeventlist().empty()) {
        VLOG(5) << "einfos.ihstrafficeventlist is empty!";
        return;
    }
    auto iter = einfos.ihstrafficeventlist().begin();
    iter++;
    while (iter != einfos.ihstrafficeventlist().end()) {
        iter = einfos.mutable_ihstrafficeventlist()->erase(iter);
    }

    // 7.2_新增代码7 :处理事件
    iter = einfos.ihstrafficeventlist().begin();

    //             7.2_转向预置位，抓拍图片并把结果转换为base64
    ptz_->reset_camera(event_pos(iter->longitude(), iter->latitude()));
    std::string pic;
    ptz_->snapshot(pic);
    LOG(INFO) << "Get snapshot size:" << pic.size();

    // std::string image_base64 = afl::base64Encode(pic);

    //             7.2_向云端发送信息，输入base64，获取到url，更新事件中的images
    std::string output = "null";
    if (get_event_url(pic, output)) {
        auto& str = *einfos.mutable_ihstrafficeventlist(0)->add_images();
        str = output;

        LOG(INFO) << "zzs_image: " << output;
        auto json = JsonPbHelper::pb2Json(einfos);

        if (!json["ihsTrafficEventList"].empty() && json["ihsTrafficEventList"].is_array()) {
            if (!json["ihsTrafficEventList"][0]["images"].empty() && json["ihsTrafficEventList"][0]["images"].is_array())
                json["ihsTrafficEventList"][0]["images"][0] = output;
        }

        mqtt_->send_event(json.dump());
        // 7.2_新增代码8 : 更新抓图时间
        events_valid_ = now;
        events_last_time_[iter->eventtype()] = now;
    } else {
        VLOG(4) << "get_event_url failed.";
    }
}

void ControlContext::get_current_ptz(double& p, double& t, double& z)
{
    ptz_->get_current_ptz(p, t, z);
}

void ControlContext::set_ptz_directly(double p, double t, double z)
{
    ptz_->reset_camera(p, t, z);
}

bool ControlContext::is_matched(const v2x::ParticipantInfos_Participants& ptc)
{
    if (1 == focus_type_) {
        return (ptc.plate() == focus_);
    }
    if (2 == focus_type_) {
        return (std::to_string(ptc.ptcid()) == focus_);
    }

    return false;
}

void ControlContext::reset_tracking()
{
    if (tracking_) {
        loop_->runAfter(10, [&]() {
            ptz_->reset_camera(ptz_->get_config().preset);
        });
    }

    see_back_ = false;
    tracking_ = false;
    is_on_preset_ = true;
}