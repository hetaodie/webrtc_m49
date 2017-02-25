/*
*  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*
*  Date: 2017/2/15
*/

#include "SomaAviFile.h"
#include "SomaCommon.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/base/logging.h"


SomaAviFile::SomaAviFile(const char* filename)
: media_file_(NULL), filename_(filename)
{
}

SomaAviFile::~SomaAviFile()
{
}

int SomaAviFile::Start()
{
	
	if (media_file_ == NULL)
	{
		media_file_ = webrtc::MediaFile::CreateMediaFile(0);
	}
	if (media_file_ == NULL)
	{
		return -1;
	}
	if (media_file_->IsRecording()) {
		LOG_T_F(LS_INFO)<<("AVIFile already started.");
		return 0;
	}
	webrtc::CodecInst audioCodec;
	memset(&audioCodec, 0, sizeof(audioCodec));
	audioCodec = kSomaAudioCodecInst;

	return 0;
}

int SomaAviFile::Stop()
{
	if (media_file_) {
		webrtc::MediaFile::DestroyMediaFile(media_file_);
		media_file_ = NULL;
		return 0;
	}
	else {
		return -1;
	}
}

int SomaAviFile::IncomingAudioData(const void *data, size_t len)
{
	if (media_file_ && media_file_->IsRecording()) {
		return media_file_->IncomingAudioData((int8_t*)data, len);
	}
	else {
		return -1;
	}
}


