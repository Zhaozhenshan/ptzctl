#ifndef YUSHI_BALL_CAMERA_H
#define YUSHI_BALL_CAMERA_H

#include "ball_camera.h"

class YuShiBallCamera : public BallCamera {
public:
    YuShiBallCamera(std::string addr, uint64_t id);

    virtual bool get_ptz(double& p, double& t, double& z) override;
    virtual bool set_ptz(double p, double t, double z) override;
    virtual bool go_to_preset(const uint64_t& preset_id) override;
    virtual bool snapshot(std::string& pic) override;

private:
    std::string user_;
    std::string pswd_;
};

#endif // YUSHI_BALL_CAMERA_H