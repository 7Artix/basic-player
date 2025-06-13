#include "PlayerCore.hpp"

namespace bplayer
{

PlayerCore::PlayerCore()
    : loader_(ctxFormat_),
        demuxer_(queuePacketVideo_, queuePacketAudio_, ctxFormat_, streamVideo_, streamAudio_, state_, config_), 
        decoderVideo_(queuePacketVideo_, queueFrameRaw_, streamVideo_, state_, config_, frameParSrc_), 
        rendererVideo_(queueFrameRaw_, queueFrameDst_, state_, config_, frameParSrc_, frameParDst_), 
        displayerVideo_(queueFrameDst_, state_, config_, frameParSrc_, frameParDst_)
{
	
}


PlayerCore::~PlayerCore()
{
    state_.running = false;

    if (threadDemuxer_.joinable()) {
        threadDemuxer_.join();
    }
    if (threadDecoderVideo_.joinable()) {
        threadDecoderVideo_.join();
    }
    if (threadRendererVideo_.joinable()) {
        threadRendererVideo_.join();
    }
    if (threadDisplayerVideo_.joinable()) {
        threadDisplayerVideo_.join();
    }

    queuePacketVideo_.flush();
    queuePacketAudio_.flush();
    queueFrameRaw_.flush();
    queueFrameDst_.flush();

    queuePacketVideo_.shutdown();
    queuePacketAudio_.shutdown();
    queueFrameRaw_.shutdown();
    queueFrameDst_.shutdown();

    if (ctxFormat_) {
        avformat_close_input(&ctxFormat_);
        ctxFormat_ = nullptr;
    }
}

bool PlayerCore::init(const std::string& path, Orientation orientation, 
    int width, int height, int offsetX, int offsetY)
{
    if (!loader_.open(path)) {
        std::cerr << "[PlayerCore] Failed when loading" << std::endl;
        return false;
    }
    if (!demuxer_.init()) {
        std::cerr << "[PlayerCore] Failed to initialize demuxer" << std::endl;
        return false;
    }
    if (!decoderVideo_.init()) {
        std::cerr << "[PlayerCore] Failed to initialize video decoder" << std::endl;
        return false;
    }
    if (!displayerVideo_.init(orientation, width, height, offsetX, offsetY)) {
        std::cerr << "[PlayerCore] Failed to initialize video displayer" << std::endl;
        return false;
    }
    if (!rendererVideo_.init()) {
        std::cerr << "[PlayerCore] Failed to initialize video renderer" << std::endl;
        return false;
    }
    return true;
}

void PlayerCore::play()
{
    queuePacketVideo_.flush();
    queuePacketAudio_.flush();
    queueFrameRaw_.flush();
    queueFrameDst_.flush();

    if (state_.running) {
        return;
    }
    
    state_.running = true;
    state_.paused = false;

    threadDemuxer_ = std::thread(&Demuxer::run, &demuxer_);
    threadDecoderVideo_ = std::thread(&DecoderVideo::run, &decoderVideo_);
    threadRendererVideo_ = std::thread(&RendererVideo::run, &rendererVideo_);
    threadDisplayerVideo_ = std::thread(&DisplayerVideo::run, &displayerVideo_);

    if (threadDemuxer_.joinable()) {
        threadDemuxer_.join();
    }
    if (threadDecoderVideo_.joinable()) {
        threadDecoderVideo_.join();
    }
    if (threadRendererVideo_.joinable()) {
        threadRendererVideo_.join();
    }
    if (threadDisplayerVideo_.joinable()) {
        threadDisplayerVideo_.join();
    }
}

void PlayerCore::stop()
{
    if (!state_.running) {
        return;
    }

    state_.running = false;
    state_.paused = false;
    
    if (threadDemuxer_.joinable()) {
        threadDemuxer_.join();
    }
    if (threadDecoderVideo_.joinable()) {
        threadDecoderVideo_.join();
    }
    if (threadRendererVideo_.joinable()) {
        threadRendererVideo_.join();
    }
    if (threadDisplayerVideo_.joinable()) {
        threadDisplayerVideo_.join();
    }

    queuePacketVideo_.flush();
    queuePacketAudio_.flush();
    queueFrameRaw_.flush();
    queueFrameDst_.flush();

    queuePacketVideo_.shutdown();
    queuePacketAudio_.shutdown();
    queueFrameRaw_.shutdown();
    queueFrameDst_.shutdown();
}

}