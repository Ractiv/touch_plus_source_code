#pragma once

#include "mono_processor_new.h"
#include "overlapping_blob_pair.h"

class StereoProcessor
{
public:
	vector<OverlappingBlobPair> matches;
	
	bool compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_procesosr1,
				 MotionProcessorNew& motion_processor0, MotionProcessorNew& motion_processor1);
};