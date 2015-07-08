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

#include "kalman.h"
#include "hungarian.h"
#include "opencv2/opencv.hpp"
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

class CTrack
{
public:
	int size_total = 1;
	float distance_travelled = 1;
	vector<Point2f> trace;
	static size_t NextTrackID;
	size_t track_id;
	size_t skipped_frames; 
	Point2f prediction;
	Point2f raw;
	TKalmanFilter* KF;
	CTrack(Point2f p, float dt, float Accel_noise_mag);
	~CTrack();
};


class CTracker
{
public:
	float dt;
	float Accel_noise_mag;
	float dist_thres;
	int maximum_allowed_skipped_frames;
	int max_trace_length;

	vector<CTrack*> tracks;
	void Update(vector<Point2f>& detections);

	CTracker();
	CTracker(float _dt, float _Accel_noise_mag, float _dist_thres, int _maximum_allowed_skipped_frames, int _max_trace_length);
	~CTracker(void);
};

