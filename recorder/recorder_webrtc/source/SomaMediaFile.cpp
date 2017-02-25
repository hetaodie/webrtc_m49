#include "SomaMediaFile.h"
#include "SomaAviFile.h"

SomaMediaFile* SomaMediaFile::Create(const char* filename, SomaMediaFormat media_format)
{
	SomaMediaFile* containerFile = NULL;

	switch (media_format)
	{
	case SomaMediaFormat_AVI:
		containerFile = new SomaAviFile(filename);
		break;
	default:
		break;
	}

	return containerFile;
}

void SomaMediaFile::Destroy(SomaMediaFile* containerFile)
{
	delete containerFile;
	containerFile = NULL;
}
