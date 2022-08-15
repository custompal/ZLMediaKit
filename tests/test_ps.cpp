#include "mpeg-ts-proto.h"
#include "Rtp/PSDecoder.h"

using namespace std;
using namespace toolkit;
using namespace mediakit;

#define DEBUG_VIDEO 0
#define DEBUG_AUDIO 1
#define REVISE_TIMESTAMP 1

#define SWITCH_CASE(codec_id)                                                                                          \
    case codec_id: return #codec_id
static const char *getCodecName(int codec_id) {
    switch (codec_id) {
        SWITCH_CASE(PSI_STREAM_MPEG1);
        SWITCH_CASE(PSI_STREAM_MPEG2);
        SWITCH_CASE(PSI_STREAM_AUDIO_MPEG1);
        SWITCH_CASE(PSI_STREAM_MP3);
        SWITCH_CASE(PSI_STREAM_AAC);
        SWITCH_CASE(PSI_STREAM_MPEG4);
        SWITCH_CASE(PSI_STREAM_MPEG4_AAC_LATM);
        SWITCH_CASE(PSI_STREAM_H264);
        SWITCH_CASE(PSI_STREAM_MPEG4_AAC);
        SWITCH_CASE(PSI_STREAM_H265);
        SWITCH_CASE(PSI_STREAM_AUDIO_AC3);
        SWITCH_CASE(PSI_STREAM_AUDIO_EAC3);
        SWITCH_CASE(PSI_STREAM_AUDIO_DTS);
        SWITCH_CASE(PSI_STREAM_VIDEO_DIRAC);
        SWITCH_CASE(PSI_STREAM_VIDEO_VC1);
        SWITCH_CASE(PSI_STREAM_VIDEO_SVAC);
        SWITCH_CASE(PSI_STREAM_AUDIO_SVAC);
        SWITCH_CASE(PSI_STREAM_AUDIO_G711A);
        SWITCH_CASE(PSI_STREAM_AUDIO_G711U);
        SWITCH_CASE(PSI_STREAM_AUDIO_G722);
        SWITCH_CASE(PSI_STREAM_AUDIO_G723);
        SWITCH_CASE(PSI_STREAM_AUDIO_G729);
        SWITCH_CASE(PSI_STREAM_AUDIO_OPUS);
        default: return "unknown codec";
    }
}

class PSDecoderImp {
public:
    PSDecoderImp() {
        _decoder = make_shared<PSDecoder>();
        _decoder->setOnDecode(
            [this](int stream, int codecid, int flags, int64_t pts, int64_t dts, const void *data, size_t bytes) {
                onDecode(stream, codecid, flags, pts, dts, data, bytes);
            });
        _decoder->setOnStream([this](int stream, int codecid, const void *extra, size_t bytes, int finish) {
            onStream(stream, codecid, extra, bytes, finish);
        });
    }
    ~PSDecoderImp() = default;

    ssize_t input(const uint8_t *data, size_t bytes) { return _decoder->input(data, bytes); }

private:
    void onDecode(int stream, int codecid, int flags, int64_t pts, int64_t dts, const void *data, size_t bytes) {
#if REVISE_TIMESTAMP
        pts /= 90;
        dts /= 90;
#endif

        int64_t diff = 0;
        if (codecid == PSI_STREAM_H264 || codecid == PSI_STREAM_H265) {
#if DEBUG_VIDEO
#if REVISE_TIMESTAMP
            _tracks[TrackVideo].stamp.revise(dts, pts, dts, pts);
#endif
            if (!_tracks[TrackVideo].last_dts) {
                _tracks[TrackVideo].last_dts = dts;
            } else {
                diff = dts - _tracks[TrackVideo].last_dts;
                if (0 == diff) {
                    WarnL << "video repeate dts: " << dts;
                }
                if (!_tracks[TrackVideo].duration) {
                    _tracks[TrackVideo].duration = diff;
                }
                //打印出前后帧dts间隔发生变化的dts
                if (_tracks[TrackVideo].duration != diff) {
                    WarnL << "video dts duration changed: " << _tracks[TrackVideo].last_dts << " --> " << dts
                          << ", diff: " << diff << ", duration: " << _tracks[TrackVideo].duration;
                }
                _tracks[TrackVideo].last_dts = dts;
            }
            //InfoL << "h264 bytes: " << bytes << ", pts: " << pts << ", dts: " << dts << ", diff: " << diff;
#endif
        } else {
#if DEBUG_AUDIO
#if REVISE_TIMESTAMP
            _tracks[TrackAudio].stamp.revise(dts, pts, dts, pts);
#endif
            if (!_tracks[TrackAudio].last_dts) {
                _tracks[TrackAudio].last_dts = dts;
            } else {
                diff = dts - _tracks[TrackAudio].last_dts;
                if (0 == diff) {
                    WarnL << "audio repeate dts: " << dts;
                }
                if (!_tracks[TrackAudio].duration) {
                    _tracks[TrackAudio].duration = diff;
                }
                //打印出前后帧dts间隔发生变化的dts
                if (_tracks[TrackAudio].duration != diff) {
                    WarnL << "audio dts duration changed: " << _tracks[TrackAudio].last_dts << " --> " << dts
                          << ", diff: " << diff
                          << ", u32diff: " << ((uint32_t)dts - (uint32_t)_tracks[TrackAudio].last_dts)
                          << ", duration: " << _tracks[TrackAudio].duration;
                }
                _tracks[TrackAudio].last_dts = dts;
            }
            InfoL << "audio bytes: " << bytes << ", pts: " << pts << ", dts: " << dts << ", diff: " << diff;
#endif
        }
    }

    void onStream(int stream, int codecid, const void *extra, size_t bytes, int finish) {
        DebugL << "stream: " << stream << ", codec id: " << codecid << ", finish: " << finish;

        switch (codecid) {
            case PSI_STREAM_H264: {
                onTrack(CodecH264);
                break;
            }

            case PSI_STREAM_H265: {
                onTrack(CodecH265);
                break;
            }

            case PSI_STREAM_MPEG4_AAC:
            case PSI_STREAM_AAC: {
                onTrack(CodecAAC);
                break;
            }

            case PSI_STREAM_AUDIO_G711A:
            case PSI_STREAM_AUDIO_G711U: {
                auto codec = codecid == PSI_STREAM_AUDIO_G711A ? CodecG711A : CodecG711U;
                // G711传统只支持 8000/1/16的规格，FFmpeg貌似做了扩展，但是这里不管它了
                onTrack(codec);
                break;
            }

            case PSI_STREAM_AUDIO_OPUS: {
                onTrack(CodecOpus);
                break;
            }

            default:
                if (codecid != 0) {
                    WarnL << "unsupported codec type:" << getCodecName(codecid) << " " << (int)codecid;
                }
                break;
        }
    }

    void onTrack(CodecId codec) {
        if (!_tracks[getTrackType(codec)]) {
            _tracks[getTrackType(codec)].codec = codec;
            InfoL << "got track: " << getCodecName(codec);
        }

#if 0
        Stamp *audio = nullptr, *video = nullptr;
        for (auto &track : _tracks) {
            if (track) {
                switch (getTrackType(track.codec)) {
                    case TrackVideo: video = &track.stamp; break;
                    case TrackAudio: audio = &track.stamp; break;
                    default: break;
                }
            }
        }
        if (audio && video) {
            //音频时间戳同步于视频，因为音频时间戳被修改后不影响播放
            audio->syncTo(*video);
        }
#endif
    }

private:
    Decoder::Ptr _decoder;

    struct TrackContext {
        CodecId codec = CodecInvalid;
        Stamp stamp;

        int64_t last_dts = 0;
        int64_t duration = 0;

        operator bool() const { return codec != CodecInvalid; }
    };
    TrackContext _tracks[TrackMax];
};

int start_main(int argc, char *argv[]) {
    //设置日志
    Logger::Instance().add(make_shared<ConsoleChannel>("ConsoleChannel"));

    const char *ps_file = "D:\\rtp_dump\\34020000001320000015_34020000001320000015.mp2";
    shared_ptr<FILE> read_fp(fopen(ps_file, "rb"), [](FILE *fp) {
        if (fp) {
            fclose(fp);
        }
    });
    if (!read_fp) {
        WarnL << "open " << ps_file << " failed!";
        return -1;
    }

    size_t len = 4 * 1024 * 1024;
    shared_ptr<char> buf(new char[len], [](char *p) {
        if (p) {
            delete[] p;
        }
    });
    assert(buf);

    auto decoder = make_shared<PSDecoderImp>();
    auto ptr = reinterpret_cast<uint8_t *>(buf.get());
    while (1) {
        auto read_len = fread(ptr, 1, len, read_fp.get());
        if (read_len <= 0) {
            break;
        }
        decoder->input(ptr, read_len);
    }

    return 0;
}

int main(int argc, char *argv[]) {
#if defined(_WIN32)
    system("chcp 65001");
#endif
    return start_main(argc, argv);
}
