/*
*  Copyright (c) 2016 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*
*  Date: 2017/2/15
*/

/*
*  ****** How To Use ******
*
*  SomaMediaRecord *mediaRecord = SomaMediaRecord::CreateMediaRecord();
*  char *filename = "***.avi";
*  mediaRecord.Init(filename);
*  mediaRecord.Start();
*  mediaRecord.ReceivePacket(data, size);
*  mediaRecord.Stop();
*  mediaRecord.Destroy();
*  SomaMediaRecord::DestroyMediaRecord(mediaRecord);
*/

#ifndef _SOMA_MEDIARECORD_SOMAMEDIARECORD_H_
#define _SOMA_MEDIARECORD_SOMAMEDIARECORD_H_

enum SomaMediaFormat
{
	SomaMediaFormat_AVI = 0,
};

enum SomaMediaType
{
	SomaMediaType_Audio = 0,
};

class SomaAudioObserver
{
public:
	SomaAudioObserver() {};
	virtual ~SomaAudioObserver() {};
	virtual void OutputAudio(unsigned char *outputBuffer, unsigned int bufferSize) = 0;
};

class SomaMediaRecord
{
public:
	static SomaMediaRecord* CreateMediaRecord();
	static long long TimeMs();
	static void DestroyMediaRecord(SomaMediaRecord* media_record);

	virtual int Init(
		const char* filename, 
		SomaMediaType media_type = SomaMediaType_Audio,
		SomaMediaFormat media_format = SomaMediaFormat_AVI) = 0;

	virtual int Init(
		SomaAudioObserver* callback) = 0;

	virtual int Start() = 0;
	virtual int StartAudioRecording() = 0;
    
	virtual int ReceiveAudioPacket(int channelID, const void* data, int size) = 0;

	virtual int Stop() = 0;
	virtual int StopAudioRecording() = 0;

	virtual int Destroy() = 0;

protected:
	SomaMediaRecord() {}
	virtual ~SomaMediaRecord() {}
};

#endif // _SOMA_MEDIARECORD_SOMAMEDIARECORD_H_

