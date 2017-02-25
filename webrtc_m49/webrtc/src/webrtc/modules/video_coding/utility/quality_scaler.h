/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_UTILITY_QUALITY_SCALER_H_
#define WEBRTC_MODULES_VIDEO_CODING_UTILITY_QUALITY_SCALER_H_

#include "webrtc/common_video/libyuv/include/scaler.h"
#include "webrtc/modules/video_coding/utility/moving_average.h"

namespace webrtc {
class QualityScaler {
 public:
  static const int kDefaultLowQpDenominator;
  static const int kDefaultMinDownscaleDimension;
  struct Resolution {
    int width;
    int height;
  };
  
  static void setMinQp(int minqp) {
    minQp_ = minqp;
  } // jiarong
  static int getMinQp() {
	  return minQp_;
  } // jiarong
  static void setMaxQp(int maxqp) {
    maxQp_ = maxqp;
  } // jiarong
  static int getMaxQp() {
    return maxQp_;
  } // jiarong
  
  static void setDownscaleShift(int start_downscale_shift, int max_downscale_shift,
    int min_downscale_shift) {
    start_downscale_shift_ = start_downscale_shift;
    max_downscale_shift_ = max_downscale_shift;
    min_downscale_shift_ = min_downscale_shift;
  } // jiarong
  
  static void setResolutionThreshold(int lowQpThreshold, int frameDropPercentThreshold) {
    lowQpThreshold_ = lowQpThreshold;
    frameDropPercentThreshold_ = frameDropPercentThreshold;
  } // jiarong

  QualityScaler();
  void Init(int low_qp_threshold,
            int high_qp_threshold,
            bool use_framerate_reduction);
  void SetMinResolution(int min_width, int min_height);
  void ReportFramerate(int framerate);
  void ReportQP(int qp);
  void ReportDroppedFrame();
  void Reset(int framerate, int bitrate, int width, int height);
  void OnEncodeFrame(const VideoFrame& frame);
  Resolution GetScaledResolution() const;
  const VideoFrame& GetScaledFrame(const VideoFrame& frame);
  int GetTargetFramerate() const;
  int downscale_shift() const { return downscale_shift_; }

 private:
  void AdjustScale(bool up);
  void ClearSamples();

  static int minQp_; // jiarong
  static int maxQp_; // jiarong
  static int start_downscale_shift_; // jiarong
  static int max_downscale_shift_; // jiarong
  static int min_downscale_shift_; // jiarong
  static int lowQpThreshold_; // jiarong
  static int frameDropPercentThreshold_; // jiarong

  Scaler scaler_;
  VideoFrame scaled_frame_;

  size_t num_samples_;
  int framerate_;
  int target_framerate_;
  int low_qp_threshold_;
  int high_qp_threshold_;
  MovingAverage<int> framedrop_percent_;
  MovingAverage<int> average_qp_;
  Resolution res_;

  int downscale_shift_;
  int framerate_down_;
  bool use_framerate_reduction_;
  int min_width_;
  int min_height_;
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_VIDEO_CODING_UTILITY_QUALITY_SCALER_H_