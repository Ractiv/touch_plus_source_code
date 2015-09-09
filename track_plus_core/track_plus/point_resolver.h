#pragma once

#include "reprojector.h"
#include "motion_processor_new.h"

Point2f resolve_point(Point pt, uchar side, Mat& image_color, MotionProcessorNew& motion_processor, Reprojector& reprojector);