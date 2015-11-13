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
		float theta0 = get_angle(anchor, point0, false);
		float theta1 = get_angle(anchor, point1, false);
		return theta0 < theta1;
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

struct IndexedPoint
{
	int index;
	Point pt;

	IndexedPoint(int _index, Point _pt)
	{
		index = _index;
		pt = _pt;
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

void draw_circle(Mat& image, Point pt, bool is_empty = false)
{
	circle(image, pt, 3, Scalar(127), is_empty ? 1 : -1);
}

BlobNew* find_thumb(vector<BlobNew>& fingertip_blobs, Point& pt_palm, Point& pt_palm_rotated, float palm_radius,
					BlobNew* blob_y_max, float hand_angle, ValueStore* value_store, Mat& image_visualization)
{
	BlobNew* candidate = &fingertip_blobs[0];

	//------------------------------------------------------------------------------------------------------------------------

	bool selected = false;
	if (candidate->skeleton_rotated.size() > 0)
	{
		Point pt_first = candidate->skeleton_rotated[0];
		Point pt_last = candidate->skeleton_rotated[candidate->skeleton_rotated.size() - 1];

		float normal_dist_max = 0;
		float normal_dist_max_abs = -1;
		Point pt_normal_dist_max;
		for (Point& pt : candidate->skeleton_rotated)
		{
			float normal_dist = distance_to_line(pt_first, pt_last, pt);
			float normal_dist_abs = abs(normal_dist);

			if (normal_dist_abs > normal_dist_max_abs)
			{
				normal_dist_max_abs = normal_dist_abs;
				normal_dist_max = normal_dist;
				pt_normal_dist_max = pt;
			}
		}
		if (normal_dist_max_abs > -1 && normal_dist_max <= -3)
			selected = true;
	}

	if (fingertip_blobs.size() >= 5)
		selected = true;

	if (pose_name == "point")
		selected = true;

	//------------------------------------------------------------------------------------------------------------------------

	if (candidate->matching_blob != NULL && candidate->matching_blob->name == "1")
		selected = false;

	Point pt_thumb_reference = value_store->get_point("pt_thumb_reference", Point(-1, -1));
	Point pt_index_reference = value_store->get_point("pt_index_reference", Point(-1, -1));
	if (pt_thumb_reference.x != -1 && pt_index_reference.x != -1)
	{
		float dist_thumb_reference = get_distance(pt_thumb_reference, candidate->pt_tip, false);
		float dist_index_reference = get_distance(pt_index_reference, candidate->pt_tip, false);

		if (dist_index_reference > dist_thumb_reference)
		{
			if (pose_name == "point")
				selected = true;
		}
		else
			selected = false;
	}

	if (candidate == blob_y_max)
		selected = false;

	if (candidate->matching_blob != NULL && candidate->matching_blob->track_index != candidate->track_index)
		selected = false;

	//------------------------------------------------------------------------------------------------------------------------

	if (selected)
	{
		candidate->name = "0";

		if (fingertip_blobs.size() > 1)
		{
			pt_thumb_reference = candidate->pt_tip;
			pt_index_reference = fingertip_blobs[1].pt_tip;
			value_store->set_point("pt_index_reference", pt_index_reference);
			value_store->set_point("pt_thumb_reference", pt_thumb_reference);
		}
	}
	else
	{
		Point pt_thumb_reference_rotated = Point(pt_palm_rotated.x - (palm_radius * 2), pt_palm_rotated.y + (palm_radius * 2));
		pt_thumb_reference = rotate_point_normal(pt_thumb_reference_rotated);
		value_store->set_point("pt_thumb_reference", pt_thumb_reference);

		pt_index_reference = blob_y_max->pt_tip;
		value_store->set_point("pt_index_reference", pt_index_reference);
	}

	//------------------------------------------------------------------------------------------------------------------------

	for (BlobNew& blob : fingertip_blobs)
	{
		fill_mat(image_visualization, blob.skeleton_rotated, 127);
		draw_circle(image_visualization, blob.pt_root_rotated);
		draw_circle(image_visualization, blob.pt_tip_rotated);
	}

	if (selected)
		fill_mat(image_visualization, candidate->data_rotated, 254);

	draw_circle(image_visualization, pt_thumb_reference);
	draw_circle(image_visualization, pt_index_reference);

	if (selected)
		return candidate;

	return NULL;
}

BlobNew* find_index(vector<BlobNew>& fingertip_blobs, Point& pt_palm, Point& pt_palm_rotated, float palm_radius,
					BlobNew* blob_y_max, BlobNew* blob_thumb, float hand_angle, ValueStore* value_store, Mat& image_visualization)
{
	BlobNew* candidate = NULL;

	if (blob_thumb != NULL)
		for (BlobNew& blob : fingertip_blobs)
			if (blob.index == blob_thumb->index + 1)
			{
				candidate = &blob;
				break;
			}

	bool selected = false;
	if (candidate != NULL)
		selected = true;

	if (selected)
		if (candidate->matching_blob != NULL && candidate->matching_blob->track_index != candidate->track_index)
			selected = false;

	if (selected)
		candidate->name = "1";

	if (selected)
		fill_mat(image_visualization, candidate->data_rotated, 127);

	return candidate;
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
	Mat image_find_contours = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_palm_segmented = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	palm_radius = value_store.get_float("palm_radius", 1);
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
		// float multiplier = palm_radius > 10 ? 10 : palm_radius;
		// multiplier = map_val(multiplier, 0, 10, 2, 1);
		// palm_radius *= multiplier * 1.5;

		const int x_offset = hand_angle > 0 ? 0 : (palm_radius / 2);

		Point palm_point_new = max_loc * 4;
		palm_point.y = palm_point_new.y;
		palm_point.x = palm_point_raw.x + x_offset;
	}

	low_pass_filter->compute(palm_radius, 0.1, "palm_radius");
	low_pass_filter->compute(palm_point.y, 0.5, "palm_point");

	static float palm_radius_static = 0;
	if (name == "1")
		palm_radius_static = palm_radius;
	else
		palm_radius = palm_radius_static;

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

	vector<vector<Point>> extension_lines;
	vector<Point> tip_points;
	vector<BlobNew> skeleton_blobs;

	if (true)
	// if (name == "1")
	{
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

		//------------------------------------------------------------------------------------------------------------------------------

		BlobDetectorNew* blob_detector_image_skeleton_segmented = value_store.get_blob_detector("blob_detector_image_skeleton_segmented");
		blob_detector_image_skeleton_segmented->compute(image_skeleton_segmented, 254,
														x_min_hand_right, x_max_hand_right,
											            y_min_hand_right, y_max_hand_right, false, true);

		BlobDetectorNew* blob_detector_image_skeleton_parts = value_store.get_blob_detector("blob_detector_image_skeleton_parts");

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

						tip_points.push_back(pt_end);
						skeleton_blobs.push_back(*blob_detector_image_skeleton_parts->blob_max_size);

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
			int extension_lines_y_max = -1;
			for (vector<Point>& line_vec : extension_lines)
			{
				Point pt = line_vec[0];
				Point pt_rotated = rotate_point_upright(pt);
				Point pt_selected = hand_angle >= -20 ? pt : pt_rotated;

				if (pt_selected.y > extension_lines_y_max)
					extension_lines_y_max = pt_selected.y;
			}
			value_store.set_int("extension_lines_y_max", extension_lines_y_max);

			float angle_mean = 0;
			int angle_count = 0;
			for (vector<Point>& line_vec : extension_lines)
			{
				Point pt = line_vec[0];
				Point pt_rotated = rotate_point_upright(pt);
				Point pt_selected = hand_angle >= -20 ? pt : pt_rotated;

				if (abs(pt_selected.y - extension_lines_y_max) <= 10)
				{
					float angle = get_angle(line_vec[0], line_vec[line_vec.size() - 1], false) - 180;
					angle_mean += angle;
					++angle_count;
				}
			}
			angle_mean /= angle_count;
			hand_angle = angle_mean;
			if (pose_name == "point")
				hand_angle += 30;

			low_pass_filter->compute(hand_angle, 0.5, "hand_angle_first_pass");
		}
	}

	static float hand_angle_static = 0;
	if (name == "1")
		hand_angle_static = hand_angle;
	else
		hand_angle = hand_angle_static;

	value_store.set_float("hand_angle", hand_angle);

	//------------------------------------------------------------------------------------------------------------------------------

	if (tip_points.size() == 0)
		return false;

	sort(tip_points.begin(), tip_points.end(), compare_point_angle(pt_palm));

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

	//------------------------------------------------------------------------------------------------------------------------------

	circle(image_palm, palm_point, palm_radius, Scalar(254), 1);

	//------------------------------------------------------------------------------------------------------------------------------

	if (vertices_0_0_y < vertices_0_3_y)
		floodFill(image_palm, Point(0, HEIGHT_SMALL_MINUS), Scalar(127));
	else
		floodFill(image_palm, Point(WIDTH_SMALL_MINUS, HEIGHT_SMALL_MINUS), Scalar(127));

	//------------------------------------------------------------------------------------------------------------------------------
	//unwrap contour for DTW processes

	vector<Point> contour_processed;
	vector<Point> contour_processed_approximated;

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
		approxPolyDP(Mat(contour_processed), contour_processed_approximated, 1, false);
		if (contour_processed_approximated[0] != contour_processed[0])
			contour_processed_approximated.insert(contour_processed_approximated.begin(), contour_processed[0]);
		if (contour_processed_approximated[contour_processed_approximated.size() - 1] != contour_processed[contour_processed.size() - 1])
			contour_processed_approximated.push_back(contour_processed[contour_processed.size() - 1]);

		//--------------------------------------------------------------------------------------------------------------------------

		int points_x_min;
		int points_x_max;
		int points_y_min;
		int points_y_max;
		get_bounds(contour_processed_approximated, points_x_min, points_x_max, points_y_min, points_y_max);

		vector<Point> contour_processed_approximated_scaled;
		for (Point& pt : contour_processed_approximated)
		{
			int x_normalized = map_val(pt.x, points_x_min, points_x_max, 0, WIDTH_SMALL_MINUS);
			int y_normalized = map_val(pt.y, points_y_min, points_y_max, 0, HEIGHT_SMALL_MINUS);
			contour_processed_approximated_scaled.push_back(Point(x_normalized, y_normalized));
		}

		pose_estimation_points = contour_processed_approximated_scaled;
		break;
	}

	if (contour_processed.size() == 0)
		return false;

	stereo_matching_points = contour_processed;

	//------------------------------------------------------------------------------------------------------------------------------

	int convex_index_min = -1;

	{
		float dist_min = 9999;
		int index_dist_min;

		int index = -1;
		for (Point& pt : contour_processed_approximated)
		{
			++index;

			float dist = get_distance(pt, tip_points[0], false);
			if (dist < dist_min)
			{
				dist_min = dist;
				index_dist_min = index;
			}
		}
		convex_index_min = index_dist_min;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	unordered_map<int, bool> convexity_checker;	//true is convex, false is concave
	for (int skip_count = 1; skip_count <= 2; ++skip_count)
	{
		const int i_max = contour_processed_approximated.size() - skip_count;

		if (i_max <= 0)
			break;

		for (int i = skip_count; i < i_max; ++i)
		{
			Point pt0 = contour_processed_approximated[i];
			Point pt1 = contour_processed_approximated[i + skip_count];
			Point pt2 = contour_processed_approximated[i - skip_count];

			float angle = get_angle(pt0, pt1, pt2);
			if (angle >= 135)
				continue;

			Point pt3 = Point((pt1.x + pt2.x) / 2, (pt1.y + pt2.y) / 2);
			Point pt3_rotated = rotate_point_upright(pt3);
			Point pt0_rotated = rotate_point_upright(pt0);

			float dist0 = get_distance(pt0, pt_palm, false);
			float dist3 = get_distance(pt3, pt_palm, false);

			while (true)
			{
				if (i < convex_index_min)
					break;

				if (pt3_rotated.y < pt0_rotated.y)
					break;

				if (dist3 < dist0)
					break;

				convexity_checker[i] = false;
				break;
			}

			while (true)
			{
				if (pt3_rotated.y > pt0_rotated.y)
					break;

				if (dist3 > dist0)
					break;

				convexity_checker[i] = true;
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------
	//only push concave indexes if they are after the first skeleton-extension tip

	vector<Point> concave_points;

	{
		int type_old = -1;
		for (int i = 0; i < contour_processed_approximated.size(); ++i)
		{
			if (convexity_checker.count(i) == 0)
				continue;
			
			int type = convexity_checker[i] ? 1 : 0;
			if (type_old != type)
				if (type == 0)
					concave_points.push_back(contour_processed_approximated[i]);

			type_old = type;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------
	//draw lines between concave points and palm to split hand into fingertip blobs

	for (Point& pt : concave_points)
		line(image_palm, pt, pt_palm, Scalar(254), 1);

	//------------------------------------------------------------------------------------------------------------------------------

	for (BlobNew& blob : blobs_hand)
		for (Point& pt : blob.data)
			if (image_palm.ptr<uchar>(pt.y, pt.x)[0] == 127)
				image_palm_segmented.ptr<uchar>(pt.y, pt.x)[0] = 127;

	BlobDetectorNew* blob_detector_image_palm_segmented = value_store.get_blob_detector("blob_detector_image_palm_segmented");
	blob_detector_image_palm_segmented->compute(image_palm_segmented, 127,
												x_min_hand_right, x_max_hand_right,
									            y_min_hand_right, y_max_hand_right, true);

	fingertip_blobs.clear();

	for (BlobNew& blob : *blob_detector_image_palm_segmented->blobs)
		if (blob.count > blob_detector_image_palm_segmented->blob_max_size->count / 10)
			fingertip_blobs.push_back(blob);

	if (fingertip_blobs.size() == 0)
		return false;

	sort(fingertip_blobs.begin(), fingertip_blobs.end(), compare_blob_angle(pt_palm));

	{
		int index = -1; 
		for (BlobNew& blob : fingertip_blobs)
		{
			++index;
			blob.index = index;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------
	//complete fingertip blob info, such as pt_tip, skeleton, and rotated data

	for (BlobNew& blob : fingertip_blobs)
	{
		for (Point& pt : blob.data)
		{
			Point pt_rotated = rotate_point_upright(pt);
			blob.data_rotated.push_back(pt_rotated);
		}
		get_bounds(blob.data_rotated, blob.x_min_rotated, blob.x_max_rotated, blob.y_min_rotated, blob.y_max_rotated);

		bool pt_tip_set = false;
		for (Point& pt : tip_points)
			if (blob.image_atlas.ptr<ushort>(pt.y, pt.x)[0] == blob.atlas_id)
			{
				blob.pt_tip = pt;
				pt_tip_set = true;
				break;
			}
		if (!pt_tip_set)
		{
			Point pt_y_max_rotated = get_y_max_point(blob.data_rotated);
			Point pt = rotate_point_normal(pt_y_max_rotated);
			blob.pt_tip = pt;
			pt_tip_set = true;
		}

		blob.pt_tip_rotated = rotate_point_upright(blob.pt_tip);

		for (BlobNew& blob_skeleton : skeleton_blobs)
			if (blob.compute_overlap(blob_skeleton) > 0 && (blob.skeleton.size() == 0 || blob_skeleton.data[0].y > blob.skeleton[0].y))
				blob.skeleton = blob_skeleton.data;

		for (Point& pt : blob.skeleton)
		{
			Point pt_rotated = rotate_point_upright(pt);
			blob.skeleton_rotated.push_back(pt_rotated);
		}

		vector<Point> root_scan_points = blob.data;
		for (Point& pt : blob.skeleton)
			root_scan_points.push_back(pt);

		get_min_dist(root_scan_points, pt_palm, false, &blob.pt_root);
		blob.pt_root_rotated = rotate_point_upright(blob.pt_root);
	}

	//------------------------------------------------------------------------------------------------------------------------------

	while (true)
	{
		BlobNew* blob_x_min_rotated = NULL;
		for (BlobNew& blob : fingertip_blobs)
			if (blob_x_min_rotated == NULL || blob.x_min_rotated < blob_x_min_rotated->x_min_rotated)
				blob_x_min_rotated = &blob;

		if (blob_x_min_rotated->index == 0)
			break;

		vector<BlobNew> fingertip_blobs_filtered;
		for (int i = blob_x_min_rotated->index; i < fingertip_blobs.size(); ++i)
			fingertip_blobs_filtered.push_back(fingertip_blobs[i]);

		fingertip_blobs = fingertip_blobs_filtered;
		sort(fingertip_blobs.begin(), fingertip_blobs.end(), compare_blob_angle(pt_palm));

		int index = -1; 
		for (BlobNew& blob : fingertip_blobs)
		{
			++index;
			blob.index = index;
		}
		break;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	if (fingertip_blobs.size() > 5)
	{
		vector<BlobNew> fingertip_blobs_count_sorted = fingertip_blobs;
		sort(fingertip_blobs_count_sorted.begin(), fingertip_blobs_count_sorted.end(), compare_blob_count());

		vector<BlobNew> fingertip_blobs_filtered;

		const int i_max = fingertip_blobs_count_sorted.size() - 1;
		const int i_min = fingertip_blobs_count_sorted.size() - 5;
		for (int i = i_max; i >= i_min; --i)
			fingertip_blobs_filtered.push_back(fingertip_blobs_count_sorted[i]);

		fingertip_blobs = fingertip_blobs_filtered;
		sort(fingertip_blobs.begin(), fingertip_blobs.end(), compare_blob_angle(pt_palm));

		int index = -1; 
		for (BlobNew& blob : fingertip_blobs)
		{
			++index;
			blob.index = index;
		}
	}
	bool has_all_fingers = fingertip_blobs.size() >= 5;

	//------------------------------------------------------------------------------------------------------------------------------

	int blob_id_max = value_store.get_int("blob_id_max", 0);

	vector<BlobNew>* fingertip_blobs_old_ptr = value_store.get_blob_vec("fingertip_blobs_old_ptr");
	vector<BlobNew> fingertip_blobs_old = *fingertip_blobs_old_ptr;

	match_blobs_by_permutation(fingertip_blobs, fingertip_blobs_old);

	for (BlobNew& blob : fingertip_blobs)
	{
		if (blob.matching_blob == NULL)
		{
			blob.track_index = blob_id_max;
			++blob_id_max;
			continue;
		}

		Point pt0_rotated = blob.pt_tip_rotated;
		Point pt1_rotated = blob.matching_blob->pt_tip_rotated;
		Point pt0 = blob.pt_tip;
		Point pt1 = blob.matching_blob->pt_tip;

		if (!check_bounds_small(pt0_rotated) || !check_bounds_small(pt1_rotated))
			continue;

		float motion_dist = get_distance(pt0_rotated, pt1_rotated, false);
		if (motion_dist > 15)
		{
			blob.matching_blob = NULL;
			blob.track_index = blob_id_max;
			++blob_id_max;
			continue;
		}

		if (blob.matching_blob == NULL)
		{
			blob.track_index = blob_id_max;
			++blob_id_max;
			continue;
		}

		blob.track_index = blob.matching_blob->track_index;
		blob.name = blob.matching_blob->name;

		line(image_visualization, pt0_rotated, pt1_rotated, Scalar(254), 1);
		line(image_visualization, pt0, pt1, Scalar(127), 1);
	}

	value_store.set_int("blob_id_max", blob_id_max);

	//------------------------------------------------------------------------------------------------------------------------------

	BlobNew* blob_y_max = NULL;
	for (BlobNew& blob : fingertip_blobs)
		if (blob_y_max == NULL || blob.pt_tip_rotated.y > blob_y_max->pt_tip_rotated.y)
			blob_y_max = &blob;

	//------------------------------------------------------------------------------------------------------------------------------

	BlobNew* blob_thumb = find_thumb(fingertip_blobs, pt_palm, pt_palm_rotated, palm_radius, blob_y_max,
									 hand_angle, &value_store, image_visualization);

	BlobNew* blob_index = find_index(fingertip_blobs, pt_palm, pt_palm_rotated, palm_radius, blob_y_max, blob_thumb,
									 hand_angle, &value_store, image_visualization);

	//------------------------------------------------------------------------------------------------------------------------------

	//mark

	//------------------------------------------------------------------------------------------------------------------------------
	//set point to false when 2 fingers are too close to each other

	{
		BlobNew* blob_y_max0 = NULL;
		BlobNew* blob_y_max1 = NULL;

		for (BlobNew& blob : fingertip_blobs)
			if (blob_y_max0 == NULL || blob.pt_tip_rotated.y > blob_y_max0->pt_tip_rotated.y)
				blob_y_max0 = &blob;

		for (BlobNew& blob : fingertip_blobs)
			if ((blob_y_max1 == NULL || blob.pt_tip_rotated.y > blob_y_max1->pt_tip_rotated.y) && blob_y_max0 != &blob)
				blob_y_max1 = &blob;

		if (blob_y_max0 != NULL && blob_y_max1 != NULL)
		{
			int y_diff = blob_y_max0->pt_tip_rotated.y - blob_y_max1->pt_tip_rotated.y;

			// if (name == "1")
				// cout << y_diff << endl;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	*fingertip_blobs_old_ptr = fingertip_blobs;

	//------------------------------------------------------------------------------------------------------------------------------
	//draw rotated hand contour

	{
		Point pt_old = Point(-1, -1);
		for (Point& pt : contour_processed_approximated)
		{
			Point pt_rotated = rotate_point_upright(pt);
			if (pt_old.x != -1)
				line(image_visualization, pt_old, pt_rotated, Scalar(254), 1);

			pt_old = pt_rotated;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	for (BlobNew& blob : fingertip_blobs)
		if (blob.name != "")
			put_text(blob.name, image_visualization, get_y_max_point(blob.data_rotated));

	//------------------------------------------------------------------------------------------------------------------------------

	if (visualize)
	{
		circle(image_visualization, pt_palm, palm_radius, Scalar(127), 1);
		circle(image_visualization, pt_palm_rotated, palm_radius, Scalar(127), 1);
		imshow("image_visualizationadlfkjhasdlkf" + name, image_visualization);
		// imshow("image_test" + name, image_test);
	}

	//------------------------------------------------------------------------------------------------------------------------------

	algo_name_vec.push_back(algo_name);
	return true;
}