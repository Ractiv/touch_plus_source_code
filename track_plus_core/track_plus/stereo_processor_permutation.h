#pragma once

#include "scopa.h"
#include "point_resolver.h"
#include "pointer_mapper.h"

void compute_stereo_permutation(SCOPA& scopa0, SCOPA& scopa1,
								PointResolver& point_resolver, PointerMapper& pointer_mapper, Mat& image0, Mat& image1);