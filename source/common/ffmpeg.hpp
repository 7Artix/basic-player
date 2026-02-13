#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

namespace bplayer
{
#ifndef SWS_DITHER_BAYER
#define SWS_DITHER_BAYER 1
#endif

#ifndef SWS_DITHER_ED
#define SWS_DITHER_ED 2
#endif

    using SwsFlags = int;
    using SwsDither = int;

inline std::shared_ptr<AVFrame> make_avframe() {
	auto deleter = [](AVFrame* frame) {
		av_frame_free(&frame);
	};
    return std::shared_ptr<AVFrame>(av_frame_alloc(), deleter);
}

inline std::shared_ptr<AVPacket> make_avpacket() {
	auto deleter = [](AVPacket* packet) {
		av_packet_free(&packet);
	};
	return std::shared_ptr<AVPacket>(av_packet_alloc(), deleter);
}

struct PlayerState {
    std::atomic<bool> running{false};
    std::atomic<bool> paused{false};
    std::atomic<bool> seeking{false};
    std::atomic<double> speed{1.0};
    std::atomic<bool> eof{false};
    std::atomic<int64_t> seekTargetUs{-1};
	std::atomic<bool> changedFrame{false};
};

struct PlayerConfig {
	std::atomic<SwsFlags> flagsScaler = SWS_BICUBIC;
    // Dithering algorithm: SWS_DITHER_BAYER / SWS_DITHER_ED
    std::atomic<SwsDither> flagsDither = SWS_DITHER_BAYER;
};

struct FrameParameter {
	std::atomic<int> width{0};
	std::atomic<int> height{0};
	std::atomic<AVPixelFormat> pixFmt{AV_PIX_FMT_RGB565};

	std::atomic<int> sar_num{1};
    std::atomic<int> sar_den{1};
    AVRational getSar() const {
        return AVRational{sar_num.load(), sar_den.load()};
    }
    void setSar(AVRational sar) {
        sar_num.store(sar.num);
        sar_den.store(sar.den);
    }
};

enum class Orientation : int {
	Portrait,
    Landscape,
    PortraitInverted,
    LandscapeInverted
};

static std::string ffmpegErrStr(int errNum) {
    char errBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    return av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, errNum);
};

}