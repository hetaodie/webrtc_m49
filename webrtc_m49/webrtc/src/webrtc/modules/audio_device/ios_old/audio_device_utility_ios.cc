/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_device/audio_device_config.h"
#include "webrtc/modules/audio_device/ios_old/audio_device_utility_ios.h"

// #include "webrtc/system_wrappers/include/critical_section_wrapper.h"
#include "webrtc/system_wrappers/include/trace.h"

namespace webrtc2 {
AudioDeviceUtilityIOS::AudioDeviceUtilityIOS(const int32_t id)
:
    _critSect(*webrtc::CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _lastError(webrtc::AudioDeviceModule::kAdmErrNone) {
    webrtc::WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}

AudioDeviceUtilityIOS::~AudioDeviceUtilityIOS() {
    webrtc::WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    {
        webrtc::CriticalSectionScoped lock(&_critSect);
    }
    delete &_critSect;
}

int32_t AudioDeviceUtilityIOS::Init() {
    webrtc::WEBRTC_TRACE(webrtc::kTraceModuleCall, webrtc::kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    webrtc::WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceAudioDevice, _id,
                 "  OS info: %s", "iOS");

    return 0;
}

}  // namespace webrtc2
