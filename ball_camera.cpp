#include "ball_camera.h"
#include "yushi_ball_camera.h"

BallCamera::BallCamera(std::string addr, uint64_t id)
    : addr_(std::move(addr))
    , id_(id)
{
}

std::string BallCamera::get_addr()
{
    return addr_;
}

std::shared_ptr<BallCamera> get_ball_camera(const std::string& brand, const std::string& addr, const uint64_t& id)
{
    if (brand == "YuShi") {
        return std::make_shared<YuShiBallCamera>(addr, id);
    }
    return nullptr;
}