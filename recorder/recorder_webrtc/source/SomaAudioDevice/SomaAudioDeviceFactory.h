/*
 *  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
 *
 *  Author: Jiarong Yu
 *
 *  Date: 2017/2/15
 */

#ifndef WEBRTC_AUDIO_DEVICE_SOMA_AUDIO_DEVICE_FACTORY_H
#define WEBRTC_AUDIO_DEVICE_SOMA_AUDIO_DEVICE_FACTORY_H

#include "webrtc/common_types.h"

namespace webrtc {

class SomaAudioDevice;

// This class is used by audio_device_impl.cc when WebRTC is compiled with
// WEBRTC_DUMMY_FILE_DEVICES. The application must include this file and set the
// filenames to use before the audio device module is initialized. This is
// intended for test tools which use the audio device module.
class SomaAudioDeviceFactory {
 public:
  static SomaAudioDevice* CreateSomaAudioDevice(const int32_t id);
};

}  // namespace webrtc

#endif  // WEBRTC_AUDIO_DEVICE_SOMA_AUDIO_DEVICE_FACTORY_H
