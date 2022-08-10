#include "mpeg-ts-proto.h"
#include "Rtp/PSDecoder.h"

using namespace std;
using namespace toolkit;
using namespace mediakit;

#define DEBUG_VIDEO 0

int start_main(int argc, char *argv[]) {
    //设置日志
    Logger::Instance().add(make_shared<ConsoleChannel>("ConsoleChannel"));

    const char *ps_file = "D:\\rtp_dump\\34010000001320000216_34010000001320000216.mp2";
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

    static bool first_frame = true;
    static int64_t last_dts = 0;
    static int64_t duration = 0;

    auto decoder = make_shared<PSDecoder>();
    decoder->setOnDecode(
        [](int stream, int codecid, int flags, int64_t pts, int64_t dts, const void *data, size_t bytes) {
            if (codecid == PSI_STREAM_H264 || codecid == PSI_STREAM_H265) {
#if DEBUG_VIDEO
                int64_t diff = 0;
                if (first_frame) {
                    last_dts = dts;
                    first_frame = false;
                    return;
                }
                diff = dts - last_dts;
                if (0 == diff) {
                    WarnL << "video repeate dts: " << dts;
                }
                // InfoL << "h264 bytes: " << bytes << ", pts: " << pts << ", dts: " << dts << ", diff: " << diff;

                if (0 == duration) {
                    duration = diff;
                }
                //打印出前后帧dts间隔发生变化的dts
                if (duration != diff) {
                    WarnL << "video dts duration changed: " << last_dts << " --> " << dts << ", diff: " << diff
                          << ", duration: " << duration;
                }
                last_dts = dts;
#endif
            } else {
#if !DEBUG_VIDEO
                int64_t diff = 0;
                if (first_frame) {
                    last_dts = dts;
                    first_frame = false;
                    return;
                }
                diff = dts - last_dts;
                if (0 == diff) {
                    WarnL << "audio repeate dts: " << dts;
                }
                // InfoL << "audio bytes: " << bytes << ", pts: " << pts << ", dts: " << dts << ", diff: " << diff;

                if (0 == duration) {
                    duration = diff;
                }
                //打印出前后帧dts间隔发生变化的dts
                if (duration != diff) {
                    WarnL << "audio dts duration changed: " << last_dts << " --> " << dts << ", diff: " << diff
                          << ", u32diff: " << ((uint32_t)dts - (uint32_t)last_dts) << ", duration: " << duration;
                }
                last_dts = dts;
#endif
            }
        });
    decoder->setOnStream([](int stream, int codecid, const void *extra, size_t bytes, int finish) {
        DebugL << "stream: " << stream << ", codec id: " << codecid << ", finish: " << finish;
    });

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
