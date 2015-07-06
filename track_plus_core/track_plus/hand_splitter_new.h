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

#include "globals.h"
#include "blob_detector_new.h"
#include "math_plus.h"
#include "filesystem.h"
#include "motion_processor_new.h"
#include "foreground_extractor_new.h"
#include "histogram_builder.h"
#include "blob_tracker.h"

struct Uchar3D
{
	uchar uchar0;
	uchar uchar1;
	uchar uchar2;
};

class HandSplitterNew
{
public:
	BlobDetectorNew blob_detector_image_histogram;

	BlobTracker blob_tracker = BlobTracker();

	HistogramBuilder histogram_builder;

	MotionProcessorNew* motion_processor_ptr;

	vector<int> count_left_vec = vector<int>();
	vector<int> count_right_vec = vector<int>();
	vector<int> index_vec0 = vector<int>();
	vector<int> index_vec1 = vector<int>();

	vector<BlobNew> primary_hand_blobs;

	int x_middle_median = 0;

	int count_left_median = 0;
	int count_right_median = 0;

	int id0 = -2;
	int id1 = -2;

	int id0_new = -2;
	int id1_new = -2;

	int x_min_on_merge = 0;
	int x_max_on_merge = 0;

	int count_left_on_merge = 0;
	int count_right_on_merge = 0;

	int x_min_result = 0;
	int x_max_result = 0;
	int y_min_result = 0;
	int y_max_result = 0;

	bool merged = false;

	bool compute(ForegroundExtractorNew& foreground_extractor, MotionProcessorNew& motion_processor, const string name);
	BlobNew* find_blob_by_id(const int id_in);
	BlobNew* find_blob_by_index(const int index_in);
	void move_x_middle(Mat& image_histogram, MotionProcessorNew& motion_processor, const int x_in);
};