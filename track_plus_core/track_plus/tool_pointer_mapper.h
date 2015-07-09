/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

#pragma once

#include "reprojector.h"
#include "tool_stereo_processor.h"
#include "tool_resolver.h"

class ToolPointerMapper
{
public:
	Point3f pt0;
	Point3f pt1;
	Point3f pt2;
	Point3f pt3;
	Point3f pt_center;

	ToolResolver tool_resolver;

	void compute(Reprojector& reprojector, ToolStereoProcessor& tool_stereo_processor, Mat& image_in0, Mat& image_in1);
};