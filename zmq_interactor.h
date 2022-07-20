#pragma once

#include "mqtt_interactor.h"

#include <base/SignalSlot.h>
#include <ihspb/pub-sub.pb.h>

#include <functional>
#include <memory>

namespace afl {
class SubscriberAbstract;
}

using VehicleMessageCallback = std::function<void(const v2x::ParticipantInfos&)>;
using EventMessageCallback = std::function<void(const v2x::EventInfos&)>;

class ZmqInteractor {
public:
    ZmqInteractor(void);

    void startZMQ();

    void set_vehicles_callback(VehicleMessageCallback cb);
    void set_evnets_callback(EventMessageCallback cb);

private:
    void onMessageHandler(const char* topic, size_t topicSize, const char* content, size_t contentSize);

private:
    std::shared_ptr<afl::SubscriberAbstract> subscriberPtr_ { nullptr };

    std::vector<afl::Slot> slots_;
    afl::Signal<void(const v2x::ParticipantInfos&)> vehicle_signal_;
    afl::Signal<void(const v2x::EventInfos&)> event_signal_;
};