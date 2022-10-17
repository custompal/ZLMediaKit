/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/xia-chu/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_H323PROCESS_H
#define ZLMEDIAKIT_H323PROCESS_H

#if defined(ENABLE_RTPPROXY)

#include "Decoder.h"
#include "ProcessInterface.h"
#include "Rtsp/RtpCodec.h"
#include "Rtsp/RtpReceiver.h"
#include "Http/HttpRequestSplitter.h"

namespace mediakit{

class H323RtpReceiverImp;

class H323Process : public mediakit::ProcessInterface {
public:
    using Ptr = std::shared_ptr<H323Process>;

    H323Process(const mediakit::MediaInfo &media_info, mediakit::MediaSinkInterface *sink);
    ~H323Process() override = default;

    /**
     * 输入rtp
     * @param data rtp数据指针
     * @param data_len rtp数据长度
     * @return 是否解析成功
     */
    bool inputRtp(bool, const char *data, size_t data_len) override;

protected:
    void onRtpSorted(mediakit::RtpPacket::Ptr rtp);

private:
    void onRtpDecode(const mediakit::Frame::Ptr &frame);

private:
    mediakit::MediaInfo _media_info;
    mediakit::MediaSinkInterface *_interface;
    std::shared_ptr<FILE> _save_file_video;
    std::shared_ptr<FILE> _save_file_audio;
    std::unordered_map<uint8_t, std::shared_ptr<mediakit::RtpCodec>> _rtp_decoder;
    std::unordered_map<uint8_t, std::shared_ptr<H323RtpReceiverImp>> _rtp_receiver;
};

}//namespace mediakit
#endif//defined(ENABLE_RTPPROXY)
#endif //ZLMEDIAKIT_H323ROCESS_H
