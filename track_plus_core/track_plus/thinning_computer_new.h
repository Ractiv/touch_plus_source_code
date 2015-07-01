#pragma once

#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "globals.h"
#include "blob_new.h"

using namespace cv;
using namespace std;

class ThinningComputerNew
{
public:
	void thinning_iteration(Mat& image_in, const int iter, vector<Point>& points, int& iterations);
	
	vector<Point> compute(Mat& image_in, BlobNew* blob_in = NULL, 
						  int x_min_in = -1, int x_max_in = -1, int y_min_in = -1, int y_max_in = -1,
						  const int max_iter = -1);
};