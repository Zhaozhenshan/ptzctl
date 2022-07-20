#pragma once

#include "base/Timestamp.h"
#include "control_context.h"
#include "mqtt_actor.h"

#include <base/SignalSlot.h>
#include <glog/logging.h>
#include <net/EventLoopThread.h>
#include <nlohmann/json.hpp>

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

struct ControlCommand {
    std::string device_serial;
    int focus_type;
    std::string focus;
    uint64_t timestamp;
};

struct BallCameraStatus {
    std::string device_serial;
    int focus_type;

    std::string focus;
    int tracking;

    double p;
    double t;
    double z;
};

using MqttCommandCallback = std::function<void(const ControlCommand&)>;

class MqttInteractor {
public:
    MqttInteractor(std::string addr);

    void set_cmd_callback(MqttCommandCallback cb);

    void send_status(const BallCameraStatus& status);
    void send_event(const std::string& ev);

private:
    std::string mqtt_addr;
    std::string mqtt_pub_topic = "/ptz/status/";
    std::string mqtt_sub_topic = "/ptz/control/all";
    std::string event_pub_topic;
    // 7-10-5 补丢掉的代码 分组订阅事件
    std::string event_sub_topic_group;

    v2x::MqttActor& mqttActor { v2x::MqttActor::getInstance() };

    afl::Signal<void(const ControlCommand&)> cmd_signal_;
    std::vector<afl::Slot> slots_;
    afl::net::EventLoopThread eventLoopThread;
};
