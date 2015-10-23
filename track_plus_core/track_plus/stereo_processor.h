#pragma once

#include "mono_processor_new.h"
#include "point_resolver.h"
#include "pointer_mapper.h"

class StereoProcessor
{
public:
	ValueStore value_store;

	vector<Point3f> pt3d_vec;
	vector<float> confidence_vec;

	void compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1,
				 PointResolver& point_resolver, PointerMapper& pointer_mapper, Mat& image0, Mat& image1, bool visualize);
};