#include "RactivJPEGDecompressor.h"
#ifdef _WIN32
int RactivJPEGDecompressor::decompress(unsigned char * in, unsigned long in_len, unsigned char * out, int width, int height)
{
	tjDecompress2(handle, in, in_len, out, width, width * 3, height, TJPF_BGR, 0);
	return 0;
}
#endif