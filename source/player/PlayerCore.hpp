#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

#include "BlockingQueue.hpp"
#include "Timer.hpp"
#include "Loader.hpp"
#include "Demuxer.hpp"
#include "DecoderVideo.hpp"
#include "RendererVideo.hpp"
#include "DisplayerVideo.hpp"


namespace bplayer {

class PlayerCore {
public:
    PlayerCore();
    ~PlayerCore();
private:
    static constexpr size_t MAX_QUEUE_SIZE_PACKET = 30;
    static constexpr size_t MAX_QUEUE_SIZE_FRAME = 30;

    AVFormatContext* ctxFormat_ = nullptr;

    AVStream* streamVideo_ = nullptr;
    AVStream* streamAudio_ = nullptr;
    
    PlayerState state_;

    PlayerConfig config_;

    FrameParameter frameParSrc_;
    FrameParameter frameParDst_;

    BlockingQueue<std::shared_ptr<AVPacket>> queuePacketVideo_ = 
        BlockingQueue<std::shared_ptr<AVPacket>>(MAX_QUEUE_SIZE_PACKET);
    BlockingQueue<std::shared_ptr<AVPacket>> queuePacketAudio_ = 
        BlockingQueue<std::shared_ptr<AVPacket>>(MAX_QUEUE_SIZE_PACKET);
    BlockingQueue<std::shared_ptr<AVFrame>> queueFrameRaw_ =
        BlockingQueue<std::shared_ptr<AVFrame>>(MAX_QUEUE_SIZE_FRAME);
    BlockingQueue<std::shared_ptr<AVFrame>> queueFrameDst_ =
        BlockingQueue<std::shared_ptr<AVFrame>>(MAX_QUEUE_SIZE_FRAME);

    Loader loader_;
    Demuxer demuxer_;
    DecoderVideo decoderVideo_;
    RendererVideo rendererVideo_;
    DisplayerVideo displayerVideo_;
    
    std::thread threadDemuxer_;
    std::thread threadDecoderVideo_;
    std::thread threadRendererVideo_;
    std::thread threadDisplayerVideo_;

public:
    bool init(const std::string& path, Orientation orientation, 
        int width, int height, int offsetX, int offsetY);
    void play();
    void stop();
};


}