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

#include "camera_initializer_new.h"

float CameraInitializerNew::exposure_val;
float CameraInitializerNew::exposure_max;

LowPassFilter CameraInitializerNew::low_pass_filter;

void CameraInitializerNew::init(Camera* camera)
{
	COUT << "initializing camera" << endl;

	exposure_max = 15;
	exposure_val = exposure_max;

	camera->disableAutoExposure(Camera::both);
	camera->disableAutoWhiteBalance(Camera::both);
	camera->turnLEDsOn();
	camera->setExposureTime(Camera::both, exposure_val);

	preset1(camera);
}

bool CameraInitializerNew::adjust_exposure(Camera* camera, Mat& image_in, bool reset)
{
	static int count = 0;
	static bool step0 = false;
	static bool step1 = false;

	if (reset)
	{
		init(camera);
		count = -1;
		step0 = false;
		step1 = false;
	}

	if (step0 == true && step1 == true)
		return true;
	else
		++count;

	static Mat image_leds_on = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	static Mat image_leds_off = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	if (step0 == false)
	{
		if (count == 5)
		{
			image_leds_on = image_in;
			GaussianBlur(image_leds_on, image_leds_on, Size(49, 49), 0, 0);
			camera->turnLEDsOff();
		}

		if (count == 10)
		{
			step0 = true;

			COUT << "setting exposure step 0 complete" << endl;
		}
	}
	else if (step1 == false)
	{
		image_leds_off = image_in;
		GaussianBlur(image_leds_off, image_leds_off, Size(49, 49), 0, 0);
		camera->turnLEDsOn();
		step1 = true;
		
		COUT << "setting exposure step 1 complete" << endl;
	}

	if (step0 == true && step1 == true)
	{
		vector<uchar> vec_leds_on;
		vector<uchar> vec_leds_off;

		for (int i = 0; i < WIDTH_SMALL; ++i)
			for (int j = HEIGHT_SMALL; j > 0; --j)
			{
				vec_leds_on.push_back(image_leds_on.ptr<uchar>(j, i)[2]);
				vec_leds_off.push_back(image_leds_off.ptr<uchar>(j, i)[2]);
			}

		sort(vec_leds_on.begin(), vec_leds_on.end());
		sort(vec_leds_off.begin(), vec_leds_off.end());

		const int i_min = vec_leds_on.size() * 0.25;
		const int i_max = vec_leds_on.size() * 0.75;

		float gray_mean_on = 0;
		float gray_mean_off = 0;
		float gray_mean_count = 0;

		for (int i = i_min; i <= i_max; ++i)
		{
			gray_mean_on += vec_leds_on[i];
			gray_mean_off += vec_leds_off[i];
			++gray_mean_count;
		}

		gray_mean_on /= gray_mean_count;
		gray_mean_off /= gray_mean_count;

		float gray_diff = gray_mean_on - gray_mean_off;
		COUT << "gray_diff is " << gray_diff << endl;

		if (gray_diff > 50)
			gray_diff = 50;
		if (gray_diff < 5)
			gray_diff = 5;

		float r_val = exponential(gray_diff - 10 < 0 ? 0 : gray_diff - 10, 0.9967884, 0.001570977, -0.1430162);
		COUT << "r_val is " << r_val << endl;

		camera->setColorGains(0, r_val, 1.0, 2.0);
		camera->setColorGains(1, r_val, 1.0, 2.0);

		exposure_val = linear(gray_diff - 10 < 0 ? 0 : gray_diff - 10, 0.31111111, -0.55555556);
		COUT << "exposure_val is " << exposure_val << endl;

		camera->setExposureTime(Camera::both, exposure_val);

		return true;		
	}

	return false;
}

void CameraInitializerNew::preset0(Camera* camera)
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 3.0, 1.5, 2.0);
	camera->setColorGains(1, 3.0, 1.5, 2.0);	
}

void CameraInitializerNew::preset1(Camera* camera)//universal indoors
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 2.0, 1.0, 2.0);
	camera->setColorGains(1, 2.0, 1.0, 2.0);
}

void CameraInitializerNew::preset2(Camera* camera)//universal outdoor
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 1.5, 1.0, 2.0);
	camera->setColorGains(1, 1.5, 1.0, 2.0);
}

void CameraInitializerNew::preset3(Camera* camera)
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 2.0, 1.0, 1.5);
	camera->setColorGains(1, 2.0, 1.0, 1.5);
}

void CameraInitializerNew::preset4(Camera* camera)//afternoon sun facing laptop screen
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 3.0, 2.0, 3.0);
	camera->setColorGains(1, 3.0, 2.0, 3.0);
}