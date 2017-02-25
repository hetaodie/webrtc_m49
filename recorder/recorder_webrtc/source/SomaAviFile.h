/*
*  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*
*  Date: 2017/2/15
*/

#ifndef _SOMA_MEDIARECORD_SOMAAVIFILE_H_
#define _SOMA_MEDIARECORD_SOMAAVIFILE_H_

#include "SomaMediaFile.h"
#include "webrtc/modules/media_file/media_file.h"

class SomaAviFile : public SomaMediaFile
{
public:
	SomaAviFile(const char* filename);
	virtual ~SomaAviFile();

	int Start();
	int Stop();
	int IncomingAudioData(const void *data, size_t len);

private:
	webrtc::MediaFile* media_file_;
	std::string filename_;
};

#endif // _SOMA_MEDIARECORD_SOMAAVIFILE_H_
