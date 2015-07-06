#pragma once

#include "value_store.h"
#include "c_tracker.h"

class ToolTrackerMonoProcessor
{
public:
	CTracker tracker = CTracker(0.2, 0.5, 9999, 0, 10);
	ValueStore value_store;
	
	int track_id_max = -1;

	void compute(Mat& image_in, const string name);
};