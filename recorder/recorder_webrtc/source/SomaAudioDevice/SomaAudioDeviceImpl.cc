/*
 *  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
 *
 *  Author: Jiarong Yu
 *
 *  Date: 2017/2/15
 */

#include "webrtc/base/logging.h"
#include "webrtc/base/refcount.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "SomaAudioDeviceImpl.h"
#include "SomaAudioDeviceFactory.h"
#include "SomaAudioDevice.h"


namespace webrtc {

// ============================================================================
//                                   Static methods
// ============================================================================

// ----------------------------------------------------------------------------
//  AudioDeviceModule::Create()
// ----------------------------------------------------------------------------

rtc::scoped_refptr<AudioDeviceModule> SomaAudioDeviceModuleImpl::Create(
    const int32_t id) {
  LOG(INFO) << __FUNCTION__;
  // Create the generic ref counted (platform independent) implementation.
  rtc::scoped_refptr<SomaAudioDeviceModuleImpl> audioDevice(
      new rtc::RefCountedObject<SomaAudioDeviceModuleImpl>(id, AudioDeviceModule::AudioLayer::kSimAudio));

  // Ensure that the current platform is supported.
  if (audioDevice->CheckPlatform() == -1) {
    return nullptr;
  }

  // Create the platform-dependent implementation.
  if (audioDevice->CreatePlatformSpecificObjects() == -1) {
    return nullptr;
  }

  // Ensure that the generic audio buffer can communicate with the
  // platform-specific parts.
  if (audioDevice->AttachAudioBuffer() == -1) {
    return nullptr;
  }

  WebRtcSpl_Init();

  return audioDevice;
}

// ============================================================================
//                            Construction & Destruction
// ============================================================================

// ----------------------------------------------------------------------------
//  AudioDeviceModuleImpl - ctor
// ----------------------------------------------------------------------------

SomaAudioDeviceModuleImpl::SomaAudioDeviceModuleImpl(const int32_t id,
                                             const AudioLayer audioLayer)
    : AudioDeviceModuleImpl(id, audioLayer) {
}

// ----------------------------------------------------------------------------
//  CreatePlatformSpecificObjects
// ----------------------------------------------------------------------------

int32_t SomaAudioDeviceModuleImpl::CreatePlatformSpecificObjects() {
  AudioDeviceGeneric* ptrAudioDevice(NULL);

  ptrAudioDevice = SomaAudioDeviceFactory::CreateSomaAudioDevice(Id());
  if (ptrAudioDevice == NULL) {
    LOG(LERROR)
        << "unable to create the platform specific audio device implementation";
    return -1;
  }

  // Store valid output pointers
  //
  _ptrAudioDevice = ptrAudioDevice;

  return 0;
}

// ----------------------------------------------------------------------------
//  ~AudioDeviceModuleImpl - dtor
// ----------------------------------------------------------------------------

SomaAudioDeviceModuleImpl::~SomaAudioDeviceModuleImpl() {

}

}  // namespace webrtc
