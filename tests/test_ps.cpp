#include "mpeg-ts-proto.h"
#include "Rtp/PSDecoder.h"

using namespace toolkit;
using namespace mediakit;

int start_main(int argc, char *argv[]) {
    //设置日志
    Logger::Instance().add(std::make_shared<ConsoleChannel>("ConsoleChannel"));

    std::shared_ptr<FILE> read_fp(fopen("C:\\Users\\ChenRG\\Desktop\\bao.raw", "rb"), [](FILE *fp) {
        if (fp) {
            fclose(fp);
        }
    });
    size_t len = 4 * 1024 * 1024;
    std::shared_ptr<char> buf(new char[len], [](char *p) {
        if (p)
            delete[] p;
    });

    auto decoder = std::make_shared<PSDecoder>();
    static int64_t last_dts = 0;
    static bool first_frame = true;
    decoder->setOnDecode(
        [](int stream, int codecid, int flags, int64_t pts, int64_t dts, const void *data, size_t bytes) {
#if 0
            if (codecid == PSI_STREAM_AUDIO_G711A) {
                int64_t diff = 0;
                if (first_frame) {
                    last_dts = dts;
                    first_frame = false;
                } else {
                    diff = dts - last_dts;
                    //if (diff == 0)
                    //    //WarnL << "g711a repeate dts: " << dts;
                    //    std::cout << "g711a repeate dts: " << dts << std::endl;
                    last_dts = dts;
                }
                //DebugL << "g711a bytes: " << bytes << ", pts: " << pts << ", dts: " << dts << ", diff: " << diff;
                std::cout << "g711a bytes: " << bytes << ", pts: " << pts << ", dts: " << dts << ", diff: " << diff << std::endl;
            }
#else
            if (codecid == PSI_STREAM_H264) {
                int64_t diff = 0;
                if (first_frame) {
                    last_dts = dts;
                    first_frame = false;
                } else {
                    diff = dts - last_dts;
                    //if (diff == 0)
                    //    // WarnL << "h264 repeate dts: " << dts;
                    //    std::cout << "h264 repeate dts: " << dts << std::endl;
                    last_dts = dts;
                }
                // DebugL << "h264 bytes: " << bytes << ", pts: " << pts << ", dts: " << dts << ", diff: " << diff;
                std::cout << "h264 bytes: " << bytes << ", pts: " << pts << ", dts: " << dts << ", diff: " << diff
                          << std::endl;
            }
#endif
        });
    decoder->setOnStream([](int stream, int codecid, const void *extra, size_t bytes, int finish) {
        DebugL << "stream: " << stream << ", codec id: " << codecid << ", finish: " << finish;
    });

    uint8_t *ptr = (uint8_t *)buf.get();
    size_t last_len = 0;
    while (1) {
        size_t read_len = fread(ptr + last_len, 1, len - last_len, read_fp.get());
        if (read_len <= 0)
            break;

        ssize_t demux_len = decoder->input(ptr, read_len);
        last_len = len - demux_len;
    }

    return 0;
}

int main(int argc, char *argv[]) {
#if defined(_WIN32)
    system("chcp 65001");
#endif
    return start_main(argc, argv);
}
