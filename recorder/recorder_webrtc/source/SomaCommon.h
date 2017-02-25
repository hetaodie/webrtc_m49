/*
*  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*
*  Date: 2017/2/15
*/

#ifndef _SOMA_MEDIARECORD_SOMACOMMON_H_
#define _SOMA_MEDIARECORD_SOMACOMMON_H_

#include "webrtc/common_types.h"


// *********************************
//	Audio CodecInst
// *********************************
const webrtc::CodecInst kSomaPcmu = { 0, "PCMU", 8000, 160, 1, 64000 };
const webrtc::CodecInst kSomaPcma = { 8, "PCMA", 8000, 160, 1, 64000 };
const webrtc::CodecInst kSomaPcmuStereo = { 110, "PCMU", 8000, 160, 2, 64000 };
const webrtc::CodecInst kSomaPcmaStereo = { 118, "PCMA", 8000, 160, 2, 64000 };
const webrtc::CodecInst kSomaIsacWb = { 103, "ISAC", 16000, 480, 1, 32000 };
const webrtc::CodecInst kSomaIsacSwb = { 104, "ISAC", 32000, 960, 1, 56000 };
const webrtc::CodecInst kSomaIlbc = { 102, "ILBC", 8000, 240, 1, 13300 };
const webrtc::CodecInst kSomaOpus = { 120, "opus", 48000, 960, 1, 64000 };
const webrtc::CodecInst kSomaOpusStereo = { 120, "opus", 48000, 960, 2, 64000 };

const webrtc::CodecInst kSomaAudioCodecInst = kSomaPcmu;

#endif // _SOMA_MEDIARECORD_SOMACOMMON_H_
