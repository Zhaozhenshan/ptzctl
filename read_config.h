#ifndef READ_CONFIG_H
#define READ_CONFIG_H

#include "comm.h"
#include "control_context.h"
#include "glog/logging.h"
#include "ptz_controller.h"
#include <common/appprotocol.h>
#include <jsonhelper/jsonhelper.h>
#include <mmw/mmwfactory.h>
#include <string>
#include <unordered_map>
#include <utility>

using namespace std::placeholders;

typedef struct
{
    typedef struct
    {
        double P;
        double I;
        double D;
        JSONHELPER(
            REGFIELD(P, true),
            REGFIELD(I, true),
            REGFIELD(D, true));

    } PidConfig;

    typedef struct BallCameraConfig {

        std::string name;
        std::string sn;

        std::string brand;
        std::string addr;

        double x;
        double y;
        double z;

        double dp;
        double dt;
        double zoom;
        double dp_back;
        double dt_back;
        double zoom_back;

        double ctrl_dist;
        double slope;
        uint64_t ctrn;
        uint64_t preset;

        JSONHELPER(
            REGFIELD(name, true),
            REGFIELD(sn, true),
            REGFIELD(brand, true),
            REGFIELD(addr, true),
            REGFIELD(x, true),
            REGFIELD(y, true),
            REGFIELD(z, true),
            REGFIELD(dp, true),
            REGFIELD(dt, true),
            REGFIELD(zoom, true),
            REGFIELD(dp_back, false),
            REGFIELD(dt_back, false),
            REGFIELD(zoom_back, false),
            REGFIELD(ctrl_dist, true),
            REGFIELD(slope, false),
            REGFIELD(preset, true),
            REGFIELD(ctrn, true))
    } BallCameraConfig;

    typedef struct ConnConfig {
        std::string mqtt_addr;
        std::string camera_username;
        std::string camera_passward;
        // 7.10-2 补丢掉的代码 4行
        std::string mqtt_cloud_addr; // 1
        JSONHELPER(
            REGFIELD(mqtt_addr, true),
            REGFIELD(camera_username, true),
            REGFIELD(camera_passward, true),
            REGFIELD(mqtt_cloud_addr, true)); // 2
    } ConnConfig;

    PidConfig pidConfig;
    std::unordered_map<std::string, BallCameraConfig> cameras;
    ConnConfig connConfig;
    std::string group_id; // 3

    JSONHELPER(
        REGFIELD(cameras, true, "sn"),
        REGFIELD(pidConfig, false),
        REGFIELD(connConfig, false),
        REGFIELD(group_id, false)); // 4
} Test;

class ReadConfig {
public:
    static ReadConfig& getInstance();

    const GlobalConfig& config()
    {
        return globalConfig_;
    }

private:
    ReadConfig();
    void get_global_config();

private:
    GlobalConfig globalConfig_;
};

#endif // READ_CONFIG_H