/*
*  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*
*  Date: 2017/2/15
*/

#include "SomaAudioRecord.h"
#include "SomaCommon.h"
#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/base/bind.h"
#include "webrtc/base/logging.h"
#include "SomaAudioDevice/SomaAudioDeviceImpl.h"

#define VALIDATE                                            \
if (res != 0) {\
	LOG_T_F(LS_INFO)<<("*** Error at line");            \
	LOG_T_F(LS_INFO)<<"*** Error code = " << ptrVoEBase_->LastError();  \
}

void SomaVoeObserver::CallbackOnError(int channel, int err_code) {
	// Add LOG_T_F(LS_INFO)<< for other error codes here
	if (err_code == VE_TYPING_NOISE_WARNING) {
		LOG_T_F(LS_INFO)<<("  TYPING NOISE DETECTED ");
	}
	else if (err_code == VE_TYPING_NOISE_OFF_WARNING) {
		LOG_T_F(LS_INFO)<<("  TYPING NOISE OFF DETECTED ");
	}
	else if (err_code == VE_RECEIVE_PACKET_TIMEOUT) {
		LOG_T_F(LS_INFO)<<("  RECEIVE PACKET TIMEOUT ");
	}
	else if (err_code == VE_PACKET_RECEIPT_RESTARTED) {
		LOG_T_F(LS_INFO)<<("  PACKET RECEIPT RESTARTED ");
	}
	else if (err_code == VE_RUNTIME_PLAY_WARNING) {
		LOG_T_F(LS_INFO)<<("  RUNTIME PLAY WARNING ");
	}
	else if (err_code == VE_RUNTIME_REC_WARNING) {
		LOG_T_F(LS_INFO)<<("  RUNTIME RECORD WARNING ");
	}
	else if (err_code == VE_SATURATION_WARNING) {
		LOG_T_F(LS_INFO)<<("  SATURATION WARNING ");
	}
	else if (err_code == VE_RUNTIME_PLAY_ERROR) {
		LOG_T_F(LS_INFO)<<("  RUNTIME PLAY ERROR ");
	}
	else if (err_code == VE_RUNTIME_REC_ERROR) {
		LOG_T_F(LS_INFO)<<("  RUNTIME RECORD ERROR ");
	}
	else if (err_code == VE_REC_DEVICE_REMOVED) {
		LOG_T_F(LS_INFO)<<("  RECORD DEVICE REMOVED ");
	}
}


int SomaAudioRecord::Init() {
	if (bInitilized_)
		return -1;
	startThread_ = rtc::Thread::Current();
	int res = 0;

	ptrVoe_ = webrtc::VoiceEngine::Create();
	if (ptrVoe_ == NULL) {
		LOG_T_F(LS_INFO)<<("webrtc::VoiceEngine::Create() failed.");
		return -1;
	}
	ptrVoEBase_ = webrtc::VoEBase::GetInterface(ptrVoe_);
	if (ptrVoEBase_ == NULL) {
		LOG_T_F(LS_INFO)<<("webrtc::VoEBase::GetInterface failed.");
		return -1;
	}
	ptrVoENetwork_ = webrtc::VoENetwork::GetInterface(ptrVoe_);
	if (ptrVoENetwork_ == NULL) {
		LOG_T_F(LS_INFO)<<("webrtc::VoENetwork::GetInterface failed.");
		return -1;
	}
	ptrVoERtpRtcp_ = webrtc::VoERTP_RTCP::GetInterface(ptrVoe_);
	if (ptrVoERtpRtcp_ == NULL) {
		LOG_T_F(LS_INFO)<<("webrtc::VoERTP_RTCP::GetInterface failed.");
		return -1;
	}
	ptrVoEFile_ = webrtc::VoEFile::GetInterface(ptrVoe_);
	if (ptrVoEFile_ == NULL) {
		LOG_T_F(LS_INFO)<<("webrtc::VoEFile::GetInterface failed.");
		return -1;
	}

	LOG_T_F(LS_INFO)<<("Init");
	//webrtc::AudioDeviceModule*adm =
	//	webrtc::CreateAudioDeviceModule(0, webrtc::AudioDeviceModule::AudioLayer::kSimAudio);
	webrtc::AudioDeviceModule*adm = webrtc::SomaAudioDeviceModuleImpl::Create(0);
	res = ptrVoEBase_->Init(adm);
	//	res = ptrVoEBase_->Init();
	if (res != 0) {
		LOG_T_F(LS_INFO)<<"Error calling Init: Error = "<<ptrVoEBase_->LastError();
		exit(1);
	}

	res = ptrVoEBase_->RegisterVoiceEngineObserver(observer_);
	VALIDATE;

	char tmp[1024];
	res = ptrVoEBase_->GetVersion(tmp);
	VALIDATE;
	LOG_T_F(LS_INFO)<<("Version = ")<< tmp;

	bInitilized_ = true;
	return 0;
}

int SomaAudioRecord::Start() {
	if (!bInitilized_)
		return -1;

	if (ptrVoEFile_) {
		//weixu:unused variable error
		//webrtc::CodecInst audioCodec = kSomaAudioCodecInst;
		webrtc::CodecInst* pAudioCodec = NULL;
		
		if (0 != ptrVoEFile_->StartRecordingPlayout(-1, outStream_, pAudioCodec)) {
			LOG_T_F(LS_INFO)<<("TbAudioRecord::Start(), ptrVoEFile_->StartRecordingPlayout failed.");
			return -1;
		}
	}

	return 0;
}

int SomaAudioRecord::ReceivePacket(int channelID, const void *data, int len) {
	if (!bInitilized_)
		return -1;
	if ((unsigned long)channelID == channels_.size()) {
		return CreateChannel();
	}
	else if ((unsigned long)channelID > channels_.size()) {
		LOG_T_F(LS_INFO) << "ptrVoENetwork_->RegisterExternalTransport, channelID "  << channelID << " not big than ." << channels_.size();
		return -1;
	}

	return ptrVoENetwork_->ReceivedRTPPacket(channels_[channelID], data, len);
}

int SomaAudioRecord::Stop() {
	if (!bInitilized_)
		return -1;

	int res = 0;

	for (unsigned long i = 0; i < channels_.size(); ++i) {
		if (ptrVoEBase_)
			res = ptrVoEBase_->StopPlayout(channels_[i]);
		VALIDATE;
		if (ptrVoEBase_)
			res = ptrVoEBase_->StopReceive(channels_[i]);
		VALIDATE;
	}

	if (ptrVoEFile_)
		ptrVoEFile_->StopRecordingPlayout(-1);

	return 0;
}

int SomaAudioRecord::Destroy() {
	if (!bInitilized_)
		return -1;

	bInitilized_ = false;

	int res = 0;

	for (unsigned long i = 0; i < channels_.size(); ++i) {
		ptrVoENetwork_->DeRegisterExternalTransport(channels_[i]);
		res = ptrVoEBase_->DeleteChannel(channels_[i]);
		VALIDATE;
	}

	ptrVoEBase_->DeRegisterVoiceEngineObserver();

	res = ptrVoEBase_->Terminate();
	VALIDATE;

	ptrVoEBase_->Release();
	ptrVoENetwork_->Release();
	ptrVoERtpRtcp_->Release();
	ptrVoEFile_->Release();

	webrtc::VoiceEngine::Delete(ptrVoe_);

	return 0;
}

int SomaAudioRecord::CreateChannel() {
	if (!startThread_->IsCurrent()) {
		return startThread_->Invoke<int>(RTC_FROM_HERE, rtc::Bind(&SomaAudioRecord::CreateChannel, this));
	}
	int channel = ptrVoEBase_->CreateChannel();
	if (channel < 0) {
		LOG_T_F(LS_INFO)<<"************ Error code = "<< ptrVoEBase_->LastError();
		return -1;
	}
	channels_.push_back(channel);

	int res = ptrVoENetwork_->RegisterExternalTransport(channel, transport_);
	if (res != 0) {
		LOG_T_F(LS_INFO)<<("ptrVoENetwork_->RegisterExternalTransport failed.");
		return -1;
	}

	if (ptrVoEBase_) {
		res = ptrVoEBase_->StartReceive(channel);
		VALIDATE;
	}

	if (ptrVoEBase_) {
		res = ptrVoEBase_->StartPlayout(channel);
		VALIDATE;
	}
	return 0;
}






