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
#include "value_store.h"
#include "value_accumulator.h"
#include "thinning_computer_new.h"
#include "pose_estimator.h"

class SCOPA
{
public:
	string algo_name = "scopa";

	ValueStore value_store;

	ValueAccumulator value_accumulator;

	Point pt_thumb;
	Point pt_index;
	Point pt_middle;
	Point pt_ring;
	Point pt_pinky;
	Point pt_palm;

	vector<Point> tip_points;

	bool compute_mono0(HandSplitterNew& hand_splitter, PoseEstimator& pose_estimator, const string name, bool visualize);
	static void compute_stereo();
	void compute_mono1(string name);
};