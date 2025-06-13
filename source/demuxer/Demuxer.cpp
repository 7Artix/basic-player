#include "Demuxer.hpp"

namespace bplayer
{

Demuxer::Demuxer(BlockingQueue<std::shared_ptr<AVPacket>>& queuePacketVideo, 
    BlockingQueue<std::shared_ptr<AVPacket>>& queuePacketAudio,
    AVFormatContext*& ctxFormat, 
    AVStream*& streamVideo,
    AVStream*& streamAudio,
    PlayerState& state, 
    PlayerConfig& config)
	: queuePacketVideo_(queuePacketVideo),
		queuePacketAudio_(queuePacketAudio),
        ctxFormat_(ctxFormat), 
        streamVideo_(streamVideo), 
        streamAudio_(streamAudio),
		state_(state),
        config_(config)
{

}

Demuxer::~Demuxer()
{

}

void Demuxer::calculateStreamScore()
{
    if (!ctxFormat_) {
        std::cerr << "[Demuxer] Stream Probe: open a file first" << std::endl;
    }
    AVStream* streamToCheck;
    AVCodecParameters* codecParToCheck;
    int scoreBestVideo = -1;
    int scoreBestAudio = -1;
    int scoreBestSubtitle = -1;
    for (unsigned int i=0; i < ctxFormat_->nb_streams; i++ ) {
        streamToCheck = ctxFormat_->streams[i];
        codecParToCheck = streamToCheck->codecpar;

        if (codecParToCheck->codec_type == AVMEDIA_TYPE_VIDEO) {
            int score = 0;
            if (streamToCheck->disposition & AV_DISPOSITION_DEFAULT) {
                score += 100;
            }
            score += codecParToCheck->width * codecParToCheck->height / 100;
            if (score > scoreBestVideo) {
                scoreBestVideo = score;
                indexStreamVideoBest = i;
            }
        } else if (codecParToCheck->codec_type == AVMEDIA_TYPE_AUDIO) {
            int score = 0;
            if (streamToCheck->disposition & AV_DISPOSITION_DEFAULT) {
                score += 100;
            }
            score += codecParToCheck->sample_rate / 1000;
            if (score > scoreBestAudio) {
                scoreBestAudio = score;
                indexStreamAudioBest = i;
            }
        } else if (codecParToCheck->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            int score = 0;
            if (streamToCheck->disposition & AV_DISPOSITION_DEFAULT) {
                score += 100;
            }
            if (score > scoreBestSubtitle) {
                scoreBestSubtitle = score;
                indexStreamSubtitleBest = i;
            }
        }
    }
}

template<typename U>
void Demuxer::smartPush(U&& packet) {
	if (!packet) {
		return;
	}

	int indexStream = packet->stream_index;

	if (indexStream == indexStreamVideo) {
		bool success = queuePacketVideo_.push(std::forward<U>(packet));
		if (!success) {
			if ((packet->flags & AV_PKT_FLAG_KEY) == 0) {
				std::cerr << "[Demuxer] Dropped non-key video packet (pts=" << packet->pts << ")" << std::endl;
			} else {
				queuePacketVideo_.push(std::forward<U>(packet));
			}
		}
	} else if (indexStream == indexStreamAudio) {
		bool success = queuePacketAudio_.push(std::forward<U>(packet));
		if (!success) {
			std::cerr << "[Demuxer] Dropped audio packet due to full queue (pts=" << packet->pts << ")" << std::endl;
		}
	}
}

bool Demuxer::selectStreamVideoBest()
{
    if (indexStreamVideoBest != -1) {
        indexStreamVideo = indexStreamVideoBest;
        streamVideo_ = ctxFormat_->streams[indexStreamVideo];
        return true;
    }
    return false;
}

bool Demuxer::selectStreamAudioBest()
{
    if (indexStreamAudioBest != -1) {
        indexStreamAudio = indexStreamAudioBest;
        streamAudio_ = ctxFormat_->streams[indexStreamAudio];
        return true;
    }
    return false;    
}

bool Demuxer::selectStreamSubtitleBest()
{
    if (indexStreamSubtitleBest != -1) {
        indexStreamSubtitle = indexStreamSubtitleBest;
        return true;
    }
    return false;
}

bool Demuxer::selectStreamAllBest()
{
    bool ret = selectStreamVideoBest() || selectStreamAudioBest();
    selectStreamSubtitleBest();
    return ret;
}

bool Demuxer::init()
{
    calculateStreamScore();
    return selectStreamAllBest();
}

void Demuxer::run()
{
    if (indexStreamVideo == -1 && indexStreamAudio == -1) {
        std::cerr << "[Demuxer] No valid streams found" << std::endl;
        return;
    }

	std::cout << "[Demuxer] Video stream time_base: " 
		<< ctxFormat_->streams[indexStreamVideo]->time_base.num
		<< "/" << ctxFormat_->streams[indexStreamVideo]->time_base.den
		<< std::endl;
	
	// std::cout << "[Demuxer] Audio stream time_base: " 
	// 	<< ctxFormat_->streams[indexStreamAudio]->time_base.num
	// 	<< "/" << ctxFormat_->streams[indexStreamAudio]->time_base.den
	// 	<< std::endl;

	while (state_.running.load()) {
		auto packet = make_avpacket();
		int ret = av_read_frame(ctxFormat_, packet.get());

		if (ret == AVERROR_EOF) {
			break;
		} else if (ret < 0) {
			char errBuf[256];
			av_strerror(ret, errBuf, sizeof(errBuf));
			std::cerr << "[Demuxer] Failed to read frame: " << errBuf << std::endl;
			break;
		}

		smartPush(std::move(packet));
	}
}

}
