// Deprecated

#include "FilterProcessor.hpp"

#include <sstream>

namespace bplayer 
{

FilterProcessor::FilterProcessor(FrameParameter& frameParSrc,
    FrameParameter& frameParSink)
    : frameParSrc_(frameParSrc), frameParSink_(frameParSink)
{

}

FilterProcessor::~FilterProcessor()
{
    reset();
}

bool FilterProcessor::init(const std::string& filterCmd)
{
    reset();

    filterGraph_ = avfilter_graph_alloc();
    if (!filterGraph_) {
        std::cerr << "[Filter Processor] Failed to allocate filter graph" << std::endl;
        return false;
    }
    std::ostringstream args;
    args << "video_size=" << frameParSrc_.width << "x" << frameParSrc_.height
         << ":pix_fmt=" << static_cast<int>(frameParSrc_.pixFmt)
         << ":time_base=" << 1 << "/" << 25
         << ":pixel_aspect=" << frameParSrc_.getSar().num << "/" << frameParSrc_.getSar().den;

    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");

    if (!buffersrc || !buffersink) {
        std::cerr << "[Filter Processor] Required filters not found" << std::endl;
        return false;
    }

    int ret = avfilter_graph_create_filter(&ctxFilterSrc_, 
        buffersrc, "in", args.str().c_str(), nullptr, filterGraph_);
    if (ret < 0) {
        std::cerr << "[Filter Processor] Failed to create buffer source: " 
            << ffmpegErrStr(ret) << std::endl;
        return false;
    }

    ret = avfilter_graph_create_filter(&ctxFilterSink_, 
        buffersink, "out", nullptr, nullptr, filterGraph_);
    if (ret < 0) {
        std::cerr << "[Filter Processor] Failed to create buffer sink: "
            << ffmpegErrStr(ret) << std::endl;
        return false;
    }

    AVPixelFormat pixFmts[] = {frameParSink_.pixFmt, AV_PIX_FMT_NONE};
    ret = av_opt_set_int_list(ctxFilterSink_, "pix_fmts", pixFmts, 
        AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        std::cerr << "[Filter Processor] Failed to set sink pixel format: " 
            << ffmpegErrStr(ret) << std::endl;
        return false;
    }

    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();

    outputs->name = av_strdup("in");
    outputs->filter_ctx = ctxFilterSrc_;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = ctxFilterSink_;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    ret = avfilter_graph_parse_ptr(filterGraph_, filterCmd.c_str(), &inputs, &outputs, nullptr);
    if (ret < 0) {
        std::cerr << "[Filter Processor] Failed to parse filter graph: "
            << ffmpegErrStr(ret) << std::endl;
        return false;
    }

    ret = avfilter_graph_config(filterGraph_, nullptr);
    if (ret < 0) {
        std::cerr << "[Filter Processor] Failed to configure filter graph: "
            << ffmpegErrStr(ret) << std::endl;
        return false;
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return true;
}

std::shared_ptr<AVFrame> FilterProcessor::process(std::shared_ptr<AVFrame> frameSrc) {
    if (!ctxFilterSrc_ || !ctxFilterSink_) {
        std::cerr << "[Filter Processor] Filter context not initialized" << std::endl;
        return nullptr;
    }

    int ret = av_buffersrc_add_frame_flags(ctxFilterSrc_, frameSrc.get(), AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        std::cerr << "[Filter Processor] Failed to feed frame: "
                  << ffmpegErrStr(ret) << std::endl;
        return nullptr;
    }

    auto frameOut = make_avframe();
    ret = av_buffersink_get_frame(ctxFilterSink_, frameOut.get());
    if (ret < 0) {
        std::cerr << "[Filter Processor] Failed to get filtered frame: "
                  << ffmpegErrStr(ret) << std::endl;
        return nullptr;
    }

    return frameOut;
}

void FilterProcessor::reset() {
    if (filterGraph_) {
        avfilter_graph_free(&filterGraph_);
        filterGraph_ = nullptr;
        ctxFilterSrc_ = nullptr;
        ctxFilterSink_ = nullptr;
    }
}

}  // namespace bplayer