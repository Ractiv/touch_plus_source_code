#pragma once

#include <opencv2/opencv.hpp>

using namespace std;

struct PointPlus
{
	Point pt;
	int x;
	int y;

	Point pt_rotated;
	int x_rotated;
	int y_rotated;

	int index = -1;
	int track_index = -1;

	Scalar color = Scalar(255, 255, 255);

	PointPlus* matching_point = NULL;

	PointPlus(int _x, int _y, int _x_rotated, int _y_rotated)
	{
		x = _x;
		y = _y;
		pt = Point(x, y);

		x_rotated = _x_rotated;
		y_rotated = _y_rotated;
		pt_rotated = Point(x_rotated, y_rotated);
	}

	PointPlus(Point _pt, Point _pt_rotated)
	{
		pt = _pt;
		x = pt.x;
		y = pt.y;

		pt_rotated = _pt_rotated;
		x_rotated = pt_rotated.x;
		y_rotated = pt_rotated.y;
	}

	PointPlus()
	{
		
	}
};