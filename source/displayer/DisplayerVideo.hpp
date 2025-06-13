#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

#include "IDisplayer.hpp"

namespace bplayer {

class DisplayerVideo {
public:
    DisplayerVideo(BlockingQueue<std::shared_ptr<AVFrame>>& queueFrame, 
        PlayerState& state,
        PlayerConfig& config,
        FrameParameter& frameParSrc, 
        FrameParameter& frameParDst);
    ~DisplayerVideo();

    void run();

    bool init(Orientation orientation, 
        int width, 
        int height, 
        int offsetX, 
        int offsetY);

private:
    std::unique_ptr<IDisplayer> screen_;
    BlockingQueue<std::shared_ptr<AVFrame>>& queueFrame_;
    
    PlayerState& state_;
    PlayerConfig& config_;
    
    FrameParameter& frameParSrc_;
    FrameParameter& frameParDst_;
};
    
}