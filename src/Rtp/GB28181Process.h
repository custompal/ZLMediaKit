/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_GB28181ROCESS_H
#define ZLMEDIAKIT_GB28181ROCESS_H

#if defined(ENABLE_RTPPROXY)

#include "Decoder.h"
#include "ProcessInterface.h"
#include "Rtsp/RtpCodec.h"
#include "Rtsp/RtpReceiver.h"
#include "Http/HttpRequestSplitter.h"

namespace mediakit{

class RtpReceiverImp;
class GB28181Process
    : public ProcessInterface
    , public MediaSinkInterface {
public:
    typedef std::shared_ptr<GB28181Process> Ptr;
    GB28181Process(const MediaInfo &media_info, MediaSinkInterface *sink);
    ~GB28181Process() override;

    /**
     * 输入rtp
     * @param data rtp数据指针
     * @param data_len rtp数据长度
     * @return 是否解析成功
     */
    bool inputRtp(bool, const char *data, size_t data_len) override;

protected:
    void onRtpSorted(RtpPacket::Ptr rtp);

private:
    void onRtpDecode(const Frame::Ptr &frame);

    /**
     * 输入frame
     * @param frame
     */
    bool inputFrame(const Frame::Ptr &frame) override;

    /**
     * 添加track，内部会调用Track的clone方法
     * 只会克隆sps pps这些信息 ，而不会克隆Delegate相关关系
     * @param track
     */
    bool addTrack(const Track::Ptr &track) override;

    /**
     * 添加Track完毕，如果是单Track，会最多等待3秒才会触发onAllTrackReady
     * 这样会增加生成流的延时，如果添加了音视频双Track，那么可以不调用此方法
     * 否则为了降低流注册延时，请手动调用此方法
     */
    void addTrackCompleted() override;

    /**
     * 重置track
     */
    void resetTracks() override;

private:
    MediaInfo _media_info;
    DecoderImp::Ptr _decoder;
    bool _drop_flag = true;
    MediaSinkInterface *_interface;
    std::shared_ptr<FILE> _save_file_ps;
    std::unordered_map<uint8_t, std::shared_ptr<RtpCodec> > _rtp_decoder;
    std::unordered_map<uint8_t, std::shared_ptr<RtpReceiverImp> > _rtp_receiver;
};

}//namespace mediakit
#endif//defined(ENABLE_RTPPROXY)
#endif //ZLMEDIAKIT_GB28181ROCESS_H
