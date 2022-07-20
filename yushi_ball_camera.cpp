#include "yushi_ball_camera.h"
#include "read_config.h"
#include <nlohmann/json.hpp>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <glog/logging.h>

using namespace httplib;

YuShiBallCamera::YuShiBallCamera(std::string addr, uint64_t id)
    : BallCamera(addr, id)
{
    const auto& conn = ReadConfig::getInstance().config().connConfig;
    user_ = conn.camera_username.size() == 0 ? "admin" : conn.camera_username;
    pswd_ = conn.camera_passward.size() == 0 ? "Ab123456" : conn.camera_passward;
}

bool YuShiBallCamera::get_ptz(double& p, double& t, double& z)
{
    /*
     *     /LAPI/V1.0/Channels/<ID>/PTZ/AbsoluteMove
     *     /LAPI/V1.0/Channels/<ID>/PTZ/AbsoluteZoom
     */

    int retry_cnt = 3;
    while (retry_cnt-- > 0) {
        try {
            httplib::Client cli(addr_, 80);
            cli.set_digest_auth(user_.c_str(), pswd_.c_str());

            const std::string url_move = "/LAPI/V1.0/Channels/0/PTZ/AbsoluteMove";
            const std::string url_zoom = "/LAPI/V1.0/Channels/0/PTZ/AbsoluteZoom";

            auto receive_move = cli.Get(url_move.c_str());
            auto receive_zoom = cli.Get(url_zoom.c_str());

            if (receive_move && receive_zoom && receive_move->status == 200 && receive_zoom->status == 200) {
                auto body_move = nlohmann::json::parse(receive_move->body);
                auto body_zoom = nlohmann::json::parse(receive_zoom->body);

                double lon = body_move["Response"]["Data"]["Longitude"];
                double lat = body_move["Response"]["Data"]["Latitude"];
                double zoom = body_zoom["Response"]["Data"]["ZoomRatio"];

                // Need to do something else
                p = lon;
                t = lat;
                z = zoom;
                LOG(INFO) << "Get PTZ success." << p << " " << t << " " << z;
                return true;
            }
            LOG(INFO) << "Preset failure, retry cnt:" << retry_cnt;
        } catch (const std::exception& e) {
            LOG(ERROR) << "get PTZ failure! " << e.what();
        }
    }

    return false;
}

bool YuShiBallCamera::set_ptz(double p, double t, double z)
{
    /*
     *     /LAPI/V1.0/Channels/<ID>/PTZ/AbsoluteMove
     *     /LAPI/V1.0/Channels/<ID>/PTZ/AbsoluteZoom
     */

    LOG(INFO) << addr_ << " begin turning to :"
              << "p:" << p << " t:" << t << " z:" << z;

    bool setpt = std::isnan(p) || std::isnan(t);
    bool setz = std::isnan(z);

    int retry_cnt = 3;
    while (retry_cnt-- > 0 && !setpt) {
        try {
            httplib::Client cli(addr_, 80);
            cli.set_digest_auth(user_.c_str(), pswd_.c_str());

            const std::string url_move = "/LAPI/V1.0/Channels/0/PTZ/AbsoluteMove";
            nlohmann::json data_move;
            data_move["Longitude"] = p;
            data_move["Latitude"] = t;
            std::string input_move = data_move.dump();
            auto receive_move = cli.Put(url_move.c_str(), input_move.c_str(), input_move.size(), "text/plain");

            if (receive_move && receive_move->status == 200) {
                LOG(INFO) << "set PT success! P:" << p << " T:" << t;
                setpt = true;
                break;
            }

            LOG(INFO) << "set PT failure P:" << p << " T:" << t << " retry cnt:" << retry_cnt;
        } catch (const std::exception& e) {
            LOG(ERROR) << "set PTZ failure! " << e.what();
        }
    }

    retry_cnt = 3;
    while (retry_cnt-- > 0 && !setz) {
        try {
            httplib::Client cli(addr_, 80);
            cli.set_digest_auth(user_.c_str(), pswd_.c_str());

            std::string url_zoom = "/LAPI/V1.0/Channels/0/PTZ/AbsoluteZoom";
            nlohmann::json data_zoom;
            data_zoom["ZoomRatio"] = z;
            std::string input_zoom = data_zoom.dump();
            auto receive_zoom = cli.Put(url_zoom.c_str(), input_zoom.c_str(), input_zoom.size(), "text/plain");

            if (receive_zoom && receive_zoom->status == 200) {
                LOG(INFO) << "set Z success, Z:" << z << " retry cnt:" << retry_cnt;
                setz = true;
                break;
            }
        } catch (const std::exception& e) {
            LOG(ERROR) << "set PTZ failure! " << e.what();
        }
    }

    return (setpt && setz);
}

bool YuShiBallCamera::go_to_preset(const uint64_t& preset_id)
{
    std::string str_id = std::to_string(preset_id);
    LOG(INFO) << addr_ << " begin turning to preset_" + str_id;

    int retry_cnt = 3;
    while (retry_cnt-- > 0) {
        try {
            httplib::Client cli(addr_, 80);
            cli.set_digest_auth(user_.c_str(), pswd_.c_str());

            const std::string url = "/LAPI/V1.0/Channels/0/PTZ/Presets/" + str_id + "/Goto";
            auto response = cli.Put(url.c_str());

            if (response && response->status == 200) {
                LOG(INFO) << "Move to preset" << str_id << " success.";
                return true;
            } else {
                LOG(INFO) << "Move to preset" << str_id << " failure. Retry cnt:" << retry_cnt;
            }
        } catch (const std::exception& e) {
            LOG(ERROR) << "Move to preset" << str_id << " failure." << e.what();
        }
    }
    return false;
}

bool YuShiBallCamera::snapshot(std::string& pic)
{
    LOG(INFO) << addr_ << " Begin to Snapshot";

    int retry_cnt = 3;

    while (retry_cnt-- > 0) {
        try {
            httplib::Client cli(addr_, 80);
            cli.set_digest_auth(user_.c_str(), pswd_.c_str());

            const std::string url = "/LAPI/V1.0/Channels/0/Media/Video/Streams/0/Snapshot";
            auto response = cli.Get(url.c_str());

            if (response && response->status == 200) {
                pic = response->body;
                return true;
            } else {
                LOG(INFO) << addr_ << " snapshot fail. Retry cnt:" << retry_cnt;
            }
        } catch (const std::exception& e) {
            LOG(ERROR) << addr_ << " snapshot fail." << e.what();
        }
    }

    return false;
}