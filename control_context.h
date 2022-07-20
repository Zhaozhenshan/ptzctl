#pragma once

#include "comm.h"
#include "mqtt_interactor.h"
#include "ptz_controller.h"

#include <ihspb/pub-sub.pb.h>
#include <net/EventLoop.h>

#include <cmath>
#include <memory>
#include <string>

class ControlCommand;
class PtzController;
class ZmqInteractor;
class MqttInteractor;

class ControlContext {
public:
    ControlContext(std::shared_ptr<PtzController> ptz, std::shared_ptr<ZmqInteractor> zmq,
        std::shared_ptr<MqttInteractor> mqtt, std::shared_ptr<afl::net::EventLoop> loop);

    void set_focus_method(int type, std::string focus);

    void on_receive_vehicles(const v2x::ParticipantInfos& participantInfos); // from zmq
    void on_receive_events(const v2x::EventInfos& eventinfos); // from zmq

    void get_current_ptz(double& p, double& t, double& z);
    void set_ptz_directly(double p, double t, double z);

    void calibrate(double x, double y, double& dp, double& dt);
    int event_pos(const double& lon, const double& lat);

private:
    void on_receive_cmd(const ControlCommand& cmd); // from mqtt
    bool is_matched(const ::v2x::ParticipantInfos_Participants& ptc);
    void reset_tracking();

private:
    // 6.30 新增路的方向
    std::pair<double, double> direction_ = std::make_pair(0.0, 0.0);

    bool tracking_ = false;
    bool see_back_ = false;
    int focus_type_;
    std::string focus_;

    size_t ctrl_cnt_ = 0;
    afl::Timestamp last_ctrl_time_ = afl::Timestamp::now();
    afl::Timestamp event_report_time_ = last_ctrl_time_;
    std::unordered_map<uint32_t, afl::Timestamp> events_last_time_;
    afl::Timestamp events_valid_ = afl::Timestamp();

    bool is_on_preset_ = true;

    std::shared_ptr<PtzController> ptz_;
    std::shared_ptr<ZmqInteractor> zmq_;
    std::shared_ptr<MqttInteractor> mqtt_;
    std::shared_ptr<afl::net::EventLoop> loop_;
};