#pragma once

#include "reprojector.h"

class PointResolver
{
public:
	Point2f compute(Point& pt_in, Mat& image_in, Mat& image_background_in, uchar diff_threshold, uchar gray_threshold,
					Reprojector& reprojector);
};