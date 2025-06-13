#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

namespace bplayer {

class Loader {
public:
    Loader(AVFormatContext*& ctxFormat);
    ~Loader();

    bool open(const std::string& path);

private:
    AVFormatContext*& ctxFormat_;
};

}