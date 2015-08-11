#pragma once

#include <opencv2/opencv.hpp>
#include "globals.h"
#include "low_pass_filter.h"
#include "math_plus.h"

using namespace cv;
using namespace std;

class SurfaceComputer
{
public:
	LowPassFilter low_pass_filter;
	
	void init(Mat& image_in);
};