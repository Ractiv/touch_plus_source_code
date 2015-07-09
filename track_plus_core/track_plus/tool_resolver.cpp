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

#include "tool_resolver.h"

void ToolResolver::compute(Mat& image_in, ToolTrackerMonoProcessor& tool_tracker_mono_processor)
{
	Point2f pt0_temp = compute_blob(image_in, tool_tracker_mono_processor.x_min0 * 4 - 10,
											  tool_tracker_mono_processor.x_max0 * 4 + 10,
											  tool_tracker_mono_processor.y_min0 * 4 - 10,
											  tool_tracker_mono_processor.y_max0 * 4 + 10);

	Point2f pt1_temp = compute_blob(image_in, tool_tracker_mono_processor.x_min1 * 4 - 10,
											  tool_tracker_mono_processor.x_max1 * 4 + 10,
											  tool_tracker_mono_processor.y_min1 * 4 - 10,
											  tool_tracker_mono_processor.y_max1 * 4 + 10);

	Point2f pt2_temp = compute_blob(image_in, tool_tracker_mono_processor.x_min2 * 4 - 10,
											  tool_tracker_mono_processor.x_max2 * 4 + 10,
											  tool_tracker_mono_processor.y_min2 * 4 - 10,
											  tool_tracker_mono_processor.y_max2 * 4 + 10);

	Point2f pt3_temp = compute_blob(image_in, tool_tracker_mono_processor.x_min3 * 4 - 10,
											  tool_tracker_mono_processor.x_max3 * 4 + 10,
											  tool_tracker_mono_processor.y_min3 * 4 - 10,
											  tool_tracker_mono_processor.y_max3 * 4 + 10);

	vector<float> x_vec;
	vector<float> y_vec;
	pt_vec = { pt0_temp, pt1_temp, pt2_temp, pt3_temp };

	for (Point2f& pt_0 : pt_vec)
		for (Point2f& pt_1 : pt_vec)
		{
			x_vec.push_back((pt_0.x + pt_1.x) / 2);
			y_vec.push_back((pt_0.y + pt_1.y) / 2);
		}

	sort(x_vec.begin(), x_vec.end());
	sort(y_vec.begin(), y_vec.end());

	pt_center = Point2f(x_vec[x_vec.size() / 2], y_vec[y_vec.size() / 2]);

	/*circle(image_in, pt0, 20, Scalar(127, 127, 127), 4);
	circle(image_in, pt1, 20, Scalar(127, 127, 127), 4);
	circle(image_in, pt2, 20, Scalar(127, 127, 127), 4);
	circle(image_in, pt3, 20, Scalar(127, 127, 127), 4);
	circle(image_in, pt_center, 20, Scalar(127, 127, 127), 4);

	imshow("image_in", image_in);*/
}

Point2f ToolResolver::compute_blob(Mat& image_in, int x_min, int x_max, int y_min, int y_max)
{
	if (x_min < 0)
		x_min = 0;
	if (x_max > WIDTH_LARGE_MINUS)
		x_max = WIDTH_LARGE_MINUS;
	if (y_min < 0)
		y_min = 0;
	if (y_max > HEIGHT_LARGE_MINUS)
		y_max = HEIGHT_LARGE_MINUS;

	Mat image_cropped = image_in(Rect(x_min, y_min, x_max - x_min, y_max - y_min));

	Mat image_preprocessed;
	compute_channel_diff_image(image_cropped, image_preprocessed, true, "image_preprocessed");

	Mat image_active_light;
	compute_active_light_image(image_cropped, image_preprocessed, image_active_light);

	float x_mean = 0;
	float y_mean = 0;
	int count = 0;

	const int width_const = x_max - x_min;
	const int height_const = y_max - y_min;
	for (int i = 0; i < width_const; ++i)
		for (int j = 0; j < height_const; ++j)
			if (image_active_light.ptr<uchar>(j, i)[0] > 200)
			{
				x_mean += i;
				y_mean += j;
				++count;
			}

	x_mean = x_mean / count + x_min;
	y_mean = y_mean / count + y_min;

	return Point2f(x_mean, y_mean);
}
