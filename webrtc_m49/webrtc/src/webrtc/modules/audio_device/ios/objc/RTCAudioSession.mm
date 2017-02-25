/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "webrtc/modules/audio_device/ios/objc/RTCAudioSession.h"

#include "webrtc/base/atomicops.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/modules/audio_device/ios/audio_device_ios.h"

#import "webrtc/base/objc/RTCLogging.h"
#import "webrtc/modules/audio_device/ios/objc/RTCAudioSession+Private.h"

#import <UIKit/UIKit.h>

NSString * const kRTCAudioSessionErrorDomain = @"org.webrtc.RTCAudioSession";
NSInteger const kRTCAudioSessionErrorLockRequired = -1;
NSInteger const kRTCAudioSessionErrorConfiguration = -2;

// This class needs to be thread-safe because it is accessed from many threads.
// TODO(tkchin): Consider more granular locking. We're not expecting a lot of
// lock contention so coarse locks should be fine for now.
@implementation RTCAudioSession {
  rtc::CriticalSection _crit;
  AVAudioSession *_session;
  volatile int _activationCount;
  volatile int _lockRecursionCount;
  volatile int _webRTCSessionCount;
  BOOL _isActive;
  BOOL _shouldDelayAudioConfiguration;
}

@synthesize session = _session;
@synthesize delegates = _delegates;
@synthesize savedConfiguration = _savedConfiguration;

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  static RTCAudioSession *sharedInstance = nil;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

- (instancetype)init {
  if (self = [super init]) {
    _session = [AVAudioSession sharedInstance];

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(handleInterruptionNotification:)
                   name:AVAudioSessionInterruptionNotification
                 object:nil];
    [center addObserver:self
               selector:@selector(handleRouteChangeNotification:)
                   name:AVAudioSessionRouteChangeNotification
                 object:nil];
    // TODO(tkchin): Maybe listen to SilenceSecondaryAudioHintNotification.


//      if (([[[UIDevice currentDevice] systemVersion] compare:@"7.0" options:NSNumericSearch] != NSOrderedAscending)) {
//          [center addObserver:self
//                     selector:@selector(handleMediaServicesWereLost:)
//                         name:AVAudioSessionMediaServicesWereLostNotification
//                       object:nil];
//      }
      

    
    [center addObserver:self
               selector:@selector(handleMediaServicesWereReset:)
                   name:AVAudioSessionMediaServicesWereResetNotification
                 object:nil];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (NSString *)description {
  NSString *format =
      @"RTCAudioSession: {\n"
       "  isActive: %d\n"
       "  sampleRate: %.2f\n"
       "  IOBufferDuration: %f\n"
       "  outputNumberOfChannels: %ld\n"
       "  inputNumberOfChannels: %ld\n"
       "  outputLatency: %f\n"
       "  inputLatency: %f\n"
       "}";
  NSString *description = [NSString stringWithFormat:format,
      self.isActive, self.sampleRate, self.IOBufferDuration,
      self.outputNumberOfChannels, self.inputNumberOfChannels,
      self.outputLatency, self.inputLatency];
  return description;
}

- (void)setIsActive:(BOOL)isActive {
  @synchronized(self) {
    _isActive = isActive;
  }
}

- (BOOL)isActive {
  @synchronized(self) {
    return _isActive;
  }
}

- (BOOL)isLocked {
  return _lockRecursionCount > 0;
}

- (void)setShouldDelayAudioConfiguration:(BOOL)shouldDelayAudioConfiguration {
  @synchronized(self) {
    // No one should be changing this while an audio device is active.
    RTC_DCHECK(!self.isConfiguredForWebRTC);
    if (_shouldDelayAudioConfiguration == shouldDelayAudioConfiguration) {
      return;
    }
    _shouldDelayAudioConfiguration = shouldDelayAudioConfiguration;
  }
}

- (BOOL)shouldDelayAudioConfiguration {
  @synchronized(self) {
    return _shouldDelayAudioConfiguration;
  }
}

// TODO(tkchin): Check for duplicates.
- (void)addDelegate:(id<RTCAudioSessionDelegate>)delegate {
  if (!delegate) {
    return;
  }
  @synchronized(self) {
    _delegates.push_back(delegate);
    [self removeZeroedDelegates];
  }
}

- (void)removeDelegate:(id<RTCAudioSessionDelegate>)delegate {
  if (!delegate) {
    return;
  }
  @synchronized(self) {
    _delegates.erase(std::remove(_delegates.begin(),
                                 _delegates.end(),
                                 delegate));
    [self removeZeroedDelegates];
  }
}

- (void)lockForConfiguration {
  _crit.Enter();
  rtc::AtomicOps::Increment(&_lockRecursionCount);
}

- (void)unlockForConfiguration {
  // Don't let threads other than the one that called lockForConfiguration
  // unlock.
  if (_crit.TryEnter()) {
    rtc::AtomicOps::Decrement(&_lockRecursionCount);
    // One unlock for the tryLock, and another one to actually unlock. If this
    // was called without anyone calling lock, we will hit an assertion.
    _crit.Leave();
    _crit.Leave();
  }
}

#pragma mark - AVAudioSession proxy methods

- (NSString *)category {
  return self.session.category;
}

- (AVAudioSessionCategoryOptions)categoryOptions {
  return self.session.categoryOptions;
}

- (NSString *)mode {
  return self.session.mode;
}

- (BOOL)secondaryAudioShouldBeSilencedHint {
  return self.session.secondaryAudioShouldBeSilencedHint;
}

- (AVAudioSessionRouteDescription *)currentRoute {
  return self.session.currentRoute;
}

- (NSInteger)maximumInputNumberOfChannels {
  return self.session.maximumInputNumberOfChannels;
}

- (NSInteger)maximumOutputNumberOfChannels {
  return self.session.maximumOutputNumberOfChannels;
}

- (float)inputGain {
  return self.session.inputGain;
}

- (BOOL)inputGainSettable {
  return self.session.inputGainSettable;
}

- (BOOL)inputAvailable {
  return self.session.inputAvailable;
}

- (NSArray<AVAudioSessionDataSourceDescription *> *)inputDataSources {
  return self.session.inputDataSources;
}

- (AVAudioSessionDataSourceDescription *)inputDataSource {
  return self.session.inputDataSource;
}

- (NSArray<AVAudioSessionDataSourceDescription *> *)outputDataSources {
  return self.session.outputDataSources;
}

- (AVAudioSessionDataSourceDescription *)outputDataSource {
  return self.session.outputDataSource;
}

- (double)sampleRate {
  return self.session.sampleRate;
}

- (NSInteger)inputNumberOfChannels {
  return self.session.inputNumberOfChannels;
}

- (NSInteger)outputNumberOfChannels {
  return self.session.outputNumberOfChannels;
}

- (float)outputVolume {
  return self.session.outputVolume;
}

- (NSTimeInterval)inputLatency {
  return self.session.inputLatency;
}

- (NSTimeInterval)outputLatency {
  return self.session.outputLatency;
}

- (NSTimeInterval)IOBufferDuration {
  return self.session.IOBufferDuration;
}

// TODO(tkchin): Simplify the amount of locking happening here. Likely that we
// can just do atomic increments / decrements.
- (BOOL)setActive:(BOOL)active
            error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  int activationCount = _activationCount;
  if (!active && activationCount == 0) {
    RTCLogWarning(@"Attempting to deactivate without prior activation.");
  }
  BOOL success = YES;
  BOOL isActive = self.isActive;
  // Keep a local error so we can log it.
  NSError *error = nil;
  BOOL shouldSetActive =
      (active && !isActive) || (!active && isActive && activationCount == 1);
  // Attempt to activate if we're not active.
  // Attempt to deactivate if we're active and it's the last unbalanced call.
  if (shouldSetActive) {
    AVAudioSession *session = self.session;
    // AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation is used to ensure
    // that other audio sessions that were interrupted by our session can return
    // to their active state. It is recommended for VoIP apps to use this
    // option.
    AVAudioSessionSetActiveOptions options =
        active ? 0 : AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation;
    success = [session setActive:active
                     withOptions:options
                           error:&error];
    if (outError) {
      *outError = error;
    }
  }
  if (success) {
    if (shouldSetActive) {
      self.isActive = active;
    }
    if (active) {
      [self incrementActivationCount];
    }
  } else {
    RTCLogError(@"Failed to setActive:%d. Error: %@",
                active, error.localizedDescription);
  }
  // Decrement activation count on deactivation whether or not it succeeded.
  if (!active) {
    [self decrementActivationCount];
  }
  RTCLog(@"Number of current activations: %d", _activationCount);
  return success;
}

- (BOOL)setCategory:(NSString *)category
        withOptions:(AVAudioSessionCategoryOptions)options
              error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  options=options|AVAudioSessionCategoryOptionMixWithOthers;
  return [self.session setCategory:category withOptions:options error:outError];
}

- (BOOL)setMode:(NSString *)mode error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setMode:mode error:outError];
}

- (BOOL)setInputGain:(float)gain error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setInputGain:gain error:outError];
}

- (BOOL)setPreferredSampleRate:(double)sampleRate error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setPreferredSampleRate:sampleRate error:outError];
}

- (BOOL)setPreferredIOBufferDuration:(NSTimeInterval)duration
                               error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setPreferredIOBufferDuration:duration error:outError];
}

- (BOOL)setPreferredInputNumberOfChannels:(NSInteger)count
                                    error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setPreferredInputNumberOfChannels:count error:outError];
}
- (BOOL)setPreferredOutputNumberOfChannels:(NSInteger)count
                                     error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setPreferredOutputNumberOfChannels:count error:outError];
}

- (BOOL)overrideOutputAudioPort:(AVAudioSessionPortOverride)portOverride
                          error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session overrideOutputAudioPort:portOverride error:outError];
}

- (BOOL)setPreferredInput:(AVAudioSessionPortDescription *)inPort
                    error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setPreferredInput:inPort error:outError];
}

- (BOOL)setInputDataSource:(AVAudioSessionDataSourceDescription *)dataSource
                     error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setInputDataSource:dataSource error:outError];
}

- (BOOL)setOutputDataSource:(AVAudioSessionDataSourceDescription *)dataSource
                      error:(NSError **)outError {
  if (![self checkLock:outError]) {
    return NO;
  }
  return [self.session setOutputDataSource:dataSource error:outError];
}

#pragma mark - Notifications

- (void)handleInterruptionNotification:(NSNotification *)notification {
  NSNumber* typeNumber =
      notification.userInfo[AVAudioSessionInterruptionTypeKey];
  AVAudioSessionInterruptionType type =
      (AVAudioSessionInterruptionType)typeNumber.unsignedIntegerValue;
  switch (type) {
    case AVAudioSessionInterruptionTypeBegan:
      RTCLog(@"Audio session interruption began.");
      self.isActive = NO;
      [self notifyDidBeginInterruption];
      break;
    case AVAudioSessionInterruptionTypeEnded: {
      RTCLog(@"Audio session interruption ended.");
      [self updateAudioSessionAfterEvent];
      NSNumber *optionsNumber =
          notification.userInfo[AVAudioSessionInterruptionOptionKey];
      AVAudioSessionInterruptionOptions options =
          optionsNumber.unsignedIntegerValue;
      BOOL shouldResume =
          options & AVAudioSessionInterruptionOptionShouldResume;
      [self notifyDidEndInterruptionWithShouldResumeSession:shouldResume];
      break;
    }
  }
}

- (void)handleRouteChangeNotification:(NSNotification *)notification {
  // Get reason for current route change.
  NSNumber* reasonNumber =
      notification.userInfo[AVAudioSessionRouteChangeReasonKey];
  AVAudioSessionRouteChangeReason reason =
      (AVAudioSessionRouteChangeReason)reasonNumber.unsignedIntegerValue;
  RTCLog(@"Audio route changed:");
  switch (reason) {
    case AVAudioSessionRouteChangeReasonUnknown:
      RTCLog(@"Audio route changed: ReasonUnknown");
      break;
    case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
      RTCLog(@"Audio route changed: NewDeviceAvailable");
      break;
    case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
      RTCLog(@"Audio route changed: OldDeviceUnavailable");
      break;
    case AVAudioSessionRouteChangeReasonCategoryChange:
      RTCLog(@"Audio route changed: CategoryChange to :%@",
             self.session.category);
      break;
    case AVAudioSessionRouteChangeReasonOverride:
      RTCLog(@"Audio route changed: Override");
      break;
    case AVAudioSessionRouteChangeReasonWakeFromSleep:
      RTCLog(@"Audio route changed: WakeFromSleep");
      break;
    case AVAudioSessionRouteChangeReasonNoSuitableRouteForCategory:
      RTCLog(@"Audio route changed: NoSuitableRouteForCategory");
      break;
    case AVAudioSessionRouteChangeReasonRouteConfigurationChange:
      RTCLog(@"Audio route changed: RouteConfigurationChange");
      break;
  }
  AVAudioSessionRouteDescription* previousRoute =
      notification.userInfo[AVAudioSessionRouteChangePreviousRouteKey];
  // Log previous route configuration.
  RTCLog(@"Previous route: %@\nCurrent route:%@",
         previousRoute, self.session.currentRoute);
  [self notifyDidChangeRouteWithReason:reason previousRoute:previousRoute];
}

- (void)handleMediaServicesWereLost:(NSNotification *)notification {
  RTCLog(@"Media services were lost.");
  [self updateAudioSessionAfterEvent];
  [self notifyMediaServicesWereLost];
}

- (void)handleMediaServicesWereReset:(NSNotification *)notification {
  RTCLog(@"Media services were reset.");
  [self updateAudioSessionAfterEvent];
  [self notifyMediaServicesWereReset];
}

#pragma mark - Private

+ (NSError *)lockError {
  NSDictionary *userInfo = @{
    NSLocalizedDescriptionKey:
        @"Must call lockForConfiguration before calling this method."
  };
  NSError *error =
      [[NSError alloc] initWithDomain:kRTCAudioSessionErrorDomain
                                 code:kRTCAudioSessionErrorLockRequired
                             userInfo:userInfo];
  return error;
}

- (std::vector<__weak id<RTCAudioSessionDelegate> >)delegates {
  @synchronized(self) {
    // Note: this returns a copy.
    return _delegates;
  }
}

- (void)setSavedConfiguration:(RTCAudioSessionConfiguration *)configuration {
  @synchronized(self) {
    if (_savedConfiguration == configuration) {
      return;
    }
    _savedConfiguration = configuration;
  }
}

- (RTCAudioSessionConfiguration *)savedConfiguration {
  @synchronized(self) {
    return _savedConfiguration;
  }
}

// TODO(tkchin): check for duplicates.
- (void)pushDelegate:(id<RTCAudioSessionDelegate>)delegate {
  @synchronized(self) {
    _delegates.insert(_delegates.begin(), delegate);
  }
}

- (void)removeZeroedDelegates {
  @synchronized(self) {
    for (auto it = _delegates.begin(); it != _delegates.end(); ++it) {
      if (!*it) {
        _delegates.erase(it);
      }
    }
  }
}

- (int)activationCount {
  return _activationCount;
}

- (int)incrementActivationCount {
  RTCLog(@"Incrementing activation count.");
  return rtc::AtomicOps::Increment(&_activationCount);
}

- (NSInteger)decrementActivationCount {
  RTCLog(@"Decrementing activation count.");
  return rtc::AtomicOps::Decrement(&_activationCount);
}

- (int)webRTCSessionCount {
  return _webRTCSessionCount;
}

- (BOOL)checkLock:(NSError **)outError {
  // Check ivar instead of trying to acquire lock so that we won't accidentally
  // acquire lock if it hasn't already been called.
  if (!self.isLocked) {
    if (outError) {
      *outError = [RTCAudioSession lockError];
    }
    return NO;
  }
  return YES;
}

- (BOOL)beginWebRTCSession:(NSError **)outError {
  if (outError) {
    *outError = nil;
  }
  if (![self checkLock:outError]) {
    return NO;
  }
  NSInteger sessionCount = rtc::AtomicOps::Increment(&_webRTCSessionCount);
  if (sessionCount > 1) {
    // Should already be configured.
    RTC_DCHECK(self.isConfiguredForWebRTC);
    return YES;
  }

  // Only perform configuration steps once. Application might have already
  // configured the session.
  if (self.isConfiguredForWebRTC) {
    // Nothing more to do, already configured.
    return YES;
  }

  // If application has prevented automatic configuration, return here and wait
  // for application to call configureWebRTCSession.
  if (self.shouldDelayAudioConfiguration) {
    [self notifyShouldConfigure];
    return YES;
  }

  // Configure audio session.
  NSError *error = nil;
  if (![self configureWebRTCSession:&error]) {
    RTCLogError(@"Error configuring audio session: %@",
                error.localizedDescription);
    if (outError) {
      *outError = error;
    }
    return NO;
  }

  return YES;
}

- (BOOL)endWebRTCSession:(NSError **)outError {
  if (outError) {
    *outError = nil;
  }
  if (![self checkLock:outError]) {
    return NO;
  }
  int sessionCount = rtc::AtomicOps::Decrement(&_webRTCSessionCount);
  RTC_DCHECK_GE(sessionCount, 0);
  if (sessionCount != 0) {
    // Should still be configured.
    RTC_DCHECK(self.isConfiguredForWebRTC);
    return YES;
  }

  // Only unconfigure if application has not done it.
  if (!self.isConfiguredForWebRTC) {
    // Nothing more to do, already unconfigured.
    return YES;
  }

  // If application has prevented automatic configuration, return here and wait
  // for application to call unconfigureWebRTCSession.
  if (self.shouldDelayAudioConfiguration) {
    [self notifyShouldUnconfigure];
    return YES;
  }

  // Unconfigure audio session.
  NSError *error = nil;
  if (![self unconfigureWebRTCSession:&error]) {
    RTCLogError(@"Error unconfiguring audio session: %@",
                error.localizedDescription);
    if (outError) {
      *outError = error;
    }
    return NO;
  }

  return YES;
}

- (NSError *)configurationErrorWithDescription:(NSString *)description {
  NSDictionary* userInfo = @{
    NSLocalizedDescriptionKey: description,
  };
  return [[NSError alloc] initWithDomain:kRTCAudioSessionErrorDomain
                                    code:kRTCAudioSessionErrorConfiguration
                                userInfo:userInfo];
}

- (void)updateAudioSessionAfterEvent {
  BOOL shouldActivate = self.activationCount > 0;
  AVAudioSessionSetActiveOptions options = shouldActivate ?
      0 : AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation;
  NSError *error = nil;
  if ([self.session setActive:shouldActivate
                  withOptions:options
                        error:&error]) {
    self.isActive = shouldActivate;
  } else {
    RTCLogError(@"Failed to set session active to %d. Error:%@",
                shouldActivate, error.localizedDescription);
  }
}

- (void)notifyDidBeginInterruption {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionDidBeginInterruption:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionDidBeginInterruption:self];
    }
  }
}

- (void)notifyDidEndInterruptionWithShouldResumeSession:
    (BOOL)shouldResumeSession {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionDidEndInterruption:shouldResumeSession:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionDidEndInterruption:self
                           shouldResumeSession:shouldResumeSession];
    }
  }
}

- (void)notifyDidChangeRouteWithReason:(AVAudioSessionRouteChangeReason)reason
    previousRoute:(AVAudioSessionRouteDescription *)previousRoute {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionDidChangeRoute:reason:previousRoute:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionDidChangeRoute:self
                                    reason:reason
                             previousRoute:previousRoute];
    }
  }
}

- (void)notifyMediaServicesWereLost {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionMediaServicesWereLost:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionMediaServicesWereLost:self];
    }
  }
}

- (void)notifyMediaServicesWereReset {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionMediaServicesWereReset:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionMediaServicesWereReset:self];
    }
  }
}

- (void)notifyShouldConfigure {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionShouldConfigure:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionShouldConfigure:self];
    }
  }
}

- (void)notifyShouldUnconfigure {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionShouldUnconfigure:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionShouldUnconfigure:self];
    }
  }
}

- (void)notifyDidConfigure {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionDidConfigure:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionDidConfigure:self];
    }
  }
}

- (void)notifyDidUnconfigure {
  for (auto delegate : self.delegates) {
    SEL sel = @selector(audioSessionDidUnconfigure:);
    if ([delegate respondsToSelector:sel]) {
      [delegate audioSessionDidUnconfigure:self];
    }
  }
}

@end
