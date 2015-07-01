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

#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "globals.h"
#include "blob_new.h"

using namespace cv;
using namespace std;

class ThinningComputerNew
{
public:
	void thinning_iteration(Mat& image_in, const int iter, vector<Point>& points, int& iterations);
	
	vector<Point> compute(Mat& image_in, BlobNew* blob_in = NULL, 
						  int x_min_in = -1, int x_max_in = -1, int y_min_in = -1, int y_max_in = -1,
						  const int max_iter = -1);
};