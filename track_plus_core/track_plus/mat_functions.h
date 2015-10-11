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

#include <opencv2/opencv.hpp>
#include "globals.h"
#include "math_plus.h"
#include "low_pass_filter.h"
#include "contour_functions.h"
#include "value_store.h"

using namespace cv;

extern LowPassFilter mat_functions_low_pass_filter;

void threshold_get_bounds(Mat& image_in, Mat& image_out, const int threshold_val, int& x_min, int& x_max, int& y_min, int& y_max);
Mat rotate_image(const Mat& image_in, const float angle, const Point origin, const int border);
Mat translate_image(Mat& image_in, const int x_diff, const int y_diff);
Mat resize_image(Mat& image_in, const float scale);
void distance_transform(Mat& image_in, float& dist_min, float& dist_max, Point& pt_dist_min, Point& pt_dist_max, bool accurate);
bool compute_channel_diff_image(Mat& image_in, Mat& image_out, bool normalize, string name, bool set_norm_range = false, bool low_pass = false);
void compute_max_image(Mat& image_in, Mat& image_out);
void compute_active_light_image(Mat& image_regular, Mat& image_channel_diff, Mat& image_out);
void compute_color_segmented_image(Mat& image_in, Mat& image_out);
void compute_motion_structure_image(Mat& image_in, Mat& image_out, string name);
void print_mat_type(Mat& image_in);
void put_text(string text, Mat& img, int x, int y);