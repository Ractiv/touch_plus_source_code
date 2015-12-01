#pragma once

#include "scopa.h"
#include "point_resolver.h"
#include "pointer_mapper.h"

class StereoProcessorNew
{
public:
	ValueStore value_store;

	void compute(SCOPA& scopa0, SCOPA& scopa1,
				 PointResolver& point_resolver, PointerMapper& pointer_mapper, Mat& image0, Mat& image1, bool visualize);
};