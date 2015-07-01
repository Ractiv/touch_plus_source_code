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

#include "thinning_computer_new.h"

void ThinningComputerNew::thinning_iteration(Mat& image_in, const int iter, vector<Point>& points, int& iterations)
{
    Mat marker = Mat::zeros(image_in.size(), CV_8UC1);

    for (Point& pt : points)
    {
    	const uchar p2 = image_in.ptr<uchar>(pt.y - 1, pt.x)[0];
        const uchar p3 = image_in.ptr<uchar>(pt.y - 1, pt.x + 1)[0];
        const uchar p4 = image_in.ptr<uchar>(pt.y, pt.x + 1)[0];
        const uchar p5 = image_in.ptr<uchar>(pt.y + 1, pt.x + 1)[0];
        const uchar p6 = image_in.ptr<uchar>(pt.y + 1, pt.x)[0];
        const uchar p7 = image_in.ptr<uchar>(pt.y + 1, pt.x - 1)[0];
        const uchar p8 = image_in.ptr<uchar>(pt.y, pt.x - 1)[0];
        const uchar p9 = image_in.ptr<uchar>(pt.y - 1, pt.x - 1)[0];

		if (p2 == 0 && p3 == 0 && p4 == 0 && p5 == 0 && p6 == 0 && p7 == 0 && p8 == 0 && p9 == 0)
			continue;

        const int A  = (p2 == 0 && p3 > 0) + (p3 == 0 && p4 > 0) + (p4 == 0 && p5 > 0) + (p5 == 0 && p6 > 0) + 
                       (p6 == 0 && p7 > 0) + (p7 == 0 && p8 > 0) + (p8 == 0 && p9 > 0) + (p9 == 0 && p2 > 0);

        const int B  = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
        const int m1 = iter == 0 ? (p2 * p4 * p6) : (p2 * p4 * p8);
        const int m2 = iter == 0 ? (p4 * p6 * p8) : (p2 * p6 * p8);

        if (A == 1 && (B >= 508 && B <= 1524) && m1 == 0 && m2 == 0)
            marker.ptr<uchar>(pt.y,pt.x)[0] = 254;
    }

    image_in &= ~marker;
    ++iterations;
}

vector<Point> ThinningComputerNew::compute(Mat& image_in, BlobNew* blob_in,
                                           int x_min_in, int x_max_in, int y_min_in, int y_max_in,
                                           const int max_iter)
{
	vector<Point> points;
    if (blob_in == NULL)
    {
    	Mat image_dilated = image_in;
    	// dilate(image_in, image_dilated, Mat(), Point(-1, -1), 1);

        const int w0 = x_min_in == -1 ? 0 : x_min_in;
        const int h0 = x_max_in == -1 ? 0 : x_max_in;
        const int w1 = x_max_in == -1 ? image_in.cols : x_max_in + 1;
        const int h1 = y_max_in == -1 ? image_in.rows : y_max_in + 1;

    	for (int i = w0; i < w1; ++i)
        	for (int j = h0; j < h1; ++j)
        		if (image_dilated.ptr<uchar>(j, i)[0] > 0)
        			points.push_back(Point(i, j));
    }
    else
        points = blob_in->data;

    Mat prev = Mat::zeros(image_in.size(), CV_8UC1);
    Mat diff = Mat::zeros(image_in.size(), CV_8UC1);
    int iterations = 0;

    do
    {
        thinning_iteration(image_in, 0, points, iterations);
        thinning_iteration(image_in, 1, points, iterations);

        for (Point& pt : points)
        {
            const uchar image_in_ptr = image_in.ptr<uchar>(pt.y, pt.x)[0];
            diff.ptr<uchar>(pt.y, pt.x)[0] = abs(image_in_ptr - prev.ptr<uchar>(pt.y, pt.x)[0]);
            prev.ptr<uchar>(pt.y, pt.x)[0] = image_in_ptr;
        }

        if (max_iter > -1 && iterations > max_iter)
            break;
    } 
    while (countNonZero(diff) > 0);

    vector<Point> result;

    for (Point& pt : points)
        if (image_in.ptr<uchar>(pt.y, pt.x)[0] > 0)
            result.push_back(pt);

    return result;
}