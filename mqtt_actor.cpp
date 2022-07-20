#include "mqtt_actor.h"
#include "libsn/sn.h"
#include <common.h>
#include <glog/logging.h>
#include <iostream>
#include <net/EventLoopThread.h>
#include <random>
#include <string>

using namespace std::placeholders;

namespace v2x {
bool MqttActor::init(const std::string& addr, const std::string& clientId, const std::string& username, const std::string& password)
{
    char snbuf[256];
    int snlen = read_mec_sn(snbuf);
    auto sn = snlen > 0 ? std::string(snbuf, snlen) : "Unknown";
    std::random_device rd;
    sn.append(std::to_string(rd()));

    mqttAsyncClientSptr_.reset(new mqtt::async_client(addr, sn));

    mqttAsyncClientSptr_->set_connected_handler(std::bind(&MqttActor::onMqttConnected, this, _1));
    mqttAsyncClientSptr_->set_connection_lost_handler(std::bind(&MqttActor::onMqttDisconnected, this, _1));

    auto connOpts = mqtt::connect_options_builder().clean_session().automatic_reconnect().finalize();
    connOpts.set_keep_alive_interval(10);
    if (!username.empty()) {
        connOpts.set_user_name(username);
    }
    if (!password.empty()) {
        connOpts.set_password(password);
    }

    mqttAsyncClientSptr_->start_consuming();
    mqttReaderEventLoopThread_.reset(new afl::net::EventLoopThread);
    mqttReaderEventLoopThread_->startLoop().runInLoop(std::bind(&MqttActor::run, this));

    mqttAsyncClientSptr_->connect(connOpts);
    return true;
}

void MqttActor::onMqttConnected(const std::string&)
{
    LOG(INFO) << "ptz onConnected";
    for (auto& i : topicSet_) {
        mqttAsyncClientSptr_->subscribe(i, 0);
    }
}

void MqttActor::onMqttDisconnected(const std::string& cause)
{
    if (cause.empty()) {
        return;
    }
    LOG(WARNING) << cause;
}

void MqttActor::subscribe(const std::string& topic)
{
    if (mqttAsyncClientSptr_ && mqttAsyncClientSptr_->is_connected()) {
        mqttAsyncClientSptr_->subscribe(topic, 0);
    }
    if (topicSet_.find(topic) == topicSet_.end()) {
        topicSet_.emplace(topic);
    }
}

void MqttActor::sendData(const std::string& topic, const std::string& content)
{
    if (mqttAsyncClientSptr_->is_connected()) {
        mqttAsyncClientSptr_->publish(topic.c_str(), content.data(), content.size(), 0, false);
    }
}

void MqttActor::registerHandler(CallBack CallBack)
{
    callBack_ = std::move(CallBack);
}

void MqttActor::run(void)
{
    auto timeout = std::chrono::milliseconds(100);

    while (true) {
        auto messagePtr = mqttAsyncClientSptr_->try_consume_message_for(timeout);
        if (!messagePtr) {
            continue;
        }
        LOG(INFO) << "get message " << messagePtr->get_payload();

        if (callBack_) {
            callBack_(std::make_shared<std::string>(messagePtr->get_topic()), std::make_shared<std::string>(std::move(messagePtr->get_payload())));
        }
    }
}

}