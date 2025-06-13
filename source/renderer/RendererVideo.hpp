#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

namespace bplayer {

class RendererVideo {
public:
    RendererVideo(BlockingQueue<std::shared_ptr<AVFrame>>& queueFrameRaw, 
        BlockingQueue<std::shared_ptr<AVFrame>>& queueFrameDst, 
        PlayerState& state, 
        PlayerConfig& config, 
        FrameParameter& frameParSrc, 
        FrameParameter& frameParDst);
    ~RendererVideo();

    bool init();
    void run();

private:
    BlockingQueue<std::shared_ptr<AVFrame>>& queueFrameRaw_;
    BlockingQueue<std::shared_ptr<AVFrame>>& queueFrameDst_;

    PlayerState& state_;
    PlayerConfig& config_;

    FrameParameter& frameParSrc_;
    FrameParameter& frameParDst_;

    SwsContext* ctxScaler_ = nullptr;

    bool setScalerVideo();
};

}