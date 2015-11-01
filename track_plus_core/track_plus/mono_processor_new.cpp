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

void sort_contour(vector<Point>& points, vector<Point>& points_sorted, Point& pivot)
{
	float dist_min = 9999;
	int index_dist_min;

	int index = 0;
	for (Point& pt : points)
	{
		float dist = get_distance(pt, pivot, false);
		if (dist < dist_min /*&& pt.x < pivot.x*/)
		{
			dist_min = dist;
			index_dist_min = index;
		}
		++index;
	}

	if (dist_min == 9999)
		return;

	const int points_size = points.size();
	for (int i = index_dist_min; i < points_size + index_dist_min; ++i)
	{
		int index_current = i;
		if (index_current >= points_size)
			index_current -= points_size;

		Point pt_current = points[index_current];
		points_sorted.push_back(pt_current);
	}
}

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

bool MonoProcessorNew::compute(HandSplitterNew& hand_splitter, const string name, bool visualize)
{
	int frame_count = value_store->get_int("frame_count", -1);
	++frame_count;
	value_store->set_int("frame_count", frame_count);

	if (value_store->get_bool("first_pass", false) == false)
		algo_name += name;

	bool algo_name_found = false;
	for (String& algo_name_current : algo_name_vec_old)
		if (algo_name_current == algo_name)
		{
			algo_name_found = true;
			break;
		}

	//------------------------------------------------------------------------------------------------------------------------

	LowPassFilter* low_pass_filter = value_store->get_low_pass_filter("low_pass_filter");

	if (!algo_name_found && value_store->get_bool("first_pass", false) == true)
	{
		delete value_store;
		value_store = new ValueStore();
		low_pass_filter->reset();
	}

	value_store->set_bool("first_pass", true);

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

	float palm_radius = value_store->get_float("palm_radius", 1);
	Point palm_point = value_store->get_point("palm_point");

	Point2f palm_point_raw = Point2f(0, 0);
	int palm_point_raw_count = 0;
	const int y_threshold = palm_point.y - palm_radius;

	float count_total = 1;
	for (BlobNew& blob : blobs_hand)
	{
		blob.fill(image_find_contours, 254);
		blob.fill(image_active_hand, 254);
		blob.fill(image_palm_segmented, 254);

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

		const int i_max = x_max_ivst;
		const int j_max = y_max_ivst;
		for (int i = (x_max_ivst - x_min_ivst) * 0.7 + x_min_ivst; i < i_max; ++i)
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

		Point palm_point_new = max_loc * 4;
		palm_point.y = palm_point_new.y;
		palm_point.x = palm_point_raw.x + (palm_radius / 2);
	}

	low_pass_filter->compute(palm_radius, 0.1, "palm_radius");
	low_pass_filter->compute(palm_point, 0.5, "palm_point");

	pt_palm = palm_point;

	value_store->set_float("palm_radius", palm_radius);
	value_store->set_point("palm_point", palm_point);

	//------------------------------------------------------------------------------------------------------------------------------

	Point pivot = value_store->get_point("pivot");

	float hand_angle = value_store->get_float("hand_angle", 0);
	float hand_angle_old = hand_angle;

	if (!value_store->get_bool("hand_angle_set", false))
	{
		static Point pt_intersection_hand_direction_stereo = Point(0, 0);

		vector<Point> contour_approximated;
		approxPolyDP(Mat(contours[0]), contour_approximated, 1, false);
		contour_approximated.insert(contour_approximated.begin(), contours[0][0]);
		contour_approximated.push_back(contours[0][contours[0].size() - 1]);

		while (true)
		{
			vector<Point3f> concave_points_indexed_raw;
			vector<Point3f> convex_points_indexed_raw;
			for (int skip_count = 1; skip_count < contour_approximated.size() / 2; ++skip_count)
			{
				const int i_max = contour_approximated.size() - skip_count;

				for (int i = skip_count; i < i_max; ++i)
				{
					Point pt0 = contour_approximated[i];
					Point pt1 = contour_approximated[i + skip_count];
					Point pt2 = contour_approximated[i - skip_count];

					bool found = false;
					for (Point3f& pt_indexed : concave_points_indexed_raw)
						if (pt_indexed.z == i || (pt_indexed.x == pt0.x && pt_indexed.y == pt0.y))
						{
							found = true;
							break;
						}
					for (Point3f& pt_indexed : convex_points_indexed_raw)
						if (pt_indexed.z == i || (pt_indexed.x == pt0.x && pt_indexed.y == pt0.y))
						{
							found = true;
							break;
						}

					if (!found)
					{
						float dist0 = get_distance(pt0, palm_point, false);
						float dist1 = get_distance(pt1, palm_point, false);
						float dist2 = get_distance(pt2, palm_point, false);

						if (dist0 <= dist1 && dist0 <= dist2)
							concave_points_indexed_raw.push_back(Point3f(pt0.x, pt0.y, i));
						else if (dist0 >= dist1 && dist0 >= dist2)
							convex_points_indexed_raw.push_back(Point3f(pt0.x, pt0.y, i));
					}
				}
			}
			int ind = 0;
			concave_points_indexed_raw.push_back(Point3f(contour_approximated[ind].x, contour_approximated[ind].y, ind));
			ind = contour_approximated.size() - 1;
			concave_points_indexed_raw.push_back(Point3f(contour_approximated[ind].x, contour_approximated[ind].y, ind));
			ind = 0;
			convex_points_indexed_raw.push_back(Point3f(contour_approximated[ind].x, contour_approximated[ind].y, ind));
			ind = contour_approximated.size() - 1;
			convex_points_indexed_raw.push_back(Point3f(contour_approximated[ind].x, contour_approximated[ind].y, ind));

			sort(concave_points_indexed_raw.begin(), concave_points_indexed_raw.end(), compare_point_z());
			sort(convex_points_indexed_raw.begin(), convex_points_indexed_raw.end(), compare_point_z());

			vector<Point3f> convex_points_indexed;
			vector<Point3f> anchor_points0;
			vector<Point3f> anchor_points1;

			const int concave_points_indexed_raw_size = concave_points_indexed_raw.size();
			for (int i = 1; i < concave_points_indexed_raw_size; ++i)
			{
				Point3f pt_concave_indexed0 = concave_points_indexed_raw[i - 1];
				Point3f pt_concave_indexed1 = concave_points_indexed_raw[i];

				const int index_begin = pt_concave_indexed0.z;
				const int index_end = pt_concave_indexed1.z;

				if (index_end - index_begin <= 1)
					continue;

				Point3f pt_dist_min;
				float dist_min = -1;
				for (Point3f& pt_convex_indexed : convex_points_indexed_raw)
					if ((pt_convex_indexed.z >= index_begin && pt_convex_indexed.z <= index_end) ||
						(pt_convex_indexed.z <= index_begin && pt_convex_indexed.z >= index_end))
					{
						float dist = get_distance(Point(pt_convex_indexed.x, pt_convex_indexed.y), palm_point, false);
						if (dist > dist_min)
						{
							dist_min = dist;
							pt_dist_min = pt_convex_indexed;
						}
					}

				if (dist_min > -1)
				{
					convex_points_indexed.push_back(pt_dist_min);
					anchor_points0.push_back(pt_concave_indexed0);
					anchor_points1.push_back(pt_concave_indexed1);
				}
			}

			if (convex_points_indexed.size() == 0)
				break;

			Point tip_min_dist;
			Point anchor_min_dist;
			float min_dist = 9999;

			Point seek_anchor = Point(x_min_hand_right, y_max_hand_right * 3);

			const int i_max = convex_points_indexed.size();
			for (int i = 0; i < i_max; ++i)
			{
				Point anchor0 = Point(anchor_points0[i].x, anchor_points0[i].y);
				Point anchor1 = Point(anchor_points1[i].x, anchor_points1[i].y);
				float dist0 = get_distance(anchor0, palm_point, false);
				float dist1 = get_distance(anchor1, palm_point, false);

				Point anchor;
				if (dist0 < dist1)
					anchor = anchor0;
				else
					anchor = anchor1;

				Point tip = Point(convex_points_indexed[i].x, convex_points_indexed[i].y);

				float current_dist = get_distance(tip, seek_anchor, false);
				if (current_dist < min_dist)
				{
					min_dist = current_dist;
					tip_min_dist = tip;
					anchor_min_dist = anchor;
				}
			}

			Point pt_intersection_hand_direction;
			if (get_intersection_at_y(tip_min_dist, anchor_min_dist, HEIGHT_SMALL, pt_intersection_hand_direction))
			{
				// pt_intersection_hand_direction.x += pt_intersection_hand_direction.x - palm_point.x;
				pt_intersection_hand_direction_stereo.x +=
					(pt_intersection_hand_direction.x - pt_intersection_hand_direction_stereo.x) * 0.5;
				pt_intersection_hand_direction_stereo.y = HEIGHT_SMALL;
				get_intersection_at_y(pt_intersection_hand_direction_stereo, palm_point, palm_point.y - (palm_radius * 2), pivot);
			}

			break;
		}
	}
	else
	{
		Point pt_palm_offset = rotate_point(hand_angle - 180, Point(pt_palm.x, pt_palm.y - 10), pt_palm);
		get_intersection_at_y(pt_palm, pt_palm_offset, palm_point.y - (palm_radius * 2), pivot);
	}

	value_store->set_point("pivot", pivot);

	//------------------------------------------------------------------------------------------------------------------------------

	vector<Point> contour_sorted;
	sort_contour(contours[0], contour_sorted, pivot);

	if (contour_sorted.size() == 0)
		return false;

	//------------------------------------------------------------------------------------------------------------------------------

	vector<Point> contour_approximated_unsorted;
	approxPolyDP(Mat(contours[0]), contour_approximated_unsorted, 1, false);

	vector<Point> contour_approximated;
	sort_contour(contour_approximated_unsorted, contour_approximated, pivot);

	contour_approximated.insert(contour_approximated.begin(), contour_sorted[0]);
	contour_approximated.push_back(contour_sorted[contour_sorted.size() - 1]);

	//------------------------------------------------------------------------------------------------------------------------------

	Mat image_palm = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	vector<Point3f> concave_points_indexed;
	vector<Point3f> convex_points_indexed;

	vector<Point> circle_vec;
	midpoint_circle(palm_point.x, palm_point.y, palm_radius, circle_vec);

	while (true)
	{
		vector<Point3f> concave_points_indexed_raw;
		vector<Point3f> convex_points_indexed_raw;
		for (int skip_count = 1; skip_count < contour_approximated.size() / 2; ++skip_count)
		{
			const int i_max = contour_approximated.size() - skip_count;

			for (int i = skip_count; i < i_max; ++i)
			{
				Point pt0 = contour_approximated[i];
				Point pt1 = contour_approximated[i + skip_count];
				Point pt2 = contour_approximated[i - skip_count];

				bool found = false;
				for (Point3f& pt_indexed : concave_points_indexed_raw)
					if (pt_indexed.z == i || (pt_indexed.x == pt0.x && pt_indexed.y == pt0.y))
					{
						found = true;
						break;
					}
				for (Point3f& pt_indexed : convex_points_indexed_raw)
					if (pt_indexed.z == i || (pt_indexed.x == pt0.x && pt_indexed.y == pt0.y))
					{
						found = true;
						break;
					}

				if (!found)
				{
					float dist0 = get_distance(pt0, palm_point, false);
					float dist1 = get_distance(pt1, palm_point, false);
					float dist2 = get_distance(pt2, palm_point, false);

					if (dist0 <= dist1 && dist0 <= dist2)
						concave_points_indexed_raw.push_back(Point3f(pt0.x, pt0.y, i));
					else if (dist0 >= dist1 && dist0 >= dist2)
						convex_points_indexed_raw.push_back(Point3f(pt0.x, pt0.y, i));
				}
			}
		}
		int ind = 0;
		concave_points_indexed_raw.push_back(Point3f(contour_approximated[ind].x, contour_approximated[ind].y, ind));
		ind = contour_approximated.size() - 1;
		concave_points_indexed_raw.push_back(Point3f(contour_approximated[ind].x, contour_approximated[ind].y, ind));
		ind = 0;
		convex_points_indexed_raw.push_back(Point3f(contour_approximated[ind].x, contour_approximated[ind].y, ind));
		ind = contour_approximated.size() - 1;
		convex_points_indexed_raw.push_back(Point3f(contour_approximated[ind].x, contour_approximated[ind].y, ind));

		sort(concave_points_indexed_raw.begin(), concave_points_indexed_raw.end(), compare_point_z());
		sort(convex_points_indexed_raw.begin(), convex_points_indexed_raw.end(), compare_point_z());

		//------------------------------------------------------------------------------------------------------------------------------

		{
			const int convex_points_indexed_raw_size = convex_points_indexed_raw.size();
			for (int i = 1; i < convex_points_indexed_raw_size; ++i)
			{
				Point3f pt_convex_indexed0 = convex_points_indexed_raw[i - 1];
				Point3f pt_convex_indexed1 = convex_points_indexed_raw[i];

				const int index_begin = pt_convex_indexed0.z;
				const int index_end = pt_convex_indexed1.z;

				if (index_end - index_begin <= 1)
					continue;

				Point3f pt_dist_min;
				float dist_min = 9999;
				for (Point3f& pt_concave_indexed : concave_points_indexed_raw)
					if ((pt_concave_indexed.z > index_begin && pt_concave_indexed.z < index_end) ||
						(pt_concave_indexed.z < index_begin && pt_concave_indexed.z > index_end))
					{
						float dist_to_convex0 = get_distance(Point(pt_concave_indexed.x, pt_concave_indexed.y),
															 Point(pt_convex_indexed0.x, pt_convex_indexed0.y), true);

						float dist_to_convex1 = get_distance(Point(pt_concave_indexed.x, pt_concave_indexed.y),
															 Point(pt_convex_indexed1.x, pt_convex_indexed1.y), true);

						float dist_to_convex_max = max(dist_to_convex0, dist_to_convex1);
						if (dist_to_convex_max <= 5)
							continue;

						float dist = get_distance(Point(pt_concave_indexed.x, pt_concave_indexed.y), palm_point, false);
						if (dist < dist_min)
						{
							dist_min = dist;
							pt_dist_min = pt_concave_indexed;
						}
					}

				if (dist_min < 9999)
					concave_points_indexed.push_back(pt_dist_min);
			}

			if (concave_points_indexed.size() == 0)
				break;

			sort(concave_points_indexed.begin(), concave_points_indexed.end(), compare_point_z());
		}

		//------------------------------------------------------------------------------------------------------------------------------

		{
			const int concave_points_indexed_raw_size = concave_points_indexed_raw.size();

			for (int i = 1; i < concave_points_indexed_raw_size; ++i)
			{
				Point3f pt_concave_indexed0 = concave_points_indexed_raw[i - 1];
				Point3f pt_concave_indexed1 = concave_points_indexed_raw[i];

				const int index_begin = pt_concave_indexed0.z;
				const int index_end = pt_concave_indexed1.z;

				if (index_end - index_begin <= 1)
					continue;

				Point3f pt_dist_max;
				float dist_max = -1;
				for (Point3f& pt_convex_indexed : convex_points_indexed_raw)
					if ((pt_convex_indexed.z > index_begin && pt_convex_indexed.z < index_end) ||
						(pt_convex_indexed.z < index_begin && pt_convex_indexed.z > index_end))
					{
						float dist = get_distance(Point(pt_convex_indexed.x, pt_convex_indexed.y), palm_point, false);
						if (dist > dist_max)
						{
							dist_max = dist;
							pt_dist_max = pt_convex_indexed;
						}
					}

				if (dist_max > -1)
					convex_points_indexed.push_back(pt_dist_max);
			}

			if (convex_points_indexed.size() == 0)
				break;

			sort(convex_points_indexed.begin(), convex_points_indexed.end(), compare_point_z());
		}

		//------------------------------------------------------------------------------------------------------------------------------

		/*int x_min_palm_lines = 9999;
		int x_max_palm_lines = 0;
		int x_min_palm_lines_index = 0;
		int x_max_palm_lines_index = 0;
		for (Point3f& pt : concave_points_indexed)
		{
			if (pt.x > x_max_palm_lines)
			{
				x_max_palm_lines = pt.x;
				x_max_palm_lines_index = pt.z;
			}
			if (pt.x < x_min_palm_lines)
			{
				x_min_palm_lines = pt.x;
				x_min_palm_lines_index = pt.z;
			}
		}

		Point pt_begin;
		Point pt_end;
		Point pt_old = Point(-1, -1);
		for (Point3f& pt : concave_points_indexed)
			if ((pt.z >= x_min_palm_lines_index && pt.z <= x_max_palm_lines_index) ||
				(pt.z <= x_min_palm_lines_index && pt.z >= x_max_palm_lines_index))
			{
				Point pt_new = Point(pt.x, pt.y);
				if (pt_old.x != -1)
				{
					line(image_palm, pt_old, pt_new, Scalar(254), 1);
					line(image_palm_segmented, pt_old, pt_new, Scalar(0), 1);
				}
				else
					pt_begin = Point(pt.x, pt.y);

				pt_end = pt_new;
				pt_old = pt_new;
			}

		if (pt_old.x != -1 && pt_old.y != -1)
		{
			line(image_palm, pt_begin, Point(pivot.x - (palm_radius * 2), pivot.y > 0 ? 0 : pivot.y), Scalar(254), 1);
			line(image_palm, pt_end, Point(pivot.x + (palm_radius * 2), pivot.y > 0 ? 0 : pivot.y), Scalar(254), 1);

			line(image_palm_segmented, pt_begin, Point(pivot.x - (palm_radius * 2), pivot.y > 0 ? 0 : pivot.y), Scalar(0), 1);
			line(image_palm_segmented, pt_end, Point(pivot.x + (palm_radius * 2), pivot.y > 0 ? 0 : pivot.y), Scalar(0), 1);
		}*/

		//------------------------------------------------------------------------------------------------------------------------------

		for (Point3f& pt : concave_points_indexed)
		{
			Point pt_new = Point(pt.x, pt.y);
			line(image_palm_segmented, pt_new, pt_palm, Scalar(0), 1);
		}

		break;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	RotatedRect r_rect0 = RotatedRect(palm_point, Size2f(500, 500), -hand_angle);
	Point2f vertices0[4];
	r_rect0.points(vertices0);

	int vertices_0_3_x = (vertices0[3].x - vertices0[2].x) / 2 + vertices0[2].x;
	int vertices_0_3_y = (vertices0[3].y - vertices0[2].y) / 2 + vertices0[2].y - 10;
	int vertices_0_0_x = (vertices0[0].x - vertices0[1].x) / 2 + vertices0[1].x;
	int vertices_0_0_y = (vertices0[0].y - vertices0[1].y) / 2 + vertices0[1].y - 10;

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

	// circle(image_palm, palm_point, palm_radius, Scalar(254), 1);
	// circle(image_palm_segmented, palm_point, palm_radius, Scalar(0), 1);

	/*if (circle_vec.size() > 0)
	{
		sort(circle_vec.begin(), circle_vec.end(), compare_point_angle(palm_point));

		Point pt_dist_contour_circle_min_old = Point(9999, 0);
		Point pt_dist_contour_circle_min_first;

		const int circle_vec_size = circle_vec.size();
		for (int i = 0; i < circle_vec_size; i += 4)
		{
			Point pt = circle_vec[i];

			float dist_contour_circle_min = 9999;
			Point pt_dist_contour_circle_min;

			const int contour_sorted_size = contour_sorted.size();
			for (int j = 0; j < contour_sorted_size; j += 4)
			{
				Point pt_contour = contour_sorted[j];

				const float dist_contour_circle = get_distance(pt, pt_contour, false);
				if (dist_contour_circle < dist_contour_circle_min)
				{
					dist_contour_circle_min = dist_contour_circle;
					pt_dist_contour_circle_min = pt_contour;
				}
			}s

			if (pt.x > 0 && pt.x < WIDTH_SMALL && pt.y > 0 && pt.y < HEIGHT_SMALL)
			{
				image_palm.ptr<uchar>(pt.y, pt.x)[0] = 254;
				image_palm_segmented.ptr<uchar>(pt.y, pt.x)[0] = 0;

				if (pt_dist_contour_circle_min_old.x != 9999)
				{
					line(image_palm, pt_dist_contour_circle_min_old, pt_dist_contour_circle_min, Scalar(254), 1);
					line(image_palm_segmented, pt_dist_contour_circle_min_old, pt_dist_contour_circle_min, Scalar(0), 1);
				}
				else
					pt_dist_contour_circle_min_first = pt_dist_contour_circle_min;

				pt_dist_contour_circle_min_old = pt_dist_contour_circle_min;
			}
		}

		line(image_palm, pt_dist_contour_circle_min_old, pt_dist_contour_circle_min_first, Scalar(254), 1);
		line(image_palm_segmented, pt_dist_contour_circle_min_old, pt_dist_contour_circle_min_first, Scalar(0), 1);
	}*/

	//------------------------------------------------------------------------------------------------------------------------------

	if (vertices_0_0_y < vertices_0_3_y)
		floodFill(image_palm, Point(0, HEIGHT_SMALL_MINUS), Scalar(127));
	else
		floodFill(image_palm, Point(WIDTH_SMALL_MINUS, HEIGHT_SMALL_MINUS), Scalar(127));

	//------------------------------------------------------------------------------------------------------------------------------

	imshow("image_palm_segmented" + name, image_palm_segmented);
	return false;

	BlobDetectorNew* blob_detector_image_palm_segmented = value_store->get_blob_detector("blob_detector_image_palm_segmented");
	blob_detector_image_palm_segmented->compute(image_palm_segmented, 254,
												x_min_hand_right, x_max_hand_right,
									            y_min_hand_right, y_max_hand_right, true);

	for (BlobNew& blob : *blob_detector_image_palm_segmented->blobs)
	{
		bool hit = false;
		for (Point& pt : blob.data)
			if (image_palm.ptr<uchar>(pt.y, pt.x)[0] != 127)
			{
				hit = true;
				break;
			}
		if (hit)
			blob.active = false;
	}

	fingertip_points.clear();
	fingertip_blobs.clear();

	vector<Point> fingertip_convexities;
	for (Point3f& pt : convex_points_indexed)
	{
		BlobNew* blob_min_dist = NULL;
		float min_dist = 9999;
		for (BlobNew& blob : *blob_detector_image_palm_segmented->blobs)
		{
			if (!blob.active)
				continue;

			float dist = blob.compute_min_dist(Point(pt.x, pt.y), NULL, false);
			if (dist < min_dist)
			{
				min_dist = dist;
				blob_min_dist = &blob;
			}
		}
		if (blob_min_dist != NULL && blob_min_dist->active && blob_min_dist->count >= 10)
		{
			blob_min_dist->fill(image_palm_segmented, 127);

			bool found = false;
			for (Point& pt_fingertip : fingertip_convexities)
				if (pt_fingertip == blob_min_dist->pt_y_max)
				{
					found = true;
					break;
				}
			if (!found)
			{
				fingertip_blobs.push_back(*blob_min_dist);
				fingertip_convexities.push_back(blob_min_dist->pt_y_max);
			}
		}
	}

	if (fingertip_blobs.size() == 0)
		return false;

	//------------------------------------------------------------------------------------------------------------------------------

	float count_max = 0;
	for (BlobNew& blob : fingertip_blobs)
		if (blob.count > count_max)
			count_max = blob.count;

	float count_significant = 0;

	vector<BlobNew> fingertip_blobs_filtered0;
	for (BlobNew& blob : fingertip_blobs)
	{
		// if (blob.count > count_max / 10)
			fingertip_blobs_filtered0.push_back(blob);

		if (blob.count > count_max / 3)
			++count_significant;
	}

	fingertip_blobs = fingertip_blobs_filtered0;

	if (fingertip_blobs.size() == 0)
		return false;

	sort(fingertip_blobs.begin(), fingertip_blobs.end(), compare_blob_angle(pt_palm));
	bool has_all_fingers = fingertip_blobs.size() >= 5;//mark

	//------------------------------------------------------------------------------------------------------------------------------

	if (has_all_fingers)
	{
		vector<BlobNew> fingertip_blobs_count_sorted = fingertip_blobs;
		// sort(fingertip_blobs_count_sorted.begin(), fingertip_blobs_count_sorted.end(), compare_blob_count());

		vector<BlobNew> fingertip_blobs_filtered1;

		const int i_max = fingertip_blobs_count_sorted.size() - 1;
		const int i_min = fingertip_blobs_count_sorted.size() - 5;
		for (int i = i_max; i >= i_min; --i)
			fingertip_blobs_filtered1.push_back(fingertip_blobs_count_sorted[i]);

		sort(fingertip_blobs_filtered1.begin(), fingertip_blobs_filtered1.end(), compare_blob_angle(pt_palm));
		fingertip_blobs = fingertip_blobs_filtered1;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	{
		int index = -1;
		for (BlobNew& blob : fingertip_blobs)
		{
			++index;
			blob.index = index;
		}
	}

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

	Mat image_skeleton_segmented1 = Mat::zeros(image_skeleton.size(), CV_8UC1);
	for (Point& pt : skeleton_points)
		image_skeleton_segmented1.ptr<uchar>(pt.y, pt.x)[0] = 254;

	vector<Point> skeleton_branch_centers;

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
		{
			circle(image_skeleton_segmented, pt, 3, Scalar(127), -1);
			circle(image_skeleton_segmented1, pt, 3, Scalar(127), -1);
			skeleton_branch_centers.push_back(pt);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	for (BlobNew& blob : blobs_hand)
		for (Point& pt : blob.data)
			if (image_palm.ptr<uchar>(pt.y, pt.x)[0] != 127)
				image_skeleton_segmented.ptr<uchar>(pt.y, pt.x)[0] = 127;

	circle(image_skeleton_segmented1, pt_palm, palm_radius, Scalar(127), -1);

	//------------------------------------------------------------------------------------------------------------------------------

	BlobDetectorNew* blob_detector_image_skeleton_segmented = value_store->get_blob_detector("blob_detector_image_skeleton_segmented");
	blob_detector_image_skeleton_segmented->compute(image_skeleton_segmented, 254,
													x_min_hand_right, x_max_hand_right,
										            y_min_hand_right, y_max_hand_right, false, true);

	BlobDetectorNew* blob_detector_image_skeleton_parts = value_store->get_blob_detector("blob_detector_image_skeleton_parts");

	unordered_map<ushort, vector<Point>> skeleton_parts_map;
	unordered_map<ushort, vector<Point>> skeleton_extensions_map;

	float dist_to_palm_max = -1;

	for (BlobNew& blob : *blob_detector_image_skeleton_segmented->blobs)
	{
		bool overlap_found = false;
		BlobNew* overlap_blob = NULL;
		for (BlobNew& blob_fingertip : fingertip_blobs)
			if (blob.compute_overlap(blob_fingertip) > 0)
			{
				overlap_found = true;
				overlap_blob = &blob_fingertip;
				break;
			}
		if (!overlap_found)
			continue;

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

					skeleton_parts_map[overlap_blob->atlas_id] = blob.data;
					skeleton_extensions_map[overlap_blob->atlas_id] = extension_line_vec;

					//---------------------------------------------------------------------------------------------------

					Point pt_first = blob_detector_image_skeleton_parts->blob_max_size->data[0];
					float dist_to_palm = get_distance(pt_first, pt_palm, true);
					dist_to_palm = dist_to_palm + blob_detector_image_skeleton_parts->blob_max_size->count - palm_radius;

					if (dist_to_palm > dist_to_palm_max)
						dist_to_palm_max = dist_to_palm;

					overlap_blob->dist = dist_to_palm;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	if (dist_to_palm_max != -1)
	{
		float dist_ratio_min = 9999;
		for (BlobNew& blob : fingertip_blobs)
		{
			if (blob.dist == -1)
				blob.dist = get_distance(blob.pt_y_max, pt_palm, true);

			float dist_ratio = blob.dist / dist_to_palm_max;
			if (dist_ratio < 0.1)
				blob.active = false;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	{
		int index = 0;
		for (BlobNew& blob : fingertip_blobs)
		{
			bool fingertip_point_pushed = false;
			if (skeleton_extensions_map.count(blob.atlas_id) > 0)
			{
				vector<Point> extension_line_vec = skeleton_extensions_map[blob.atlas_id];
				for (Point& pt : extension_line_vec)
				{
					if (pt.x < 0 || pt.x >= WIDTH_SMALL || pt.y < 0 || pt.y >= HEIGHT_SMALL)
						continue;

					if (image_active_hand.ptr<uchar>(pt.y, pt.x)[0] == 0)
					{
						fingertip_point_pushed = true;
						fingertip_points.push_back(blob.active ? pt : Point(-1, -1));
						blob.pt_tip = pt;
						break;					
					}
				}
			}
			if (!fingertip_point_pushed)
			{
				fingertip_points.push_back(blob.active ? fingertip_convexities[index] : Point(-1, -1));
				blob.pt_tip = fingertip_convexities[index];
			}
			++index;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	Mat image_visualization = image_active_hand.clone();
	// Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	const int x_diff_rotation = WIDTH_SMALL / 2 - palm_point.x;
	const int y_diff_rotation = HEIGHT_SMALL / 2 - palm_point.y;

	Point pt_palm_rotated = pt_palm;
	pt_palm_rotated.x += x_diff_rotation;
	pt_palm_rotated.y += y_diff_rotation;

	{
		BlobDetectorNew* blob_detector_image_skeleton_segmented1 =
			value_store->get_blob_detector("blob_detector_image_skeleton_segmented1");

		blob_detector_image_skeleton_segmented1->compute(image_skeleton_segmented1, 254,
														 x_min_hand_right, x_max_hand_right,
											             y_min_hand_right, y_max_hand_right, false, true);

		Mat image_sort_skeleton = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
		BlobDetectorNew* blob_detector_image_sort_skeleton = value_store->get_blob_detector("blob_detector_image_sort_skeleton");

		float angle_mean = 0;
		int angle_count = 0;

		int index = -1;
		for (BlobNew& blob : fingertip_blobs)
		{
			++index;
			if (skeleton_extensions_map.count(blob.atlas_id) > 0)
			{
				vector<Point> extension_line_vec = skeleton_extensions_map[blob.atlas_id];
				Point pt0 = extension_line_vec[0];
				Point pt1 = extension_line_vec[extension_line_vec.size() - 1];
				float angle = get_angle(pt0, pt1, true) - 90;

				int weight = 1000 / get_distance(pt1, Point(pt_palm.x + palm_radius, HEIGHT_SMALL), true) * blob.count;

				angle_mean += angle * weight;
				angle_count += weight;
			}

			//-----------------------------------------------------------------------------------------------------------------

			BlobNew* blob_skeleton_y_min = NULL;
			for (BlobNew& blob_skeleton : *blob_detector_image_skeleton_segmented1->blobs)
				if (blob.compute_overlap(blob_skeleton) > 0)
					if (blob_skeleton_y_min == NULL || blob_skeleton.y_min < blob_skeleton_y_min->y_min)
						blob_skeleton_y_min = &blob_skeleton;

			if (blob_skeleton_y_min != NULL)
			{
				fill_mat(blob_skeleton_y_min->data, image_sort_skeleton, 254);

				Point pt_origin = blob_skeleton_y_min->data[0];
				blob_detector_image_sort_skeleton->
					compute_location(image_sort_skeleton, 254, pt_origin.x, pt_origin.y, false, false, true);

				const int skeleton_size = blob_detector_image_sort_skeleton->blob_max_size->data.size();

				pt_origin = blob_detector_image_sort_skeleton->blob_max_size->data[skeleton_size - 1];
				blob_detector_image_sort_skeleton->
					compute_location(image_sort_skeleton, 254, pt_origin.x, pt_origin.y, true, false, true);

				Point pt_compare0 = blob_detector_image_sort_skeleton->blob_max_size->data[0];
				Point pt_compare1 = blob_detector_image_sort_skeleton->blob_max_size->data[skeleton_size / 2];

				float dist_compare0 = get_distance(pt_compare0, Point(pt_palm.x, 0), false);
				float dist_compare1 = get_distance(pt_compare1, Point(pt_palm.x, 0), false);

				int i_begin;
				int i_end;
				int i_increment;
				if (dist_compare0 < dist_compare1)
				{
					i_begin = 0;
					i_end = skeleton_size;
					i_increment = 1;
				}
				else
				{
					i_begin = skeleton_size - 1;
					i_end = -1;
					i_increment = -1;
				}

				blob.skeleton.clear();

				const int i_begin_const = i_begin;
				const int i_end_const = i_end;
				const int i_increment_const = i_increment;
				for (int i = i_begin_const; i != i_end_const; i += i_increment_const)
					blob.skeleton.push_back(blob_detector_image_sort_skeleton->blob_max_size->data[i]);
			}
		}
		angle_mean /= angle_count;
		hand_angle = angle_mean;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	while (count_significant <= 2 || value_store->get_float("y_max_diff_ratio", 0) >= 0.5)
	{
		hand_angle = (hand_angle + hand_angle_old) / 2;

		Point pt_anchor = Point(pt_palm_rotated.x, HEIGHT_SMALL);
		Point pt_search_hand_angle = value_store->get_point("pt_search_hand_angle", Point(pt_palm.x, HEIGHT_SMALL));

		float dist_min = 9999;
		vector<Point> extension_line_vec_dist_min;
		Point pt_search_hand_angle_candidate;

		int index = -1;
		for (BlobNew& blob : fingertip_blobs)
		{
			if (fingertip_blobs.size() >= 4 && index == fingertip_blobs.size() - 1)
				continue;

			if (skeleton_extensions_map.count(blob.atlas_id) > 0)
			{
				vector<Point> extension_line_vec = skeleton_extensions_map[blob.atlas_id];
				Point pt_root = extension_line_vec[0];
				Point pt_skeleton_root_rotated = rotate_point(-hand_angle, pt_root, palm_point);
				pt_skeleton_root_rotated.x += x_diff_rotation;
				pt_skeleton_root_rotated.y += y_diff_rotation;

				Point pt_search_adjusted = pt_search_hand_angle;
					/*Point(pt_search_hand_angle.x - (palm_radius / 1), pt_search_hand_angle.y);*/

				float dist = get_distance(pt_anchor, pt_skeleton_root_rotated, false) +
							 get_distance(blob.pt_y_min, pt_palm, false) +
							 get_distance(blob.pt_tip, pt_search_adjusted, false);

				if (dist < dist_min)
				{
					dist_min = dist;
					extension_line_vec_dist_min = extension_line_vec;
					pt_search_hand_angle_candidate = blob.pt_tip;
				}
			}
		}

		if (extension_line_vec_dist_min.size() == 0)
			break;

		pt_search_hand_angle = pt_search_hand_angle_candidate;
		low_pass_filter->compute(pt_search_hand_angle, 0.5, "pt_search_hand_angle");
		value_store->set_point("pt_search_hand_angle", pt_search_hand_angle);

		Point pt_first = extension_line_vec_dist_min[0];
		Point pt_last = extension_line_vec_dist_min[extension_line_vec_dist_min.size() - 1];
		float hand_angle_new = get_angle(pt_first, pt_last, true) - 90;

		hand_angle = (hand_angle + hand_angle_new) / 2;
		hand_angle = hand_angle_new;

		vector<Point> extension_line_vec_dist_min_rotated;
		for (Point& pt : extension_line_vec_dist_min)
		{
			Point pt_rotated = rotate_point(-hand_angle, pt, palm_point);
			pt_rotated.x += x_diff_rotation;
			pt_rotated.y += y_diff_rotation;
			extension_line_vec_dist_min_rotated.push_back(pt_rotated);
		}
		fill_mat(extension_line_vec_dist_min_rotated, image_visualization, 127);
		circle(image_visualization, pt_palm_rotated, palm_radius, Scalar(254), 1);

		break;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	low_pass_filter->compute(hand_angle, 0.5, "hand_angle");
	value_store->set_float("hand_angle", hand_angle);
	value_store->set_bool("hand_angle_set", true);

	//------------------------------------------------------------------------------------------------------------------------------

	{//mark
		for (BlobNew& blob : fingertip_blobs)
		{
			vector<Point> blob_contour;
			for (Point& pt : contour_sorted)
				if (blob.image_atlas.ptr<ushort>(pt.y, pt.x)[0] == blob.atlas_id)
					blob_contour.push_back(pt);

			fill_mat(blob_contour, image_visualization, 127);
		}

	}

	// stereo_matching_points = contour_sorted;
	// stereo_matching_points.push_back(pt_alignment);

	//------------------------------------------------------------------------------------------------------------------------------

	for (BlobNew& blob : fingertip_blobs)
	{
		for (Point& pt : blob.data)
		{
			Point pt_rotated = rotate_point(-hand_angle, pt, palm_point);
			pt_rotated.x += x_diff_rotation;
			pt_rotated.y += y_diff_rotation;
			blob.data_rotated.push_back(pt_rotated);
		}
		for (Point& pt : blob.skeleton)
		{
			Point pt_rotated = rotate_point(-hand_angle, pt, palm_point);
			pt_rotated.x += x_diff_rotation;
			pt_rotated.y += y_diff_rotation;
			blob.skeleton_rotated.push_back(pt_rotated);
		}

		Point pt_tip_rotated = rotate_point(-hand_angle, blob.pt_tip, palm_point);
		pt_tip_rotated.x += x_diff_rotation;
		pt_tip_rotated.y += y_diff_rotation;

		blob.pt_tip_rotated = pt_tip_rotated;

		//--------------------------------------------------------------------------------------------------------------------------

		sort(blob.data.begin(), blob.data.end(), compare_point_dist(pt_palm));
		Point pt_root = blob.data[blob.data.size() * 0.3];

		Point pt_root_rotated = rotate_point(-hand_angle, pt_root, palm_point);
		pt_root_rotated.x += x_diff_rotation;
		pt_root_rotated.y += y_diff_rotation;

		blob.pt_root = pt_root;
		blob.pt_root_rotated = pt_root_rotated;

		//--------------------------------------------------------------------------------------------------------------------------

		int x_min_rotated;
		int x_max_rotated;
		int y_min_rotated;
		int y_max_rotated;
 		get_bounds(blob.data_rotated, x_min_rotated, x_max_rotated, y_min_rotated, y_max_rotated);

 		blob.x_min_rotated = x_min_rotated;
 		blob.x_max_rotated = x_max_rotated;
 		blob.y_min_rotated = y_min_rotated;
 		blob.y_max_rotated = y_max_rotated;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	for (BlobNew& blob : fingertip_blobs)
	{
		fill_mat(blob.data_rotated, image_visualization, 127);
		fill_mat(blob.skeleton_rotated, image_visualization, 254);
	}

	//------------------------------------------------------------------------------------------------------------------------------

	int blob_id_max = value_store->get_int("blob_id_max", 0);

	vector<BlobNew>* fingertip_blobs_old_ptr = value_store->get_blob_vec("fingertip_blobs_old_ptr");
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

		vector<Point> line_points;
		bresenham_line(pt0.x, pt0.y, pt1.x, pt1.y, line_points, 9999);

		int cross_count = 0;
		for (Point& pt : line_points)
			if (image_skeleton.ptr<uchar>(pt.y, pt.x)[0] > 0)
				++cross_count;

		float motion_dist = get_distance(pt0_rotated, pt1_rotated, false);

		if (cross_count > 0 || motion_dist > 15)
		{
			blob.matching_blob = NULL;
			continue;
		}

		line(image_visualization, pt0_rotated, pt1_rotated, Scalar(254), 1);
		line(image_visualization, pt0, pt1, Scalar(127), 1);
	}

	value_store->set_int("blob_id_max", blob_id_max);

	//------------------------------------------------------------------------------------------------------------------------------
	//find thumb

	bool has_thumb = false;
	int thumb_index = -1;
	BlobNew* blob_thumb = NULL;

	{
		vector<BlobNew*> candidate_vec;

		//--------------------------------------------------------------------------------------------------------------------------

		BlobNew* blob_y_max = NULL;
		for (BlobNew& blob : fingertip_blobs)
			if ((blob_y_max == NULL || blob.y_max_rotated > blob_y_max->y_max_rotated))
				blob_y_max = &blob;

		BlobNew* blob_y_max_second = NULL;
		for (BlobNew& blob : fingertip_blobs)
			if ((blob_y_max_second == NULL || blob.y_max_rotated > blob_y_max_second->y_max_rotated) && &blob != blob_y_max)
				blob_y_max_second = &blob;

		float y_max_diff_ratio = 999;
		if (blob_y_max != NULL && blob_y_max_second != NULL)
			y_max_diff_ratio = (blob_y_max->y_max_rotated - blob_y_max_second->y_max_rotated) / palm_radius;

		//--------------------------------------------------------------------------------------------------------------------------

		int index = -1;
		for (BlobNew& blob : fingertip_blobs)
		{
			++index;

			//--------------------------------------------------------------------------------------------------------------------------

			{
				float dist_index = get_distance(blob.pt_tip, pt_index, false);
				float dist_thumb = get_distance(blob.pt_tip, pt_thumb, false);

				if (dist_index < dist_thumb)
					continue;
			}

			//--------------------------------------------------------------------------------------------------------------------------

			int range_min = pt_palm_rotated.y;
			int range_max = blob_y_max->pt_tip_rotated.y;

			bool verification_required = false;
			if (blob.pt_tip_rotated.y >= range_min && blob.pt_tip_rotated.y <= range_max)
			{
				float range_ratio = map_val(blob.pt_tip_rotated.y, range_min, range_max, 0, 1);
				if (range_ratio > 0.6)
					verification_required = true;
			}

			bool verification_passed = false;
			if (verification_required)
			{
				int in_range_count = 0;
				for (BlobNew& blob : fingertip_blobs)
				{
					if (!(blob.pt_tip_rotated.y >= range_min && blob.pt_tip_rotated.y <= range_max))
						continue;

					float range_ratio = map_val(blob.pt_tip_rotated.y, range_min, range_max, 0, 1);
					if (range_ratio > 0.6)
						++in_range_count;
				}
				if (in_range_count > 1)
					verification_passed = true;
			}
			else
				verification_passed = true;

			if (verification_required && !verification_passed)
				continue;

			//--------------------------------------------------------------------------------------------------------------------------

			if (has_all_fingers && index == 0)
				push_blob(candidate_vec, &fingertip_blobs[0]);

			//--------------------------------------------------------------------------------------------------------------------------

			Point pt_x_min = get_x_min_point(blob.data_rotated);
			if (pt_x_min.x > pt_palm_rotated.x - palm_radius)
				continue;

			if (blob.pt_root_rotated.y - pt_palm_rotated.y < 0 && &blob != blob_y_max)
				push_blob(candidate_vec, &blob);

			//-----------------------------------------------------------------------------------------------------------------------

			if (blob.skeleton_rotated.size() == 0)
				continue;

			Point pt_first = blob.skeleton_rotated[0];
			Point pt_last = blob.skeleton_rotated[blob.skeleton_rotated.size() - 1];

			float normal_dist_max = 0;
			float normal_dist_max_abs = -1;
			Point pt_normal_dist_max;
			for (Point& pt : blob.skeleton_rotated)
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

			if (normal_dist_max_abs > -1 && normal_dist_max <= -3 && &blob != blob_y_max)
			{
				push_blob(candidate_vec, &blob);
				circle(image_visualization, pt_normal_dist_max, 3, Scalar(254), 1);
			}

			//-----------------------------------------------------------------------------------------------------------------------

			while (index <= 1)
			{
				const int size_threshold = 3;
				const float size_ratio = 0.2;

				if (blob.skeleton_rotated.size() < size_threshold)
					break;

				const int index_first_top = blob.skeleton_rotated.size() * size_ratio < (size_threshold - 1) ?
											(size_threshold - 1) : blob.skeleton_rotated.size() * size_ratio;
				const int index_last_top = 0;

				if (index_first_top == index_last_top)
					break;

				Point pt_first_top = blob.skeleton_rotated[index_first_top];
				Point pt_last_top = blob.skeleton_rotated[index_last_top];

				if (pt_first_top.x > pt_last_top.x)
					break;

				float angle = get_angle(pt_first_top, pt_last_top, true);
				if (angle > 170 && &blob != blob_y_max)
					push_blob(candidate_vec, &blob);

				//-----------------------------------------------------------------------------------------------------------------------

				Point pt0 = get_x_min_point(blob.data_rotated);
				Point pt1 = get_y_min_point(blob.data_rotated);
				Point pt2 = get_y_max_point(blob.data_rotated);

				if (pt0 == pt1 || pt0 == pt2 || pt1 == pt2)
					break;

				float angle_knuckle = get_angle(pt0, pt1, pt2);
				if (angle_knuckle < 100 && &blob != blob_y_max)
					push_blob(candidate_vec, &blob);

				break;
			}
		}

		//------------------------------------------------------------------------------------------------------------------------------

		Point pt_search = Point(0, pt_palm_rotated.y);

		float dist_min = 9999;
		BlobNew* blob_dist_min = NULL;
		for (BlobNew* blob_candidate : candidate_vec)
		{
			float dist0 = get_distance(pt_search, Point(blob_candidate->x, blob_candidate->y), false);
			float dist1 = get_distance(blob_candidate->pt_root_rotated, pt_thumb_root, false);
			float dist = dist0 + dist1;

			if (dist < dist_min)
			{
				dist_min = dist;
				blob_dist_min = blob_candidate;
			}
		}

		//------------------------------------------------------------------------------------------------------------------------------

		// if (blob_dist_min != NULL && !verify_detection(blob_dist_min, value_store))
			// blob_dist_min = NULL;

		//------------------------------------------------------------------------------------------------------------------------------

		if (blob_dist_min != NULL)
		{
			has_thumb = true;
			thumb_index = blob_dist_min->index;
			blob_dist_min->name = "0";
			blob_thumb = blob_dist_min;
			pt_thumb = blob_dist_min->pt_tip;

			if (blob_dist_min->skeleton_rotated.size() > 0)
			{
				pt_thumb_root = blob_dist_min->skeleton_rotated[0];
				low_pass_filter->compute(pt_thumb_root, 0.01, "pt_thumb_root");
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	BlobNew* blob_y_max = NULL;
	for (BlobNew& blob : fingertip_blobs)
		if ((blob_y_max == NULL || blob.y_max_rotated > blob_y_max->y_max_rotated) && blob.name != "0")
			blob_y_max = &blob;

	BlobNew* blob_y_max_second = NULL;
	for (BlobNew& blob : fingertip_blobs)
		if ((blob_y_max_second == NULL || blob.y_max_rotated > blob_y_max_second->y_max_rotated) &&
				&blob != blob_y_max && blob.name != "0")
		{
			blob_y_max_second = &blob;
		}

	float y_max_diff_ratio = 999;
	if (blob_y_max != NULL && blob_y_max_second != NULL)
		y_max_diff_ratio = (blob_y_max->y_max_rotated - blob_y_max_second->y_max_rotated) / palm_radius;

	value_store->set_float("y_max_diff_ratio", y_max_diff_ratio);

	//------------------------------------------------------------------------------------------------------------------------------
	//find index finger

	bool has_index = false;
	int index_index = -1;
	BlobNew* blob_index = NULL;

	{
		vector<BlobNew*> candidate_vec;

		if (has_all_fingers)
			push_blob(candidate_vec, &fingertip_blobs[1]);

		//------------------------------------------------------------------------------------------------------------------------------

		if (has_thumb)
		{
			float dist_min = 9999;
			BlobNew* blob_dist_min = NULL;
			for (BlobNew& blob : fingertip_blobs)
			{
				if (blob.index <= thumb_index)
					continue;

				float dist0 = get_distance(blob.pt_root_rotated, pt_index_root, false);
				float dist1 = get_distance(blob.pt_root_rotated, Point(0, HEIGHT_SMALL), false);
				float dist = dist0 + dist1;
				if (dist < dist_min)
				{
					dist_min = dist;
					blob_dist_min = &blob;
				}
			}
			if (blob_dist_min != NULL)
				push_blob(candidate_vec, blob_dist_min);
		}

		//------------------------------------------------------------------------------------------------------------------------------

		{
			int y_max0 = 0;
			BlobNew* blob_y_max0 = NULL;
			for (BlobNew& blob : fingertip_blobs)
				if (blob.y_max_rotated > y_max0)
				{
					y_max0 = blob.y_max_rotated;
					blob_y_max0 = &blob;
				}

			int y_max1 = 0;
			BlobNew* blob_y_max1 = NULL;
			for (BlobNew& blob : fingertip_blobs)
			{
				if (&blob == blob_y_max0)
					continue;

				if (blob.y_max_rotated > y_max1)
				{
					y_max1 = blob.y_max_rotated;
					blob_y_max1 = &blob;
				}
			}

			if (blob_y_max0 != NULL && blob_y_max1 != NULL)
			{
				float ratio = (float)(y_max0 - y_max1) / palm_radius;
				if (ratio > 1)
					push_blob(candidate_vec, blob_y_max0);
			}
			else if (blob_y_max0 != NULL)
				push_blob(candidate_vec, blob_y_max0);
		}

		//------------------------------------------------------------------------------------------------------------------------------

		{
			vector<BlobNew*> candidate_vec_filtered;
			for (BlobNew* blob : candidate_vec)
			{
				if (blob->index <= thumb_index)
					continue;

				bool verification_passed = false;
				if (has_thumb)
				{
					if (blob->index == thumb_index + 1)
						verification_passed = true;
				}
				else
					verification_passed = true;

				//----------------------------------------------------------------------------------------------------------------------

				if (verification_passed)
				{
					int height_diff = blob->pt_tip_rotated.y - pt_palm_rotated.y - palm_radius;
					if (height_diff < 0)
						verification_passed = false;
				}

				//----------------------------------------------------------------------------------------------------------------------

				if (verification_passed)
					if (y_max_diff_ratio > 0.3 && blob->pt_tip_rotated.x > (pt_palm_rotated.x + (palm_radius / 2)))
						verification_passed = false;

				//----------------------------------------------------------------------------------------------------------------------

				if (verification_passed)
					candidate_vec_filtered.push_back(blob);
			}
			candidate_vec = candidate_vec_filtered;
		}

		//------------------------------------------------------------------------------------------------------------------------------

		Point pt_search = Point(0, HEIGHT_SMALL);

		float dist_min = 9999;
		BlobNew* blob_dist_min = NULL;
		for (BlobNew* blob_candidate : candidate_vec)
		{
			float dist = blob_candidate->pt_tip_rotated.x;
			if (dist < dist_min)
			{
				dist_min = dist;
				blob_dist_min = blob_candidate;
			}
		}

		//------------------------------------------------------------------------------------------------------------------------------

		if (blob_dist_min != NULL && !verify_detection(blob_dist_min, value_store))
			blob_dist_min = NULL;

		//------------------------------------------------------------------------------------------------------------------------------

		if (blob_dist_min != NULL)
		{
			has_index = true;
			index_index = blob_dist_min->index;
			blob_dist_min->name = "1";
			blob_index = blob_dist_min;
			pt_index = blob_dist_min->pt_tip;

			if (blob_dist_min->skeleton_rotated.size() > 0)
			{
				pt_index_root = blob_dist_min->skeleton_rotated[0];
				low_pass_filter->compute(pt_index_root, 0.01, "pt_index_root");
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------
	//track unnamed blobs

	for (BlobNew& blob : fingertip_blobs)
		if (blob.name == "" && blob.matching_blob != NULL && blob.matching_blob->name != "")
		{
			blob.name = blob.matching_blob->name;

			if (blob.name == "0" && has_thumb)
				blob.name = "";
			else if (blob.name == "0")
			{
				has_thumb = true;
				thumb_index = blob.index;
				blob_thumb = &blob;
			}

			if (blob.name == "1" && has_index)
				blob.name = "";
			else if (blob.name == "1")
			{
				has_index = true;
				index_index = blob.index;
				blob_index = &blob;
			}
		}

	//------------------------------------------------------------------------------------------------------------------------------

	if (has_index && has_thumb && blob_thumb->pt_tip_rotated.x > blob_index->pt_tip_rotated.x)
	{
		bool has_thumb = false;
		int thumb_index = -1;
		BlobNew* blob_thumb = NULL;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	if (has_index && !has_thumb)
	{
		BlobNew* blob_x_min = NULL;
		for (BlobNew& blob : fingertip_blobs)
		{
			if (blob.index >= index_index || blob.pt_tip_rotated.x > blob_index->pt_tip_rotated.x)
				continue;

			if (blob_x_min == NULL || blob.x_min_rotated < blob_x_min->x_min_rotated)
				blob_x_min = &blob;
		}
		if (blob_x_min != NULL)
		{
			blob_x_min->name = "0";
			has_thumb = true;
			thumb_index = blob_x_min->index;
			blob_thumb = blob_x_min;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	*fingertip_blobs_old_ptr = fingertip_blobs;

	//------------------------------------------------------------------------------------------------------------------------------

	circle(image_visualization, pt_thumb_root, 5, Scalar(254), 1);
	circle(image_visualization, pt_index_root, 5, Scalar(254), 1);

	for (BlobNew& blob : fingertip_blobs)
		circle(image_visualization, blob.pt_tip, 3, Scalar(127), -1);

	//------------------------------------------------------------------------------------------------------------------------------

	for (BlobNew& blob : fingertip_blobs)
	{
		if (blob.name == "")
			continue;

		put_text(blob.name, image_visualization, get_y_max_point(blob.data_rotated));
	}

	circle(image_visualization, pt_palm, palm_radius, Scalar(127), 1);
	imshow("image_visualizationadlfkjhasdlkf" + name, image_visualization);
	imshow("image_palm_segmented" + name, image_palm_segmented);

	//------------------------------------------------------------------------------------------------------------------------------

	algo_name_vec.push_back(algo_name);
	return true;
}