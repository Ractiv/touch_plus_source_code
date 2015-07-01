#pragma once

#include <opencv2/opencv.hpp>
#include <math.h>

using namespace cv;

float get_distance(float& x0, float& y0, float& x1, float& y1);
float get_distance(const int x0, const int y0, const int x1, const int y1);
float get_distance(Point& pt0, Point& pt1);
float get_distance(Point2f& pt0, Point2f& pt1);
float get_distance(Point3f& pt0, Point3f& pt1);
float map_val(const float value, const float left_min, const float left_max, const float right_min, const float right_max);
float get_angle(Point& pt0, Point& pt1, Point& pt2);
float get_angle(const float x0, const float y0, const float x1, const float y1);
float get_slope(Point& pt0, Point& pt1);
bool get_intersection_at_y(Point& pt0, Point& pt1, const int i, Point& result);
bool get_intersection_at_y(const int x0, const int y0, const int x1, const int y1, const int y, Point& result);
float get_mean(std::vector<uchar>& value_vec);
Point rotate_point(float theta, Point pt, Point origin);
Point3f cross_product(Point3f u, Point3f v);
float dot_product(Point3f u, Point3f v);
Point3f normalize(Point3f& value);