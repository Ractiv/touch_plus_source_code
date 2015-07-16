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

#include "mono_processor_new.h"
#include "motion_processor_new.h"
#include "mat_functions.h"
#include "reprojector.h"
#include "math_plus.h"

class HandResolver
{
public:
	const int window_width = 50;
	const int window_height = 30;
	const int window_width_half = window_width / 2;
	const int window_height_half = window_height / 2;

	BlobDetectorNew blob_detector_image_subtraction;

	Point2f pt_precise_index0 = Point(-1, -1);
	Point2f pt_precise_index1 = Point(-1, -1);

	Point2f pt_precise_thumb0 = Point(-1, -1);
	Point2f pt_precise_thumb1 = Point(-1, -1);

	void compute(MonoProcessorNew& mono_processor0,     MonoProcessorNew& mono_processor1,
				 MotionProcessorNew& motion_processor0, MotionProcessorNew& motion_processor1,
				 Mat& image0,                           Mat& image1,
				 Reprojector& reprojector);

	Point2f increase_resolution(Point& pt_in, Mat& image_in, Mat& image_background_in, uchar diff_threshold, uchar gray_threshold,
								Reprojector& reprojector, uchar side);

private:
	struct compare_dist_to_pt
	{
		Point pivot;

		compare_dist_to_pt(Point& pivot_in)
		{
			pivot = pivot_in;
		}

		bool operator() (Point& pt0, Point& pt1)
		{
			const float dist0 = get_distance(pt0, pivot);
			const float dist1 = get_distance(pt1, pivot);
			return dist0 < dist1;
		}
	};
};