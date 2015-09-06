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

#include "hand_splitter_new.h"
#include "mat_functions.h"
#include "contour_functions.h"
#include "permutation.h"
#include "id_point.h"
#include "value_store.h"

class MonoProcessorNew
{
public:
	ValueStore value_store;

	LowPassFilter low_pass_filter;

	Point pt_hand_anchor;
	Point pt_hand_anchor_rotated;

	Point pt_index;
	Point pt_thumb;

	int dest_diff_x = 0;
	int dest_diff_y = 0;

	float angle_final = 0;

	vector<Point> pose_estimation_points;
	vector<Point> stereo_matching_points;

	bool compute(HandSplitterNew& hand_splitter, const string name, bool visualize);
	void sort_contour(vector<Point>& points, vector<Point>& points_sorted, Point& pivot);
	void compute_extension_line(Point pt_start, Point pt_end, const uchar length, vector<Point>& line_points, const bool reverse);
	BlobNew* find_parent_blob_before_rotation(BlobNew* blob, BlobDetectorNew* blob_detector);
	Point rotate(Point pt, float angle = 9999, Point anchor_in = Point(9999, 9999));
	Point unrotate(Point pt, float angle = 9999, Point anchor_in = Point(9999, 9999));
	Point find_pt_extremum(BlobNew* blob, Point pt_anchor);
};