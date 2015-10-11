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

#include "point_resolver.h"
#include "mat_functions.h"

BlobDetectorNew blob_detector_point_resolver;

const int window_width = 50;
const int window_height = 20;
const int window_width_half = window_width / 2;
const int window_height_half = window_height / 2;

Point2f do_resolve(Point& pt_in,                    Mat image_in,
	               Mat image_background_in,        const uchar diff_threshold,
			   	   const uchar gray_threshold_left, const uchar gray_threshold_right,
				   Reprojector* reprojector,        const uchar side,
										  		    const int x_separator_middle)
{
	Point pt_large = pt_in * 4;

	int x0 = pt_large.x - window_width_half;
	int y0 = pt_large.y - window_height;
	int x1 = pt_large.x + window_width_half;
	int y1 = pt_large.y;

	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 > WIDTH_LARGE_MINUS)
		x1 = WIDTH_LARGE_MINUS;
	if (y1 > HEIGHT_LARGE_MINUS)
		y1 = HEIGHT_LARGE_MINUS;

	Rect crop_rect = Rect(x0, y0, x1 - x0, y1 - y0);

	int crop_rect_small_width = (x1 - x0) / 4;
	if (crop_rect_small_width > WIDTH_SMALL_MINUS)
		crop_rect_small_width = WIDTH_SMALL_MINUS;

	int crop_rect_small_height = (y1 - y0) / 4;
	if (crop_rect_small_height > HEIGHT_SMALL_MINUS)
		crop_rect_small_height = HEIGHT_SMALL_MINUS;

	Rect crop_rect_small = Rect(x0 / 4, y0 / 4, crop_rect_small_width, crop_rect_small_height);

	Mat image_cropped = image_in(crop_rect);
	// GaussianBlur(image_cropped, image_cropped, Size(21, 21), 0, 0);

	Mat image_cropped_preprocessed;
	compute_channel_diff_image(image_cropped, image_cropped_preprocessed, true, "image_cropped_preprocessed");
	Point pt_offset0;
	image_cropped_preprocessed = reprojector->remap(&image_cropped_preprocessed, x0, y0, side, pt_offset0);

	Mat image_background_cropped = image_background_in(crop_rect_small).clone();

	const int image_background_cropped_width = image_background_cropped.cols;
	const int image_background_cropped_height = image_background_cropped.rows;

	uchar gray_max = 0;
	for (int i = 0; i < image_background_cropped_width; ++i)
		for (int j = 0; j < image_background_cropped_height; ++j)
		{
			const uchar gray = image_background_cropped.ptr<uchar>(j, i)[0];
			if (gray < 255 && gray > gray_max)
				gray_max = gray;
		}

	for (int i = 0; i < image_background_cropped_width; ++i)
		for (int j = 0; j < image_background_cropped_height; ++j)
			if (image_background_cropped.ptr<uchar>(j, i)[0] == 255)
				image_background_cropped.ptr<uchar>(j, i)[0] = gray_max;

	Mat image_background_cropped_scaled;
	resize(image_background_cropped, image_background_cropped_scaled, crop_rect.size(), 0, 0, INTER_LINEAR);

	Point pt_offset1;
	image_background_cropped_scaled = reprojector->remap(&image_background_cropped_scaled, x0, y0, side, pt_offset1);

	if (image_background_cropped_scaled.cols == 0)
		return Point(-1, -1);

	const int image_width = image_cropped_preprocessed.cols;
	const int image_height = image_cropped_preprocessed.rows;

	Mat image_subtraction = Mat::zeros(image_height, image_width, CV_8UC1);
	for (int i = 0; i < image_width; ++i)
		for (int j = 0; j < image_height; ++j)
			if (image_cropped_preprocessed.ptr<uchar>(j, i)[0] > gray_threshold_right)
				image_subtraction.ptr<uchar>(j, i)[0] = abs(image_cropped_preprocessed.ptr<uchar>(j, i)[0] - 
														    image_background_cropped_scaled.ptr<uchar>(j, i)[0]);

	threshold(image_subtraction, image_subtraction, diff_threshold, 254, THRESH_BINARY);
	GaussianBlur(image_subtraction, image_subtraction, Size(5, 5), 0, 0);
	threshold(image_subtraction, image_subtraction, 100, 254, THRESH_BINARY);

	float point_x = 0;
	float point_y = 0;
	float point_count = 0;

	blob_detector_point_resolver.compute(image_subtraction, 254, 0, image_subtraction.cols, 0, image_subtraction.rows, true);
	if (blob_detector_point_resolver.blobs->size() <= 1)
	{
		for (BlobNew& blob : *blob_detector_point_resolver.blobs)
			for (Point& pt : blob.data)
			{
				point_x += pt.x;
				point_y += pt.y;
				++point_count;
			}
	}
	else
	{
		const int x_middle = image_subtraction.cols / 2;
		BlobNew* blob_x_diff_min = NULL;
		int x_diff_min = 9999;

		for (BlobNew& blob : *blob_detector_point_resolver.blobs)
		{
			int x_diff = abs(blob.pt_y_min.x - x_middle);
			if (x_diff < x_diff_min)
			{
				blob_x_diff_min = &blob;
				x_diff_min = x_diff;
			}
		}
		for (Point& pt : blob_x_diff_min->data)
		{
			point_x += pt.x;
			point_y += pt.y;
			++point_count;
		}
	}
	if (point_count == 0)
		point_count = 1;

	point_x /= point_count;
	point_y /= point_count;

	if (point_x == 0 && point_y == 0)
		return Point2f(9999, 9999);

	return Point2f(point_x + pt_offset0.x, point_y + pt_offset0.y);
}

PointResolver::PointResolver(MotionProcessorNew& _motion_processor0, MotionProcessorNew& _motion_processor1, Reprojector& _reprojector)
{
	motion_processor0 = &_motion_processor0;
	motion_processor1 = &_motion_processor1;
	reprojector = &_reprojector;
}

Point2f PointResolver::compute(Point pt, Mat image_color, uchar side)
{
	MotionProcessorNew* motion_processor;
	if (side == 0)
		motion_processor = motion_processor0;
	else
		motion_processor = motion_processor1;

	return do_resolve(pt,
					  image_color,
					  motion_processor->image_background_static,
					  motion_processor->diff_threshold,
					  motion_processor->gray_threshold_left,
					  motion_processor->gray_threshold_right,
					  reprojector,
					  side,
					  motion_processor->x_separator_middle);
}