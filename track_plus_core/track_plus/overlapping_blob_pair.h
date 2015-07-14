#pragma once

#include "blob_detector_new.h"

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

struct compare_overlap_count
{
	bool operator() (const OverlappingBlobPair& pair0, const OverlappingBlobPair& pair1)
	{
		return (pair0.overlap_count > pair1.overlap_count);
	}
};