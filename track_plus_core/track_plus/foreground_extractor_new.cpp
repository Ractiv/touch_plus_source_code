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
#include "mat_functions.h"

bool ForegroundExtractorNew::compute(Mat& image_in, MotionProcessorNew& motion_processor, const string name, const bool visualize)
{
	const int x_separator_middle = motion_processor.x_separator_middle;
	const uchar gray_threshold_left = motion_processor.gray_threshold_left;
	const uchar gray_threshold_right = motion_processor.gray_threshold_right;
	const uchar diff_threshold = motion_processor.diff_threshold;
	const Mat image_background_static = motion_processor.image_background_static;

	Mat image_foreground = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
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

	for (BlobNew& blob : *blob_detector.blobs)
		if (blob.y > motion_processor.y_separator_down ||
			blob.x_max < motion_processor.x_separator_left ||
			blob.x_min > motion_processor.x_separator_right)
		{
			blob.active = false;
			blob.fill(image_foreground, 0);
		}
		else if (blob.width <= 3 || blob.height <= 3)
			blob.active = false;

	if (visualize && enable_imshow)
		imshow("image_foreground" + name, image_foreground);

	return true;
}