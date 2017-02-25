/*
*  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*
*  Date: 2017/2/15
*/

#ifndef _SOMA_MEDIARECORD_SOMAMEDIARECORDIMPL_H_
#define _SOMA_MEDIARECORD_SOMAMEDIARECORDIMPL_H_

#include "SomaMediaRecord.h"
#include "SomaAudioRecord.h"
#include "SomaMediaFile.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncinvoker.h"

class SomaMediaRecordImpl :
	public SomaMediaRecord,
	public webrtc::OutStream
{
public:
	SomaMediaRecordImpl();

	int Init(const char* filename, SomaMediaType media_type, SomaMediaFormat media_format);
	int Init(SomaAudioObserver* callback);
	int Start();
	int StartAudioRecording();
	int ReceiveAudioPacket(int channelID, const void* data, int len);
	int Stop();
	int StopAudioRecording();
	int Destroy();

	// webrtc::OutStrea
	virtual bool Write(const void *buf, size_t len);
	virtual int Rewind();

private:
	int Init_w(const char* filename, SomaMediaType media_type, SomaMediaFormat media_format);
	int Init_w(SomaAudioObserver* callback);
	int Start_w();
	int StartAudioRecording_w();
	int Stop_w();
	int StopAudioRecording_w();
	int StartFile();
	int StopFile(bool audioStart);
	int ReceiveAudioPacket_w(int channelID, const void* data, int len);
	int Destroy_w();
	bool Write_w(const void *buf, size_t len);
//	int Rewind_w();

protected:
	virtual ~SomaMediaRecordImpl();

protected:
	bool bInitilized_;
	SomaMediaType	media_type_;
	SomaMediaFormat	media_format_;
	bool audio_started_;
	SomaAudioRecord* audio_record_;
	SomaMediaFile* media_file_;

	SomaAudioObserver* audio_observer_;

	rtc::Thread workerThread_;
	rtc::CriticalSection apicrit_;
	//	rtc::AsyncInvoker asyncInvoker_;
};

#endif // _SOMA_MEDIARECORD_SOMAMEDIARECORDIMPL_H_
