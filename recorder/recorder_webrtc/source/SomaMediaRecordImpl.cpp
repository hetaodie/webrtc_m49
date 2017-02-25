/*
*  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*
*  Date: 2017/2/15
*/

#include "SomaMediaRecordImpl.h"
#include "SomaCommon.h"
#include "webrtc/base/bind.h"
#include "webrtc/base/logging.h"
#include <sys/timeb.h>

#ifdef WIN32
#define RECORD_IN_WORKTHREAD
#else
#define RECORD_IN_WORKTHREAD
#endif


static long long getLocalTimeMilliseconds(){
	timeb buf;
	ftime(&buf);
	return int64_t(buf.time) * 1000 + buf.millitm;
}

// SomaMediaRecord

SomaMediaRecord* SomaMediaRecord::CreateMediaRecord() {
	SomaMediaRecordImpl* media_record = new SomaMediaRecordImpl();
	return (SomaMediaRecord*)media_record;
}

void SomaMediaRecord::DestroyMediaRecord(SomaMediaRecord* media_record) {
	delete media_record;
	media_record = NULL;
}

long long SomaMediaRecord::TimeMs() {
	return getLocalTimeMilliseconds();
}

// SomaMediaRecordImpl
static int g_mediaRecordingApiNum = 1;
SomaMediaRecordImpl::SomaMediaRecordImpl()
: bInitilized_(false)
, media_format_(SomaMediaFormat_AVI)
, audio_started_(false)
, audio_record_(NULL)
, media_file_(NULL)
, audio_observer_(NULL)
 {
#ifdef RECORD_IN_WORKTHREAD
	std::string name = std::string("MediaRecordingApiThread") + std::to_string(g_mediaRecordingApiNum++);
	workerThread_.SetName(name.c_str(), NULL);
	workerThread_.Start();

#endif
}

SomaMediaRecordImpl::~SomaMediaRecordImpl() {
#ifdef RECORD_IN_WORKTHREAD
	workerThread_.Stop();
#endif
}

int SomaMediaRecordImpl::Init(const char* filename, SomaMediaType media_type, SomaMediaFormat media_format) {
#ifdef RECORD_IN_WORKTHREAD
	return workerThread_.Invoke<int>(RTC_FROM_HERE, rtc::Bind(&SomaMediaRecordImpl::Init_w, this, filename, media_type, media_format));
#else
	return Init_w(filename, media_type, record_mode, media_format, video_codec);
#endif
}

int SomaMediaRecordImpl::Init_w(const char* filename, SomaMediaType media_type, SomaMediaFormat media_format) {
	rtc::CritScope cs(&apicrit_);
	if (bInitilized_) {
		LOG_T_F(LS_INFO) << ("TbMediaRecordImpl has already been initilized.");
		return -1;
	}

	media_type_ = media_type;
	media_format_ = media_format;

	audio_record_ = new SomaAudioRecord(this);
	if (audio_record_ == NULL) {
		LOG_T_F(LS_INFO) << ("new TbAudioRecord failed.");
		return -1;
	}
	int res = audio_record_->Init();
	if (res != 0) {
		LOG_T_F(LS_INFO) << ("audio_record_->Init() failed.");
		return -1;
	}

	media_file_ = SomaMediaFile::Create(filename, media_format);
	if (media_file_ == NULL) {
		LOG_T_F(LS_INFO) << ("TbMediaFile::Create failed.");
		return -1;
	}

	bInitilized_ = true;
	LOG_T_F(LS_INFO) << ("TbMediaRecordImpl::Init_w Success.");
	return 0;
}

int SomaMediaRecordImpl::Init(SomaAudioObserver* callback) {
#ifdef RECORD_IN_WORKTHREAD
	return workerThread_.Invoke<int>(RTC_FROM_HERE, rtc::Bind(&SomaMediaRecordImpl::Init_w, this, callback));
#else
	return Init_w(callback, codec);
#endif
}

int SomaMediaRecordImpl::Init_w(SomaAudioObserver* callback) {
	rtc::CritScope cs(&apicrit_);
	if (bInitilized_) {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl has already been initilized.");
		return -1;
	}

	media_type_ = SomaMediaType_Audio;

	audio_record_ = new SomaAudioRecord(this);
	if (audio_record_ == NULL) {
		LOG_T_F(LS_INFO)<<("new TbAudioRecord failed.");
		return -1;
	}
	int res = audio_record_->Init();
	if (res != 0) {
		LOG_T_F(LS_INFO)<<("audio_record_->Init() failed.");
		return -1;
	}

	audio_observer_ = callback;
	if (audio_observer_ == NULL) {
		LOG_T_F(LS_INFO)<<("Set TbAudioCallback failed.");
		return -1;
	}

	bInitilized_ = true;
	LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::Init_w Success.");
	return 0;
}

int SomaMediaRecordImpl::Start() {
#ifdef RECORD_IN_WORKTHREAD
	return workerThread_.Invoke<int>(RTC_FROM_HERE, rtc::Bind(&SomaMediaRecordImpl::Start_w, this));
#else
	return Start_w();
#endif
}

int SomaMediaRecordImpl::Start_w() {
	rtc::CritScope cs(&apicrit_);
	if (!bInitilized_) {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl hasn't been initilized.");
		return -1;
	}

	if (StartAudioRecording_w() != 0) {
		return -1;
	}

	LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::Start_w Success.");
	return 0;
}

int SomaMediaRecordImpl::StartAudioRecording() {
#ifdef RECORD_IN_WORKTHREAD
	return workerThread_.Invoke<int>(RTC_FROM_HERE, rtc::Bind(&SomaMediaRecordImpl::StartAudioRecording_w, this));
#else
	return StartAudioRecording_w();
#endif
}

int SomaMediaRecordImpl::StartAudioRecording_w() {
	rtc::CritScope cs(&apicrit_);
	if (!bInitilized_) {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl hasn't been initilized.");
		return -1;
	}
	if (audio_started_) {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl audio already started.");
		return 0;
	}

	if (!audio_record_
		|| (audio_record_ && audio_record_->Start() != 0)
		|| 0 != StartFile()) {
		LOG_T_F(LS_INFO)<<("audio_record_->Start() failed.");
		return -1;
	}
	audio_started_ = true;
	LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::StartAudioRecording_w Success.");
	return 0;
}

int SomaMediaRecordImpl::ReceiveAudioPacket(int channelID, const void* data, int len) {
	rtc::TryCritScope cs(&apicrit_);
	if (!cs.locked()) {
		return 0;
	}
//#ifdef RECORD_IN_WORKTHREAD
//	return workerThread_.Invoke<int>(rtc::Bind(&TbMediaRecordImpl::ReceiveAudioPacket_w, this, channelID, data, len));
//#else
	return ReceiveAudioPacket_w(channelID, data, len);
//#endif
}

int SomaMediaRecordImpl::ReceiveAudioPacket_w(int channelID, const void* data, int len) {
	if (!bInitilized_) {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::ReceiveAudioPacket, TbMediaRecordImpl hasn't been initilized.");
		return -1;
	}

	if (!audio_started_) {
		LOG_T_F(LS_INFO) << ("TbMediaRecordImpl::ReceiveAudioPacket, Audio hasn't been start.");
		return -1;
	}

	if (len < 12) {
		LOG_T_F(LS_INFO) << ("TbMediaRecordImpl::ReceiveAudioPacket, the packet is not a valid rtp packet.");
		return -1;
	}

	if (audio_record_) {
		int res = audio_record_->ReceivePacket(channelID, data, len);
		if (res != 0) {
			LOG_T_F(LS_INFO) << ("audio_record_->ReceivePacket failed.");
			return -1;
		}
	}
	else {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::ReceiveAudioPacket, audio_record_ is null.");
		return -1;
	}

	return 0;
}

int SomaMediaRecordImpl::Stop() {
#ifdef RECORD_IN_WORKTHREAD
	return workerThread_.Invoke<int>(RTC_FROM_HERE, rtc::Bind(&SomaMediaRecordImpl::Stop_w, this));
#else
	return Stop_w();
#endif
}

int SomaMediaRecordImpl::Stop_w() {
	rtc::CritScope cs(&apicrit_);
	if (!bInitilized_) {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::Stop, TbMediaRecordImpl hasn't been initilized.");
		return -1;
	}
	StopAudioRecording_w();
	StopFile(audio_started_);
	LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::Stop_w Success.");
	return 0;
}

int SomaMediaRecordImpl::StopAudioRecording() {
#ifdef RECORD_IN_WORKTHREAD
	return workerThread_.Invoke<int>(RTC_FROM_HERE, rtc::Bind(&SomaMediaRecordImpl::StopAudioRecording_w, this));
#else
	return StopAudioRecording_w();
#endif
}

int SomaMediaRecordImpl::StopAudioRecording_w() {
	rtc::CritScope cs(&apicrit_);
	if (!bInitilized_) {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::StopAudioRecording_w, TbMediaRecordImpl hasn't been initilized.");
		return -1;
	}
	if (!audio_started_) {
		LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::StopAudioRecording_w, Audio hasn't started.");
		return -1;
	}
	if (!audio_record_ || (audio_record_ && audio_record_->Stop() != 0)) {
		return -1;
	}
	audio_started_ = false;
	StopFile(audio_started_);
	LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::StopAudioRecording_w Success.");
	return 0;
}

int SomaMediaRecordImpl::Destroy() {
#ifdef RECORD_IN_WORKTHREAD
    return workerThread_.Invoke<int>(RTC_FROM_HERE, rtc::Bind(&SomaMediaRecordImpl::Destroy_w, this));
#else
	return Destroy_w();
#endif
}

int SomaMediaRecordImpl::Destroy_w() {
	rtc::CritScope cs(&apicrit_);
	bInitilized_ = false;

	if (audio_record_) {
		audio_record_->Destroy();
		delete audio_record_;
		audio_record_ = NULL;
	}

	if (media_file_) {
		SomaMediaFile::Destroy(media_file_);
	}
	LOG_T_F(LS_INFO)<<("TbMediaRecordImpl::Destroy_w Success.");
	return 0;
}

// webrtc::OutStream

bool SomaMediaRecordImpl::Write(const void *buf, size_t len) {
	return Write_w(buf, len);
}

bool SomaMediaRecordImpl::Write_w(const void *buf, size_t len) {
	rtc::TryCritScope cs(&apicrit_);
	if (!cs.locked()) {
		return true;
	}
	//LOG_T_F(LS_INFO)<<(" Write_w, length = %d", len);
	if (media_file_ && buf && len > 0) {
		if (audio_started_) {
			int res = media_file_->IncomingAudioData(buf, len);
			if (res != 0) {
				return true;
			}
		}
	}

	return true;
}

int SomaMediaRecordImpl::Rewind() {
	return 0;
}

int SomaMediaRecordImpl::StartFile() {
	if (!media_file_ || (media_file_ && media_file_->Start() != 0)) {
		LOG_T_F(LS_ERROR) << ("media_file_->Start() failed.");
		return -1;
	}
	return 0;
}

int SomaMediaRecordImpl::StopFile(bool audioStart) {
	if (!audioStart) {
		if (!media_file_ || (media_file_ && media_file_->Stop() != 0)) {
			LOG_T_F(LS_ERROR) << ("media_file_->Stop() failed.");
			return -1;
		}
	}
	return 0;
}

