#include "read_config.h"

ReadConfig::ReadConfig()
{
    get_global_config();
}

ReadConfig& ReadConfig::getInstance()
{
    static ReadConfig instance;
    return instance;
}

void ReadConfig::get_global_config()
{
    std::string cfgstr;
    afl::readFileAllDataToString(misc::getWorkrootPath() + "/etc/" + afl::getProcessName() + ".cfg", cfgstr);
    Test test;

    auto ret = JsonHelper::jsonToObject(test, cfgstr);
    LOG_IF(FATAL, ret == false) << "Parameter parse error:" << JsonHelper::getLastErrStr();

    globalConfig_.pid.P = test.pidConfig.P;
    globalConfig_.pid.I = test.pidConfig.I;
    globalConfig_.pid.D = test.pidConfig.D;

    globalConfig_.connConfig.mqtt_addr = test.connConfig.mqtt_addr;
    globalConfig_.connConfig.camera_username = test.connConfig.camera_username;
    globalConfig_.connConfig.camera_passward = test.connConfig.camera_passward;
    // 7.10-3 补丢掉的代码 2行
    globalConfig_.connConfig.mqtt_cloud_addr = test.connConfig.mqtt_cloud_addr; // 1
    globalConfig_.group_id = test.group_id; // 2

    for (auto& value : test.cameras) {
        auto& item = value.second;
        BallCameraConfig ballCameraConfig = BallCameraConfig(
            BallCameraBuilder::camera()
                ->setName(item.name)
                ->setDeviceSerial(item.sn)
                ->setBrand(item.brand)
                ->setAddr(item.addr)
                ->setX(item.x)
                ->setY(item.y)
                ->setZ(item.z)
                ->setDp(item.dp)
                ->setDt(item.dt)
                ->setZoom(item.zoom)
                ->setBackDp(item.dp_back)
                ->setBackDt(item.dt_back)
                ->setBackZoom(item.zoom_back)
                ->setCtrlDist(item.ctrl_dist)
                ->setSlope(item.slope)
                ->setPreset(item.preset)
                ->setCtrn(item.ctrn)
                ->build());

        globalConfig_.cameras.emplace_back(std::move(ballCameraConfig));
    }
}
