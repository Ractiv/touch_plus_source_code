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

#include "tool_stereo_processor.h"

bool ToolStereoProessor::compute(ToolTrackerMonoProcessor& tool_tarcker_mono_processor0,
							     ToolTrackerMonoProcessor& tool_tarcker_mono_processor1)
{
	int x_diff = tool_tarcker_mono_processor0.pt_led_center.x - tool_tarcker_mono_processor1.pt_led_center.x;
	int y_diff = tool_tarcker_mono_processor0.pt_led_center.y - tool_tarcker_mono_processor1.pt_led_center.y;

	vector<OverlappingBlobPair> overlapping_blob_pair_vec;

	int index0 = 0;
	int index1 = 0;

	for (BlobNew* blob0 : tool_tarcker_mono_processor0.blob_vec)
	{
		index1 = 0;
		for (BlobNew* blob1 : tool_tarcker_mono_processor1.blob_vec)
		{
			/*int overlap_count = 0;
			for (Point& pt1 : blob1->data)
			{
				int x_offsetted = pt1.x + x_diff;
				int y_offsetted = pt1.y + y_diff;

				if (blob0->image_atlas.ptr<ushort>(y_offsetted, x_offsetted)[0] == blob0->atlas_id)
					++overlap_count;
			}*/

			int overlap_count = blob0->compute_overlap(*blob1, -x_diff, -y_diff, 10);

			overlapping_blob_pair_vec.push_back(OverlappingBlobPair(blob0, blob1, overlap_count, index0, index1));
			++index1;
		}
		++index0;
	}

	sort(overlapping_blob_pair_vec.begin(), overlapping_blob_pair_vec.end(), compare_overlap_count());

	bool checker0[100];
	bool checker1[100];

	matches.clear();

	for (OverlappingBlobPair& pair : overlapping_blob_pair_vec)
	{
		bool occupied0 = checker0[pair.index0] == true;
		bool occupied1 = checker1[pair.index1] == true;

		if (!occupied0 && !occupied1)
		{
			checker0[pair.index0] = true;
			checker1[pair.index1] = true;

			matches.push_back(pair);
		}
	}

	if (matches.size() == 0)
		return false;

	Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	for (OverlappingBlobPair& pair : matches)
	{
		pair.blob0->fill(image_visualization, 254);
		pair.blob1->fill(image_visualization, 127);
		line(image_visualization, Point(pair.blob0->x, pair.blob0->y), Point(pair.blob1->x, pair.blob1->y), Scalar(64), 2);
	}

	imshow("image_visualization", image_visualization);
	return true;
}