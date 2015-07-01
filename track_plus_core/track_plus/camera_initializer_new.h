#pragma once

#include <opencv2/opencv.hpp>
#include "camera.h"
#include "math_plus.h"
#include "globals.h"
#include "mat_functions.h"
#include "low_pass_filter.h"

using namespace cv;

class CameraInitializerNew
{
public:
	static float exposure_val;
	static float exposure_max;

	static uchar l_exposure_old;

	static LowPassFilter low_pass_filter;
	
	static void init(Camera* camera);
	static bool adjust_exposure(Camera* camera, Mat& image_in);
	static void preset0(Camera* camera);
	static void preset1(Camera* camera);
	static void preset2(Camera* camera);
	static void preset3(Camera* camera);
	static void preset4(Camera* camera);
};