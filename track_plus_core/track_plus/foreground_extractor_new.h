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
#include "value_store.h"

class ForegroundExtractorNew
{
public:
	string algo_name = "foreground_extractor";

	BlobDetectorNew blob_detector;
	ValueStore value_store;

	int x_min_result = 0;
	int x_max_result = 0;
	int y_min_result = 0;
	int y_max_result = 0;

	bool compute(Mat& image_in, MotionProcessorNew& motion_processor, const string name, const bool visualize);
};