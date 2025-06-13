#include "DecoderVideo.hpp"

namespace bplayer
{

DecoderVideo::DecoderVideo(BlockingQueue<std::shared_ptr<AVPacket>>& queuePacket, 
    BlockingQueue<std::shared_ptr<AVFrame>>& queueFrame, 
    AVStream*& stream,
    PlayerState& state, 
    PlayerConfig& config, 
    FrameParameter& frameParSrc)
    : queuePacket_(queuePacket), 
        queueFrame_(queueFrame), 
        stream_(stream), 
        state_(state), 
        config_(config),
        frameParSrc_(frameParSrc)
{

}

DecoderVideo::~DecoderVideo()
{

}

bool DecoderVideo::init()
{
    if (stream_ == nullptr) {
        std::cerr << "[Decoder] AVStream is null" << std::endl;
        return false;
    }
    bool ret = openCodecVideo() && syncFramePar();
    return ret;
}

void DecoderVideo::run()
{
    // Lambda for checking the pts of a frame
    // If the frame doesn't contain a pts, pass the one of packet
    auto resolve_pts = [](const AVFrame* frame, const AVPacket* pkt) -> int64_t {
        if (frame->pts != AV_NOPTS_VALUE)
            return frame->pts;
        if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
            return frame->best_effort_timestamp;
        if (pkt && pkt->pts != AV_NOPTS_VALUE)
            return pkt->pts;
        return AV_NOPTS_VALUE;
    };

    while (state_.running.load()) {
        auto packet = make_avpacket();
        queuePacket_.pop(packet);

        int ret = avcodec_send_packet(ctxCodec_, packet.get());
        if (ret < 0) {
            char errBuf[256];
			av_strerror(ret, errBuf, sizeof(errBuf));
			std::cerr << "[Decoder] Failed to send packet: " << errBuf << std::endl;
			continue;
        }

        while (state_.running.load() && ret >= 0) {
            auto frame = make_avframe();
            ret = avcodec_receive_frame(ctxCodec_, frame.get());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0 ) {
                char errBuf[256];
                av_strerror(ret, errBuf, sizeof(errBuf));
                std::cerr << "[Decoder] Failed to receive a frame: " << errBuf << std::endl;
                break;
            }
            frame->pts = resolve_pts(frame.get(), packet.get());
            queueFrame_.push(std::move(frame));
        }
    }

    // Flush decoder for the rest frames
    avcodec_send_packet(ctxCodec_, nullptr);
    int ret = 0;
    while (ret >= 0) {
        auto frame = make_avframe();
        ret = avcodec_receive_frame(ctxCodec_, frame.get());
        if (ret < 0 ) {
            char errBuf[256];
            av_strerror(ret, errBuf, sizeof(errBuf));
            std::cerr << "[Decoder] Failed to flush decoder: " << errBuf << std::endl;
            break;
        }
        queueFrame_.push(std::move(frame));
    }
}

bool DecoderVideo::openCodecVideo()
{
    bool ret = false;
    switch (stream_->codecpar->codec_id)
    {
    case AV_CODEC_ID_H264:
        ret = openCodecVideoByName("h264_v4l2m2m");
        break;
    case AV_CODEC_ID_HEVC:
        ret = openCodecVideoByName("hevc_v4l2m2m");
        break;
    default:
        break;
    }

    if (ret == false) {
        codec_ = avcodec_find_decoder(stream_->codecpar->codec_id);
        if (!codec_) {
            std::cerr << "[Video Decoder] Failed to find soft decoder by type" << std::endl;
            return false;
        }
        ctxCodec_ = avcodec_alloc_context3(codec_);
        if (!ctxCodec_) {
            std::cerr << "[Video Decoder] Failed to create codec context" << std::endl;
            return false;
        }
        if (avcodec_parameters_to_context(ctxCodec_, stream_->codecpar) < 0) {
            std::cerr << "[Video Decoder] Failed to copy codec parameters" << std::endl;
            avcodec_free_context(&ctxCodec_);
            return false;
        }
        if (avcodec_open2(ctxCodec_, codec_, nullptr) < 0) {
            std::cerr << "[Video Decoder] Failed to open decoder" << std::endl;
            avcodec_free_context(&ctxCodec_);
            return false;
        }
        std::cout << "[Video Decoder] Using software decoder: " << codec_->name << std::endl;
        return true;
    } else {
        std::cout << "[Video Decoder] Using hardware decoder: " << codec_->name << std::endl;
        return true;
    }
}

bool DecoderVideo::openCodecVideoByName(const char* name)
{
    codec_ = avcodec_find_decoder_by_name(name);
    if (!codec_) {
        return false;
    }
    ctxCodec_ = avcodec_alloc_context3(codec_);
    if (ctxCodec_) {
        return false;
    }
    if (avcodec_parameters_to_context(ctxCodec_, stream_->codecpar) < 0) {
        avcodec_free_context(&ctxCodec_);
        return false;
    }
    if (avcodec_open2(ctxCodec_, codec_, nullptr) < 0) {
        avcodec_free_context(&ctxCodec_);
        return false;
    }
    return true;
}

bool DecoderVideo::syncFramePar()
{
    if (!ctxCodec_) {
        return false;
    }
    frameParSrc_.pixFmt = ctxCodec_->pix_fmt;
    frameParSrc_.width = ctxCodec_->width;
    frameParSrc_.height = ctxCodec_->height;
    return true;
}
    
}