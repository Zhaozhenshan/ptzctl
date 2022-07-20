#include "zmq_interactor.h"

#include <common/appprotocol.h>
#include <iomanip>
#include <iostream>
#include <mmw/mmwfactory.h>
#include <mutex>

using namespace v2x;
using namespace std::placeholders;
extern std::unordered_map<std::string, std::shared_ptr<ControlContext>> controlContexts;
extern std::unordered_map<std::string, ControlCommand> cmds;
extern std::mutex mttx;

ZmqInteractor::ZmqInteractor()
{
}

void ZmqInteractor::startZMQ()
{
    // subscriberPtr_ = afl::MMWFactory().getSubscriber(afl::MMWSelector::ZMQ, std::set<std::string>{algoResultPublisherAddr, sensorPublisherAddr});
    subscriberPtr_ = afl::MMWFactory().getSubscriber(afl::MMWSelector::ZMQ, std::set<std::string> { algoResultPublisherAddr, sensorPublisherAddr });
    LOG_IF(FATAL, subscriberPtr_ == nullptr) << "This MQ type is not supported!";
    LOG_IF(FATAL, subscriberPtr_->init() == false) << "MQ init return false!";
    LOG_IF(ERROR, subscriberPtr_->subscribe(algoTargetTopic) == false) << "subscribe " << algoTargetTopic << " return false!";
    LOG_IF(ERROR, subscriberPtr_->subscribe(receivedRadarEventsTopic) == false) << "subscribe " << algoTargetTopic << " return false!";
    LOG_IF(FATAL, subscriberPtr_->readStart(std::bind(&ZmqInteractor::onMessageHandler, this, _1, _2, _3, _4)) == false) << "subscriber readStart return false!";
}

void ZmqInteractor::set_vehicles_callback(VehicleMessageCallback cb)
{
    slots_.push_back(vehicle_signal_.connect(std::move(cb)));
}

void ZmqInteractor::set_evnets_callback(EventMessageCallback cb)
{
    slots_.push_back(event_signal_.connect(std::move(cb)));
}

// notice: This is a multithreaded function. So use the mutex lock on shared variables.
void ZmqInteractor::onMessageHandler(const char* topic, size_t topicSize, const char* content, size_t contentSize)
{
    const auto topic_type = std::string(topic, topicSize);

    v2x::ParticipantInfos participantInfos;
    if (topic_type == algoTargetTopic && participantInfos.ParseFromArray(content, contentSize)) {
        vehicle_signal_.call(participantInfos);
    }

    v2x::EventInfos eventInfos;
    if (topic_type == receivedRadarEventsTopic && eventInfos.ParseFromArray(content, contentSize)) {
        event_signal_.call(eventInfos);
    }
}
