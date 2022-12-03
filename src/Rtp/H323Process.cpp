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
#include "H323Process.h"
#include "Extension/CommonRtp.h"
#include "Extension/Factory.h"
#include "Extension/G711.h"
#include "Extension/H264Rtp.h"
#include "Extension/H265.h"
#include "Extension/AAC.h"
#include "Extension/Opus.h"
#include "Http/HttpTSPlayer.h"
#include "Util/File.h"
#include "Common/config.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

class H323RtpReceiverImp : public RtpTrackImp {
public:
    using Ptr = std::shared_ptr<H323RtpReceiverImp>;

    H323RtpReceiverImp(int sample_rate, RtpTrackImp::OnSorted cb, RtpTrackImp::BeforeSorted cb_before = nullptr) {
        _sample_rate = sample_rate;
        setOnSorted(std::move(cb));
        setBeforeSorted(std::move(cb_before));
        // H323推流不支持ntp时间戳
        setNtpStamp(0, 0);
    }

    ~H323RtpReceiverImp() override = default;

    bool inputRtp(TrackType type, uint8_t *ptr, size_t len) {
        if (len < RtpPacket::kRtpHeaderSize) {
            WarnL << "rtp size(" << len << ") less than 12";
            return false;
        }
        GET_CONFIG(uint32_t, rtpMaxSize, Rtp::kRtpMaxSize);
        if (len > 1024 * rtpMaxSize) {
            WarnL << "超大的rtp包:" << len << " > " << 1024 * rtpMaxSize;
            return false;
        }
        if (!_sample_rate) {
            //无法把时间戳转换成毫秒
            return false;
        }
        RtpHeader *header = (RtpHeader *)ptr;
        // if (header->version != RtpPacket::kRtpVersion) {
        //    throw BadRtpException("invalid rtp version");
        //}
        // if (header->getPayloadSize(len) < 0) {
        //    // rtp有效负载小于0，非法
        //    throw BadRtpException("invalid rtp payload size");
        //}

        //比对缓存ssrc
        auto ssrc = ntohl(header->ssrc);
        auto pt = header->pt;
        auto it = _ssrc.find(pt);
        if (it == _ssrc.end()) {
            //记录并锁定ssrc
            _ssrc[pt] = ssrc;
            _ssrc_alive[pt] = make_shared<Ticker>();
        } else if (_ssrc[pt] == ssrc) {
            // ssrc匹配正确,刷新计时器
            _ssrc_alive[pt]->resetTime();
        } else {
            // ssrc错误
            if (_ssrc_alive[pt]->elapsedTime() < 3 * 1000) {
                //接受正确ssrc的rtp在10秒内，那么我们认为存在多路rtp,忽略掉ssrc不匹配的rtp
                WarnL << "ssrc mismatch, rtp dropped:" << ssrc << " != " << _ssrc[pt];
                return false;
            }
            InfoL << "rtp ssrc changed:" << _ssrc[pt] << " -> " << ssrc;
            _ssrc[pt] = ssrc;
            _ssrc_alive[pt]->resetTime();
        }

        auto rtp = RtpPacket::create();
        //需要添加4个字节的rtp over tcp头
        rtp->setCapacity(RtpPacket::kRtpTcpHeaderSize + len);
        rtp->setSize(RtpPacket::kRtpTcpHeaderSize + len);
        rtp->sample_rate = _sample_rate;
        rtp->type = type;

        //赋值4个字节的rtp over tcp头
        uint8_t *data = (uint8_t *)rtp->data();
        data[0] = '$';
        data[1] = 2 * type;
        data[2] = (len >> 8) & 0xFF;
        data[3] = len & 0xFF;
        //拷贝rtp
        memcpy(&data[4], ptr, len);
        //不支持ntp时间戳，直接使用rtp时间戳
        rtp->ntp_stamp = rtp->getStamp() * uint64_t(1000) / _sample_rate;

        onBeforeRtpSorted(rtp);
        sortPacket(rtp->getSeq(), rtp);
        return rtp.operator bool();
    }

private:
    int _sample_rate;
    std::unordered_map<uint8_t, uint32_t> _ssrc;
    std::unordered_map<uint8_t, shared_ptr<Ticker>> _ssrc_alive;
};

///////////////////////////////////////////////////////////////////////////////////////////

H323Process::H323Process(const MediaInfo &media_info, MediaSinkInterface *sink) {
    assert(sink);
    _media_info = media_info;
    _interface = sink;
}

void H323Process::onRtpSorted(RtpPacket::Ptr rtp) {
    _rtp_decoder[rtp->getHeader()->pt]->inputRtp(rtp, false);
}

bool H323Process::inputRtp(bool, const char *data, size_t data_len) {
    RtpHeader *header = (RtpHeader *)data;

    //先过滤无效的rtp包
    if (header->version != RtpPacket::kRtpVersion) {
        return false;
    }
    if (!header->getPayloadSize(data_len)) {
        //无有效负载的rtp包
        return false;
    }

    auto pt = header->pt;
    auto &ref = _rtp_receiver[pt];
    if (!ref) {
        if (_rtp_receiver.size() > 2) {
            //防止pt类型太多导致内存溢出
            WarnL << "drop payload " << pt;
            throw std::invalid_argument("rtp pt类型不得超过2种!");
        }
        switch (pt) {
            case 0:
                // CodecG711U
            case 8: {
                // CodecG711A
                ref = std::make_shared<H323RtpReceiverImp>(
                    8000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });

                auto track = std::make_shared<G711Track>(pt == 0 ? CodecG711U : CodecG711A, 8000, 1, 16);
                _interface->addTrack(track);
                _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                break;
            }
            case 99: {
                // aac负载
                ref = std::make_shared<H323RtpReceiverImp>(
                    90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });

                auto track = std::make_shared<AACTrack>();
                _interface->addTrack(track);
                _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                break;
            }
            case 100: {
                // opus负载
                ref = std::make_shared<H323RtpReceiverImp>(
                    48000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });

                auto track = std::make_shared<OpusTrack>();
                _interface->addTrack(track);
                _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                break;
            }
            default: {
                //统一当作H264负载
                ref = std::make_shared<H323RtpReceiverImp>(
                    90000, [this](RtpPacket::Ptr rtp) { onRtpSorted(std::move(rtp)); });

                auto track = std::make_shared<H264Track>();
                _interface->addTrack(track);
                _rtp_decoder[pt] = Factory::getRtpDecoderByTrack(track);
                break;
            }
        }

        //设置dump目录
        GET_CONFIG(string, dump_dir, RtpProxy::kDumpDir);
        if (!dump_dir.empty()) {
            auto video_path = File::absolutePath(_media_info._streamid + "_video.mp2", dump_dir);
            auto audio_path = File::absolutePath(_media_info._streamid + "_audio.mp2", dump_dir);
            _save_file_video.reset(File::create_file(video_path.data(), "wb"), [](FILE *fp) {
                if (fp) {
                    fclose(fp);
                }
            });
            _save_file_audio.reset(File::create_file(audio_path.data(), "wb"), [](FILE *fp) {
                if (fp) {
                    fclose(fp);
                }
            });
        }

        //设置frame回调
        _rtp_decoder[pt]->addDelegate([this](const Frame::Ptr &frame) {
            onRtpDecode(frame);
            return true;
        });
    }

    return ref->inputRtp(TrackVideo, (unsigned char *)data, data_len);
}

void H323Process::onRtpDecode(const Frame::Ptr &frame) {
    if (frame->getCodecId() != CodecInvalid) {
        if (frame->getTrackType() == TrackVideo && _save_file_video) {
            fwrite(frame->data(), frame->size(), 1, _save_file_video.get());
        }
        if (frame->getTrackType() == TrackAudio && _save_file_audio) {
            fwrite(frame->data(), frame->size(), 1, _save_file_audio.get());
        }
        _interface->inputFrame(frame);
        return;
    }

    WarnL << "未识别的帧类型!";
}

} // namespace mediakit
#endif // defined(ENABLE_RTPPROXY)
