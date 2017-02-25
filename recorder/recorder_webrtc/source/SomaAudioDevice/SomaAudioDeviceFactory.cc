/*
 *  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
 *
 *  Author: Jiarong Yu
 *
 *  Date: 2017/2/15
 */

#include "SomaAudioDeviceFactory.h"

#include <cstdlib>
#include <cstring>

#include "webrtc/base/logging.h"
#include "SomaAudioDevice.h"

namespace webrtc {

SomaAudioDevice* SomaAudioDeviceFactory::CreateSomaAudioDevice(
    const int32_t id) {
  return new SomaAudioDevice(id);
}

}  // namespace webrtc
