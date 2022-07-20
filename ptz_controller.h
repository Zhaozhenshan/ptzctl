#ifndef PTZ_CONTROLLER_H
#define PTZ_CONTROLLER_H

#include "ball_camera.h"
#include "pid_method.h"

#include <utils/singleton.h>

#include <memory>

class PtzController {
public:
    PtzController(const BallCameraConfig& ballCameraConfig, const PidConfig& pidConfig);

    void on_vehicle_detected_adjust_zoom(double x, double y, double z, double vx, double vy);
    void on_vehicle_detected(double x, double y, double z, double vx, double vy);

    const BallCameraConfig& get_config();

    void reset_camera(const uint64_t& preset);

    void reset_camera_immediately(const uint64_t& preset);

    void reset_camera(double p, double t, double z);

    bool snapshot(std::string& pic);

    void get_current_ptz(double& P, double& T, double& Z);
    void get_needed_corrected_ptz(double x, double y, double z, double& P, double& T, double& Z, double dist);

    void calibrate(double x, double y, double& dp, double& dt);

private:
    void adjust_by_bias(double& degree_by_zero);
    void get_needed_ptz(double x, double y, double z, double& P, double& T, double& Z);

private:
    std::shared_ptr<BallCamera> camera_;
    PidMethod pid_p_;
    PidMethod pid_t_;
    PidMethod pid_z_;
    BallCameraConfig config_;
};

#endif // PTZ_CONTROLLER_H