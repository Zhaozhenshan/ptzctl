#ifndef PID_METHOD_H
#define PID_METHOD_H

#include "comm.h"
class PidMethod {
public:
    PidMethod() = default;
    explicit PidMethod(const PidConfig& config);

    double calc(double err);

    void reset();

private:
    int64_t last_time_;
    double last_err_;
    double err_acc_;
    PidConfig config_;
};

#endif // PID_METHOD_H