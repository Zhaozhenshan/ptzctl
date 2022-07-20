#include "pid_method.h"
#include "base/Timestamp.h"

PidMethod::PidMethod(const PidConfig& config)
    : config_(config)
{
}

double PidMethod::calc(double err)
{
    int64_t now = afl::Timestamp::now().microSecondsSinceEpoch();
    double diff_time = (now - last_time_) * 1e-6;

    auto retv = (last_time_ > 0) ? (config_.P * err + config_.I * (err_acc_ += err) + config_.D * (err - last_err_) / diff_time) : 0;

    last_time_ = now;
    last_err_ = err;
    return retv;
}

void PidMethod::reset()
{
    err_acc_ = 0;
    last_time_ = -1;
}
