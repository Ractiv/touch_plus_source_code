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

void ToolTrackerMonoProcessor::compute(Mat& image_active_light_in, Mat& image_preprocessed_in, const string name)
{
	Mat image_preprocessed_old = value_store.get_mat("image_preprocessed_old", true);
	Mat image_subtraction = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	int diff_max = 0;

	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			int diff = abs(image_preprocessed_in.ptr<uchar>(j, i)[0] - image_preprocessed_old.ptr<uchar>(j, i)[0]);
			if (diff > diff_max)
				diff_max = diff;

			image_subtraction.ptr<uchar>(j, i)[0] = diff;
		}

	value_store.set_mat("image_preprocessed_old", image_preprocessed_in);

	threshold(image_subtraction, image_subtraction, diff_max * 0.25, 254, THRESH_BINARY);

	BlobDetectorNew* blob_detector_image_subtraction = value_store.get_blob_detector("blob_detector_image_subtraction");
	blob_detector_image_subtraction->compute(image_subtraction, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

	if (blob_detector_image_subtraction->blobs->size() > 50 || blob_detector_image_subtraction->blobs->size() == 0)
		return;

	vector<int> x_vec;
	vector<int> y_vec;
	for (BlobNew& blob : (*(blob_detector_image_subtraction->blobs)))
		for (Point& pt : blob.data)
		{
			x_vec.push_back(pt.x);
			y_vec.push_back(pt.y);
		}

	int x_mean = 0;
	for (int& x : x_vec)
		x_mean += x;

	int y_mean = 0;
	for (int& y : y_vec)
		y_mean += y;

	x_mean /= x_vec.size();
	y_mean /= y_vec.size();

	Point pt_motion_center = Point(x_mean, y_mean);
	Point pt_motion_center_old = value_store.get_point("pt_motion_center_old", pt_motion_center);

	pt_motion_center.x = (pt_motion_center.x - pt_motion_center_old.x) * 0.25 + pt_motion_center.x;
	pt_motion_center.y = (pt_motion_center.y - pt_motion_center_old.y) * 0.25 + pt_motion_center.y;

	if (pt_motion_center.x < 0)
		pt_motion_center.x = 0;
	if (pt_motion_center.x > WIDTH_SMALL_MINUS)
		pt_motion_center.x = WIDTH_SMALL_MINUS;

	if (pt_motion_center.y < 0)
		pt_motion_center.y = 0;
	if (pt_motion_center.y > HEIGHT_SMALL_MINUS)
		pt_motion_center.y = HEIGHT_SMALL_MINUS;

	value_store.set_point("pt_motion_center_old", pt_motion_center);

	vector<float> dist_vec;
	for (BlobNew& blob : (*(blob_detector_image_subtraction->blobs)))
		for (Point& pt : blob.data)
		{
			float dist = get_distance(pt, pt_motion_center);
			dist_vec.push_back(dist);
		}

	sort(dist_vec.begin(), dist_vec.end());
	float radius = dist_vec[dist_vec.size() * 0.8] * 1.5;

	Mat image_thresholded;
	threshold(image_active_light_in, image_thresholded, 200, 254, THRESH_BINARY);
	dilate(image_thresholded, image_thresholded, Mat(), Point(-1, -1), 2);

	BlobDetectorNew* blob_detector_image_thresholded = value_store.get_blob_detector("blob_detector_image_thresholded");
	blob_detector_image_thresholded->compute(image_thresholded, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

	if (blob_detector_image_thresholded->blobs->size() == 0)
		return;

	Mat image_circle = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	circle(image_circle, pt_motion_center, radius, Scalar(254), 1);
	floodFill(image_circle, pt_motion_center, Scalar(254));

	BlobDetectorNew* blob_detector_image_circle = value_store.get_blob_detector("blob_detector_image_circle");
	blob_detector_image_circle->compute_location(image_circle, 254, pt_motion_center.x, pt_motion_center.y, true);

	vector<BlobNew*> blob_vec;
	for (BlobNew& blob : *blob_detector_image_thresholded->blobs)
		for (Point& pt : blob_detector_image_circle->blob_max_size->data)
			if (blob.image_atlas.ptr<uchar>(pt.y, pt.x)[0] == blob.atlas_id)
			{
				blob_vec.push_back(&blob);
				break;
			}

	vector<Point>* model = value_store.get_point_vec("model");

	if (blob_vec.size() == 4)
	{
		model->clear();

		sort(blob_vec.begin(), blob_vec.end(), compare_blob_angle(pt_motion_center));
		for (BlobNew* blob : blob_vec)
			model->push_back(Point(blob->x, blob->y));

		BlobNew* blob_old = NULL;
		for (BlobNew* blob_new : blob_vec)
		{
			if (blob_old != NULL)
				line(image_thresholded, Point(blob_new->x, blob_new->y), Point(blob_old->x, blob_old->y), Scalar(127), 2);

			blob_old = blob_new;
		}
		line(image_thresholded, Point(blob_vec[0]->x, blob_vec[0]->y), Point(blob_old->x, blob_old->y), Scalar(127), 2);
	}
	else if (model->size() == 4 && blob_vec.size() < 4)
	{
		/*vector<Point> pt_vec;
		for (BlobNew* blob : blob_vec)
			pt_vec.push_back(Point(blob->x, blob->y));

		if (blob_detector_image_thresholded->blobs->size() > permutation_k)
		{
			vector<Point> circle_vec;
			midpoint_circle(pt_motion_center.x, pt_motion_center.y, radius, circle_vec);

			for (BlobNew& blob : (*(blob_detector_image_thresholded->blobs)))
			{
				float dist_min = 9999;
				for (Point& pt : circle_vec)
				{
					float dist = get_distance(blob.x, blob.y, pt.x, pt.y);
					if (dist < dist_min)
						dist_min = dist;
				}

				blob.dist = dist_min;
			}

			blob_detector_image_thresholded->sort_blobs_by_dist();
			for (BlobNew& blob : *blob_detector_image_thresholded->blobs)
			{
				bool found = false;
				for (BlobNew* blob_compare : blob_vec)
					if (&blob == blob_compare)
					{
						found = true;
						break;
					}

				if (found)
					continue;

				pt_vec.push_back(Point(blob.x, blob.y));
				if (pt_vec.size() == permutation_k)
					break;
			}
		}
		else
			for (BlobNew& blob : *blob_detector_image_thresholded->blobs)
			{
				bool found = false;
				for (BlobNew* blob_compare : blob_vec)
					if (&blob == blob_compare)
					{
						found = true;
						break;
					}

				if (found)
					continue;

				pt_vec.push_back(Point(blob.x, blob.y));
				if (pt_vec.size() == permutation_k)
					break;
			}

		Point pt0_dist_min;
		Point pt1_dist_min;
		Point pt2_dist_min;
		Point pt3_dist_min;
		float dist_min = 9999;

		compute_permutations(permutation_k, 4);
		for (vector<int> rows : permutations)
		{
			const int index0 = rows[0];
			const int index1 = rows[1];
			const int index2 = rows[2];
			const int index3 = rows[3];

			Point pt0 = pt_vec[index0];
			Point pt1 = pt_vec[index1];
			Point pt2 = pt_vec[index2];
			Point pt3 = pt_vec[index3];

			const int x_min = std::min(pt0.x, std::min(pt1.x, std::min(pt2.x, pt3.x)));
			const int x_max = std::max(pt0.x, std::max(pt1.x, std::max(pt2.x, pt3.x)));
			const int y_min = std::min(pt0.y, std::min(pt1.y, std::min(pt2.y, pt3.y)));
			const int y_max = std::max(pt0.y, std::max(pt1.y, std::max(pt2.y, pt3.y)));

			if (x_min < pt_motion_center.x && x_max > pt_motion_center.x && y_min < pt_motion_center.y && y_max > pt_motion_center.y)
			{
				const float dist0 = get_distance(pt0, (*model)[0]);
				const float dist1 = get_distance(pt1, (*model)[1]);
				const float dist2 = get_distance(pt2, (*model)[2]);
				const float dist3 = get_distance(pt3, (*model)[3]);
				const float dist_sum = dist0 + dist1 + dist2 + dist3;

				if (dist_sum < dist_min)
				{
					dist_min = dist_sum;
					pt0_dist_min = pt0;
					pt1_dist_min = pt1;
					pt2_dist_min = pt2;
					pt3_dist_min = pt3;
				}
			}
		}

		line(image_thresholded, pt0_dist_min, pt1_dist_min, Scalar(127), 2);
		line(image_thresholded, pt1_dist_min, pt2_dist_min, Scalar(127), 2);
		line(image_thresholded, pt2_dist_min, pt3_dist_min, Scalar(127), 2);
		line(image_thresholded, pt3_dist_min, pt0_dist_min, Scalar(127), 2);*/
	}

	circle(image_thresholded, pt_motion_center, radius, Scalar(127), 2);
	imshow("image_thresholded" + name, image_thresholded);

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