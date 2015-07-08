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

#include "tool_tracker_mono_processor.h"

void ToolTrackerMonoProcessor::compute(Mat& image_in, const string name)
{
	bool proceed = false;
	BlobDetectorNew* blob_detector = value_store.get_blob_detector("blob_detector");

	{
		Mat image_fade = value_store.get_mat("image_fade", true);
		for (int i = 0; i < WIDTH_SMALL; ++i)
			for (int j = 0; j < HEIGHT_SMALL; ++j)
			{
				image_fade.ptr<uchar>(j, i)[0] =
					(image_in.ptr<uchar>(j, i)[0] - image_fade.ptr<uchar>(j, i)[0]) * 0.05 + image_fade.ptr<uchar>(j, i)[0];

				if (image_in.ptr<uchar>(j, i)[0] >= 200)
					image_fade.ptr<uchar>(j, i)[0] = 254;
			}

	    Mat image_thresholded;
		threshold(image_fade, image_thresholded, 200, 254, THRESH_BINARY);
		dilate(image_thresholded, image_thresholded, Mat(), Point(-1, -1), 1);

		Mat image_thresholded_old = value_store.get_mat("image_thresholded_old", true);
		Mat image_subtraction = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

		for (int i = 0; i < WIDTH_SMALL; ++i)
			for (int j = 0; j < HEIGHT_SMALL; ++j)
				if (image_thresholded.ptr<uchar>(j, i)[0] > 0 && image_thresholded_old.ptr<uchar>(j, i)[0] == 0)
					image_subtraction.ptr<uchar>(j, i)[0] = 254;

		if (value_store.get_bool("first_frame", true))
			value_store.set_mat("image_thresholded_old", image_thresholded);

		value_store.get_bool("first_frame", false);

		blob_detector->compute(image_subtraction, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

		if (blob_detector->blobs->size() < 4 || blob_detector->blobs->size() >= 8)
			return;

		int area_result = (blob_detector->x_max_result - blob_detector->x_min_result) *
						  (blob_detector->y_max_result - blob_detector->y_min_result);

		int count_result = 0;
		for (BlobNew& blob : (*(blob_detector->blobs)))
			count_result += blob.count;

		// if (count_result < 50)
			// return;

		LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");
		low_pass_filter->compute(area_result, 0.1, "area_result");
		low_pass_filter->compute(count_result, 0.1, "area_result");

		// if (area_result > 2000 || count_result > 1500)
			// return;

		proceed = true;
		imshow("image_subtraction", image_subtraction);
		value_store.set_mat("image_thresholded_old", image_thresholded);
	}

	/*if (proceed)
	{
		Mat image_thresholded;
		threshold(image_in, image_thresholded, 150, 254, THRESH_BINARY);
		dilate(image_thresholded, image_thresholded, Mat(), Point(-1, -1), 1);

		BlobDetectorNew* blob_detector0 = value_store.get_blob_detector("blob_detector0");
		blob_detector0->compute(image_thresholded, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

		if (blob_detector0->blobs->size() == 0)
			return;

		vector<BlobNew*> blob_vec;

		for (BlobNew& blob : (*(blob_detector->blobs)))
		{
			float dist_min = 9999;
			BlobNew* blob_dist_min = NULL;

			for (BlobNew& blob0 : (*(blob_detector0->blobs)))
				for (Point& pt0 : blob0.data)
				{
					float dist = blob.compute_min_dist(pt0);
					if (dist < dist_min)
					{
						dist_min = dist;
						blob_dist_min = &blob0;
					}
				}

			blob_vec.push_back(blob_dist_min);
		}

		for (BlobNew* blob : blob_vec)
			blob->fill(image_thresholded, 64);

		vector<int> x_vec;
		vector<int> y_vec;

		for (BlobNew* blob0 : blob_vec)
			for (Point& pt0 : blob0->data)
				for (BlobNew* blob1 : blob_vec)
					for (Point& pt1 : blob1->data)
					{
						int x_middle = (pt0.x + pt1.x) / 2;
						int y_middle = (pt0.y + pt1.y) / 2;
						x_vec.push_back(x_middle);
						y_vec.push_back(y_middle);
					}

		sort(x_vec.begin(), x_vec.end());
		sort(y_vec.begin(), y_vec.end());
		Point pt_center_of_gravity = Point(x_vec[x_vec.size() / 2], y_vec[y_vec.size() / 2]);

		vector<Rect2D> rect_vec;

		int permutation_k = blob_detector0->blobs->size();
		if (permutation_k > 6)
			permutation_k = 6;

		compute_permutations(permutation_k, 4);
		for (vector<int> rows : permutations)
		{
			const int index0 = rows[0];
			const int index1 = rows[1];
			const int index2 = rows[2];
			const int index3 = rows[3];

			BlobNew* blob0 = &((*(blob_detector0->blobs))[index0]);
			BlobNew* blob1 = &((*(blob_detector0->blobs))[index1]);
			BlobNew* blob2 = &((*(blob_detector0->blobs))[index2]);
			BlobNew* blob3 = &((*(blob_detector0->blobs))[index3]);

			Point pt0 = Point(blob0->x, blob0->y);
			Point pt1 = Point(blob1->x, blob1->y);
			Point pt2 = Point(blob2->x, blob2->y);
			Point pt3 = Point(blob3->x, blob3->y);

			int x_min = std::min(pt0.x, std::min(pt1.x, std::min(pt2.x, pt3.x)));
			int x_max = std::max(pt0.x, std::max(pt1.x, std::max(pt2.x, pt3.x)));
			int y_min = std::min(pt0.y, std::min(pt1.y, std::min(pt2.y, pt3.y)));
			int y_max = std::max(pt0.y, std::max(pt1.y, std::max(pt2.y, pt3.y)));

			if (x_min < pt_center_of_gravity.x && x_max > pt_center_of_gravity.x &&
				y_min < pt_center_of_gravity.y && y_max > pt_center_of_gravity.y)
			{
				float dist0 = get_distance(pt0, pt_center_of_gravity);
				float dist1 = get_distance(pt1, pt_center_of_gravity);
				float dist2 = get_distance(pt2, pt_center_of_gravity);
				float dist3 = get_distance(pt3, pt_center_of_gravity);

				float dist_min = std::min(dist0, std::min(dist1, std::min(dist2, dist3)));
				float dist_max = std::max(dist0, std::max(dist1, std::max(dist2, dist3)));
				float dist_ratio = dist_max / dist_min;

				rect_vec.push_back(Rect2D(pt0, pt1, pt2, pt3, dist_ratio));
			}
		}

		circle(image_thresholded, pt_center_of_gravity, 5, Scalar(127), 2);
		imshow("image_thresholded", image_thresholded);
	}*/

	/*static Scalar Colors[] = { Scalar(255, 0, 0),
		                       Scalar(0, 255, 0),
		                       Scalar(0, 0, 255),
							   Scalar(255, 255, 0),
	                           Scalar(0, 255, 255),
							   Scalar(255, 0, 255),
							   Scalar(255, 127, 255),
							   Scalar(127, 0, 255),
							   Scalar(127, 0, 127) };

	Mat image_fade = value_store.get_mat("image_fade", true);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			image_fade.ptr<uchar>(j, i)[0] =
				(image_in.ptr<uchar>(j, i)[0] - image_fade.ptr<uchar>(j, i)[0]) * 0.05 + image_fade.ptr<uchar>(j, i)[0];

			if (image_in.ptr<uchar>(j, i)[0] >= 250)
				image_fade.ptr<uchar>(j, i)[0] = 254;
		}

    Mat image_thresholded;
	threshold(image_fade, image_thresholded, 250, 254, THRESH_BINARY);
	dilate(image_thresholded, image_thresholded, Mat(), Point(-1, -1), 1);

	BlobDetectorNew* blob_detector = value_store.get_blob_detector("blob_detector");
	blob_detector->compute(image_thresholded, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

	vector<Point2f> points_to_track;
	for (BlobNew& blob : *(blob_detector->blobs))
		points_to_track.push_back(Point2f(blob.x, blob.y));

	points_to_track.push_back(Point2f(0, HEIGHT_SMALL));

	tracker.Update(points_to_track);
	ToolTrackerMonoFrame frame;

	for (int i = 0; i < tracker.tracks.size(); ++i)
	{
		int track_id = tracker.tracks[i]->track_id;
		if (track_id > track_id_max)
			track_id_max = track_id;

		if (tracker.tracks[i]->trace.size() > 1 && tracker.tracks[i]->raw.x != 0 && tracker.tracks[i]->raw.y != HEIGHT_SMALL)
		{
			const int trace_size = tracker.tracks[i]->trace.size();
			float distance = get_distance(tracker.tracks[i]->trace[trace_size - 1], tracker.tracks[i]->trace[trace_size - 2]);
			tracker.tracks[i]->distance_travelled += distance;
			++tracker.tracks[i]->size_total;
		}

		frame.trace_vec.push_back(Trace2D(tracker.tracks[i]->raw,
										  tracker.tracks[i]->track_id,
										  tracker.tracks[i]->trace,
										  tracker.tracks[i]->distance_travelled,
										  tracker.tracks[i]->size_total));
	}

	if (frame_vec.size() <= cache_num)
	{
		frame_vec.push_back(frame);
		return;
	}

	int index_current = value_store.get_int("index_current", cache_num);

	if (index_current == cache_num)
		value_store.set_int("index_current", -1);

	value_store.set_int("index_current", value_store.get_int("index_current") + 1);
	frame_vec[index_current] = frame;

	Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC3);
	cvtColor(image_thresholded, image_visualization, CV_GRAY2BGR);

	ToolTrackerMonoFrame frame_cached = get_cached_frame(scan_num);

	for (Trace2D& trace : frame_cached.trace_vec)
	{
		if (trace.x != 0 && trace.y != HEIGHT_SMALL && (trace.dist / trace.size_total) > 1)
		{
			Point2f pt_old = Point2f(-1, -1);
			for (Point2f& pt_new : trace.points)
			{
				if (pt_old.x != -1 && pt_old.y != -1)
					line(image_visualization, pt_old, pt_new, Colors[trace.id % 9], 2, CV_AA);
				
				pt_old = pt_new;
			}

			circle(image_visualization, trace.point, 5, Colors[trace.id % 9], 2);
		}
	}

	flip(image_visualization, image_visualization, 0);
	imshow("image_visualization" + name, image_visualization);*/
}

ToolTrackerMonoFrame ToolTrackerMonoProcessor::get_cached_frame(const int past_frame_num)
{
	int index_current = value_store.get_int("index_current", cache_num);

	int index = index_current - past_frame_num;
	if (index < 0)
		index = cache_num + 1 + index;

	return frame_vec[index];
}