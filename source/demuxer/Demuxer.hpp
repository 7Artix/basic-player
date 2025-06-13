#pragma once

#include "common.hpp"
#include "ffmpeg.hpp"

namespace bplayer {

class Demuxer {
public:
    Demuxer(BlockingQueue<std::shared_ptr<AVPacket>>& queuePacketVideo, 
        BlockingQueue<std::shared_ptr<AVPacket>>& queuePacketAudio,
        AVFormatContext*& ctxFormat, 
        AVStream*& streamVideo,
        AVStream*& streamAudio,
        PlayerState& state,
        PlayerConfig& config);
    ~Demuxer();

    bool init();
    void run();

    bool selectStreamVideoBest();
    bool selectStreamAudioBest();
    bool selectStreamSubtitleBest();
    bool selectStreamAllBest();

    // void selectStreamVideoManual(int index);
    // void selectStreamAudioManual(int index);
    // void selectStreamSubtitleManual(int index);

private:
    BlockingQueue<std::shared_ptr<AVPacket>>& queuePacketVideo_;
    BlockingQueue<std::shared_ptr<AVPacket>>& queuePacketAudio_;
    AVFormatContext*& ctxFormat_;

    PlayerState& state_;
    PlayerConfig& config_;

    AVStream*& streamVideo_;
    AVStream*& streamAudio_;

    int indexStreamVideo = -1;
    int indexStreamAudio = -1;
    int indexStreamSubtitle = -1;
    int indexStreamVideoBest = -1;
    int indexStreamAudioBest = -1;
    int indexStreamSubtitleBest = -1;

    void calculateStreamScore();
    
    template<typename U>
    void smartPush(U&& packet);
};

}