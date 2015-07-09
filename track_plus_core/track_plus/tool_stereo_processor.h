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

#include "tool_tracker_mono_processor.h"

struct OverlappingBlobPair
{
	BlobNew* blob0;
	BlobNew* blob1;
	int overlap_count;
	int index0;
	int index1;

	OverlappingBlobPair(BlobNew* blob_in0, BlobNew* blob_in1, int overlap_count_in, int index0_in, int index1_in)
	{
		blob0 = blob_in0;
		blob1 = blob_in1;
		overlap_count = overlap_count_in;
		index0 = index0_in;
		index1 = index1_in;
	}
};

class ToolStereoProessor
{
public:
	vector<OverlappingBlobPair> matches;
	
	bool compute(ToolTrackerMonoProcessor& tool_tarcker_mono_processor0, ToolTrackerMonoProcessor& tool_tarcker_mono_processor1);

private:
	struct compare_overlap_count
	{
		bool operator() (const OverlappingBlobPair& pair0, const OverlappingBlobPair& pair1)
		{
			return (pair0.overlap_count > pair1.overlap_count);
		}
	};
};