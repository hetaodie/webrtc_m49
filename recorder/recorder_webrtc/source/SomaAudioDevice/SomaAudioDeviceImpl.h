/*
 *  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
 *
 *  Author: Jiarong Yu
 *
 *  Date: 2017/2/15
 */

#ifndef WEBRTC_AUDIO_DEVICE_SOMA_AUDIO_DEVICE_IMPL_H
#define WEBRTC_AUDIO_DEVICE_SOMA_AUDIO_DEVICE_IMPL_H

#include "webrtc/modules/audio_device/audio_device_impl.h"

namespace webrtc {

class AudioDeviceGeneric;
class AudioManager;
class CriticalSectionWrapper;

class SomaAudioDeviceModuleImpl : public AudioDeviceModuleImpl {
 public:
  int32_t CreatePlatformSpecificObjects();

  SomaAudioDeviceModuleImpl(const int32_t id, const AudioLayer audioLayer);
  ~SomaAudioDeviceModuleImpl() override;
    
public:
    static rtc::scoped_refptr<AudioDeviceModule> Create(const int32_t id);
};

}  // namespace webrtc

#endif  // WEBRTC_AUDIO_DEVICE_SOMA_AUDIO_DEVICE_IMPL_H
