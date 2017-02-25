/*
 *  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
 *
 *  Author: Jiarong Yu
 *
 *  Date: 2017/2/15
 */
#include "webrtc/base/logging.h"
#include "webrtc/base/platform_thread.h"
#include "SomaAudioDevice.h"
#include "webrtc/system_wrappers/include/sleep.h"

namespace webrtc {
    
    const int kRecordingFixedSampleRate = 48000;
    const size_t kRecordingNumChannels = 2;
    const int kPlayoutFixedSampleRate = 48000;
    const size_t kPlayoutNumChannels = 2;
    const size_t kPlayoutBufferSize =
    kPlayoutFixedSampleRate / 100 * kPlayoutNumChannels * 2;
    
    SomaAudioDevice::SomaAudioDevice(const int32_t id):
    _ptrAudioBuffer(NULL),
    _recordingBuffer(NULL),
    _playoutBuffer(NULL),
    _recordingFramesLeft(0),
    _playoutFramesLeft(0),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _recordingBufferSizeIn10MS(0),
    _recordingFramesIn10MS(0),
    _playoutFramesIn10MS(0),
    _playing(false),
    _recording(false),
    _lastCallPlayoutMillis(0) {
    }
    
    SomaAudioDevice::~SomaAudioDevice() {

    }
    
    int32_t SomaAudioDevice::ActiveAudioLayer(
                                              AudioDeviceModule::AudioLayer& audioLayer) const {
        return -1;
    }
    
    AudioDeviceGeneric::InitStatus SomaAudioDevice::Init() {
        return InitStatus::OK;
    }
    
    int32_t SomaAudioDevice::Terminate() { return 0; }
    
    bool SomaAudioDevice::Initialized() const { return true; }
    
    int16_t SomaAudioDevice::PlayoutDevices() {
        return 1;
    }
    
    int16_t SomaAudioDevice::RecordingDevices() {
        return 1;
    }
    
    int32_t SomaAudioDevice::PlayoutDeviceName(uint16_t index,
                                               char name[kAdmMaxDeviceNameSize],
                                               char guid[kAdmMaxGuidSize]) {
        const char* kName = "dummy_device";
        const char* kGuid = "dummy_device_unique_id";
        if (index < 1) {
            memset(name, 0, kAdmMaxDeviceNameSize);
            memset(guid, 0, kAdmMaxGuidSize);
            memcpy(name, kName, strlen(kName));
            memcpy(guid, kGuid, strlen(guid));
            return 0;
        }
        return -1;
    }
    
    int32_t SomaAudioDevice::RecordingDeviceName(uint16_t index,
                                                 char name[kAdmMaxDeviceNameSize],
                                                 char guid[kAdmMaxGuidSize]) {
        const char* kName = "dummy_device";
        const char* kGuid = "dummy_device_unique_id";
        if (index < 1) {
            memset(name, 0, kAdmMaxDeviceNameSize);
            memset(guid, 0, kAdmMaxGuidSize);
            memcpy(name, kName, strlen(kName));
            memcpy(guid, kGuid, strlen(guid));
            return 0;
        }
        return -1;
    }
    
    int32_t SomaAudioDevice::SetPlayoutDevice(uint16_t index) {
        if (index == 0) {
            _playout_index = index;
            return 0;
        }
        return -1;
    }
    
    int32_t SomaAudioDevice::SetPlayoutDevice(
                                              AudioDeviceModule::WindowsDeviceType device) {
        return -1;
    }
    
    int32_t SomaAudioDevice::SetRecordingDevice(uint16_t index) {
        if (index == 0) {
            _record_index = index;
            return _record_index;
        }
        return -1;
    }
    
    int32_t SomaAudioDevice::SetRecordingDevice(
                                                AudioDeviceModule::WindowsDeviceType device) {
        return -1;
    }
    
    int32_t SomaAudioDevice::PlayoutIsAvailable(bool& available) {
        if (_playout_index == 0) {
            available = true;
            return _playout_index;
        }
        available = false;
        return -1;
    }
    
    int32_t SomaAudioDevice::InitPlayout() {
        if (_ptrAudioBuffer) {
            // Update webrtc audio buffer with the selected parameters
            _ptrAudioBuffer->SetPlayoutSampleRate(kPlayoutFixedSampleRate);
            _ptrAudioBuffer->SetPlayoutChannels(kPlayoutNumChannels);
        }
        return 0;
    }
    
    bool SomaAudioDevice::PlayoutIsInitialized() const {
        return true;
    }
    
    int32_t SomaAudioDevice::RecordingIsAvailable(bool& available) {
        if (_record_index == 0) {
            available = true;
            return _record_index;
        }
        available = false;
        return -1;
    }
    
    int32_t SomaAudioDevice::InitRecording() {
        CriticalSectionScoped lock(&_critSect);
        
        if (_recording) {
            return -1;
        }
        
        _recordingFramesIn10MS = static_cast<size_t>(kRecordingFixedSampleRate / 100);
        
        if (_ptrAudioBuffer) {
            _ptrAudioBuffer->SetRecordingSampleRate(kRecordingFixedSampleRate);
            _ptrAudioBuffer->SetRecordingChannels(kRecordingNumChannels);
        }
        return 0;
    }
    
    bool SomaAudioDevice::RecordingIsInitialized() const {
        return _recordingFramesIn10MS != 0;
    }
    
    int32_t SomaAudioDevice::StartPlayout() {
        if (_playing) {
            return 0;
        }
        
        _playoutFramesIn10MS = static_cast<size_t>(kPlayoutFixedSampleRate / 100);
        _playing = true;
        _playoutFramesLeft = 0;
        
        if (!_playoutBuffer) {
            _playoutBuffer = new int8_t[kPlayoutBufferSize];
        }
        if (!_playoutBuffer) {
            _playing = false;
            return -1;
        }
        
        _ptrThreadPlay.reset(new rtc::PlatformThread(
                                                     PlayThreadFunc, this, "webrtc_audio_module_play_thread"));
        _ptrThreadPlay->Start();
        _ptrThreadPlay->SetPriority(rtc::kRealtimePriority);
        
        LOG(LS_INFO) << "Started playout capture to output";
        return 0;
    }
    
    int32_t SomaAudioDevice::StopPlayout() {
        {
            CriticalSectionScoped lock(&_critSect);
            _playing = false;
        }
        
        // stop playout thread first
        if (_ptrThreadPlay) {
            _ptrThreadPlay->Stop();
            _ptrThreadPlay.reset();
        }
        
        CriticalSectionScoped lock(&_critSect);
        
        _playoutFramesLeft = 0;
        delete [] _playoutBuffer;
        _playoutBuffer = NULL;
        
        LOG(LS_INFO) << "Stopped playout capture to output";
        return 0;
    }
    
    bool SomaAudioDevice::Playing() const {
        return true;
    }
    
    int32_t SomaAudioDevice::StartRecording() {
        _recording = true;
        
        // Make sure we only create the buffer once.
        _recordingBufferSizeIn10MS = _recordingFramesIn10MS *
        kRecordingNumChannels *
        2;
        if (!_recordingBuffer) {
            _recordingBuffer = new int8_t[_recordingBufferSizeIn10MS];
        }
        
        _ptrThreadRec.reset(new rtc::PlatformThread(
                                                    RecThreadFunc, this, "webrtc_audio_module_capture_thread"));
        
        _ptrThreadRec->Start();
        _ptrThreadRec->SetPriority(rtc::kRealtimePriority);
        
        LOG(LS_INFO) << "Started recording from input";
        
        return 0;
    }
    
    
    int32_t SomaAudioDevice::StopRecording() {
        {
            CriticalSectionScoped lock(&_critSect);
            _recording = false;
        }
        
        if (_ptrThreadRec) {
            _ptrThreadRec->Stop();
            _ptrThreadRec.reset();
        }
        
        CriticalSectionScoped lock(&_critSect);
        _recordingFramesLeft = 0;
        if (_recordingBuffer) {
            delete [] _recordingBuffer;
            _recordingBuffer = NULL;
        }
        
        LOG(LS_INFO) << "Stopped recording from input";
        return 0;
    }
    
    bool SomaAudioDevice::Recording() const {
        return _recording;
    }
    
    int32_t SomaAudioDevice::SetAGC(bool enable) { return -1; }
    
    bool SomaAudioDevice::AGC() const { return false; }
    
    int32_t SomaAudioDevice::SetWaveOutVolume(uint16_t volumeLeft,
                                              uint16_t volumeRight) {
        return -1;
    }
    
    int32_t SomaAudioDevice::WaveOutVolume(uint16_t& volumeLeft,
                                           uint16_t& volumeRight) const {
        return -1;
    }
    
    int32_t SomaAudioDevice::InitSpeaker() { return -1; }
    
    bool SomaAudioDevice::SpeakerIsInitialized() const { return false; }
    
    int32_t SomaAudioDevice::InitMicrophone() { return 0; }
    
    bool SomaAudioDevice::MicrophoneIsInitialized() const { return true; }
    
    int32_t SomaAudioDevice::SpeakerVolumeIsAvailable(bool& available) {
        return -1;
    }
    
    int32_t SomaAudioDevice::SetSpeakerVolume(uint32_t volume) { return -1; }
    
    int32_t SomaAudioDevice::SpeakerVolume(uint32_t& volume) const { return -1; }
    
    int32_t SomaAudioDevice::MaxSpeakerVolume(uint32_t& maxVolume) const {
        return -1;
    }
    
    int32_t SomaAudioDevice::MinSpeakerVolume(uint32_t& minVolume) const {
        return -1;
    }
    
    int32_t SomaAudioDevice::SpeakerVolumeStepSize(uint16_t& stepSize) const {
        return -1;
    }
    
    int32_t SomaAudioDevice::MicrophoneVolumeIsAvailable(bool& available) {
        return -1;
    }
    
    int32_t SomaAudioDevice::SetMicrophoneVolume(uint32_t volume) { return -1; }
    
    int32_t SomaAudioDevice::MicrophoneVolume(uint32_t& volume) const {
        return -1;
    }
    
    int32_t SomaAudioDevice::MaxMicrophoneVolume(uint32_t& maxVolume) const {
        return -1;
    }
    
    int32_t SomaAudioDevice::MinMicrophoneVolume(uint32_t& minVolume) const {
        return -1;
    }
    
    int32_t SomaAudioDevice::MicrophoneVolumeStepSize(uint16_t& stepSize) const {
        return -1;
    }
    
    int32_t SomaAudioDevice::SpeakerMuteIsAvailable(bool& available) { return -1; }
    
    int32_t SomaAudioDevice::SetSpeakerMute(bool enable) { return -1; }
    
    int32_t SomaAudioDevice::SpeakerMute(bool& enabled) const { return -1; }
    
    int32_t SomaAudioDevice::MicrophoneMuteIsAvailable(bool& available) {
        return -1;
    }
    
    int32_t SomaAudioDevice::SetMicrophoneMute(bool enable) { return -1; }
    
    int32_t SomaAudioDevice::MicrophoneMute(bool& enabled) const { return -1; }
    
    int32_t SomaAudioDevice::MicrophoneBoostIsAvailable(bool& available) {
        return -1;
    }
    
    int32_t SomaAudioDevice::SetMicrophoneBoost(bool enable) { return -1; }
    
    int32_t SomaAudioDevice::MicrophoneBoost(bool& enabled) const { return -1; }
    
    int32_t SomaAudioDevice::StereoPlayoutIsAvailable(bool& available) {
        available = true;
        return 0;
    }
    int32_t SomaAudioDevice::SetStereoPlayout(bool enable) {
        return 0;
    }
    
    int32_t SomaAudioDevice::StereoPlayout(bool& enabled) const {
        enabled = true;
        return 0;
    }
    
    int32_t SomaAudioDevice::StereoRecordingIsAvailable(bool& available) {
        available = true;
        return 0;
    }
    
    int32_t SomaAudioDevice::SetStereoRecording(bool enable) {
        return 0;
    }
    
    int32_t SomaAudioDevice::StereoRecording(bool& enabled) const {
        enabled = true;
        return 0;
    }
    
    int32_t SomaAudioDevice::SetPlayoutBuffer(
                                              const AudioDeviceModule::BufferType type,
                                              uint16_t sizeMS) {
        return 0;
    }
    
    int32_t SomaAudioDevice::PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                           uint16_t& sizeMS) const {
        type = _playBufType;
        return 0;
    }
    
    int32_t SomaAudioDevice::PlayoutDelay(uint16_t& delayMS) const {
        return 0;
    }
    
    int32_t SomaAudioDevice::RecordingDelay(uint16_t& delayMS) const { return -1; }
    
    int32_t SomaAudioDevice::CPULoad(uint16_t& load) const { return -1; }
    
    bool SomaAudioDevice::PlayoutWarning() const { return false; }
    
    bool SomaAudioDevice::PlayoutError() const { return false; }
    
    bool SomaAudioDevice::RecordingWarning() const { return false; }
    
    bool SomaAudioDevice::RecordingError() const { return false; }
    
    void SomaAudioDevice::ClearPlayoutWarning() {}
    
    void SomaAudioDevice::ClearPlayoutError() {}
    
    void SomaAudioDevice::ClearRecordingWarning() {}
    
    void SomaAudioDevice::ClearRecordingError() {}
    
    void SomaAudioDevice::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
        CriticalSectionScoped lock(&_critSect);
        
        _ptrAudioBuffer = audioBuffer;
        
        // Inform the AudioBuffer about default settings for this implementation.
        // Set all values to zero here since the actual settings will be done by
        // InitPlayout and InitRecording later.
        _ptrAudioBuffer->SetRecordingSampleRate(0);
        _ptrAudioBuffer->SetPlayoutSampleRate(0);
        _ptrAudioBuffer->SetRecordingChannels(0);
        _ptrAudioBuffer->SetPlayoutChannels(0);
    }
    
    bool SomaAudioDevice::PlayThreadFunc(void* pThis)
    {
        return (static_cast<SomaAudioDevice*>(pThis)->PlayThreadProcess());
    }
    
    bool SomaAudioDevice::RecThreadFunc(void* pThis)
    {
        return (static_cast<SomaAudioDevice*>(pThis)->RecThreadProcess());
    }
    
    bool SomaAudioDevice::PlayThreadProcess()
    {
        if (!_playing) {
            return false;
        }
        int64_t currentTime = rtc::TimeMillis();
        _critSect.Enter();
        
        if (_lastCallPlayoutMillis == 0 ||
            currentTime - _lastCallPlayoutMillis >= 10) {
            _critSect.Leave();
            _ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
            _critSect.Enter();
            
            _playoutFramesLeft = _ptrAudioBuffer->GetPlayoutData(_playoutBuffer);
            //assert(_playoutFramesLeft == _playoutFramesIn10MS);

            _lastCallPlayoutMillis = currentTime;
        }
        _playoutFramesLeft = 0;
        _critSect.Leave();
        
        int64_t deltaTimeMillis = rtc::TimeMillis() - currentTime;
        if (deltaTimeMillis < 10) {
            SleepMs(10 - deltaTimeMillis);
        }
        
        return true;
    }
    
    bool SomaAudioDevice::RecThreadProcess()
    {
        return true;
    }
    
}  // namespace webrtc
