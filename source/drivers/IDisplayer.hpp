#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

namespace bplayer
{

class IDisplayer {
public:
    explicit IDisplayer(FrameParameter& frameParSrc, 
        FrameParameter& frameParDst,
        PlayerConfig& config)
        : frameParSrc_(frameParSrc), 
            frameParDst_(frameParDst),
            config_(config) {}
    virtual ~IDisplayer() = default;

    virtual bool init() = 0;
    virtual void reset() = 0;
    virtual void clear() = 0;
    virtual void setOrientation(Orientation orientation) {
        orientation_ = orientation;
    }
    virtual void setArea(int width = -1, int height = -1, 
        int offsetX = -1, int offsetY = -1) = 0;
    virtual bool syncFramePar() = 0;
    virtual void display(std::shared_ptr<AVFrame> frame) = 0;

    FrameParameter& frameParSrc_;
    FrameParameter& frameParDst_;

    PlayerConfig& config_;

protected:
    Orientation orientation_ = Orientation::Landscape;
};

}