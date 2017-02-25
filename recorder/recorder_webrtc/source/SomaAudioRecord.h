/*
*  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*  
*  Date: 2017/2/15
*/

#ifndef _SOMA_MEDIARECORD_SOMAAUDIORECORD_H_
#define _SOMA_MEDIARECORD_SOMAAUDIORECORD_H_

#include "webrtc/transport.h"
#include "webrtc/base/basictypes.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/include/voe_file.h"
#include "webrtc/voice_engine/include/voe_network.h"
#include "webrtc/voice_engine/include/voe_rtp_rtcp.h"
#include "webrtc/voice_engine/include/voe_errors.h"
#include "webrtc/base/thread.h"

class SomaVoeObserver : public webrtc::VoiceEngineObserver {
public:
	virtual void CallbackOnError(int channel, int err_code);
};

class SomaVoeTransport : public webrtc::Transport {
    virtual bool SendRtp(const uint8_t* packet,
                         size_t length,
                         const webrtc::PacketOptions& options) {
        return true;
    }
    
    virtual bool SendRtcp(const uint8_t* packet, size_t length) {
        return true;
    }
};

class SomaAudioRecord
{
public:
	SomaAudioRecord(webrtc::OutStream *outStream)
		: bInitilized_(false), outStream_(outStream),
		ptrVoe_(NULL), ptrVoEBase_(NULL), ptrVoENetwork_(NULL),
		ptrVoEFile_(NULL), ptrVoERtpRtcp_(NULL),  startThread_(NULL)
	{
	}

	virtual ~SomaAudioRecord()
	{
	}

	int Init();
	int Start();
	int ReceivePacket(int channelID, const void *data, int len);
	int Stop();
	int Destroy();
	int CreateChannel();

private:
	bool bInitilized_;
	webrtc::OutStream *outStream_;

	webrtc::VoiceEngine* ptrVoe_;
	webrtc::VoEBase* ptrVoEBase_;
	webrtc::VoENetwork* ptrVoENetwork_;
	webrtc::VoEFile* ptrVoEFile_;
	webrtc::VoERTP_RTCP* ptrVoERtpRtcp_;

	std::vector<int> channels_;
	SomaVoeTransport transport_;
	SomaVoeObserver observer_;
	rtc::Thread*	startThread_;
};

#endif // _SOMA_MEDIARECORD_SOMAAUDIORECORD_H_

