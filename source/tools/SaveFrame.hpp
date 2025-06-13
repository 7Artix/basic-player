#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

namespace bplayer
{

static AVCodecID detectCodecByExt(const std::string& filename);

bool saveFrame(const AVFrame* frame, const std::string& filename);

}