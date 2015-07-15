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

#include "tool_pointer_mapper.h"

void ToolPointerMapper::compute(Reprojector& reprojector, ToolStereoProcessor& tool_stereo_processor, Mat& image_in0, Mat& image_in1)
{
	Point2f pt00 = tool_resolver.compute(image_in0,
										 tool_stereo_processor.matches[0].blob0->x_min,
										 tool_stereo_processor.matches[0].blob0->x_max,
										 tool_stereo_processor.matches[0].blob0->y_min,
										 tool_stereo_processor.matches[0].blob0->y_max);

	Point2f pt01 = tool_resolver.compute(image_in1,
										 tool_stereo_processor.matches[0].blob1->x_min,
										 tool_stereo_processor.matches[0].blob1->x_max,
										 tool_stereo_processor.matches[0].blob1->y_min,
										 tool_stereo_processor.matches[0].blob1->y_max);

	Point2f pt10 = tool_resolver.compute(image_in0,
										 tool_stereo_processor.matches[1].blob0->x_min,
										 tool_stereo_processor.matches[1].blob0->x_max,
										 tool_stereo_processor.matches[1].blob0->y_min,
										 tool_stereo_processor.matches[1].blob0->y_max);

	Point2f pt11 = tool_resolver.compute(image_in1,
										 tool_stereo_processor.matches[1].blob1->x_min,
										 tool_stereo_processor.matches[1].blob1->x_max,
										 tool_stereo_processor.matches[1].blob1->y_min,
										 tool_stereo_processor.matches[1].blob1->y_max);

	Point2f pt20 = tool_resolver.compute(image_in0,
										 tool_stereo_processor.matches[2].blob0->x_min,
										 tool_stereo_processor.matches[2].blob0->x_max,
										 tool_stereo_processor.matches[2].blob0->y_min,
										 tool_stereo_processor.matches[2].blob0->y_max);

	Point2f pt21 = tool_resolver.compute(image_in1,
										 tool_stereo_processor.matches[2].blob1->x_min,
										 tool_stereo_processor.matches[2].blob1->x_max,
										 tool_stereo_processor.matches[2].blob1->y_min,
										 tool_stereo_processor.matches[2].blob1->y_max);

	Point2f pt30 = tool_resolver.compute(image_in0,
										 tool_stereo_processor.matches[3].blob0->x_min,
										 tool_stereo_processor.matches[3].blob0->x_max,
										 tool_stereo_processor.matches[3].blob0->y_min,
										 tool_stereo_processor.matches[3].blob0->y_max);

	Point2f pt31 = tool_resolver.compute(image_in1,
										 tool_stereo_processor.matches[3].blob1->x_min,
										 tool_stereo_processor.matches[3].blob1->x_max,
										 tool_stereo_processor.matches[3].blob1->y_min,
										 tool_stereo_processor.matches[3].blob1->y_max);

	pt0 = reprojector.reproject_to_3d(pt00.x, pt00.y, pt01.x, pt01.y);
	pt1 = reprojector.reproject_to_3d(pt10.x, pt10.y, pt11.x, pt11.y);
	pt2 = reprojector.reproject_to_3d(pt20.x, pt20.y, pt21.x, pt21.y);
	pt3 = reprojector.reproject_to_3d(pt30.x, pt30.y, pt31.x, pt31.y);

	vector<float> x_vec0;
	vector<float> y_vec0;
	vector<float> x_vec1;
	vector<float> y_vec1;

	vector<Point2f> pt_vec0 = { pt00, pt10, pt20, pt30 };
	vector<Point2f> pt_vec1 = { pt01, pt11, pt21, pt31 };

	for (Point2f& pt0 : pt_vec0)
		for (Point2f& pt1 : pt_vec0)
		{
			x_vec0.push_back((pt0.x + pt1.x) / 2);
			y_vec0.push_back((pt0.y + pt1.y) / 2);
		}

	for (Point2f& pt0 : pt_vec1)
		for (Point2f& pt1 : pt_vec1)
		{
			x_vec1.push_back((pt0.x + pt1.x) / 2);
			y_vec1.push_back((pt0.y + pt1.y) / 2);
		}

	sort(x_vec0.begin(), x_vec0.end());
	sort(x_vec1.begin(), x_vec1.end());
	sort(y_vec0.begin(), y_vec0.end());
	sort(y_vec1.begin(), y_vec1.end());

	Point2f center0 = Point2f(x_vec0[x_vec0.size() / 2], y_vec0[y_vec0.size() / 2]);
	Point2f center1 = Point2f(x_vec1[x_vec1.size() / 2], y_vec1[y_vec1.size() / 2]);

	pt_center = reprojector.reproject_to_3d(center0.x, center0.y, center1.x, center1.y);

	Mat image_visualization_tool_pointer_mapper = Mat::zeros(480, 640, CV_8UC3);

	circle(image_visualization_tool_pointer_mapper, Point(pt0.x + 320, pt0.y + 240), pow(pt0.z / 100, 2), Scalar(127, 127, 127), 4);
	circle(image_visualization_tool_pointer_mapper, Point(pt1.x + 320, pt1.y + 240), pow(pt1.z / 100, 2), Scalar(127, 127, 127), 4);
	circle(image_visualization_tool_pointer_mapper, Point(pt2.x + 320, pt2.y + 240), pow(pt2.z / 100, 2), Scalar(127, 127, 127), 4);
	circle(image_visualization_tool_pointer_mapper, Point(pt3.x + 320, pt3.y + 240), pow(pt3.z / 100, 2), Scalar(127, 127, 127), 4);
	circle(image_visualization_tool_pointer_mapper, Point(pt_center.x + 320, pt_center.y + 240),
												    pow(pt_center.z / 100, 2), Scalar(127, 127, 127), 4);

	putText(image_visualization_tool_pointer_mapper, "0", cvPoint(pt0.x + 320, pt0.y + 240), 
													      FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(255, 255, 255), 1, CV_AA);
	putText(image_visualization_tool_pointer_mapper, "1", cvPoint(pt1.x + 320, pt1.y + 240), 
													      FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(255, 255, 255), 1, CV_AA);
	putText(image_visualization_tool_pointer_mapper, "2", cvPoint(pt2.x + 320, pt2.y + 240), 
													      FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(255, 255, 255), 1, CV_AA);
	putText(image_visualization_tool_pointer_mapper, "3", cvPoint(pt3.x + 320, pt3.y + 240), 
													      FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(255, 255, 255), 1, CV_AA);
	putText(image_visualization_tool_pointer_mapper, "center", cvPoint(pt_center.x + 320, pt_center.y + 240),
													           FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(255, 255, 255), 1, CV_AA);

	imshow("image_visualization_tool_pointer_mapper", image_visualization_tool_pointer_mapper);
}