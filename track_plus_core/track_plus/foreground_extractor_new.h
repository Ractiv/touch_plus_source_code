#pragma once

#include "motion_processor_new.h"
#include "mat_functions.h"

class ForegroundExtractorNew
{
public:
	BlobDetectorNew blob_detector;
	Mat image_foreground;

	void init();
	bool compute(Mat& image_in, Mat& image_smoothed_in, MotionProcessorNew& motion_processor, const string name, const bool visualize);
};