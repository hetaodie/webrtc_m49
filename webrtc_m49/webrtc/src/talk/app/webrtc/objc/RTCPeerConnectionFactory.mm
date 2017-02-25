/*
 * libjingle
 * Copyright 2013 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "RTCPeerConnectionFactory+Internal.h"

#include <vector>

#import "RTCAudioTrack+Internal.h"
#import "RTCICEServer+Internal.h"
#import "RTCMediaConstraints+Internal.h"
#import "RTCMediaSource+Internal.h"
#import "RTCMediaStream+Internal.h"
#import "RTCMediaStreamTrack+Internal.h"
#import "RTCPeerConnection+Internal.h"
#import "RTCPeerConnectionDelegate.h"
#import "RTCPeerConnectionInterface+Internal.h"
#import "RTCVideoCapturer+Internal.h"
#import "RTCVideoSource+Internal.h"
#import "RTCVideoTrack+Internal.h"

#include "talk/app/webrtc/audiotrack.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/videotrack.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/ssladapter.h"

// begin, add by jiarong
#include "webrtc/video/video_capture_input.h"
#include "webrtc/modules/video_coding/utility/quality_scaler.h"
#include "webrtc/modules/audio_device/include/audio_device.h"
#include "talk/media/webrtc/constants.h"
#include "talk/app/webrtc/proxy.h"
#include "webrtc/p2p/base/stunport.h"//jay add
#include "webrtc/p2p/base/p2ptransportchannel.h"
#include "webrtc/base/physicalsocketserver.h"
// end

@implementation RTCPeerConnectionFactory {
  rtc::scoped_ptr<rtc::Thread> _signalingThread;
  rtc::scoped_ptr<rtc::Thread> _workerThread;
}

@synthesize nativeFactory = _nativeFactory;

// begin, add by jiarong
+ (void)setSendVideo:(BOOL)enable {
    webrtc::internal::VideoCaptureInput::setSendVideo(enable);
}

+ (void)setVideoMinBitrate:(int)minBitrate {
    cricket::kMinVideoBitrate = minBitrate;
}

+ (void)setVideoQp:(int)minQp
             maxQp:(int)maxQp {
    webrtc::QualityScaler::setMinQp(minQp);
    webrtc::QualityScaler::setMaxQp(maxQp);
}

+ (void)setDownscaleShift:(int)startDownscaleShift
        maxDownscaleShift:(int)maxDownscaleShift
        minDownscaleShift:(int)minDownscaleShift {
    webrtc::QualityScaler::setDownscaleShift(startDownscaleShift, maxDownscaleShift,
                                             minDownscaleShift);
}

+ (void)setResolutionThreshold:(int)lowQpThreshold
     frameDropPercentThreshold:(int)frameDropPercentThreshold {
    webrtc::QualityScaler::setResolutionThreshold(lowQpThreshold, frameDropPercentThreshold);
}

+ (NSString*)getRoutePath {
    return [NSString stringWithCString:cricket::P2PTransportChannel::getRoutePath().c_str() encoding:[NSString defaultCStringEncoding]];
}

+ (void)clearRoute {
    cricket::P2PTransportChannel::clearRoute();
}


int webrtc::internal::AsynchronousMethodCall::g_enablevoice_waittime = 0;

+ (void)setEnableVoiceWaitTime:(int)enablevoice_waittime {
    webrtc::internal::AsynchronousMethodCall::g_enablevoice_waittime = enablevoice_waittime;
}

bool webrtc::AudioDeviceModule::useNewAudioDevice = true;

+ (void)setUseNewAudioDevice:(BOOL)flag {
    webrtc::AudioDeviceModule::useNewAudioDevice = flag;
}

+ (void)setChannelRtpEncryptKey:(NSString*)aesKey {
    std::string strAesKey([aesKey UTF8String]);
    cricket::BaseChannel::setChannelRtpEncryptKey(strAesKey);
}

+ (void)setStunEncryptKey:(NSString*)aesKey
{
    std::string strAesKey([aesKey UTF8String]);
    cricket::UDPPort::setChannelRtpEncryptKey(strAesKey);
}

+ (void)setUdpEncryptKey:(NSString*)aesKey {
    std::string strAesKey([aesKey UTF8String]);
    rtc::PhysicalSocket::setUdpSocketEncryptKey(strAesKey);
}

+ (void)setUdpEncrypt2:(BOOL)flag {
    rtc::PhysicalSocket::udpEncrypt2_ = flag;
}

+ (void)startCall{
    //rtc::PhysicalSocket::startCall();
}

+ (void)addUdpCRCKey:(NSString*)key forAddress:(NSString*)addr andPort:(int)port{
    //rtc::PhysicalSocket::addUdpCRCKey([key UTF8String], [addr UTF8String], port);
}

// end

+ (void)initializeSSL {
  BOOL initialized = rtc::InitializeSSL();
  NSAssert(initialized, @"Failed to initialize SSL library");
}

+ (void)deinitializeSSL {
  BOOL deinitialized = rtc::CleanupSSL();
  NSAssert(deinitialized, @"Failed to deinitialize SSL library");
}

- (id)init {
  if ((self = [super init])) {
    _signalingThread.reset(new rtc::Thread());
    BOOL result = _signalingThread->Start();
    NSAssert(result, @"Failed to start signaling thread.");
    _workerThread.reset(new rtc::Thread());
    result = _workerThread->Start();
    NSAssert(result, @"Failed to start worker thread.");

    _nativeFactory = webrtc::CreatePeerConnectionFactory(
        _signalingThread.get(), _workerThread.get(), nullptr, nullptr, nullptr);
    NSAssert(_nativeFactory, @"Failed to initialize PeerConnectionFactory!");
    // Uncomment to get sensitive logs emitted (to stderr or logcat).
    // rtc::LogMessage::LogToDebug(rtc::LS_SENSITIVE);
  }
  return self;
}

- (RTCPeerConnection *)peerConnectionWithConfiguration:(RTCConfiguration *)configuration
                                           constraints:(RTCMediaConstraints *)constraints
                                              delegate:(id<RTCPeerConnectionDelegate>)delegate {
  return [[RTCPeerConnection alloc] initWithFactory:self.nativeFactory.get()
                                             config:configuration.nativeConfiguration
                                        constraints:constraints.constraints
                                           delegate:delegate];
}

- (RTCPeerConnection*)
    peerConnectionWithICEServers:(NSArray*)servers
                     constraints:(RTCMediaConstraints*)constraints
                        delegate:(id<RTCPeerConnectionDelegate>)delegate {
  webrtc::PeerConnectionInterface::IceServers iceServers;
  for (RTCICEServer* server in servers) {
    iceServers.push_back(server.iceServer);
  }
  RTCPeerConnection* pc =
      [[RTCPeerConnection alloc] initWithFactory:self.nativeFactory.get()
                                      iceServers:iceServers
                                     constraints:constraints.constraints];
  pc.delegate = delegate;
  return pc;
}

- (RTCMediaStream*)mediaStreamWithLabel:(NSString*)label {
  rtc::scoped_refptr<webrtc::MediaStreamInterface> nativeMediaStream =
      self.nativeFactory->CreateLocalMediaStream([label UTF8String]);
  return [[RTCMediaStream alloc] initWithMediaStream:nativeMediaStream];
}

- (RTCVideoSource*)videoSourceWithCapturer:(RTCVideoCapturer*)capturer
                               constraints:(RTCMediaConstraints*)constraints {
  if (!capturer) {
    return nil;
  }
  rtc::scoped_refptr<webrtc::VideoSourceInterface> source =
      self.nativeFactory->CreateVideoSource([capturer takeNativeCapturer],
                                            constraints.constraints);
  return [[RTCVideoSource alloc] initWithMediaSource:source];
}

- (RTCVideoTrack*)videoTrackWithID:(NSString*)videoId
                            source:(RTCVideoSource*)source {
  rtc::scoped_refptr<webrtc::VideoTrackInterface> track =
      self.nativeFactory->CreateVideoTrack([videoId UTF8String],
                                           source.videoSource);
  return [[RTCVideoTrack alloc] initWithMediaTrack:track];
}

- (RTCAudioTrack*)audioTrackWithID:(NSString*)audioId {
  rtc::scoped_refptr<webrtc::AudioTrackInterface> track =
      self.nativeFactory->CreateAudioTrack([audioId UTF8String], NULL);
  return [[RTCAudioTrack alloc] initWithMediaTrack:track];
}

@end
