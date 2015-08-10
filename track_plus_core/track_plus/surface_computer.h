#pragma once

#include <opencv2/opencv.hpp>
#include "globals.h"
#include "low_pass_filter.h"

using namespace cv;
using namespace std;

class SurfaceComputer
{
public:
	LowPassFilter low_pass_filter;

	vector<Point> pt_start_vec;
    vector<Point> pt_end_vec;

	void init(Mat& image_in);
	void compute(Mat& image_in);
};