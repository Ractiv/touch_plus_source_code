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

#include "reprojector.h"
#include "low_pass_filter.h"
#include "warper.h"
#include "ray.h"
#include "value_store.h"
#include "hand_resolver.h"

class PointerMapper
{
public:
	bool active = false;
	bool calibrated = false;
	bool do_reset = false;

	bool thumb_down = false;
	bool index_down = false;

	Point3f pt_palm;
	Point3f pt_index;
	Point3f pt_thumb;

	Point2f pt_cursor_index;
	Point2f pt_cursor_thumb;
	Point2f pt_pinch_to_zoom_index;
	Point2f pt_pinch_to_zoom_thumb;

	vector<Point3f> pt_calib_vec0;
	vector<Point3f> pt_calib_vec1;
	vector<Point3f> pt_calib_vec2;
	vector<Point3f> pt_calib_vec3;

	Point3f pt_calib0;
	Point3f pt_calib1;
	Point3f pt_calib2;
	Point3f pt_calib3;

	Point3f pt_calib0_projected;
	Point3f pt_calib1_projected;
	Point3f pt_calib2_projected;
	Point3f pt_calib3_projected;

	float dist_calib0_plane;
	float dist_calib1_plane;
	float dist_calib2_plane;
	float dist_calib3_plane;

	float d_max;

	float dist_cursor_index_plane;
	float dist_cursor_thumb_plane;

    Point3f direction_plane;

    Plane plane;

	ValueStore value_store;

	CWarper rect_warper;

	void compute(HandResolver& hand_resolver, Reprojector& reprojector);
	void add_calibration_point(const uchar index);
	void reset_calibration(const uchar index);
	void compute_calibration_points();
	bool project_to_plane(Point3f& pt, Point3f& result, float& dist_to_plane);
	float compute_hit_dist(Point3f& pt);
	void compute_cursor_point(bool& target_down, Point2f& pt_target0, Point2f& pt_target1, Point3f& pt_target, Reprojector& reprojector,
							  Point2f& pt_cursor, float& dist_cursor_target_plane, const float actuation_dist, string name);

	void compute_pinch_to_zoom(HandResolver& hand_resolver);
	void reset();
};