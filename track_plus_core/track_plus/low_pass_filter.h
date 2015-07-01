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

#include <unordered_map>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class LowPassFilter
{
public:
	unordered_map<string, float> value_map;

	void compute(float& value, const float alpha, const string name);
	void compute_if_smaller(float& value, const float alpha, const string name);
	void compute_if_smaller(uchar& value, const float alpha, const string name);
	void compute_if_larger(float& value, const float alpha, const string name);
	void compute_if_larger(uchar& value, const float alpha, const string name);
	void compute(int& value, const float alpha, const string name);
	void compute(uchar& value, const float alpha, const string name);
	void compute(Point& value, const float alpha, const string name);
	void compute(Point2f& value, const float alpha, const string name);
	void compute(Point3f& value, const float alpha, const string name);
	void reset();
};