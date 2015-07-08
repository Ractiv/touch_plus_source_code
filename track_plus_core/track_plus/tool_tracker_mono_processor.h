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

#include "value_store.h"

class ToolTrackerMonoProcessor
{
public:
	ValueStore value_store;
	Mat image_background_static = Mat(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1, Scalar(255));

	void compute(Mat& image_active_light_in, Mat& image_preprocessed_in, const string name);
	inline void fill_image_background_static(const int x, const int y, Mat& image_in);


private:

	struct compare_blob_angle
	{
		Point pivot;

		compare_blob_angle(Point& pivot_in)
		{
			pivot = pivot_in;
		}

		bool operator() (const BlobNew* blob0, const BlobNew* blob1)
		{
			float theta0 = get_angle(blob0->x, blob0->y, pivot.x, pivot.y);
			float theta1 = get_angle(blob1->x, blob1->y, pivot.x, pivot.y);

			return theta0 > theta1;
		}
	};
};