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

Point2f ToolResolver::compute(Mat& image_in, int x_min, int x_max, int y_min, int y_max)
{
	x_min = x_min * 4 - 10;
	x_max = x_max * 4 + 10;
	y_min = y_min * 4 - 10;
	y_max = y_max * 4 + 10;

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
