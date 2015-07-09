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

#include "tool_mono_processor.h"

bool ToolMonoProcessor::compute(Mat& image_active_light_in, Mat& image_preprocessed_in, const string name)
{
	Mat image_preprocessed_old = value_store.get_mat("image_preprocessed_old", true);
	Mat image_subtraction = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	int diff_max = 0;

	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			int diff = abs(image_preprocessed_in.ptr<uchar>(j, i)[0] - image_preprocessed_old.ptr<uchar>(j, i)[0]);
			if (diff > diff_max)
				diff_max = diff;

			image_subtraction.ptr<uchar>(j, i)[0] = diff;
		}

	value_store.set_mat("image_preprocessed_old", image_preprocessed_in);

	threshold(image_subtraction, image_subtraction, diff_max * 0.25, 254, THRESH_BINARY);

	BlobDetectorNew* blob_detector_image_subtraction = value_store.get_blob_detector("blob_detector_image_subtraction");
	blob_detector_image_subtraction->compute(image_subtraction, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

	const int width_result = blob_detector_image_subtraction->x_max_result - blob_detector_image_subtraction->x_min_result;
	const int height_result = blob_detector_image_subtraction->y_max_result - blob_detector_image_subtraction->y_min_result;
	const int area_result = width_result * height_result;

	bool proceed = true;
	if (blob_detector_image_subtraction->blobs->size() > 50 || blob_detector_image_subtraction->blobs->size() == 0 || area_result > 8000)
		proceed = false;

	if (proceed)
	{
		vector<int> x_vec;
		vector<int> y_vec;
		for (BlobNew& blob : (*(blob_detector_image_subtraction->blobs)))
			for (Point& pt : blob.data)
			{
				x_vec.push_back(pt.x);
				y_vec.push_back(pt.y);
			}

		int x_mean = 0;
		for (int& x : x_vec)
			x_mean += x;

		int y_mean = 0;
		for (int& y : y_vec)
			y_mean += y;

		x_mean /= x_vec.size();
		y_mean /= y_vec.size();

		pt_motion_center = Point(x_mean, y_mean);
		Point pt_motion_center_old = value_store.get_point("pt_motion_center_old", pt_motion_center);

		pt_motion_center.x = (pt_motion_center.x - pt_motion_center_old.x) * 0.25 + pt_motion_center.x;
		pt_motion_center.y = (pt_motion_center.y - pt_motion_center_old.y) * 0.25 + pt_motion_center.y;

		if (pt_motion_center.x < 0)
			pt_motion_center.x = 0;
		if (pt_motion_center.x > WIDTH_SMALL_MINUS)
			pt_motion_center.x = WIDTH_SMALL_MINUS;

		if (pt_motion_center.y < 0)
			pt_motion_center.y = 0;
		if (pt_motion_center.y > HEIGHT_SMALL_MINUS)
			pt_motion_center.y = HEIGHT_SMALL_MINUS;

		value_store.set_point("pt_motion_center_old", pt_motion_center);

		vector<float> dist_vec;
		for (BlobNew& blob : (*(blob_detector_image_subtraction->blobs)))
			for (Point& pt : blob.data)
			{
				float dist = get_distance(pt, pt_motion_center);
				dist_vec.push_back(dist);
			}

		sort(dist_vec.begin(), dist_vec.end());
		radius = dist_vec[dist_vec.size() * 0.8] * 1.5;

		LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");
		low_pass_filter->compute(radius, 0.5, "radius");
	}

	Mat image_active_light_subtraction = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
			if (image_background_static.ptr<uchar>(j, i)[0] == 255)
				image_active_light_subtraction.ptr<uchar>(j, i)[0] = image_active_light_in.ptr<uchar>(j, i)[0];
			else
			{
				int diff = image_active_light_in.ptr<uchar>(j, i)[0] - image_background_static.ptr<uchar>(j, i)[0];
				if (diff < 0)
					diff = 0;

				image_active_light_subtraction.ptr<uchar>(j, i)[0] = diff;
			}

	Mat image_thresholded;
	threshold(image_active_light_subtraction, image_thresholded, 200, 254, THRESH_BINARY);
	dilate(image_thresholded, image_thresholded, Mat(), Point(-1, -1), 1);

	BlobDetectorNew* blob_detector_image_thresholded = value_store.get_blob_detector("blob_detector_image_thresholded");
	blob_detector_image_thresholded->compute(image_thresholded, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

	if (blob_detector_image_thresholded->blobs->size() == 0)
		return false;

	Mat image_circle = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	circle(image_circle, pt_motion_center, radius, Scalar(254), 1);
	floodFill(image_circle, pt_motion_center, Scalar(254));

	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
			if (image_circle.ptr<uchar>(j, i)[0] == 0)
				fill_image_background_static(i, j, image_active_light_in);

	BlobDetectorNew* blob_detector_image_circle = value_store.get_blob_detector("blob_detector_image_circle");
	blob_detector_image_circle->compute_location(image_circle, 254, pt_motion_center.x, pt_motion_center.y, true);

	blob_vec.clear();

	for (BlobNew& blob : *blob_detector_image_thresholded->blobs)
		for (Point& pt : blob_detector_image_circle->blob_max_size->data)
			if (blob.image_atlas.ptr<ushort>(pt.y, pt.x)[0] == blob.atlas_id)
			{
				blob_vec.push_back(&blob);
				break;
			}

	if (blob_vec.size() >= 4)
	{
		vector<int> blob_x_vec;
		vector<int> blob_y_vec;

		for (BlobNew* blob0 : blob_vec)
			for (BlobNew* blob1 : blob_vec)
			{
				blob_x_vec.push_back((blob0->x + blob1->x) / 2);
				blob_y_vec.push_back((blob0->y + blob1->y) / 2);
			}

		sort(blob_x_vec.begin(), blob_x_vec.end());
		sort(blob_y_vec.begin(), blob_y_vec.end());

		pt_led_center = Point(blob_x_vec[blob_x_vec.size() / 2], blob_y_vec[blob_y_vec.size() / 2]);
	}
	else
		return false;
	
	if (blob_vec.size() == 5)
	{
		float dist_min = 9999;
		BlobNew* blob_dist_min = NULL;

		for (BlobNew* blob : blob_vec)
		{
			float dist = blob->compute_min_dist(pt_led_center);
			if (dist < dist_min)
			{
				dist_min = dist;
				blob_dist_min = blob;
			}
		}

		vector<BlobNew*> blob_vec_new;
		for (BlobNew* blob : blob_vec)
			if (blob != blob_dist_min)
				blob_vec_new.push_back(blob);

		blob_vec = blob_vec_new;

		blob_dist_min->fill(image_thresholded, 127);
	}

	pt_led0 = Point(blob_vec[0]->x, blob_vec[0]->y);
	pt_led1 = Point(blob_vec[1]->x, blob_vec[1]->y);
	pt_led2 = Point(blob_vec[2]->x, blob_vec[2]->y);
	pt_led3 = Point(blob_vec[3]->x, blob_vec[3]->y);

	x_min0 = blob_vec[0]->x_min;
	x_max0 = blob_vec[0]->x_max;
	y_min0 = blob_vec[0]->y_min;
	y_max0 = blob_vec[0]->y_max;

	x_min1 = blob_vec[1]->x_min;
	x_max1 = blob_vec[1]->x_max;
	y_min1 = blob_vec[1]->y_min;
	y_max1 = blob_vec[1]->y_max;

	x_min2 = blob_vec[2]->x_min;
	x_max2 = blob_vec[2]->x_max;
	y_min2 = blob_vec[2]->y_min;
	y_max2 = blob_vec[2]->y_max;

	x_min3 = blob_vec[3]->x_min;
	x_max3 = blob_vec[3]->x_max;
	y_min3 = blob_vec[3]->y_min;
	y_max3 = blob_vec[3]->y_max;

	circle(image_thresholded, pt_motion_center, radius, Scalar(127), 2);
	imshow("image_thresholded" + name, image_thresholded);

	return true;
}

inline void ToolMonoProcessor::fill_image_background_static(const int x, const int y, Mat& image_in)
{
	uchar* pix_ptr = &(image_background_static.ptr<uchar>(y, x)[0]);

	if (*pix_ptr == 255)
		*pix_ptr = image_in.ptr<uchar>(y, x)[0];
	else
		*pix_ptr = *pix_ptr + ((image_in.ptr<uchar>(y, x)[0] - *pix_ptr) * 0.25);
}