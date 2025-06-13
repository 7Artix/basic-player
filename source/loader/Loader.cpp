#include "Loader.hpp"

namespace bplayer {

Loader::Loader(AVFormatContext*& ctxFormat)
    :ctxFormat_(ctxFormat)
{

}

Loader::~Loader()
{

}

bool Loader::open(const std::string& path)
{
    if (ctxFormat_) {
        std::cerr << "[Loader] Format context exist already" << std::endl;
        return false;
    }
    
    int ret = avformat_open_input(&ctxFormat_, path.c_str(), nullptr, nullptr);
    
    if (ret < 0) {
        char errBuf[256];
        av_strerror(ret, errBuf, sizeof(errBuf));
        std::cerr << "[Loader] Failed to open file: " << errBuf << std::endl;
        return false;
    }

    ret = avformat_find_stream_info(ctxFormat_, nullptr);
    if (ret < 0) {
        char errBuf[256];
        av_strerror(ret, errBuf, sizeof(errBuf));
        std::cerr << "[Loader] Failed to find stream info: " << errBuf << std::endl;
        return false;
    }
    
    return true;
}

}