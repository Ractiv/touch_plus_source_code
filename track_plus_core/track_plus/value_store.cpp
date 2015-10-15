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

#include "value_store.h"
#include "console_log.h"

void ValueStore::set_bool(string name, bool value)
{
	bool_map[name] = value;
}

void ValueStore::set_float(string name, float value)
{
	float_map[name] = value;
}

void ValueStore::set_int(string name, int value)
{
	int_map[name] = value;
}

void ValueStore::set_point(string name, Point value)
{
	int_map[name + "x"] = value.x;
	int_map[name + "y"] = value.y;
}

void ValueStore::set_point2f(string name, Point2f value)
{
	float_map[name + "x"] = value.x;
	float_map[name + "y"] = value.y;
}

void ValueStore::set_point3f(string name, Point3f value)
{
	float_map[name + "x"] = value.x;
	float_map[name + "y"] = value.y;
	float_map[name + "z"] = value.z;
}

void ValueStore::set_mat(string name, Mat value)
{
	mat_map[name] = value;
}

vector<int>* ValueStore::push_int(string name, int value)
{
	if (int_vec_map.count(name) == 0)
	{
		int_vec_map[name] = &int_vec_pool[int_vec_pool_index];
		++int_vec_pool_index;

		if (int_vec_pool_index == pool_size)
			console_log("overflow int_vec_pool_index");
	}

	vector<int>* vec_ptr = int_vec_map[name];
	vec_ptr->push_back(value);

	return vec_ptr;
}

vector<float>* ValueStore::push_float(string name, float value)
{
	if (float_vec_map.count(name) == 0)
	{
		float_vec_map[name] = &float_vec_pool[float_vec_pool_index];
		++float_vec_pool_index;

		if (float_vec_pool_index == pool_size)
			console_log("overflow float_vec_pool_index");
	}

	vector<float>* vec_ptr = float_vec_map[name];
	vec_ptr->push_back(value);

	return vec_ptr;
}

vector<Point>* ValueStore::push_point(string name, Point value)
{
	if (point_vec_map.count(name) == 0)
	{
		point_vec_map[name] = &point_vec_pool[point_vec_pool_index];
		++point_vec_pool_index;

		if (point_vec_pool_index == pool_size)
			console_log("overflow point_vec_pool_index");
	}

	vector<Point>* vec_ptr = point_vec_map[name];
	vec_ptr->push_back(value);

	return vec_ptr;
}

vector<BlobNew>* ValueStore::push_blob(string name, BlobNew value)
{
	if (blob_vec_map.count(name) == 0)
	{
		blob_vec_map[name] = &blob_vec_pool[blob_vec_pool_index];
		++blob_vec_pool_index;

		if (blob_vec_pool_index == pool_size)
			console_log("overflow blob_vec_pool_index");
	}

	vector<BlobNew>* vec_ptr = blob_vec_map[name];
	vec_ptr->push_back(value);

	return vec_ptr;
}

vector<Mat>* ValueStore::push_mat(string name, Mat value)
{
	if (mat_vec_map.count(name) == 0)
	{
		mat_vec_map[name] = &mat_vec_pool[mat_vec_pool_index];
		++mat_vec_pool_index;

		if (mat_vec_pool_index == pool_size)
			console_log("overflow mat_vec_pool_index");
	}

	vector<Mat>* vec_ptr = mat_vec_map[name];
	vec_ptr->push_back(value);

	return vec_ptr;
}

bool ValueStore::get_bool(string name, bool if_not_exist_result)
{
	if (bool_map.count(name) == 0)
		bool_map[name] = if_not_exist_result;

	return bool_map[name];
}

float ValueStore::get_float(string name, float if_not_exist_result)
{
	if (float_map.count(name) == 0)
		float_map[name] = if_not_exist_result;

	return float_map[name];
}

int ValueStore::get_int(string name, int if_not_exist_result)
{
	if (int_map.count(name) == 0)
		int_map[name] = if_not_exist_result;

	return int_map[name];
}

Point ValueStore::get_point(string name, Point if_not_exist_result)
{
	if (int_map.count(name + "x") == 0)
	{
		int_map[name + "x"] = if_not_exist_result.x;
		int_map[name + "y"] = if_not_exist_result.y;
	}

	return Point(int_map[name + "x"], int_map[name + "y"]);
}

Point2f ValueStore::get_point2f(string name, Point2f if_not_exist_result)
{
	if (float_map.count(name + "x") == 0)
	{
		float_map[name + "x"] = if_not_exist_result.x;
		float_map[name + "y"] = if_not_exist_result.y;
	}

	return Point2f(float_map[name + "x"], float_map[name + "y"]);
}

Point3f ValueStore::get_point3f(string name, Point3f if_not_exist_result)
{
	if (float_map.count(name + "z") == 0)
	{
		float_map[name + "x"] = if_not_exist_result.x;
		float_map[name + "y"] = if_not_exist_result.y;
		float_map[name + "z"] = if_not_exist_result.z;
	}

	return Point3f(float_map[name + "x"], float_map[name + "y"], float_map[name + "z"]);
}

Mat ValueStore::get_mat(string name, bool if_not_exist_return_zero_mat)
{
	if (mat_map.count(name) == 0)
    {
		if (if_not_exist_return_zero_mat)
			mat_map[name] = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
		else
			mat_map[name] = Mat();
    }

	return mat_map[name];
}

vector<int>* ValueStore::get_int_vec(string name)
{
	if (int_vec_map.count(name) == 0)
	{
		int_vec_map[name] = &int_vec_pool[int_vec_pool_index];
		++int_vec_pool_index;

		if (int_vec_pool_index == pool_size)
			console_log("overflow int_vec_pool_index");
	}

	return int_vec_map[name];
}

vector<float>* ValueStore::get_float_vec(string name)
{
	if (float_vec_map.count(name) == 0)
	{
		float_vec_map[name] = &float_vec_pool[float_vec_pool_index];
		++float_vec_pool_index;

		if (float_vec_pool_index == pool_size)
			console_log("overflow float_vec_pool_index");
	}

	return float_vec_map[name];
}

vector<Point>* ValueStore::get_point_vec(string name)
{
	if (point_vec_map.count(name) == 0)
	{
		point_vec_map[name] = &point_vec_pool[point_vec_pool_index];
		++point_vec_pool_index;

		if (point_vec_pool_index == pool_size)
			console_log("overflow point_vec_pool_index");
	}

	return point_vec_map[name];
}

vector<BlobNew>* ValueStore::get_blob_vec(string name)
{
	if (blob_vec_map.count(name) == 0)
	{
		blob_vec_map[name] = &blob_vec_pool[blob_vec_pool_index];
		++blob_vec_pool_index;

		if (blob_vec_pool_index == pool_size)
			console_log("overflow blob_vec_pool_index");
	}

	return blob_vec_map[name];
}

vector<Mat>* ValueStore::get_mat_vec(string name)
{
	if (mat_vec_map.count(name) == 0)
	{
		mat_vec_map[name] = &mat_vec_pool[mat_vec_pool_index];
		++mat_vec_pool_index;

		if (mat_vec_pool_index == pool_size)
			console_log("overflow mat_vec_pool_index");
	}

	return mat_vec_map[name];
}

BlobDetectorNew* ValueStore::get_blob_detector(string name)
{
	if (blob_detector_map.count(name) == 0)
	{
		blob_detector_map[name] = &blob_detector_pool[blob_detector_pool_index];
		++blob_detector_pool_index;

		if (blob_detector_pool_index == pool_size)
			console_log("overflow blob_detector_pool_index");
	}

	return blob_detector_map[name];
}

HistogramBuilder* ValueStore::get_histogram_builder(string name)
{
	if (histogram_builder_map.count(name) == 0)
	{
		histogram_builder_map[name] = &histogram_builder_pool[histogram_builder_pool_index];
		++histogram_builder_pool_index;

		if (histogram_builder_pool_index == pool_size)
			console_log("overflow histogram_builder_pool_index");
	}

	return histogram_builder_map[name];
}

LowPassFilter* ValueStore::get_low_pass_filter(string name)
{
	if (low_pass_filter_map.count(name) == 0)
	{
		low_pass_filter_map[name] = &low_pass_filter_pool[low_pass_filter_pool_index];
		++low_pass_filter_pool_index;

		if (low_pass_filter_pool_index == pool_size)
			console_log("overflow low_pass_filter_pool_index");
	}

	return low_pass_filter_map[name];
}

bool ValueStore::has_point2f(string name)
{
	return float_map.count(name + "x") > 0;
}

bool ValueStore::has_mat(string name)
{
	return mat_map.count(name) > 0;
}