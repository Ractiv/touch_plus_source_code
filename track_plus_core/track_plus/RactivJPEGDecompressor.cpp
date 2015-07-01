/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

#include "RactivJPEGDecompressor.h"
#ifdef _WIN32
int RactivJPEGDecompressor::decompress(unsigned char * in, unsigned long in_len, unsigned char * out, int width, int height)
{
	tjDecompress2(handle, in, in_len, out, width, width * 3, height, TJPF_BGR, 0);
	return 0;
}
#endif