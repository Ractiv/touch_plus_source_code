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
#include "globals.h"
#include "blob_detector_new.h"
#include "histogram_builder.h"
#include "low_pass_filter.h"

using namespace std;
using namespace cv;

class ValueStore
{
public:
	const int pool_size = 100;

	unordered_map<string, float> float_map;
	unordered_map<string, int> int_map;
	unordered_map<string, bool> bool_map;
	unordered_map<string, vector<int>*> int_vec_map;
	unordered_map<string, vector<float>*> float_vec_map;
	unordered_map<string, vector<Point>*> point_vec_map;
	unordered_map<string, vector<BlobNew>*> blob_vec_map;
	unordered_map<string, vector<Mat>*> mat_vec_map;
	unordered_map<string, Mat> mat_map;
	unordered_map<string, BlobDetectorNew*> blob_detector_map;
	unordered_map<string, HistogramBuilder*> histogram_builder_map;
	unordered_map<string, LowPassFilter*> low_pass_filter_map;

	vector<int>* int_vec_pool = new vector<int>[pool_size];
	vector<float>* float_vec_pool = new vector<float>[pool_size];
	vector<Point>* point_vec_pool = new vector<Point>[pool_size];
	vector<BlobNew>* blob_vec_pool = new vector<BlobNew>[pool_size];
	vector<Mat>* mat_vec_pool = new vector<Mat>[pool_size];
	BlobDetectorNew* blob_detector_pool = new BlobDetectorNew[pool_size];
	HistogramBuilder* histogram_builder_pool = new HistogramBuilder[pool_size];
	LowPassFilter* low_pass_filter_pool = new LowPassFilter[pool_size];

	int int_vec_pool_index = 0;
	int float_vec_pool_index = 0;
	int point_vec_pool_index = 0;
	int blob_vec_pool_index = 0;
	int mat_vec_pool_index = 0;
	int blob_detector_pool_index = 0;
	int histogram_builder_pool_index = 0;
	int low_pass_filter_pool_index = 0;

	void reset();
	void set_bool(string name, bool value);
	void set_float(string name, float value);
	void set_int(string name, int value);
	void set_point(string name, Point value);
	void set_point2f(string name, Point2f value);
	void set_point3f(string name, Point3f value);
	void set_mat(string name, Mat value);

	vector<int>* push_int(string name, int value);
	vector<float>* push_float(string name, float value);
	vector<Point>* push_point(string name, Point value);
	vector<BlobNew>* push_blob(string name, BlobNew value);
	vector<Mat>* push_mat(string name, Mat value);

	bool get_bool(string name, bool if_not_exist_result = false);
	float get_float(string name, float if_not_exist_result = 0);
	int get_int(string name, int if_not_exist_result = 0);
	Point get_point(string name, Point if_not_exist_result = Point(0, 0));
	Point2f get_point2f(string name, Point2f if_not_exist_result = Point2f(0, 0));
	Point3f get_point3f(string name, Point3f if_not_exist_result = Point3f(0, 0, 0));
	Mat get_mat(string name, bool if_not_exist_return_zero_mat = false);

	vector<int>* get_int_vec(string name);
	vector<float>* get_float_vec(string name);
	vector<Point>* get_point_vec(string name);
	vector<BlobNew>* get_blob_vec(string name);
	vector<Mat>* get_mat_vec(string name);
	BlobDetectorNew* get_blob_detector(string name);
	HistogramBuilder* get_histogram_builder(string name);
	LowPassFilter* get_low_pass_filter(string name);

	bool has_point2f(string name);
	bool has_mat(string name);
};