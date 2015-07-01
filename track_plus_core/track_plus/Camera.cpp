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

#include "Camera.h"
#include <Windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <algorithm>

#define TP_CAMERA_VID   "1e4e"
#define TP_CAMERA_PID   "0107"
#define USE_DIRECT_SHOW 0

function<void (Mat& image_in)> Camera::callback;

static RactivJPEGDecompressor * decomp = new RactivJPEGDecompressor();

unsigned char * myBuffer;
Mat image_out = Mat(480, 1280, CV_8UC3);
static char  fname[256];
long long s_upload_file_size = 0;

bool ZipFile(const std::vector<std::string>& sourceFiles, char * destZip);

void * pHandle = NULL;

Camera::Camera(){}

Camera::Camera(bool _useMJPEG, int _width, int _height, function<void (Mat& image_in)> callback_in)
{
	height	= _height;
	width	= _width;
	useMJPEG = _useMJPEG;
	callback = callback_in;
	doSetup(1);
}

Camera::~Camera()
{
	if (ds_camera_)
	{
		ds_camera_->CloseCamera();
		ds_camera_ = NULL;
	}
	free(&buffer);
	free(&bufferRGB);
	free(&depth);
}

static void frameCallback(BYTE * pBuffer, long lBufferSize)
{
	decomp->decompress(pBuffer, lBufferSize, myBuffer, 1280, 480);
	Camera::callback(image_out);
}

string Camera::getSerialNumber()
{
	string result = "";
	unsigned char serialNumber[10];
	eSPAEAWB_ReadFlash(serialNumber, 10);
	for (int i = 0; i < 10; i++) {
		int digit = serialNumber[i];
		result += to_string(digit);
	}
	return result;
}

int Camera::isCameraPresent()
{	
	int devCount;
	int didInit = EtronDI_Init(&pHandle);
	int devNumber = EtronDI_GetDeviceNumber(pHandle);

	WCHAR name[100];
	if (!EtronDI_FindDevice(pHandle)) {
		COUT << "Device not found!" << endl;
		return 0;
	}
	int code = eSPAEAWB_EnumDevice(&devCount);
	eSPAEAWB_SelectDevice(0);
	eSPAEAWB_SetSensorType(1);
	WCHAR myName[255];
	WCHAR targetName[255] = L"Touch+ Camera";
	int deviceWasSelected = 0;
	for (int i = 0; i < devCount; i++){
		eSPAEAWB_GetDevicename(i, myName, 255);

		wstring ws(myName);
		string str(ws.begin(), ws.end());
		COUT << str << endl;

		myName[14] = 0;
		bool found = true;
		int j;
		for (j = 0; j < 13; j++){
			if (myName[j] != targetName[j]){
				found = false;
			}
		}
		if (found){
			eSPAEAWB_SelectDevice(i);
			eSPAEAWB_SetSensorType(1);
			deviceWasSelected = 1;
			COUT << "Touch+ Camera found" << endl;
		}

	}
	if (!deviceWasSelected){
		device_not_detected = true;
		COUT << "Did not find a Touch+ Camera" << endl;
		return 0;
	}
	return 1;
}

int Camera::do_software_unlock()
{
	int present = isCameraPresent();
	int retVal = eSPAEAWB_SWUnlock(0x0107);
	return present;
}


int Camera::doSetup(const int & format)
{
	do_software_unlock();
	
	ds_camera_ = new CCameraDS();
	int camera_count = CCameraDS::CameraCount();
	char camera_name[255] = { 0 };
	char camera_vid[10] = { 0 };
	char camera_pid[10] = { 0 };
	int i = 0, touchCameraId = -1;
	
	while (i < camera_count)
	{
		CCameraDS::CameraInfo(i, camera_vid, camera_pid);

		// VID PID is more reasonable
		if (0 == strncmp(camera_vid, TP_CAMERA_VID, 4) &&
			0 == strncmp(camera_pid, TP_CAMERA_PID, 4))
		{
			touchCameraId = i;
			break;
		}
		i++;
	}

	if (-1 == touchCameraId)
	{
		return false;
	}
	const int fmt = 1;
	myBuffer = (unsigned char *)malloc(1280 * 480 * 3);
	image_out.data = myBuffer;
	bool retV = ds_camera_->OpenCamera(touchCameraId, format, 1280, 480, 60, frameCallback);
	COUT << "camera opened = " << retV << endl;
	return retV;
}

unsigned char* Camera::getDataPointer()
{
	return myBuffer;
}

int Camera::setExposureTime(int whichSide, float expTime)
{
	int retCode= eSPAEAWB_SetExposureTime(whichSide, expTime);
	return retCode;
}

float Camera::getExposureTime(int whichSide)
{
	float eTime = -1.0;
	int retCode = eSPAEAWB_GetExposureTime(whichSide, &eTime);
	return eTime;
}

int Camera::setGlobalGain(int whichSide, float gain)
{
	return eSPAEAWB_SetGlobalGain(whichSide, gain);
}

float Camera::getGlobalGain(int whichSide)
{
	float globalGain = -1.0;
	eSPAEAWB_GetGlobalGain(whichSide, &globalGain);
	return globalGain;
}

int Camera::turnLEDsOn()
{
	BYTE gpio_code;
	int retCode = eSPAEAWB_GetGPIOValue(1, &gpio_code);
    gpio_code |= 0x08;
	retCode = eSPAEAWB_SetGPIOValue(1, gpio_code);
	return retCode;
}

int Camera::turnLEDsOff()
{
	BYTE gpio_code;
	int retCode = eSPAEAWB_GetGPIOValue(1, &gpio_code);
	gpio_code &= 0xf7;
	retCode = eSPAEAWB_SetGPIOValue(1, gpio_code);
	return retCode;
}

int Camera::getAccelerometerValues(int *x, int *y, int *z)
{
	return eSPAEAWB_GetAccMeterValue(x, y, z);
}

int	Camera::setColorGains(int whichSide, float red, float green, float blue)
{
	return eSPAEAWB_SetColorGain(whichSide, red, green, blue);
}

int	Camera::getColorGains(int whichSide, float *red, float *green, float * blue)
{
	return eSPAEAWB_GetColorGain(whichSide, red, green, blue);
}

int Camera::enableAutoExposure(int whichSide)
{
	eSPAEAWB_SelectDevice(whichSide);
	return eSPAEAWB_EnableAE();
}

int Camera::disableAutoExposure(int whichSide)
{
	eSPAEAWB_SelectDevice(whichSide);
	return eSPAEAWB_DisableAE();
}

int Camera::enableAutoWhiteBalance(int whichSide)
{
	eSPAEAWB_SelectDevice(whichSide);
	return eSPAEAWB_EnableAWB();
}

int Camera::disableAutoWhiteBalance(int whichSide)
{
	eSPAEAWB_SelectDevice(whichSide);
	return eSPAEAWB_DisableAWB();
}

void Camera::YUY2_to_RGB24_Microsoft(BYTE *pSrc, BYTE *pDst, int cx, int cy)
{
	int nSrcBPS, nDstBPS, x, y, x2, x3, m;
	int ma0, mb0, m02, m11, m12, m21;
	BYTE *pS0, *pD0;
	int Y, U, V, Y2;
	BYTE R, G, B, R2, G2, B2;

	nSrcBPS = cx * 2;
	nDstBPS = ((cx * 3 + 3) / 4) * 4;

	pS0 = pSrc;
	pD0 = pDst + nDstBPS*(cy - 1);
	for (y = 0; y<cy; y++)
	{
		for (x3 = 0, x2 = 0, x = 0; x<cx; x += 2, x2 += 4, x3 += 6)
		{
			Y = (int)pS0[x2 + 0] - 16;
			Y2 = (int)pS0[x2 + 2] - 16;
			U = (int)pS0[x2 + 1] - 128;
			V = (int)pS0[x2 + 3] - 128;
			//
			ma0 = 298 * Y;
			mb0 = 298 * Y2;
			m02 = 409 * V + 128;
			m11 = -100 * U;
			m12 = -208 * V + 128;
			m21 = 516 * U + 128;
			//
			m = (ma0 + m02) >> 8;
			R = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
			m = (ma0 + m11 + m12) >> 8;
			G = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
			m = (ma0 + m21) >> 8;
			B = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
			//
			m = (mb0 + m02) >> 8;
			R2 = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
			m = (mb0 + m11 + m12) >> 8;
			G2 = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
			m = (mb0 + m21) >> 8;
			B2 = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
			//
			pD0[x3] = B;
			pD0[x3 + 1] = G;
			pD0[x3 + 2] = R;
			pD0[x3 + 3] = B2;
			pD0[x3 + 4] = G2;
			pD0[x3 + 5] = R2;
		}
		pS0 += nSrcBPS;
		pD0 -= nDstBPS;
	}
}