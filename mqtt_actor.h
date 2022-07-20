#pragma once

#include "mqtt/async_client.h"
#include <atomic>
#include <functional>
#include <set>
#include <utils/singleton.h>

namespace afl {
namespace net {
    class EventLoopThread;
}
}

namespace v2x {
class MqttActor : public afl::Singleton<MqttActor> {
    friend class afl::Singleton<MqttActor>;
    using CallBack = std::function<void(std::shared_ptr<std::string> topic, std::shared_ptr<std::string> content)>;

private:
    MqttActor(void) = default;

    void onMqttConnected(const std::string& cause);
    void onMqttDisconnected(const std::string& cause);
    void run(void);

public:
    bool init(const std::string& addr, const std::string& clientId = "", const std::string& username = "", const std::string& password = "");
    void subscribe(const std::string& topic);
    void sendData(const std::string& topic, const std::string& content);
    void registerHandler(CallBack CallBack);

private:
    std::shared_ptr<mqtt::async_client> mqttAsyncClientSptr_ { nullptr };
    std::shared_ptr<afl::net::EventLoopThread> mqttReaderEventLoopThread_ { nullptr };
    std::set<std::string> topicSet_;
    CallBack callBack_ { nullptr };
};

}