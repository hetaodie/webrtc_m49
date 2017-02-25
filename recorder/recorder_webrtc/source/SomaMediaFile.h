/*
*  Copyright (c) 2017 Instanza Inc. All Rights Reserved.
*
*  Author: Jiarong Yu
*
*  Date: 2017/2/15
*/

#ifndef _SOMA_MEDIARECORD_MEDIAFILE_H_
#define _SOMA_MEDIARECORD_MEDIAFILE_H_

#include "SomaMediaRecord.h"
#include <stddef.h>

class SomaMediaFile
{
public:
	static SomaMediaFile* Create(const char* filename, SomaMediaFormat media_format);
	static void Destroy(SomaMediaFile* containerFile);

	SomaMediaFile() {}
	virtual ~SomaMediaFile() {}

	virtual int Start() = 0;
	virtual int Stop() = 0;
	virtual int IncomingAudioData(const void *data, size_t len) = 0;
};

#endif // _SOMA_MEDIARECORD_MEDIAFILE_H_
