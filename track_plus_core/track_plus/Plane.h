#pragma once

#include <opencv2/opencv.hpp>
#include "math_plus.h"

using namespace cv;

class Plane
{
public:
	Point3f normal;
	float d;

	Plane();
	Plane(Point3f a, Point3f b, Point3f c);
};