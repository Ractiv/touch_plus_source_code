#pragma once

#include "mono_processor_new.h"
#include "point_resolver.h"
#include "pointer_mapper.h"

void compute_stereo_overlap(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1,
							PointResolver& point_resolver, PointerMapper& pointer_mapper, Mat& image0, Mat& image1);