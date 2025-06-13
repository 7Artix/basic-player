#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

namespace bplayer
{

class DecoderVideo {
public:
    DecoderVideo(BlockingQueue<std::shared_ptr<AVPacket>>& queuePacket, 
        BlockingQueue<std::shared_ptr<AVFrame>>& queueFrame, 
        AVStream*& stream,
        PlayerState& state, 
        PlayerConfig& config, 
        FrameParameter& frameParSrc);
    ~DecoderVideo();

    bool init();
    void run();

private:
    AVStream*& stream_;
    const AVCodec* codec_ = nullptr;
    AVCodecContext* ctxCodec_ = nullptr;

    BlockingQueue<std::shared_ptr<AVPacket>>& queuePacket_;
    BlockingQueue<std::shared_ptr<AVFrame>>& queueFrame_;
    
    PlayerState& state_;
    PlayerConfig& config_;

    FrameParameter& frameParSrc_;

    bool openCodecVideo();
    bool openCodecVideoByName(const char* name);

    bool syncFramePar();
};

}