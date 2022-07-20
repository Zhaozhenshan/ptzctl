#ifndef BALL_CAMERA_H
#define BALL_CAMERA_H

#include <base/Noncopyable.h>
#include <memory>
#include <string>
class BallCamera : public afl::Noncopyable {
public:
    BallCamera(std::string addr, uint64_t id);

    virtual bool get_ptz(double& p, double& t, double& z) = 0;
    virtual bool set_ptz(double p, double t, double z) = 0;
    virtual bool go_to_preset(const uint64_t& preset_id) = 0;
    virtual bool snapshot(std::string& pic) = 0;

    std::string get_addr();

protected:
    std::string addr_;
    uint64_t id_;
};

std::shared_ptr<BallCamera> get_ball_camera(const std::string& brane, const std::string& addr, const uint64_t& id);
#endif // BALL_CAMERA_H