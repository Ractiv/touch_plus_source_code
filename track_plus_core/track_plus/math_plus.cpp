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

#include "math_plus.h"

float get_distance(float& x0, float& y0, float& x1, float& y1)
{
	return sqrt(pow(x0 - x1, 2) + pow(y0 - y1, 2));
}

float get_distance(const int x0, const int y0, const int x1, const int y1)
{
	return sqrt(pow(x0 - x1, 2) + pow(y0 - y1, 2));
}

float get_distance(Point& pt0, Point& pt1)
{
	return sqrt(pow(pt0.x - pt1.x, 2) + pow(pt0.y - pt1.y, 2));
}

float get_distance(Point2f& pt0, Point2f& pt1)
{
	return sqrt(pow(pt0.x - pt1.x, 2) + pow(pt0.y - pt1.y, 2));
}

float get_distance(Point3f& pt0, Point3f& pt1)
{
	return sqrt(pow(pt0.x - pt1.x, 2) + pow(pt0.y - pt1.y, 2) + pow(pt0.z - pt1.z, 2));
}

float map_val(const float value, const float left_min, const float left_max, const float right_min, const float right_max)
{
    float left_span = left_max - left_min;
    float right_span = right_max - right_min;
    float value_scaled = (value - left_min) / left_span;
    return right_min + (value_scaled * right_span);
}

float get_angle(Point& p1, Point& p2, Point& p3)
{
	float p12 = get_distance(p1, p2);
	float p13 = get_distance(p1, p3);
	float p23 = get_distance(p2, p3);
	return acos((pow(p12, 2) + pow(p13, 2) - pow(p23, 2)) / (2 * p12 * p13)) * 180 / CV_PI;
}

float get_angle(const float x0, const float y0, const float x1, const float y1)
{
	return atan2(y1 - y0, x1 - x0) * 180 / CV_PI;
}

bool get_intersection_at_y(Point& pt0, Point& pt1, const int y, Point& result)
{
	Point pt_y_min;
	Point pt_y_max;

	if (pt0.y < pt1.y)
	{
		pt_y_min = pt0;
		pt_y_max = pt1;
	}
	else if (pt0.y > pt1.y)
	{
		pt_y_min = pt1;
		pt_y_max = pt0;
	}
	else
		return false;

	float y_diff = pt_y_max.y - pt_y_min.y;
	float x_diff = pt_y_max.x - pt_y_min.x;
	float y_diff_from_y = pt_y_min.y - y;
	float scale = y_diff_from_y / y_diff;
	float x_diff_scaled = x_diff * scale;
	int x = pt_y_min.x - x_diff_scaled;
	result = Point(x, y);

	return true;
}

float get_slope(Point& pt0, Point& pt1)
{
	float val0 = (pt1.y - pt0.y) + 1;
	float val1 = (pt1.x - pt0.x) + 1;
	return val0 / val1;
}

bool get_intersection_at_y(const int x0, const int y0, const int x1, const int y1, const int y, Point& result)
{
	Point pt_y_min;
	Point pt_y_max;

	if (y0 < y1)
	{
		pt_y_min = Point(x0, y0);
		pt_y_max = Point(x1, y1);
	}
	else if (y0 > y1)
	{
		pt_y_min = Point(x1, y1);
		pt_y_max = Point(x0, y0);
	}
	else
		return false;

	float y_diff = pt_y_max.y - pt_y_min.y;
	float x_diff = pt_y_max.x - pt_y_min.x;
	float y_diff_from_y = pt_y_min.y - y;
	float scale = y_diff_from_y / y_diff;
	float x_diff_scaled = x_diff * scale;
	int x = pt_y_min.x - x_diff_scaled;
	result = Point(x, y);

	return true;
}

float get_mean(vector<uchar>& value_vec)
{
	float result = 0;
	for (uchar& val : value_vec)
		result += val;

	result /= value_vec.size();
	return result;
}

Point rotate_point(float theta, Point pt, Point origin)
{
	theta = theta * CV_PI / 180;

	Point result;
	result.x = cos(theta) * (pt.x - origin.x) - sin(theta) * (pt.y - origin.y) + origin.x;
	result.y = sin(theta) * (pt.x - origin.x) + cos(theta) * (pt.y - origin.y) + origin.y;

	return result;
}

Point3f cross_product(Point3f u, Point3f v)
{
    Point3f result;
    result.x = (u.y * v.z - u.z * v.y);
    result.y = (u.z * v.x - u.x * v.z);
    result.z = (u.x * v.y - u.y * v.x);
    return  result;
}

float dot_product(Point3f u, Point3f v)
{
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

Point3f normalize(Point3f& value)
{
	Point3f result;
    float factor = get_distance(value, Point3f(0, 0, 0));
    factor = 1 / factor;
    result.x = value.x * factor;
    result.y = value.y * factor;
    result.z = value.z * factor;
    return result;
}