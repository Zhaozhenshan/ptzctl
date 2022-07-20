#include "comm.h"
#include <iostream>
BallCameraBuilder::BallCameraBuilder() = default;

BallCameraBuilder* BallCameraBuilder::camera() { return new BallCameraBuilder(); }

BallCameraBuilder* BallCameraBuilder::setName(std::string name)
{
    this->name = name;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setDeviceSerial(std::string device_serial)
{
    this->device_serial = device_serial;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setBrand(std::string brand)
{
    this->brand = brand;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setAddr(std::string addr)
{
    this->addr = addr;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setX(double x)
{
    this->x = x;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setY(double y)
{
    this->y = y;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setZ(double z)
{
    this->z = z;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setDp(double dp)
{
    this->dp = dp;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setDt(double dt)
{
    this->dt = dt;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setZoom(double zoom)
{
    this->zoom = zoom;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setBackDp(double dp_back)
{
    this->dp_back = dp_back;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setBackDt(double dt_back)
{
    this->dt_back = dt_back;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setBackZoom(double zoom_back)
{
    this->zoom_back = zoom_back;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setPreset(uint64_t preset)
{
    this->preset = preset;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setCtrn(uint64_t ctrn)
{
    this->ctrn = ctrn;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setCtrlDist(double dist)
{
    this->ctrl_dist = dist;
    return this;
}

BallCameraBuilder* BallCameraBuilder::setSlope(double slope)
{
    this->slope = slope;
    return this;
}

BallCameraBuilder BallCameraBuilder::build() { return *this; }

BallCameraConfig::BallCameraConfig(const BallCameraBuilder& builder)
{
    name = builder.name;
    device_serial = builder.device_serial;

    brand = builder.brand;
    addr = builder.addr;

    x = builder.x;
    y = builder.y;
    z = builder.z;

    dp = builder.dp;
    dt = builder.dt;
    zoom = builder.zoom;

    dp_back = builder.dp_back;
    dt_back = builder.dt_back;
    zoom_back = builder.zoom_back;

    ctrl_dist = builder.ctrl_dist;
    slope = builder.slope;
    preset = builder.preset;
    ctrn = builder.ctrn;
}