#include "RendererVideo.hpp"

#include "SaveFrame.hpp"

namespace bplayer
{

RendererVideo::RendererVideo(BlockingQueue<std::shared_ptr<AVFrame>>& queueFrameRaw, 
    BlockingQueue<std::shared_ptr<AVFrame>>& queueFrameDst, 
    PlayerState& state, 
    PlayerConfig& config, 
    FrameParameter& frameParSrc, 
    FrameParameter& frameParDst)
    : queueFrameRaw_(queueFrameRaw), 
        queueFrameDst_(queueFrameDst), 
        state_(state),
        config_(config),
        frameParSrc_(frameParSrc),
        frameParDst_(frameParDst)
{

}

RendererVideo::~RendererVideo()
{
    sws_freeContext(ctxScaler_);
}

bool RendererVideo::init()
{
    return setScalerVideo();
}

void RendererVideo::run()
{
    // int i = 1;
    while (state_.running.load()) {
        auto frameSrc = make_avframe();
        auto frameDst = make_avframe();

        frameDst->width = frameParDst_.width;
        frameDst->height = frameParDst_.height;
        frameDst->format = frameParDst_.pixFmt;
        
        queueFrameRaw_.pop(frameSrc);
        int ret = av_image_alloc(frameDst->data,
            frameDst->linesize, 
            frameDst->width, 
            frameDst->height, 
            frameParDst_.pixFmt, 
            32);
        if (ret < 0) {
            char errBuf[256];
			av_strerror(ret, errBuf, sizeof(errBuf));
			std::cerr << "[Video Renderer] Failed to allocate image buffer" 
                << errBuf << std::endl;
			return;
        }

        sws_scale(ctxScaler_, 
            frameSrc->data, frameSrc->linesize, 0, 
            frameSrc->height, 
            frameDst->data, frameDst->linesize);
        frameDst->pts = frameSrc->pts;
        // saveFrame(frameDst.get(), "../temp/" + std::to_string(i) + ".png");
        // i++;
        queueFrameDst_.push(frameDst);
    }
}

// Dependencies: DisplayerVideo
bool RendererVideo::setScalerVideo()
{
    if (frameParSrc_.width <= 0 || 
        frameParSrc_.height <= 0 || 
        frameParDst_.width <= 0 ||
        frameParDst_.height <= 0)
    {
        std::cerr << "[Video Renderer] Failed to create scaler: size not set" 
            << std::endl;
        return false; 
    }
    ctxScaler_ = sws_getContext(frameParSrc_.width, 
        frameParSrc_.height, 
        frameParSrc_.pixFmt, 
        frameParDst_.width, 
        frameParDst_.height, 
        frameParDst_.pixFmt, 
        config_.flagsScaler, 
        nullptr, 
        nullptr, 
        nullptr);
    if (!ctxScaler_) {
        std::cerr << "[Video Renderer] Failed to create scaler" << std::endl;
        return false;
    }
    av_opt_set_int(ctxScaler_, "dither", config_.flagsDither, 0);
    return true;
}

}