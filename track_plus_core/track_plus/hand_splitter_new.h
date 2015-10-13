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

#include "motion_processor_new.h"
#include "foreground_extractor_new.h"

class HandSplitterNew
{
public:
	string algo_name = "hand_splitter";

	ValueStore value_store;
	ValueAccumulator value_accumulator;

	vector<BlobNew> blobs_right;
	vector<BlobNew> blobs_left;

	int x_min_result_right = 0;
	int x_max_result_right = 0;
	int y_min_result_right = 0;
	int y_max_result_right = 0;

	int x_min_result_left = 0;
	int x_max_result_left = 0;
	int y_min_result_left = 0;
	int y_max_result_left = 0;

	bool compute(ForegroundExtractorNew& foreground_extractor, MotionProcessorNew& motion_processor, string name, bool visualize);
};