#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

namespace bplayer
{

class FilterProcessor {
public:
    FilterProcessor(FrameParameter& frameParSrc, 
        FrameParameter& frameParSink);
    ~FilterProcessor();

    bool init(const std::string& filterCmd);
    std::shared_ptr<AVFrame> process(std::shared_ptr<AVFrame> frameSrc);
    void reset();

private:
    AVFilterGraph* filterGraph_ = nullptr;
    AVFilterContext* ctxFilterSrc_ = nullptr;
    AVFilterContext* ctxFilterSink_ = nullptr;

    FrameParameter& frameParSrc_;
    FrameParameter& frameParSink_;
};

}