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

#include "blob_new.h"
#include "globals.h"

struct CollectionInfo
{
	int position0;
	int position1;

	int index0;
	int index1;

	ushort atlas_id0;
	ushort atlas_id1;

	int overlap_count;

	float overlap_ratio0;
	float overlap_ratio1;
	float overlap_ratio_max;

	bool active = false;
};

class BlobTracker
{
public:
	bool merge = false;
	bool split = false;

	int index = 0;

	vector<BlobNew> blobs_new;

	vector<vector<BlobNew>> history;

	void compute(vector<BlobNew>& blobs_in);
	bool find_in_history(const int id, const int frames_back, BlobNew*& result);

private:
	struct compare_overlap_count
	{
		bool operator() (const CollectionInfo& info0, const CollectionInfo& info1)
		{
			return (info0.overlap_count > info1.overlap_count);
		}
	};
};