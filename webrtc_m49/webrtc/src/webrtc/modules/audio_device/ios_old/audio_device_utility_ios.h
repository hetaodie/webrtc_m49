/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_IOS_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_IOS_H

#include "webrtc/modules/audio_device/ios_old/audio_device_utility.h"
#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/system_wrappers/include/critical_section_wrapper.h"

namespace webrtc2 {
// class CriticalSectionWrapper;

class AudioDeviceUtilityIOS: public AudioDeviceUtility {
 public:
    AudioDeviceUtilityIOS(const int32_t id);
    AudioDeviceUtilityIOS();
    virtual ~AudioDeviceUtilityIOS();

    virtual int32_t Init();

 private:
    webrtc::CriticalSectionWrapper& _critSect;
    int32_t _id;
    webrtc::AudioDeviceModule::ErrorCode _lastError;
};

}  // namespace webrtc2

#endif  // WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_UTILITY_IOS_H
