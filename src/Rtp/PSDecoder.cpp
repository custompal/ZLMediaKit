/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#if defined(ENABLE_RTPPROXY)

#include "PSDecoder.h"
#include "mpeg-ps.h"
#include "mpeg-ts-proto.h"

using namespace toolkit;

namespace mediakit{
static inline bool isAudio(int codecid) {
    //暂时只碰到G711A/U的音频时钟频率不统一问题，其他AAC和Opus待测试，按正常处理
    if (codecid == PSI_STREAM_AUDIO_G711A || codecid == PSI_STREAM_AUDIO_G711U) {
        return true;
    }
    return false;
}

PSDecoder::PSDecoder() {
    _ps_demuxer = ps_demuxer_create([](void* param,
                                       int stream,
                                       int codecid,
                                       int flags,
                                       int64_t pts,
                                       int64_t dts,
                                       const void* data,
                                       size_t bytes){
        PSDecoder *thiz = (PSDecoder *)param;
        if(thiz->_on_decode){
            //DebugL << "codec id: " << codecid << ", pts: " << pts << ", dts: " << dts;
            if (isAudio(codecid)) {
                if (0 == thiz->_audio_clock_rate)
                    thiz->guessAudioClockRate(codecid, dts);
                thiz->modifyAudioTimestamp(pts, dts);
            }
            thiz->_on_decode(stream, codecid, flags, pts, dts, data, bytes);
        }
        return 0;
    },this);

    ps_demuxer_notify_t notify = {
            [](void *param, int stream, int codecid, const void *extra, int bytes, int finish) {
                PSDecoder *thiz = (PSDecoder *) param;
                if (thiz->_on_stream) {
                    thiz->_on_stream(stream, codecid, extra, bytes, finish);
                }
            }
    };
    ps_demuxer_set_notify((struct ps_demuxer_t *) _ps_demuxer, &notify, this);
}

PSDecoder::~PSDecoder() {
    ps_demuxer_destroy((struct ps_demuxer_t*)_ps_demuxer);
}

ssize_t PSDecoder::input(const uint8_t *data, size_t bytes) {
    HttpRequestSplitter::input(reinterpret_cast<const char *>(data), bytes);
    return bytes;
}

const char *PSDecoder::onSearchPacketTail(const char *data, size_t len) {
    try {
        auto ret = ps_demuxer_input(static_cast<struct ps_demuxer_t *>(_ps_demuxer), reinterpret_cast<const uint8_t *>(data), len);
        if (ret >= 0) {
            //解析成功全部或部分
            return data + ret;
        }

        //解析失败，丢弃所有数据
        return data + len;
    } catch (std::exception &ex) {
        InfoL << "解析 ps 异常: bytes=" << len
              << ", exception=" << ex.what()
              << ", hex=" << hexdump(data, MIN(len, 32));
        //触发断言，解析失败，丢弃所有数据
        return data + len;
    }
}

void PSDecoder::guessAudioClockRate(int codecid, int64_t dts) {
    if (_first_audio_frame) {
        _first_audio_frame = false;
        _last_audio_dts = dts;
    } else {
        do {
            DebugL << "audio codec id: " << codecid << ", dts: " << dts << ", last dts: " << _last_audio_dts;
            if (dts > _last_audio_dts && dts - _last_audio_dts < 10 * 90) { // 假定音频每帧间隔大于 10ms，如果是标准流，后一帧与前一帧音频时间戳之差必大于 10 * 90
                if (codecid == PSI_STREAM_AUDIO_G711A || codecid == PSI_STREAM_AUDIO_G711U) { // 暂时只处理G711A/U
                    _audio_clock_rate = 8;
                    break;
                }
            }
            _audio_clock_rate = 90;
        } while (0);

        DebugL << "audio codec id: " << codecid << ", clock rate: " << _audio_clock_rate;
    }
}

void PSDecoder::modifyAudioTimestamp(int64_t &pts, int64_t &dts) {
    if (_audio_clock_rate != 0 && _audio_clock_rate != 90) {
        dts = (dts / _audio_clock_rate) * 90;
        pts = dts;
    }
}

}//namespace mediakit
#endif//#if defined(ENABLE_RTPPROXY)
