#ifndef COMM_H
#define COMM_H

#include <string>
#include <vector>

class BallCameraBuilder {
public:
    BallCameraBuilder();
    static BallCameraBuilder* camera();

    BallCameraBuilder* setName(std::string name);

    BallCameraBuilder* setDeviceSerial(std::string device_serial);

    BallCameraBuilder* setBrand(std::string brand);

    BallCameraBuilder* setAddr(std::string addr);

    BallCameraBuilder* setX(double x);

    BallCameraBuilder* setY(double y);

    BallCameraBuilder* setZ(double z);

    BallCameraBuilder* setDp(double dp);

    BallCameraBuilder* setDt(double dt);

    BallCameraBuilder* setZoom(double zoom);

    BallCameraBuilder* setBackDp(double dp_back);

    BallCameraBuilder* setBackDt(double dt_back);

    BallCameraBuilder* setBackZoom(double zoom_back);

    BallCameraBuilder* setPreset(uint64_t preset);

    BallCameraBuilder* setCtrn(uint64_t ctrn);

    BallCameraBuilder* setCtrlDist(double dist);

    BallCameraBuilder* setSlope(double slope);

    BallCameraBuilder build();

public:
    std::string name;
    std::string device_serial;

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

    uint64_t ctrn;
    uint64_t preset;
    double slope;
};

struct BallCameraConfig {
    BallCameraConfig(const BallCameraBuilder& builder);

    std::string name;
    std::string device_serial;

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

    uint64_t ctrn;
    uint64_t preset;
    double slope;
};

struct PidConfig {
    double P;
    double I;
    double D;
};

// 7.10-1 补丢掉的代码 2行
struct ConnConfig {
    std::string mqtt_addr;
    std::string camera_username;
    std::string camera_passward;
    std::string mqtt_cloud_addr; // 7.10.1
};

struct GlobalConfig {
    std::vector<BallCameraConfig> cameras;
    PidConfig pid;
    ConnConfig connConfig;
    std::string group_id; // 7.10.2
};
#endif // COMM_H