#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "ptz_controller.h"
#include "glog/logging.h"
#include "httplib.h"
#include "yushi_ball_camera.h"

#include <gflags/gflags.h>
#include <iostream>
#include <math.h>
DEFINE_double(zoom, NAN, "");
DEFINE_double(dp, NAN, "");
DEFINE_double(dt, NAN, "");
DEFINE_double(dz, 0, "");

PtzController::PtzController(const BallCameraConfig& ballCameraConfig, const PidConfig& pidConfig)
    : camera_(get_ball_camera(ballCameraConfig.brand,
        ballCameraConfig.addr,
        ballCameraConfig.preset))
    , config_(ballCameraConfig)
{
    PidMethod cfg(pidConfig);
    pid_p_ = cfg;
    pid_t_ = cfg;
    pid_z_ = cfg;
}

void PtzController::on_vehicle_detected_adjust_zoom(double x, double y, double z,
    double vx, double vy)
{
    auto dist = std::hypot(x - config_.x, y - config_.y);
    auto sign = (x - config_.x) * vx + (y - config_.y) * vy;
    dist = std::copysign(dist, sign);
    z += dist * tan(config_.slope / 180 * M_PI);

    double abs_p = 0, abs_t = 0, abs_z = 1;
    camera_->get_ptz(abs_p, abs_t, abs_z);

    double needed_p = abs_p, needed_t = abs_t, needed_z = abs_z;
    get_needed_corrected_ptz(x, y, z, needed_p, needed_t, needed_z, dist);

    camera_->set_ptz(NAN, NAN, needed_z);
}

void PtzController::on_vehicle_detected(double x, double y, double z,
    double vx, double vy)
{
    double err_p = 0.0;
    double err_t = 0.0;
    double err_z = 0.0;

    auto dist = std::hypot(x - config_.x, y - config_.y);
    auto sign = (x - config_.x) * vx + (y - config_.y) * vy;
    dist = std::copysign(dist, sign);
    z += dist * tan(config_.slope / 180 * M_PI);

    double abs_p = 0, abs_t = 0, abs_z = 1;
    camera_->get_ptz(abs_p, abs_t, abs_z);

    double needed_p = abs_p, needed_t = abs_t, needed_z = abs_z;
    get_needed_corrected_ptz(x, y, z, needed_p, needed_t, needed_z, dist);

    err_p = needed_p - abs_p;
    err_t = needed_t - abs_t;
    err_z = needed_z - abs_z;
    // camera_->set_ptz(abs_p + pid_p_.calc(err_p), abs_t + pid_t_.calc(err_t), 1);
    camera_->set_ptz(abs_p + pid_p_.calc(err_p), abs_t + pid_t_.calc(err_t), abs_z + pid_z_.calc(err_z));
}

const BallCameraConfig& PtzController::get_config()
{
    return config_;
}

void PtzController::reset_camera(const uint64_t& preset_id)
{
    camera_->go_to_preset(preset_id);
    usleep(1000 * 1000);
}

void PtzController::reset_camera_immediately(const uint64_t& preset_id)
{
    camera_->go_to_preset(preset_id);
}

void PtzController::reset_camera(double p, double t, double z)
{
    camera_->set_ptz(p, t, z);
    pid_p_.reset();
    pid_t_.reset();
    pid_z_.reset();
}

bool PtzController::snapshot(std::string& pic)
{
    return camera_->snapshot(pic);
}

void PtzController::get_current_ptz(double& P, double& T, double& Z)
{
    camera_->get_ptz(P, T, Z);
}

/**
 * @brief Compute the needed ptz according to the target's UTM coord.
 *
 * @param x the target's utm_x
 * @param y the target's utm_y
 * @param z the target's high
 * @param P
 * @param T
 * @param Z
 */
void PtzController::get_needed_corrected_ptz(double x, double y, double z, double& P, double& T, double& Z, double dist)
{
    auto dp = (dist < 0) ? config_.dp : config_.dp_back;
    auto dt = (dist < 0) ? config_.dt : config_.dt_back;

    get_needed_ptz(x, y, z, P, T, Z);
    P += std::isnan(FLAGS_dp) ? dp : FLAGS_dp;
    adjust_by_bias(P);

    T += std::isnan(FLAGS_dt) ? dt : FLAGS_dt;

    auto zoom = config_.zoom;
    Z = (fabs(dist) / config_.ctrl_dist) * zoom;
    if (Z > zoom) {
        Z = zoom;
    } else if (Z < 1) {
        Z = 1;
    }

    Z += FLAGS_dz;
}

void PtzController ::calibrate(double x, double y, double& dp, double& dt)
{
    double P = 0;
    double T = 0;
    double Z = 0;
    get_needed_ptz(x, y, 0, P, T, Z);

    double CP = 0;
    double CT = 0;
    double CZ = 0;
    get_current_ptz(CP, CT, CZ);

    dp = CP - P;
    dt = CT - T;
}

void PtzController::adjust_by_bias(double& degree2north)
{
    degree2north = fmod(degree2north, 360.00);
}

/**
 * @brief Compute the needed ptz according to the target's UTM coord.
 *
 * @param x the target's utm_x
 * @param y the target's utm_y
 * @param z the target's high
 * @param P
 * @param T
 * @param Z
 */
void PtzController::get_needed_ptz(double x, double y, double z, double& P, double& T, double& Z)
{
    const double pi = 3.141592654; // 2 * acos(0);

    double delta_x = x - config_.x;
    double delta_y = y - config_.y;
    double delta_h = config_.z - z;

    double dis_2 = sqrt(delta_x * delta_x + delta_y * delta_y);
    double dis_3 = sqrt(delta_x * delta_x + delta_y * delta_y + delta_h * delta_h);

    if (dis_2 < 1e-6 || dis_3 < 1e-6)
        return;

    // Use theta to compute the value of P. Theta's range is [-90°, 90°].
    double theta = asin(delta_y / dis_2) * 180 / pi;
    double phi = asin(delta_h / dis_3) * 180 / pi;

    P = delta_x > 0 ? (90 - theta) : 360 - (90 - theta);
    T = phi;
}