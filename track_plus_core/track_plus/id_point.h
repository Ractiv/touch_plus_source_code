#pragma once
#include "opencv2/opencv.hpp"

using namespace std;

struct IDPoint
{
	int id;
	int x;
	int y;

	Point pt;
	Point pt_predict = Point(-1, -1);

	IDPoint(int id_in, Point pt_in)
	{
		id = id_in;
		pt = pt_in;
		x = pt.x;
		y = pt.y;
	}
};