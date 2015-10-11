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

#pragma once

#include <opencv2/opencv.hpp>
#include <math.h>

using namespace cv;

float get_distance(float x0, float y0, float x1, float y1, bool accurate);
float get_distance(int x0, int y0, int x1, int y1, bool accurate);
float get_distance(Point pt0, Point pt1, bool accurate);
float get_distance(Point2f pt0, Point2f pt1, bool accurate);
float get_distance(Point3f pt0, Point3f pt1, bool accurate);
float map_val(float value, float left_min, float left_max, float right_min, float right_max);
float get_angle(Point pt0, Point pt1, Point pt2);
float get_angle(float x0, float y0, float x1, float y1);
float get_slope(Point pt0, Point pt1);
bool get_intersection_at_y(Point pt0, Point pt1, int i, Point& result);
bool get_intersection_at_y(int x0, int y0, int x1, int y1, int y, Point& result);
float get_mean(std::vector<uchar> value_vec);
Point rotate_point(float theta, Point pt, Point origin);
Point3f cross_product(Point3f u, Point3f v);
float dot_product(Point3f u, Point3f v);
Point3f normalize(Point3f value);
uchar get_quadrant(int x, int y, int pivot_x, int pivot_y);

float linear(float x, float m, float c);
float exponential(float x, float a, float b, float c);
float power(float x, float a, float b);
float quadratic(float x, float a, float b, float c);
float cubic(float x, float a, float b, float c, float d);

//abc are sides, ABC are angles opposing abc sides in radians
float solve_triangle_A_abc(float a, float b, float c);
float solve_triangle_B_abc(float a, float b, float c);
float solve_triangle_C_abc(float a, float b, float c);
float solve_triangle_area_abC(float a, float b, float C);
float solve_triangle_area_cbA(float c, float b, float A);
float solve_triangle_area_acB(float a, float c, float B);
float solve_triangle_bisector_a_bcA(float b, float c, float A);
float solve_triangle_bisector_b_acB(float a, float c, float B);
float solve_triangle_bisector_c_abC(float a, float b, float C);