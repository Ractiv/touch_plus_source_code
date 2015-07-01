#pragma once

#include <opencv2/opencv.hpp>
#include "globals.h"
#include "math_plus.h"

using namespace cv;

class HistogramBuilder
{
public:
	void compute_vertical(Mat& image_in, Mat& image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max);
	void compute_vertical(vector<int>& vec, Mat& image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max);
	void compute_horizontal(Mat& image_in, Mat& image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max);
	void compute_horizontal(vector<int>& vec, Mat& image_out, int gaussian_val, int& x_min, int& x_max, int& y_min, int& y_max);
};