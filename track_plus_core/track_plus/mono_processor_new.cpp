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

#include "mono_processor_new.h"
#include "mat_functions.h"
#include "contour_functions.h"
#include "dtw.h"
#include "thinning_computer_new.h"
#include "permutation.h"

struct compare_blob_angle
{
	Point pivot;

	compare_blob_angle(Point& pivot_in)
	{
		pivot = pivot_in;
	}

	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		float theta0 = get_angle(pivot, Point(blob0.x, blob0.y), false);
		float theta1 = get_angle(pivot, Point(blob1.x, blob1.y), false);
		return theta0 < theta1;
	}
};

struct compare_blob_count
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.count > blob1.count);
	}
};

struct compare_point_angle
{
	Point anchor;

	compare_point_angle(Point& anchor_in)
	{
		anchor = anchor_in;
	}

	bool operator() (const Point& point0, const Point& point1)
	{
		float theta0 = get_angle(point0.x, point0.y, anchor.x, anchor.y);
		float theta1 = get_angle(point1.x, point1.y, anchor.x, anchor.y);
		return theta0 > theta1;
	}
};

struct compare_point_z
{
	bool operator() (const Point3f& pt0, const Point3f& pt1)
	{
		return (pt0.z < pt1.z);
	}
};

struct compare_point_dist
{
	Point anchor;

	compare_point_dist(Point& anchor_in)
	{
		anchor = anchor_in;
	}

	bool operator() (const Point& point0, const Point& point1)
	{
		float dist0 = get_distance(point0, anchor, false);
		float dist1 = get_distance(point1, anchor, false);
		return dist0 < dist1;
	}
};

BlobNew* find_blob_dist_min(Point pt, vector<BlobNew>* blob_vec)
{
	float dist_min = 9999;
	BlobNew* blob_dist_min = NULL;
	for (BlobNew& blob : *blob_vec)
	{
		float dist = blob.compute_min_dist(pt, NULL, false);
		if (dist < dist_min)
		{
			dist_min = dist;
			blob_dist_min = &blob;
		}
	}
	return blob_dist_min;
}

BlobNew* find_blob_dist_min_rotated(Point pt, vector<BlobNew>* blob_vec)
{
	float dist_min = 9999;
	BlobNew* blob_dist_min = NULL;
	for (BlobNew& blob : *blob_vec)
	{
		float dist = get_distance(blob.pt_tip_rotated, pt, false);
		if (dist < dist_min)
		{
			dist_min = dist;
			blob_dist_min = &blob;
		}
	}
	return blob_dist_min;
}

bool has_blob(vector<BlobNew*>& blob_vec, BlobNew* blob)
{
	bool found = false;
	for (BlobNew* blob_current : blob_vec)
		if (blob_current == blob)
		{
			found = true;
			break;
		}
	return found;
}

void push_blob(vector<BlobNew*>& blob_vec, BlobNew* blob)
{
	if (!has_blob(blob_vec, blob))
		blob_vec.push_back(blob);
}

void match_blobs_by_permutation(vector<BlobNew>& blobs0, vector<BlobNew>& blobs1)
{
	vector<BlobNew>* large_array = &blobs0;
	vector<BlobNew>* small_array = &blobs1;
	bool flipped = false;

	if (blobs0.size() < blobs1.size())
	{
		large_array = &blobs1;
		small_array = &blobs0;
		flipped = true;
	}

	const int large_array_size = large_array->size();
	const int small_array_size = small_array->size();

	compute_permutations(large_array_size, small_array_size);

	float dist_sigma_min = FLT_MAX;
	vector<int> large_array_index_vec_result;
	vector<int> small_array_index_vec_result;

	Mat image_atlas_rotated;
	BlobDetectorNew::reconstruct_atlas_image_rotated(image_atlas_rotated, blobs1);

	for (vector<int>& rows : permutations)
	{
		float dist_sigma = 0;

		vector<int> large_array_index_vec;
		vector<int> small_array_index_vec;

		int small_array_index = -1;
		for (int large_array_index : rows)
		{
			++small_array_index;

			large_array_index_vec.push_back(large_array_index);
			small_array_index_vec.push_back(small_array_index);

			BlobNew blob_small_array = (*small_array)[small_array_index];
			BlobNew blob_large_array = (*large_array)[large_array_index];

			float dist_tip_rotated = get_distance(blob_small_array.pt_tip_rotated, blob_large_array.pt_tip_rotated, false);

			int width_diff = abs(blob_small_array.width - blob_large_array.width);
			int height_diff = abs(blob_small_array.height - blob_large_array.height);

			dist_sigma += (dist_tip_rotated + width_diff + height_diff) * 1000;

			int overlap_count = 0;
			if (!flipped)
			{
				for (Point& pt : blob_large_array.data_rotated)
					if (image_atlas_rotated.ptr<ushort>(pt.y, pt.x)[0] == blob_small_array.atlas_id)
						++overlap_count;
			}
			else
			{
				for (Point& pt : blob_small_array.data_rotated)
					if (image_atlas_rotated.ptr<ushort>(pt.y, pt.x)[0] == blob_large_array.atlas_id)
						++overlap_count;
			}
			dist_sigma -= overlap_count;
		}

		if (dist_sigma < dist_sigma_min)
		{
			dist_sigma_min = dist_sigma;
			large_array_index_vec_result = large_array_index_vec;
			small_array_index_vec_result = small_array_index_vec;
		}
	}

	for (int i = 0; i < large_array_index_vec_result.size(); ++i)
	{
		const int index_large = large_array_index_vec_result[i];
		const int index_small = small_array_index_vec_result[i];

		(*large_array)[index_large].matching_blob = &(*small_array)[index_small];
		(*small_array)[index_small].matching_blob = &(*large_array)[index_large];
	}
}

bool verify_detection(BlobNew* blob, ValueStore* value_store)
{
	int frame_count = value_store->get_int("frame_count", -1);
	int frame_count_old = value_store->get_int("frame_count_old" + blob->name, 0);
	value_store->set_int("frame_count_old" + blob->name, frame_count);

	if (frame_count != frame_count_old + 1)
	{
		value_store->set_point("pt_tip_old", Point(-1, -1));
		value_store->set_point("pt_tip_old_old", Point(-1, -1));
	}

	Point pt_tip = blob->pt_tip;
	Point pt_tip_old = value_store->get_point("pt_tip_old", Point(-1, -1));
	Point pt_tip_old_old = value_store->get_point("pt_tip_old_old", Point(-1, -1));

	if (pt_tip_old.x == -1)
	{
		value_store->set_point("pt_tip_old_old", pt_tip_old);
		value_store->set_point("pt_tip_old", pt_tip);

		return true;
	}

	float dist;
	if (pt_tip_old_old.x != -1 && pt_tip_old_old.y != -1)
	{
		Point pt_tip_prediction = pt_tip_old;
		int x_diff_prediction = pt_tip_old.x - pt_tip_old_old.x;
		int y_diff_prediction = pt_tip_old.y - pt_tip_old_old.y;
		pt_tip_prediction.x += x_diff_prediction;
		pt_tip_prediction.y += y_diff_prediction;

		float dist0 = get_distance(pt_tip_prediction, pt_tip, false);
		float dist1 = get_distance(pt_tip_old, pt_tip, false);
		dist = (dist0 + dist1) / 2;
	}
	else if (pt_tip_old.x != -1)
		dist = get_distance(pt_tip_old, pt_tip, false);

	if (dist > 10)
	{
		value_store->set_int("frame_count_old" + blob->name, 0);
		return false;
	}

	value_store->set_point("pt_tip_old_old", pt_tip_old);
	value_store->set_point("pt_tip_old", pt_tip);

	return true;
}

Point to_pt(Point3f& pt3f)
{
	return Point(pt3f.x, pt3f.y);
}

int x_diff_rotation;
int y_diff_rotation;
float hand_angle;
Point palm_point;
Point palm_point_rotated;

Point rotate_point_upright(Point& pt)
{
	Point pt_rotated = rotate_point(-hand_angle, pt, palm_point);
	pt_rotated.x += x_diff_rotation;
	pt_rotated.y += y_diff_rotation;
	return pt_rotated;
}

Point rotate_point_normal(Point& pt)
{
	Point pt_rotated = rotate_point(hand_angle, pt, palm_point_rotated);
	pt_rotated.x -= x_diff_rotation;
	pt_rotated.y -= y_diff_rotation;
	return pt_rotated;
}

bool MonoProcessorNew::compute(HandSplitterNew& hand_splitter, const string name, bool visualize)
{
	int frame_count = value_store.get_int("frame_count", -1);
	++frame_count;
	value_store.set_int("frame_count", frame_count);

	if (value_store.get_bool("first_pass", false) == false)
		algo_name += name;

	bool algo_name_found = false;
	for (String& algo_name_current : algo_name_vec_old)
		if (algo_name_current == algo_name)
		{
			algo_name_found = true;
			break;
		}

	//------------------------------------------------------------------------------------------------------------------------

	LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");

	if (!algo_name_found && value_store.get_bool("first_pass", false) == true)
	{
		value_store.reset();
		low_pass_filter->reset();
	}

	value_store.set_bool("first_pass", true);

	//------------------------------------------------------------------------------------------------------------------------------

	if (hand_splitter.blobs_right.size() == 0)
		return false;

	vector<BlobNew> blobs_hand = hand_splitter.blobs_right;

	const int x_min_hand_right = hand_splitter.x_min_result_right;
	const int x_max_hand_right = hand_splitter.x_max_result_right;
	const int y_min_hand_right = hand_splitter.y_min_result_right;
	const int y_max_hand_right = hand_splitter.y_max_result_right;

	//------------------------------------------------------------------------------------------------------------------------------

	Mat image_active_hand = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_palm_segmented = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_find_contours = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	float palm_radius = value_store.get_float("palm_radius", 1);
	palm_point = value_store.get_point("palm_point");

	Point2f palm_point_raw = Point2f(0, 0);
	int palm_point_raw_count = 0;
	const int y_threshold = palm_point.y - palm_radius;

	float count_total = 1;
	for (BlobNew& blob : blobs_hand)
	{
		blob.fill(image_find_contours, 254);
		blob.fill(image_active_hand, 254);
		blob.fill(image_palm_segmented, 254);
		blob.fill(image_visualization, 254);

		for (Point& pt : blob.data)
			if (pt.y > y_threshold)
			{
				palm_point_raw.x += pt.x;
				palm_point_raw.y += pt.y;
				++palm_point_raw_count;
			}
		count_total += blob.count;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	float count_current = count_total;
	value_accumulator.compute(count_total, "count_total", 1000, HEIGHT_SMALL_MINUS, 0.9, true);

	if (count_current / count_total < 0.3)
		return false;

	//------------------------------------------------------------------------------------------------------------------------------

	palm_point_raw.x /= palm_point_raw_count;
	palm_point_raw.y /= palm_point_raw_count;

	vector<vector<Point>> contours = legacyFindContours(image_find_contours);
	const int contours_size = contours.size();	

	if (contours_size == 0)
	{
		return false;
	}
	else if (contours_size > 1)
	{
		vector<vector<Point>> contours_reduced(contours_size);

		for (int i = 0; i < contours_size; ++i)
		{
			vector<Point>* contour_reduced = &(contours_reduced[i]);
			vector<Point>* contour = &(contours[i]);

			const int contour_size = contour->size();
			for (int a = 0; a < contour_size; a += 4)
				contour_reduced->push_back((*contour)[a]);
		}

		image_find_contours = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
		for (BlobNew& blob : blobs_hand)
			blob.fill(image_find_contours, 254);

		for (int i = 0; i < contours_reduced.size(); ++i)
		{
			if (contours_reduced.size() == 1)
				break;

			vector<Point>* contour0 = &(contours_reduced[i]);

			float dist_max = 9999;
			int index_contour_dist_max;
			Point pt_dist_max0;
			Point pt_dist_max1;

			for (int a = 0; a < contours_reduced.size(); ++a)
				if (a != i)
				{
					vector<Point>* contour1 = &(contours_reduced[a]);

					for (Point& pt0 : *contour0)
						for (Point& pt1 : *contour1)
						{
							const float dist_current = get_distance(pt0, pt1, false);

							if (dist_current < dist_max)
							{
								dist_max = dist_current;
								index_contour_dist_max = a;
								pt_dist_max0 = pt0;
								pt_dist_max1 = pt1;
							}
						}
				}

			for (Point& pt : contours_reduced[index_contour_dist_max])
				contour0->push_back(pt);

			contours_reduced.erase(contours_reduced.begin() + index_contour_dist_max);
			--i;

			line(image_find_contours, pt_dist_max0, pt_dist_max1, Scalar(254), 2);
		}

		contours = legacyFindContours(image_find_contours);
	}

	//------------------------------------------------------------------------------------------------------------------------------

	hand_angle = value_store.get_float("hand_angle", 0);

	Mat image_very_small;
	resize(image_active_hand, image_very_small, Size(WIDTH_MIN / 2, HEIGHT_MIN / 2), 0, 0, INTER_LINEAR);
	threshold(image_very_small, image_very_small, 250, 254, THRESH_BINARY);

	{
		Mat image_distance_transform;
		distanceTransform(image_very_small, image_distance_transform, CV_DIST_L2, CV_DIST_MASK_PRECISE);

		double min;
		double max;
		Point min_loc;
		Point max_loc;
		minMaxLoc(image_distance_transform, &min, &max, &min_loc, &max_loc);

		palm_point = max_loc * 4;
		palm_radius = max * 4;
	}
	{
		int x_min_ivst;
		int x_max_ivst;
		int y_min_ivst;
		int y_max_ivst;
		threshold_get_bounds(image_very_small, image_very_small, 250, x_min_ivst, x_max_ivst, y_min_ivst, y_max_ivst);

		x_min_ivst += 2;
		x_max_ivst += 2;

		const float mask_ratio = 0.7;

		const int i_max = x_max_ivst;
		const int j_max = y_max_ivst;
		for (int i = (x_max_ivst - x_min_ivst) * mask_ratio + x_min_ivst; i < i_max; ++i)
			for (int j = y_min_ivst; j < j_max; ++j)
				image_very_small.ptr<uchar>(j, i)[0] = 0;

		Mat image_distance_transform;
		distanceTransform(image_very_small, image_distance_transform, CV_DIST_L2, CV_DIST_MASK_PRECISE);

		double min;
		double max;
		Point min_loc;
		Point max_loc;
		minMaxLoc(image_distance_transform, &min, &max, &min_loc, &max_loc);

		palm_radius = max * 4;
		float multiplier = palm_radius > 10 ? 10 : palm_radius;
		multiplier = map_val(multiplier, 0, 10, 2, 1);
		palm_radius *= multiplier * 1.5;

		const int x_offset = hand_angle > 0 ? 0 : (palm_radius / 2);

		Point palm_point_new = max_loc * 4;
		palm_point.y = palm_point_new.y;
		palm_point.x = palm_point_raw.x + x_offset;
	}

	low_pass_filter->compute(palm_radius, 0.1, "palm_radius");
	low_pass_filter->compute(palm_point.y, 0.5, "palm_point");

	pt_palm = palm_point;

	value_store.set_float("palm_radius", palm_radius);
	value_store.set_point("palm_point", palm_point);

	//------------------------------------------------------------------------------------------------------------------------------

	x_diff_rotation = WIDTH_SMALL / 2 - palm_point.x;
	y_diff_rotation = HEIGHT_SMALL / 2 - palm_point.y;

	Point pt_palm_rotated = pt_palm;
	pt_palm_rotated.x += x_diff_rotation;
	pt_palm_rotated.y += y_diff_rotation;
	palm_point_rotated = pt_palm_rotated;

	//------------------------------------------------------------------------------------------------------------------------------

	Mat image_skeleton = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (BlobNew& blob : blobs_hand)
		blob.fill(image_skeleton, 254);

	vector<Point> subject_points;
	for (BlobNew& blob : blobs_hand)
		for (Point& pt : blob.data)
			subject_points.push_back(pt);

	{
		Point attach_pivot = Point(pt_palm.x, 0);

		BlobNew blob0;
		int count_max = 0;

		for (BlobNew& blob : blobs_hand)
			if (blob.count > count_max)
			{
				count_max = blob.count;
				blob0 = blob;
			}

		Point pt_attach0;
		float dist0 = blob0.compute_min_dist(attach_pivot, &pt_attach0, false);
		for (BlobNew& blob1 : blobs_hand)
			if (blob0.atlas_id != blob1.atlas_id)
			{
				Point pt_attach1;
				float dist1 = blob1.compute_min_dist(attach_pivot, &pt_attach1, false);

				Point pt_attach;
				Point pt_base;
				if (dist0 < dist1)
				{
					pt_attach = pt_attach1;
					blob0.compute_min_dist(pt_attach, &pt_base, false);
				}
				else
				{
					pt_attach = pt_attach0;
					blob1.compute_min_dist(pt_attach, &pt_base, false);
				}

				line(image_skeleton, pt_attach, pt_base, Scalar(254), 1);

				vector<Point> line_vec;
				bresenham_line(pt_attach.x, pt_attach.y, pt_base.x, pt_base.y, line_vec, 1000);

				for (Point& pt : line_vec)
				{
					subject_points.push_back(pt);
					image_skeleton.ptr<uchar>(pt.y, pt.x)[0] = 254;
				}
			}
	}

	vector<Point> skeleton_points = thinning_computer.compute_thinning(image_skeleton, subject_points, 10);

	//------------------------------------------------------------------------------------------------------------------------------

	Mat image_skeleton_segmented = Mat::zeros(image_skeleton.size(), CV_8UC1);
	for (Point& pt : skeleton_points)
		image_skeleton_segmented.ptr<uchar>(pt.y, pt.x)[0] = 254;

	for (Point& pt : skeleton_points)
	{
		const bool a = image_skeleton.ptr<uchar>(pt.y - 1, pt.x)[0] > 0;
		const bool b = image_skeleton.ptr<uchar>(pt.y + 1, pt.x)[0] > 0;
		const bool c = image_skeleton.ptr<uchar>(pt.y, pt.x - 1)[0] > 0;
		const bool d = image_skeleton.ptr<uchar>(pt.y, pt.x + 1)[0] > 0;

		const bool e = image_skeleton.ptr<uchar>(pt.y - 1, pt.x - 1)[0] > 0;
		const bool f = image_skeleton.ptr<uchar>(pt.y - 1, pt.x + 1)[0] > 0;
		const bool g = image_skeleton.ptr<uchar>(pt.y + 1, pt.x - 1)[0] > 0;
		const bool h = image_skeleton.ptr<uchar>(pt.y + 1, pt.x + 1)[0] > 0;

		const bool b0 = c && f && h;
		const bool b1 = e && d && g;
		const bool b2 = a && g && h;
		const bool b3 = e && f && b;
		const bool b4 = a && d && g;
		const bool b5 = e && d && b;
		const bool b6 = f && b && c;
		const bool b7 = c && a && h;

		if ((a + b + c + d) >= 3 || (e + f + g + h) >= 3 || b0 || b1 || b2 || b3 || b4 || b5 || b6 || b7)
			circle(image_skeleton_segmented, pt, 3, Scalar(127), -1);
	}
	circle(image_skeleton_segmented, pt_palm, palm_radius, Scalar(127), -1);

	/*if (value_store.get_bool("image_palm_set", false))
	{
		Mat image_palm = value_store.get_mat("image_palm", true);
		for (Point& pt : skeleton_points)
			if (image_palm.ptr<uchar>(pt.y, pt.x)[0] == 0)
				circle(image_skeleton_segmented, pt, 5, Scalar(127), -1);
	}*/

	//------------------------------------------------------------------------------------------------------------------------------

	BlobDetectorNew* blob_detector_image_skeleton_segmented = value_store.get_blob_detector("blob_detector_image_skeleton_segmented");
	blob_detector_image_skeleton_segmented->compute(image_skeleton_segmented, 254,
													x_min_hand_right, x_max_hand_right,
										            y_min_hand_right, y_max_hand_right, false, true);

	BlobDetectorNew* blob_detector_image_skeleton_parts = value_store.get_blob_detector("blob_detector_image_skeleton_parts");
	unordered_map<ushort, vector<Point>> skeleton_extensions_map;

	vector<vector<Point>> extension_lines;

	float dist_to_palm_max = -1;
	for (BlobNew& blob : *blob_detector_image_skeleton_segmented->blobs)
	{
		Point pt_origin = Point(-1, -1);
		for (Point& pt : blob.data)
		{
			bool to_break = false;
			for (int a = -1; a <= 1; ++a)
			{
				int x = pt.x + a;
				if (x < 0)
					x = 0;
				if (x >= WIDTH_SMALL)
					x = WIDTH_SMALL - 1;

				for (int b = -1; b <= 1; ++b)
				{
					int y = pt.y + b;
					if (y < 0)
						y = 0;
					if (y >= HEIGHT_SMALL)
						y = HEIGHT_SMALL - 1;

					if (image_skeleton_segmented.ptr<uchar>(y, x)[0] == 127)
					{
						pt_origin = pt;
						a = 9999;
						to_break = true;
						break;
					}
				}
			}
			if (to_break)
				break;
		}

		if (pt_origin.x != -1)
		{
			blob_detector_image_skeleton_parts->
				compute_location(image_skeleton_segmented, 254, pt_origin.x, pt_origin.y, true, false, true);

			bool caught_between_2_centers = false;

			const int i_max = blob_detector_image_skeleton_parts->blob_max_size->count - 1;
			for (int i = /*blob_detector_image_skeleton_parts->blob_max_size->count / 2*/ i_max; i <= i_max; ++i)
			{
				Point pt = 	blob_detector_image_skeleton_parts->blob_max_size->data[i];
				for (int a = -1; a <= 1; ++a)
				{
					int x = pt.x + a;
					if (x < 0)
						x = 0;
					if (x >= WIDTH_SMALL)
						x = WIDTH_SMALL - 1;

					for (int b = -1; b <= 1; ++b)
					{
						int y = pt.y + b;
						if (y < 0)
							y = 0;
						if (y >= HEIGHT_SMALL)
							y = HEIGHT_SMALL - 1;

						if (image_skeleton_segmented.ptr<uchar>(y, x)[0] == 127)
						{
							caught_between_2_centers = true;
							i = 9999;
							a = 9999;
							break;
						}
					}
				}
			}
			if (!caught_between_2_centers)
			{
				const int index_start = blob_detector_image_skeleton_parts->blob_max_size->count >= 10 ? 
										blob_detector_image_skeleton_parts->blob_max_size->count * 0.7 : 0;

				const int index_end = blob_detector_image_skeleton_parts->blob_max_size->count - 1;

				if (index_start != index_end)
				{
					Point pt_start = blob_detector_image_skeleton_parts->blob_max_size->data[index_start];
					Point pt_end = blob_detector_image_skeleton_parts->blob_max_size->data[index_end];

					vector<Point> extension_line_vec;
					extension_line(pt_end, pt_start, 20, extension_line_vec, false);
					extension_lines.push_back(extension_line_vec);

					//---------------------------------------------------------------------------------------------------

					Point pt_first = blob_detector_image_skeleton_parts->blob_max_size->data[0];
					float dist_to_palm = get_distance(pt_first, pt_palm, true);
					dist_to_palm = dist_to_palm + blob_detector_image_skeleton_parts->blob_max_size->count - palm_radius;

					if (dist_to_palm > dist_to_palm_max)
						dist_to_palm_max = dist_to_palm;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	if (extension_lines.size() > 0)
	{
		int y_max = -1;
		for (vector<Point>& line_vec : extension_lines)
		{
			Point pt = line_vec[0];
			Point pt_rotated = rotate_point_upright(pt);
			Point pt_selected = hand_angle >= -20 ? pt : pt_rotated;

			if (pt_selected.y > y_max)
				y_max = pt_selected.y;
		}

		float angle_mean = 0;
		int angle_count = 0;
		for (vector<Point>& line_vec : extension_lines)
		{
			Point pt = line_vec[0];
			Point pt_rotated = rotate_point_upright(pt);
			Point pt_selected = hand_angle >= -20 ? pt : pt_rotated;

			if (abs(pt_selected.y - y_max) <= 10)
			{
				float angle = get_angle(line_vec[0], line_vec[line_vec.size() - 1], false) - 180;
				angle_mean += angle;
				++angle_count;
			}
		}
		angle_mean /= angle_count;
		hand_angle = angle_mean;
		
		low_pass_filter->compute(hand_angle, 0.5, "hand_angle");
		value_store.set_float("hand_angle", hand_angle);
	}

	//------------------------------------------------------------------------------------------------------------------------------

	/*vector<Point> contour_rotated;
	for (Point& pt : contours[0])
	{
		Point pt_rotated = rotate_point_upright(pt);
		contour_rotated.push_back(pt_rotated);
	}

	Point pt_rotated_y_min = get_y_min_point(contour_rotated);
	Point pivot = rotate_point_normal(pt_rotated_y_min);

	vector<Point> contour_sorted;
	sort_contour(contours[0], contour_sorted, pivot);*/

	//------------------------------------------------------------------------------------------------------------------------------
	//mask arm part of image so that only palm and fingers are left

	Mat image_palm = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	RotatedRect r_rect0 = RotatedRect(palm_point, Size2f(500, 500), -hand_angle);
	Point2f vertices0[4];
	r_rect0.points(vertices0);

	int vertices_0_3_x = (vertices0[3].x - vertices0[2].x) / 2 + vertices0[2].x;
	int vertices_0_3_y = (vertices0[3].y - vertices0[2].y) / 2 + vertices0[2].y - palm_radius;
	int vertices_0_0_x = (vertices0[0].x - vertices0[1].x) / 2 + vertices0[1].x;
	int vertices_0_0_y = (vertices0[0].y - vertices0[1].y) / 2 + vertices0[1].y - palm_radius;

	line(image_palm, Point(vertices_0_3_x, vertices_0_3_y), Point(vertices_0_0_x, vertices_0_0_y), Scalar(254), 1);
	line(image_palm_segmented, Point(vertices_0_3_x, vertices_0_3_y), Point(vertices_0_0_x, vertices_0_0_y), Scalar(0), 1);

	RotatedRect r_rect1 = RotatedRect(palm_point, Size2f(palm_radius * 2, 500), -hand_angle);
	Point2f vertices1[4];
	r_rect1.points(vertices1);

	int vertices_1_3_x = (vertices1[3].x - vertices1[2].x) / 2 + vertices1[2].x;
	int vertices_1_3_y = (vertices1[3].y - vertices1[2].y) / 2 + vertices1[2].y;
	int vertices_1_0_x = (vertices1[0].x - vertices1[1].x) / 2 + vertices1[1].x;
	int vertices_1_0_y = (vertices1[0].y - vertices1[1].y) / 2 + vertices1[1].y;

	line(image_palm, Point(vertices_1_0_x, vertices_1_0_y), vertices1[1], Scalar(254), 1);
	line(image_palm, vertices1[1], vertices1[2], Scalar(254), 1);
	line(image_palm, vertices1[2], Point(vertices_1_3_x, vertices_1_3_y), Scalar(254), 1);
	line(image_palm, Point(vertices_1_3_x, vertices_1_3_y), Point(vertices_1_0_x, vertices_1_0_y), Scalar(254), 1);

	line(image_palm_segmented, Point(vertices_1_0_x, vertices_1_0_y), vertices1[1], Scalar(0), 1);
	line(image_palm_segmented, vertices1[1], vertices1[2], Scalar(0), 1);
	line(image_palm_segmented, vertices1[2], Point(vertices_1_3_x, vertices_1_3_y), Scalar(0), 1);
	line(image_palm_segmented, Point(vertices_1_3_x, vertices_1_3_y), Point(vertices_1_0_x, vertices_1_0_y), Scalar(0), 1);

	//------------------------------------------------------------------------------------------------------------------------------

	circle(image_palm, palm_point, palm_radius, Scalar(254), 1);
	circle(image_palm_segmented, palm_point, palm_radius, Scalar(0), 1);

	//------------------------------------------------------------------------------------------------------------------------------

	if (vertices_0_0_y < vertices_0_3_y)
		floodFill(image_palm, Point(0, HEIGHT_SMALL_MINUS), Scalar(127));
	else
		floodFill(image_palm, Point(WIDTH_SMALL_MINUS, HEIGHT_SMALL_MINUS), Scalar(127));

	value_store.set_mat("image_palm", image_palm);
	value_store.set_bool("image_palm_set", true);

	//------------------------------------------------------------------------------------------------------------------------------
	//unwrap contour for DTW processes

	vector<Point> contour_processed;

	while (true)
	{
		Mat image_contour_processed = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

		{
			Point pt_old = Point(-1, -1);
			for (Point& pt : contours[0])
			{
				if (pt_old.x != -1 && image_palm.ptr<uchar>(pt.y, pt.x)[0] == 127 && image_palm.ptr<uchar>(pt_old.y, pt_old.x)[0] == 127)
					line(image_contour_processed, pt_old, pt, Scalar(254), 1);
				
				pt_old = pt;
			}
		}

		BlobDetectorNew* blob_detector_image_contour_processed = value_store.get_blob_detector("blob_detector_image_contour_processed");
		blob_detector_image_contour_processed->compute_region(image_contour_processed, 254, contours[0], false, true);

		if (blob_detector_image_contour_processed->blobs->size() == 0)
			break;

		blob_detector_image_contour_processed->sort_blobs_by_angle(pt_palm);

		BlobNew* blob_first = &(*blob_detector_image_contour_processed->blobs)[0];
		BlobNew* blob_last = &(*blob_detector_image_contour_processed->blobs)[blob_detector_image_contour_processed->blobs->size() - 1];

		Point pt_first = blob_first->data[0];
		Point pt_last = blob_last->data[blob_last->count - 1];

		{
			bool first_hit = false;
			bool last_hit = false;

			int index = -1;
			while (true)
			{
				++index;

				if ((float)index / contours[0].size() >= 2)
					break;

				int contour_index = index;
				if (contour_index >= contours[0].size())
					contour_index -= contours[0].size();

				Point pt = contours[0][contour_index];
				if (!first_hit && pt == pt_first)
					first_hit = true;
				else if (first_hit && !last_hit && pt == pt_last)
					last_hit = true;

				if (first_hit)
					contour_processed.push_back(pt);

				if (last_hit)
					break;
			}
			if (!first_hit || !last_hit)
				break;
		}

		//--------------------------------------------------------------------------------------------------------------------------

		vector<Point> pose_estimation_points_raw;

		for (Point& pt : contour_processed)
			pose_estimation_points_raw.push_back(pt);

		int points_x_min;
		int points_x_max;
		int points_y_min;
		int points_y_max;
		get_bounds(pose_estimation_points_raw, points_x_min, points_x_max, points_y_min, points_y_max);

		vector<Point> pose_estimation_points_normalized;
		for (Point& pt : pose_estimation_points_raw)
		{
			int x_normalized = map_val(pt.x, points_x_min, points_x_max, 0, WIDTH_SMALL_MINUS);
			int y_normalized = map_val(pt.y, points_y_min, points_y_max, 0, HEIGHT_SMALL_MINUS);
			pose_estimation_points_normalized.push_back(Point(x_normalized, y_normalized));
		}

		vector<Point> pose_estimation_points_approximated;
		approxPolyDP(Mat(pose_estimation_points_normalized), pose_estimation_points_approximated, 2, false);

		pose_estimation_points = pose_estimation_points_approximated;
		stereo_matching_points = pose_estimation_points_normalized;

		break;
	}

	if (contour_processed.size() == 0)
		return false;

	//------------------------------------------------------------------------------------------------------------------------------

	{
		vector<Point> histogram_vec;

		int dist_max = 0;
		int index = -1;
		for (Point& pt : contour_processed)
		{
			++index;

			int dist = get_distance(pt, pt_palm, false);
			if (dist > dist_max)
				dist_max = dist;

			histogram_vec.push_back(Point(index, dist));
		}
		const int hist_width = index + 1;
		const int hist_height = dist_max + 1;

		Mat image_histogram = Mat::zeros(hist_height, hist_width, CV_8UC1);
		draw_contour(histogram_vec, image_histogram, 254, 1);

		for (int i = 0; i < hist_width; ++i)
			for (int j = hist_height - 1; j >= 0; --j)
				if (image_histogram.ptr<uchar>(j, i)[0] > 0)
				{
					line(image_histogram, Point(i, j), Point(i, 0), Scalar(254), 1);
					break;
				}

		/*vector<int> convex_indexes;
		for (int j = 0; j < hist_height; j += 2)
		{
			int entry_x = -1;
			for (int i = 0; i < hist_width; ++i)
				if (image_histogram.ptr<uchar>(j, i)[0] == 254 && entry_x == -1)
					entry_x = i;
				else if (image_histogram.ptr<uchar>(j, i)[0] == 0 && entry_x != -1)
				{
					const int exit_x = i - 1;

					int y_max = -1;
					int x_y_max = -1;
					for (int x = entry_x; x <= exit_x; ++x)
					{
						int y = histogram_vec[x].y;
						if (y > y_max)
						{
							y_max = y;
							x_y_max = x;
						}
					}
					if (find(convex_indexes.begin(), convex_indexes.end(), x_y_max) == convex_indexes.end())
						convex_indexes.push_back(x_y_max);

					entry_x = -1;
				}
		}*/

		vector<int> concave_indexes;
		for (int j = 0; j < hist_height; j += 2)
		{
			int entry_x = -1;
			for (int i = 0; i < hist_width; ++i)
				if (image_histogram.ptr<uchar>(j, i)[0] == 0 && entry_x == -1)
					entry_x = i;
				else if (image_histogram.ptr<uchar>(j, i)[0] == 254 && entry_x != -1)
				{
					const int exit_x = i - 1;

					int y_min = 9999;
					int x_y_min = 9999;
					for (int x = entry_x; x <= exit_x; ++x)
					{
						int y = histogram_vec[x].y;
						if (y < y_min)
						{
							y_min = y;
							x_y_min = x;
						}
					}
					if (find(concave_indexes.begin(), concave_indexes.end(), x_y_min) == concave_indexes.end())
						concave_indexes.push_back(x_y_min);

					entry_x = -1;
				}
		}

		vector<Point> contour_processed_rotated;
		for (Point& pt : contour_processed)
		{
			Point pt_rotated = rotate_point_upright(pt);
			contour_processed_rotated.push_back(pt_rotated);
		}

		Mat image_contour_processed_rotated = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
		draw_contour(contour_processed_rotated, image_contour_processed_rotated, 254, 1);

		/*for (int i : convex_indexes)
			circle(image_visualization, contour_processed[i], 3, Scalar(127), -1);*/

		for (int i : concave_indexes)
		{
			Point pt = contour_processed[i];
			Point pt_rotated = rotate_point_upright(pt);

			if (pt_rotated.x < 1)
				continue;

			bool is_white_old = false;
			int contour_hit_count = 0;
			for (int i = pt_rotated.x; i >= 0; --i)
			{
				bool is_white = image_contour_processed_rotated.ptr<uchar>(pt_rotated.y, i)[0] > 0;
				if (!is_white && is_white_old)
					++contour_hit_count;

				is_white_old = is_white;
			}

			if (contour_hit_count <= 1)
				continue;

			line(image_visualization, contour_processed[i], pt_palm, Scalar(127), 1);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	if (name == "0")
	{
		circle(image_visualization, pt_palm, palm_radius, Scalar(127), 1);
		imshow("image_visualizationadlfkjhasdlkf" + name, image_visualization);
		imshow("image_skeleton" + name, image_skeleton);
		imshow("image_skeleton_segmented" + name, image_skeleton_segmented);
		imshow("image_palm_segmented" + name, image_palm_segmented);
	}

	//------------------------------------------------------------------------------------------------------------------------------

	algo_name_vec.push_back(algo_name);
	return true;
}