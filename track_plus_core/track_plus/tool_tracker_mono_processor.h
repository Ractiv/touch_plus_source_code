#pragma once

#include "value_store.h"
#include "c_tracker.h"

class ToolTrackerMonoProcessor
{
public:
	void compute(Mat& image_in, const string name);
};