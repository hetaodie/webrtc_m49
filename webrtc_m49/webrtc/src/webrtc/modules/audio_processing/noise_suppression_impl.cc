/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_processing/noise_suppression_impl.h"

#include "webrtc/modules/audio_processing/audio_buffer.h"
#if defined(WEBRTC_NS_FLOAT)
#include "webrtc/modules/audio_processing/ns/noise_suppression.h"
#define NS_CREATE WebRtcNs_Create
#define NS_FREE WebRtcNs_Free
#define NS_INIT WebRtcNs_Init
#define NS_SET_POLICY WebRtcNs_set_policy
typedef NsHandle NsState;
#elif defined(WEBRTC_NS_FIXED)
#include "webrtc/modules/audio_processing/ns/noise_suppression_x.h"
#define NS_CREATE WebRtcNsx_Create
#define NS_FREE WebRtcNsx_Free
#define NS_INIT WebRtcNsx_Init
#define NS_SET_POLICY WebRtcNsx_set_policy
typedef NsxHandle NsState;
#endif

namespace webrtc {
class NoiseSuppressionImpl::Suppressor {
 public:
  explicit Suppressor(int sample_rate_hz) {
    state_ = NS_CREATE();
    RTC_CHECK(state_);
    int error = NS_INIT(state_, sample_rate_hz);
    RTC_DCHECK_EQ(0, error);
  }
  ~Suppressor() {
    NS_FREE(state_);
  }
  NsState* state() { return state_; }
 private:
  NsState* state_ = nullptr;
  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(Suppressor);
};

NoiseSuppressionImpl::NoiseSuppressionImpl(rtc::CriticalSection* crit)
    : crit_(crit) {
  RTC_DCHECK(crit);
}

NoiseSuppressionImpl::~NoiseSuppressionImpl() {}

void NoiseSuppressionImpl::Initialize(size_t channels, int sample_rate_hz) {
  rtc::CritScope cs(crit_);
  channels_ = channels;
  sample_rate_hz_ = sample_rate_hz;
  std::vector<rtc::scoped_ptr<Suppressor>> new_suppressors;
  if (enabled_) {
    new_suppressors.resize(channels);
    for (size_t i = 0; i < channels; i++) {
      new_suppressors[i].reset(new Suppressor(sample_rate_hz));
    }
  }
  suppressors_.swap(new_suppressors);
  set_level(level_);
}

void NoiseSuppressionImpl::AnalyzeCaptureAudio(AudioBuffer* audio) {
  RTC_DCHECK(audio);
#if defined(WEBRTC_NS_FLOAT)
  rtc::CritScope cs(crit_);
  if (!enabled_) {
    return;
  }

  RTC_DCHECK_GE(160u, audio->num_frames_per_band());
  RTC_DCHECK_EQ(suppressors_.size(), audio->num_channels());
  for (size_t i = 0; i < suppressors_.size(); i++) {
    WebRtcNs_Analyze(suppressors_[i]->state(),
                     audio->split_bands_const_f(i)[kBand0To8kHz]);
  }
#endif
}

void NoiseSuppressionImpl::ProcessCaptureAudio(AudioBuffer* audio) {
  RTC_DCHECK(audio);
  rtc::CritScope cs(crit_);
  if (!enabled_) {
    return;
  }

  RTC_DCHECK_GE(160u, audio->num_frames_per_band());
  RTC_DCHECK_EQ(suppressors_.size(), audio->num_channels());
  for (size_t i = 0; i < suppressors_.size(); i++) {
#if defined(WEBRTC_NS_FLOAT)
    WebRtcNs_Process(suppressors_[i]->state(),
                     audio->split_bands_const_f(i),
                     audio->num_bands(),
                     audio->split_bands_f(i));
#elif defined(WEBRTC_NS_FIXED)
    WebRtcNsx_Process(suppressors_[i]->state(),
                      audio->split_bands_const(i),
                      audio->num_bands(),
                      audio->split_bands(i));
#endif
  }
}

int NoiseSuppressionImpl::Enable(bool enable) {
  rtc::CritScope cs(crit_);
  if (enabled_ != enable) {
    enabled_ = enable;
    Initialize(channels_, sample_rate_hz_);
  }
  return AudioProcessing::kNoError;
}

bool NoiseSuppressionImpl::is_enabled() const {
  rtc::CritScope cs(crit_);
  return enabled_;
}

int NoiseSuppressionImpl::set_level(Level level) {
  int policy = 1;
  switch (level) {
    case NoiseSuppression::kLow:
      policy = 0;
      break;
    case NoiseSuppression::kModerate:
      policy = 1;
      break;
    case NoiseSuppression::kHigh:
      policy = 2;
      break;
    case NoiseSuppression::kVeryHigh:
      policy = 3;
      break;
    default:
      RTC_NOTREACHED();
  }
  rtc::CritScope cs(crit_);
  level_ = level;
  for (auto& suppressor : suppressors_) {
    int error = NS_SET_POLICY(suppressor->state(), policy);
    RTC_DCHECK_EQ(0, error);
  }
  return AudioProcessing::kNoError;
}

NoiseSuppression::Level NoiseSuppressionImpl::level() const {
  rtc::CritScope cs(crit_);
  return level_;
}

float NoiseSuppressionImpl::speech_probability() const {
  rtc::CritScope cs(crit_);
#if defined(WEBRTC_NS_FLOAT)
  float probability_average = 0.0f;
  for (auto& suppressor : suppressors_) {
    probability_average +=
        WebRtcNs_prior_speech_probability(suppressor->state());
  }
  if (!suppressors_.empty()) {
    probability_average /= suppressors_.size();
  }
  return probability_average;
#elif defined(WEBRTC_NS_FIXED)
  // TODO(peah): Returning error code as a float! Remove this.
  // Currently not available for the fixed point implementation.
  return AudioProcessing::kUnsupportedFunctionError;
#endif
}
}  // namespace webrtc
