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

#include "hand_splitter_new.h"

bool HandSplitterNew::compute(ForegroundExtractorNew& foreground_extractor, MotionProcessorNew& motion_processor, const string name)
{
	int x_separator_middle = 0;

	if (mode == "tool")
		x_separator_middle = WIDTH_SMALL / 2;
	else if (mode == "surface")
		x_separator_middle = motion_processor.x_separator_middle;

	primary_hand_blobs = vector<BlobNew>();
	x_min_result = 9999;
	x_max_result = 0;
	y_min_result = 9999;
	y_max_result = 0;

	for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
		if (blob.active)
			if (blob.x > x_separator_middle)
			{
				primary_hand_blobs.push_back(blob);

				if (blob.x_min < x_min_result)
					x_min_result = blob.x_min;
				if (blob.x_max > x_max_result)
					x_max_result = blob.x_max;
				if (blob.y_min < y_min_result)
					y_min_result = blob.y_min;
				if (blob.y_max > y_max_result)
					y_max_result = blob.y_max;
			}

	if (primary_hand_blobs.size() > 0)
		return true;
	else
		return false;
}