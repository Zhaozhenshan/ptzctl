#include "control_context.h"
#include "mqtt_interactor.h"
#include "read_config.h"
#include "zmq_interactor.h"

#include <common/appprotocol.h>
#include <ihspb/pub-sub.pb.h>
#include <mmw/mmwfactory.h>
#include <net/EventLoop.h>
#include <net/EventLoopManager.h>
#include <net/EventLoopThread.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>
#include <map>
#include <mutex>
#include <stdlib.h>

DEFINE_string(mode, "auto", "values : set get or auto");
DEFINE_string(ctrl, "auto", "values : camera's name or device_serial");
DEFINE_string(focus, "auto", "Command of setting cameras");
DEFINE_double(p, 404.0, "the p value");
DEFINE_double(t, 404.0, "the t value");
DEFINE_double(z, 404.0, "the z value");

DEFINE_double(x, 0, "the x value");
DEFINE_double(y, 0, "the y value");

DEFINE_string(addr, "tcp://172.18.32.4:1883", "mqtt addr");

using namespace v2x;

bool check_coord_ptz(const double& x)
{
    double bis = 1e-4;
    if (x >= -bis && x <= 360 + bis) {
        return true;
    }
    return false;
}

void on_command_mode(std::map<std::string, std::shared_ptr<ControlContext>>& ctx)
{
    std::string cmd_mode = FLAGS_mode;
    std::string cmd_ctrl = FLAGS_ctrl; //用球机本地名称即可
    std::string cmd_focus = FLAGS_focus;
    double p = FLAGS_p;
    double t = FLAGS_t;
    double z = FLAGS_z;

    auto iter = ctx.find(cmd_ctrl);
    if (iter == ctx.end()) {
        LOG(ERROR) << "Serial " << cmd_ctrl << " Not exist!";
        exit(-1);
    }

    if (cmd_mode == "set") {
        if (cmd_focus == "auto") { //直接设置PTZ
            if (!check_coord_ptz(p) || !check_coord_ptz(t) || !check_coord_ptz(z)) {
                LOG(ERROR) << "Invalid PTZ!";
                exit(-2);
            }
            iter->second->set_ptz_directly(p, t, z);
        } else { // 跟踪模式
            std::string focus_type_string = cmd_focus.substr(0, 5);
            if ((focus_type_string != "plate" && focus_type_string != "track") || (cmd_focus.substr(5, 3) != "://")) {
                LOG(ERROR) << cmd_focus << " is invalid.";
                exit(-3);
            }
            iter->second->set_focus_method(focus_type_string == "plate" ? 1 : 2, cmd_focus.substr(8));
        }
    }

    if (cmd_mode == "get") {
        double p = 0, t = 0, z = 0;
        iter->second->get_current_ptz(p, t, z);
        LOG(INFO) << p << " " << t << " " << z << std::endl;
    }

    if (cmd_mode == "cali") {
        double dp = 0;
        double dt = 0;
        iter->second->calibrate(FLAGS_x, FLAGS_y, dp, dt);
        std::cout << "delta P:" << dp << " delta T:" << dt << std::endl;
    }
};

int main(int argc, char** argv)
{
    LOG(INFO) << "ptzctl start.";

    misc::enableFileSharing();
    misc::glogInit();

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    //读取配置
    const auto& conf = ReadConfig::getInstance().config();

    bool cmd_mode = (FLAGS_mode == "get" || FLAGS_mode == "set" || FLAGS_mode == "cali");

    //建立 MQTT 与 ZMQ连接
    const std::string mqtt_addr = conf.connConfig.mqtt_addr.size() == 0 ? "tcp://172.18.32.4:1883" : conf.connConfig.mqtt_addr;
    auto mqtt = cmd_mode ? nullptr : std::make_shared<MqttInteractor>(mqtt_addr);
    auto zmq = std::make_shared<ZmqInteractor>();
    zmq->startZMQ();

    afl::net::EventLoopManager evm;

    // 创建球机控制上下文
    std::map<std::string, std::shared_ptr<ControlContext>> cctx;
    for (auto& c : conf.cameras) {
        auto ptz = std::make_shared<PtzController>(c, conf.pid);
        cctx[c.device_serial] = std::make_shared<ControlContext>(ptz, zmq, mqtt, evm.getEventLoop());
    }

    // 命令行模式
    if (cmd_mode) {
        on_command_mode(cctx);
        return 0;
    }
    //后台模式
    misc::instanceRun();
    evm.run();

    return 0;
}
