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