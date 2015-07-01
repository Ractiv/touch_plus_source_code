#pragma once

#ifdef _WIN32
#include "turbojpeg.h"

class RactivJPEGDecompressor
{
public:
	tjhandle handle = tjInitDecompress();

	int decompress(unsigned char * in, unsigned long in_len,unsigned char * out, int width, int height);
};
#endif