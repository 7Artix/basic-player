#include "DisplayerVideo.hpp"

#include "ST7735S.hpp"
#include "SSD1306.hpp"

namespace bplayer
{

DisplayerVideo::DisplayerVideo(BlockingQueue<std::shared_ptr<AVFrame>>& queueFrame, 
    PlayerState& state,
    PlayerConfig& config,
    FrameParameter& frameParSrc, 
    FrameParameter& frameParDst)
    : queueFrame_(queueFrame), 
        state_(state), 
        config_(config),
        frameParSrc_(frameParSrc), 
        frameParDst_(frameParDst)
{

}

DisplayerVideo::~DisplayerVideo()
{
    
}

bool DisplayerVideo::init(Orientation orientation, 
    int width, int height, int offsetX, int offsetY)
{
    screen_ = std::make_unique<DisplayerSSD1306>(frameParSrc_, frameParDst_, config_);
    bool ret = screen_->init();
    screen_->clear();
    screen_->setOrientation(orientation);
    screen_->setArea(width, height, offsetX, offsetY);
    ret = ret && screen_->syncFramePar();
    return ret;
}

void DisplayerVideo::run()
{
    while (state_.running.load()) {
        auto frame = make_avframe();
        queueFrame_.pop(frame);
        screen_->display(frame);
    }
}

}