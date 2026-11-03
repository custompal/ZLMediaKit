/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "WebApi.h"
#include "ExtendWebApi.h"

using namespace std;
using namespace mediakit;

// XML 特殊字符转义
static string xmlEscape(const string &s) {
    string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += c;        break;
        }
    }
    return out;
}

#define XML_ESCAPE(str) string(str) // 暂不使用xmlEscape转义

// 浮点格式化：24.0 -> "24"，29.97 -> "29.97"
static string formatFloat(float v) {
    ostringstream oss;
    oss << std::defaultfloat << v;
    return oss.str();
}

static string zlmStatToPingoXml(const list<MediaSource::Ptr> &ml) {
    // 1. 分组
    map<string, map<string, MediaSource::Ptr>> mapp;
    size_t total_streams = 0;

    for (auto &media : ml) {
        string appname = media->getMediaTuple().app;
        // if (appname.size() < 2)
        //     continue;

        total_streams++;
        auto it = mapp.find(appname);
        if (it == mapp.end()) {
            mapp[appname] = map<string, MediaSource::Ptr>();
            mapp[appname][media->getMediaTuple().stream] = media;
        } else {
            it->second[media->getMediaTuple().stream] = media;
        }
    }

    // 2. 构造XML字符串
    string xml;
    // 2.1. 先计算预留空间大小，避免多次内存分配
    constexpr size_t kBaseReserve = 4  * 1024; // 头部 + server 信息
    constexpr size_t kPerStreamEst = 1024; // 单路上限估算（含余量）
    size_t expected = kBaseReserve + total_streams * kPerStreamEst;
    xml.reserve(expected);

    // 2.2. 拼接
    xml += "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n";
    xml += "<?xml-stylesheet type=\"text/xsl\" href=\"/json.xsl\" ?>\n";
    xml += "<rtmp>\n";
    xml += " <nginx_version>202504081</nginx_version>\n";
    xml += " <nginx_rtmp_version>202504081</nginx_rtmp_version>\n";
    xml += " <pid>Pid</pid>\n";
    xml += " <built>202504081</built>\n";
    xml += " <compiler>gcc</compiler>\n";
    xml += " <server>\n";

    for (const auto& p : mapp) {
        xml += "  <application>\n";
        xml += "   <name>" + XML_ESCAPE(p.first) + "</name>\n";
        xml += "   <live>\n";
        xml += "    <nclients>" + to_string(p.second.size()) + "</nclients>\n";
        for (const auto& s : p.second) {
            auto &media = s.second;
            xml += "    <stream>\n";
            xml += "     <name>" + XML_ESCAPE(media->getMediaTuple().stream) + "</name>\n";
            xml += "     <time>" + to_string(media->getAliveSecond() * 1000) + "</time>\n";
            xml += "     <bw_in>" + to_string(media->getBytesSpeed() * 8) + "</bw_in>\n";
            xml += "     <bytes_in>" + to_string(media->getBytesSpeed()) + "</bytes_in>\n";
            xml += "     <bw_out>" + to_string(0) + "</bw_out>\n";
            xml += "     <bytes_out>" + to_string(0) + "</bytes_out>\n";
            xml += "     <bw_audio>" + to_string(0) + "</bw_audio>\n";
            xml += "     <bw_video>" + to_string((media->getBytesSpeed() * 8) > 150000 ? (media->getBytesSpeed() * 8) : 150000) + "</bw_video>\n";

            auto tracks = media->getTracks(false);
            for (auto &track : tracks) {
                if (track->getTrackType() == TrackVideo) {
                    auto videoTrack = dynamic_pointer_cast<VideoTrack>(track);
                    xml += "     <meta>\n";
                    xml += "      <video>\n";
                    xml += "       <width>" + to_string(videoTrack->getVideoWidth()) + "</width>\n";
                    xml += "       <height>" + to_string(videoTrack->getVideoHeight()) + "</height>\n";
                    xml += "       <frame_rate>" + formatFloat(videoTrack->getVideoFps()) + "</frame_rate>\n";
                    xml += "       <codec>" + XML_ESCAPE(videoTrack->getCodecName()) + "</codec>\n";
                    xml += "       <profile></profile>\n";
                    xml += "      </video>\n";
                    xml += "     </meta>\n";
                } else if (track->getTrackType() == TrackAudio) {
                    auto audioTrack = dynamic_pointer_cast<AudioTrack>(track);
                    xml += "     <meta>\n";
                    xml += "      <audio>\n";
                    xml += "       <codec>" + XML_ESCAPE(audioTrack->getCodecName()) + "</codec>\n";
                    xml += "       <profile></profile>\n";
                    xml += "       <channels>" + to_string(audioTrack->getAudioChannel()) + "</channels>\n";
                    xml += "       <sample_rate>" + to_string(audioTrack->getAudioSampleRate()) + "</sample_rate>\n";
                    xml += "      </audio>\n";
                    xml += "     </meta>\n";
                }
            }
            xml += "    </stream>\n";
        }
        xml += "   </live>\n";
        xml += "  </application>\n";
    }

    xml += " </server>\n";
    xml += "</rtmp>\n";
    return xml;
}

/**
 * 安装拓展api接口
 * 所有api都支持GET和POST两种方式
 * POST方式参数支持application/json和application/x-www-form-urlencoded方式
 * Install api interface
 * All apis support GET and POST methods
 * POST method parameters support application/json and application/x-www-form-urlencoded methods

 * [AUTO-TRANSLATED:62e68c43]
 */
void installExtendWebApi() {

    static auto s_get_rtmp_stat = [](API_ARGS_MAP_ASYNC){
        CHECK_SECRET();
        // 1. 获取所有RTMP MediaSource列表  [AUTO-TRANSLATED:7bf16dc2]
        // 1. Get all RTMP MediaSource lists
        list<MediaSource::Ptr> lst;
        MediaSource::for_each_media([&](const MediaSource::Ptr &media) {
            lst.emplace_back(media);
        }, allArgs["schema"].empty() ? "rtmp" : allArgs["schema"], allArgs["vhost"], allArgs["app"], allArgs["stream"]);

        // 2. 构造XML字符串
        string xml = zlmStatToPingoXml(lst);

        // 3. 返回XML响应
        // 注意：ZLMediaKit默认API返回JSON，需手动设置Content-Type并返回XML字符串
        headerOut["Content-Type"] = "application/xml";
        invoker(200, headerOut, xml);
    };

    // 获取rtmp流状态，返回xml格式  [AUTO-TRANSLATED:7bf16dc2]
    // Get rtmp stream status, return xml format
    // 测试url http://127.0.0.1/index/api/stat  [AUTO-TRANSLATED:7bf16dc2]
    // Test url http://127.0.0.1/index/api/stat
    api_regist("/index/api/stat",[](API_ARGS_MAP_ASYNC){
        s_get_rtmp_stat(API_ARGS_VALUE, invoker);
    });

    // 获取rtmp流状态，返回xml格式  [AUTO-TRANSLATED:7bf16dc2]
    // Get rtmp stream status, return xml format
    // 测试url http://127.0.0.1/index/api/allstat  [AUTO-TRANSLATED:7bf16dc2]
    // Test url http://127.0.0.1/index/api/allstat
    // api_regist("/allstat",[](API_ARGS_MAP_ASYNC){
    //     s_get_rtmp_stat(API_ARGS_VALUE, invoker);
    // });

}

void unInstallExtendWebApi(){

}
