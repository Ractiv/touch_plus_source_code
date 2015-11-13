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

#pragma once

#ifdef _WIN32
#include <Windows.h>
#include "EtronDI_O.h"
#include "eSPAEAWBCtrl.h"
#include "camerads.h"

#elif __APPLE__
#include "libusb.h"
#include "libuvc.h"
#endif

#include <stdio.h>
#include <functional>
#include "opencv2/opencv.hpp"
#include <time.h>
#include <fstream>
#include <string>
#include <iostream>
#include <algorithm>

#include "globals.h"
#include "jpeg_decompressor.h"

using namespace cv;
using namespace std;

class Camera
{
#define MJPEG 1
#define UNCOMPRESSED 0
    
public:
	Camera();
	Camera(bool _useMJPEG, int _width, int _height, function<void (Mat& image_in, bool dummy_tick)> callback_in);
	~Camera();

	unsigned char* frame;

	bool device_not_detected = false;

	unsigned static const left = 0;
	unsigned static const right = 1;
	unsigned static const both = 2;

	static function<void (Mat& image_in, bool dummy_tick)> callback;
	
	//accelerometer acquisition
	int getAccelerometerValues(int *x, int *y, int *z);
	
	// Camera Parameters
	int		setExposureTime(int whichSide, float expTime);
	float	getExposureTime(int whichSide);

	int		setGlobalGain(int whichSide, float gain);
	float	getGlobalGain(int whichSide);

	int		setColorGains(int whichSide, float red, float green, float blue);
	int		getColorGains(int whichSide, float *red, float *green, float * blue);

	int		turnLEDsOn();
	int		turnLEDsOff();

	int		enableAutoExposure(int whichSide);
	int		disableAutoExposure(int whichSide);

	int		enableAutoWhiteBalance(int whichSide);
	int		disableAutoWhiteBalance(int whichSide);

	int     do_software_unlock();
	int     isCameraPresent();
    
	string  getSerialNumber();

#ifdef _WIN32
	void YUY2_to_RGB24_Microsoft(BYTE *pSrc, BYTE *pDst, int cx, int cy);
    
    BYTE* buffer;
    BYTE* bufferRGB;
    BYTE* depth;
    
    CCameraDS* ds_camera_;
    
#elif __APPLE__
    //static void cb(uvc_frame_t *frame, void *ptr);
    int startVideoStream(int width, int height, int framerate, int format);
    int stopVideoStream();
    void read_ADDR_85(libusb_device_handle* dev_handle, unsigned short wValue = 0x0300);
    unsigned char read_ADDR_81(libusb_device_handle* dev_handle, unsigned short wValue = 0x0300, int length = 4);
    unsigned char read_ADDR_81(libusb_device_handle* dev_handle, unsigned char* data,
                               unsigned short wValue = 0x0300, int length = 4);
    
    void write_ADDR_01(libusb_device_handle *dev_handle, unsigned char* data, unsigned short wValue = 0x0300, int length = 4);
    int readFlash(unsigned char* data, int length);
#endif
    
	int width;
	int height;
    
	Mat result;
	Mat decoded;
    
	void* pHandle;
    
	Rect left_roi;
	Rect right_roi;
    
	int doSetup(const int & format);
};

