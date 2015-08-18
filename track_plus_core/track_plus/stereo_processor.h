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
#include "overlapping_blob_pair.h"
#include "dtw.h"

class StereoProcessor
{
public:
	vector<OverlappingBlobPair> matches;
	
	bool compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_procesosr1,
				 MotionProcessorNew& motion_processor0, MotionProcessorNew& motion_processor1);

	void compute_dtw_stereo_match(vector<Point>& pt_vec0, vector<Point>& pt_vec1);
};