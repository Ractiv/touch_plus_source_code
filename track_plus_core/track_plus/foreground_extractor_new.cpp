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

#include "foreground_extractor_new.h"

bool ForegroundExtractorNew::compute(Mat& image_in, MotionProcessorNew& motion_processor, const string name, const bool visualize)
{
	Mat image_foreground;

	if (mode == "tool")
	{
		threshold(image_in, image_foreground, max(motion_processor.gray_threshold_left,
												  motion_processor.gray_threshold_right), 254, THRESH_BINARY);
		
		blob_detector.compute(image_foreground, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, false);

		if (visualize && enable_imshow)
			imshow("image_foreground" + name, image_foreground);

		return true;
	}

	const int x_separator_middle = motion_processor.x_separator_middle;
	const uchar gray_threshold_left = motion_processor.gray_threshold_left;
	const uchar gray_threshold_right = motion_processor.gray_threshold_right;
	const uchar diff_threshold = motion_processor.diff_threshold;
	const Mat image_background_static = motion_processor.image_background_static;

	image_foreground = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			const uchar gray_current = image_in.ptr<uchar>(j, i)[0];

			if ((i <= x_separator_middle && gray_current > gray_threshold_left) ||
				(i > x_separator_middle && gray_current > gray_threshold_right))
			{
				if (image_background_static.ptr<uchar>(j, i)[0] == 255)
					image_foreground.ptr<uchar>(j, i)[0] = 254;
				else
					image_foreground.ptr<uchar>(j, i)[0] = abs(gray_current - image_background_static.ptr<uchar>(j, i)[0]);
			}
		}

	threshold(image_foreground, image_foreground, diff_threshold, 254, THRESH_BINARY);
	blob_detector.compute(image_foreground, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, false);

	int y_max = 0;
	for (BlobNew& blob : *blob_detector.blobs)
		if (blob.count < motion_processor.noise_size ||
			blob.y > motion_processor.y_separator_down ||
			blob.x_max < motion_processor.x_separator_left ||
			blob.x_min > motion_processor.x_separator_right)
		{
			blob.active = false;
			blob.fill(image_foreground, 0);
		}
		else if (blob.width <= 3 || blob.height <= 3)
			blob.active = false;
		else
			if (blob.y_max > y_max)
				y_max = blob.y_max;

	int y_max_old = value_store.get_int("y_max_old", y_max);
	value_store.set_int("y_max_old", y_max);

	if (y_max > y_max_old)
		y_max += (y_max - y_max_old) + 10;

	if (y_max > HEIGHT_SMALL_MINUS)
		y_max = HEIGHT_SMALL_MINUS;
	else if (y_max < HEIGHT_SMALL_MINUS / 3)
		y_max = HEIGHT_SMALL_MINUS / 3;

	motion_processor.compute_y_separator_down = false;
	motion_processor.y_separator_down = y_max;

	if (visualize && enable_imshow)
		imshow("image_foreground" + name, image_foreground);

	return true;
}