#include "SaveFrame.hpp"

#include <fstream>

namespace bplayer
{

static AVCodecID detectCodecByExt(const std::string& filename)
{
    auto extIndex = filename.find_last_of('.');
    if (extIndex == std::string::npos) {
        return AV_CODEC_ID_NONE;
    }

    std::string ext = filename.substr(extIndex + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "jpeg" || ext == "jpg") {
        return AV_CODEC_ID_MJPEG;
    }
    if (ext == "png") {
        return AV_CODEC_ID_PNG;
    }

    return AV_CODEC_ID_NONE;
}

bool saveFrame(const AVFrame* frame, const std::string& filename)
{
    if (!frame) {
        std::cerr << "[SaveFrame] Invalid frame" << std::endl;
        return false;
    }

    AVCodecID codecID = detectCodecByExt(filename);
    if (codecID == AV_CODEC_ID_NONE) {
        std::cerr << "[SaveFrame] Unsupported file extension in: " << filename << std::endl;
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder(codecID);
    if (!codec) {
        std::cerr << "[SaveFrame] Failed to find encoder" << std::endl;
        return false;
    }

    AVCodecContext* ctxCodec = avcodec_alloc_context3(codec);
    if (!ctxCodec) {
        std::cerr << "[SaveFrame] Failed to create codec context" << std::endl;
        return false;
    }

    ctxCodec->bit_rate = 400000;
    ctxCodec->width = frame->width;
    ctxCodec->height = frame->height;
    ctxCodec->time_base = AVRational{1, 25};
    ctxCodec->pix_fmt = (codecID == AV_CODEC_ID_PNG) ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_YUVJ420P;

    if (avcodec_open2(ctxCodec, codec, nullptr) < 0) {
        std::cerr << "[SaveFrame] Failed to open codec" << std::endl;
        avcodec_free_context(&ctxCodec);
        return false;
    }

    auto converted = make_avframe();
    converted->format = ctxCodec->pix_fmt;
    converted->width = ctxCodec->width;
    converted->height = ctxCodec->height;
    av_frame_get_buffer(converted.get(), 0);
    av_frame_make_writable(converted.get());

    SwsContext* ctxScaler = sws_getContext(
        frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
        converted->width, converted->height, static_cast<AVPixelFormat>(converted->format),
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );

    sws_scale(ctxScaler, frame->data, frame->linesize, 0, frame->height,
              converted->data, converted->linesize);
    sws_freeContext(ctxScaler);

    auto packet = make_avpacket();

    int ret = avcodec_send_frame(ctxCodec, converted.get());
    if (ret < 0) {
        std::cerr << "[SaveFrame] Failed to send frame!" << std::endl;
        avcodec_free_context(&ctxCodec);
        return false;
    }

    ret = avcodec_receive_packet(ctxCodec, packet.get());
    if (ret < 0) {
        std::cerr << "[SaveFrame] Failed to receive packet!" << std::endl;
        avcodec_free_context(&ctxCodec);
        return false;
    }

    std::ofstream out(filename, std::ios::binary);
    out.write(reinterpret_cast<const char*>(packet->data), packet->size);
    out.close();

    avcodec_free_context(&ctxCodec);
    return true;
}

}