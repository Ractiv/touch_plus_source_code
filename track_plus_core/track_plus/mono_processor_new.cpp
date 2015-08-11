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

bool MonoProcessorNew::compute(HandSplitterNew& hand_splitter, const string name, bool visualize, bool secondary)
{
	pt_index = Point(-1, -1);
	pt_thumb = Point(-1, -1);

	if (value_store.get_bool("reset", true)) {}

	//------------------------------------------------------------------------------------------------------------------------------

	if (hand_splitter.primary_hand_blobs.size() == 0)
	{
		value_store.set_bool("reset", true);
		return false;
	}

	Mat image_active_hand = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_find_contours = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	pt_palm = Point2f(0, 0);
	int pt_palm_count = 0;
	const int y_threshold = pt_hand_anchor.y - palm_radius;

	for (BlobNew& blob : hand_splitter.primary_hand_blobs)
	{
		blob.fill(image_find_contours, 254);
		blob.fill(image_active_hand, 254);

		for (Point& pt : blob.data)
			if (pt.y > y_threshold)
			{
				pt_palm.x += pt.x;
				pt_palm.y += pt.y;
				++pt_palm_count;
			}
	}

	pt_palm.x /= pt_palm_count;
	pt_palm.y /= pt_palm_count;

	// vector<Vec4i> hiearchy;
	// vector<vector<Point>> contours;
	// findContours(image_find_contours, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	vector<vector<Point>> contours = legacyFindContours(image_find_contours);

	const int contours_size = contours.size();

	if (contours_size == 0)
	{
		value_store.set_bool("reset", true);
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
		for (BlobNew& blob : hand_splitter.primary_hand_blobs)
			blob.fill(image_find_contours, 254);

		for (int i = 0; i < contours_reduced.size(); ++i)
		{
			if (contours_reduced.size() == 1)
				break;

			vector<Point>* contour0 = &(contours_reduced[i]);

			float dist_min = 9999;
			int index_contour_dist_min;
			Point pt_dist_min0;
			Point pt_dist_min1;

			for (int a = 0; a < contours_reduced.size(); ++a)
				if (a != i)
				{
					vector<Point>* contour1 = &(contours_reduced[a]);

					for (Point& pt0 : *contour0)
						for (Point& pt1 : *contour1)
						{
							const float dist_current = get_distance(pt0, pt1);

							if (dist_current < dist_min)
							{
								dist_min = dist_current;
								index_contour_dist_min = a;
								pt_dist_min0 = pt0;
								pt_dist_min1 = pt1;
							}
						}
				}

			for (Point& pt : contours_reduced[index_contour_dist_min])
				contour0->push_back(pt);

			contours_reduced.erase(contours_reduced.begin() + index_contour_dist_min);
			--i;

			line(image_find_contours, pt_dist_min0, pt_dist_min1, Scalar(254), 2);
		}

		// hiearchy = vector<Vec4i>();
		// contours = vector<vector<Point>>();
		// findContours(image_find_contours, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
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

		pt_hand_anchor = max_loc * 4;
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

		Point pt_hand_anchor_new = max_loc * 4;
		pt_hand_anchor.y = pt_hand_anchor_new.y;
		pt_hand_anchor.x = pt_hand_anchor_new.x;
		pt_hand_anchor.x = pt_palm.x;

		palm_radius = max * 4;
		float multiplier = palm_radius > 10 ? 10 : palm_radius;
		multiplier = map_val(multiplier, 0, 10, 2, 1);
		palm_radius *= multiplier;
	}

	static float angle_final_diff = 0;
	if (angle_final_diff > 10)
		angle_final_diff = 10;

	float alpha = map_val(angle_final_diff, 0, 10, 0.01, 1);
	low_pass_filter.compute(pt_hand_anchor, alpha, "pt_hand_anchor");

	low_pass_filter.compute(palm_radius, 0.1, "palm_radius");

	//------------------------------------------------------------------------------------------------------------------------------

	Point pivot = Point(pt_hand_anchor.x + palm_radius, 0);

	vector<Point> contour_sorted;
	sort_contour(contours[0], contour_sorted, pivot);

	if (contour_sorted.size() == 0)
	{
		value_store.set_bool("reset", true);
		return false;
	}

	vector<Point> contour_approximated;
	approxPolyDP(Mat(contour_sorted), contour_approximated, 2, true);
	contour_approximated.insert(contour_approximated.begin(), contour_sorted[0]);
	contour_approximated.push_back(contour_sorted[contour_sorted.size() - 1]);

	{
		points_unwrapped_result = contour_approximated;
		// points_unwrapped_result = vector<Point>();
		// compute_unwrap2(contour_approximated, pivot, points_unwrapped_result);
	}

	//------------------------------------------------------------------------------------------------------------------------------

	Mat image_palm = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	vector<int> convex_indexes;
	vector<int> concave_indexes;
	vector<Point> points_unwrapped;
	compute_unwrap(contour_approximated, pivot, convex_indexes, concave_indexes, points_unwrapped);

	{
		if (convex_indexes.size() == 0)
		{
			value_store.set_bool("reset", true);
			return false;
		}

		//rotated rect masking of palm
		RotatedRect r_rect0 = RotatedRect(pt_hand_anchor, Size2f(500, 500), -angle_final);
		Point2f vertices0[4];
		r_rect0.points(vertices0);

		int vertices_0_3_x = (vertices0[3].x - vertices0[2].x) / 2 + vertices0[2].x;
		int vertices_0_3_y = (vertices0[3].y - vertices0[2].y) / 2 + vertices0[2].y - palm_radius;
		int vertices_0_0_x = (vertices0[0].x - vertices0[1].x) / 2 + vertices0[1].x;
		int vertices_0_0_y = (vertices0[0].y - vertices0[1].y) / 2 + vertices0[1].y - palm_radius;

		line(image_palm, Point(vertices_0_3_x, vertices_0_3_y), Point(vertices_0_0_x, vertices_0_0_y), Scalar(254), 1);

		RotatedRect r_rect1 = RotatedRect(pt_hand_anchor, Size2f(palm_radius * 2, 500), -angle_final);
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
		//

		if (concave_indexes.size() > 0)
		{
			vector<Point> concave_points;

			int concave_index_old = 9999;
			const int concave_indexes_size = concave_indexes.size();
			for (int i = 0; i < concave_indexes_size; ++i)
			{
				const int concave_index_new = concave_indexes[i];
				concave_points.push_back(contour_approximated[concave_index_new]);

				if (concave_index_old != 9999)
				{
					float dist_max = 0;
					Point pt_dist_max;

					for (int a = concave_index_old; a < concave_index_new; ++a)
					{
						Point pt_current = contour_approximated[a];
						const float dist_current = get_distance(pivot, pt_current);

						if (dist_current > dist_max)
						{
							dist_max = dist_current;
							pt_dist_max = pt_current;
						}
					}
				}
				concave_index_old = concave_index_new;
			}

			for (int i = 1; i < concave_indexes_size; ++i)
			{
				Point* pt_new_concave_points = &concave_points[i];
		    	Point* pt_old_concave_points = &concave_points[i - 1];
				line(image_palm, *pt_old_concave_points, *pt_new_concave_points, Scalar(254), 1);
			}

			const int extension_num_x = 0;
			const int extension_num_y = 200;

			if (concave_points[0].x == concave_points[concave_indexes_size - 1].x)
			{
				concave_points[0].x = pt_hand_anchor.x;
				concave_points[concave_indexes_size - 1].x = pt_hand_anchor.x;
			}

			Point concave_point_first = concave_points[0];
			Point concave_point_last = concave_points[concave_indexes_size - 1];

			concave_point_first.x -= extension_num_x;
			concave_point_last.x += extension_num_x;
			concave_point_first.y -= extension_num_y;
			concave_point_last.y -= extension_num_y;

			line(image_palm, concave_points[0], concave_point_first, Scalar(254), 1);
			line(image_palm, concave_points[concave_indexes_size - 1], concave_point_last, Scalar(254), 1);
		}

		//
		circle(image_palm, pt_hand_anchor, palm_radius, Scalar(254), 1);

		vector<Point> circle_vec;
		midpoint_circle(pt_hand_anchor.x, pt_hand_anchor.y, palm_radius, circle_vec);

		if (circle_vec.size() > 0)
		{
			sort(circle_vec.begin(), circle_vec.end(), compare_point_angle(pt_hand_anchor));

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

					const float dist_contour_circle = get_distance(pt, pt_contour);
					if (dist_contour_circle < dist_contour_circle_min)
					{
						dist_contour_circle_min = dist_contour_circle;
						pt_dist_contour_circle_min = pt_contour;
					}
				}

				if (pt.x > 0 && pt.x < WIDTH_SMALL && pt.y > 0 && pt.y < HEIGHT_SMALL)
				{
					image_palm.ptr<uchar>(pt.y, pt.x)[0] = 254;

					if (pt_dist_contour_circle_min_old.x != 9999)
						line(image_palm, pt_dist_contour_circle_min_old, pt_dist_contour_circle_min, Scalar(254), 1);
					else
						pt_dist_contour_circle_min_first = pt_dist_contour_circle_min;

					pt_dist_contour_circle_min_old = pt_dist_contour_circle_min;
				}
			}

			line(image_palm, pt_dist_contour_circle_min_old, pt_dist_contour_circle_min_first, Scalar(254), 1);
		}
		//
		
		if (vertices_0_0_y < vertices_0_3_y)
			floodFill(image_palm, Point(0, HEIGHT_SMALL_MINUS), Scalar(127));
		else
			floodFill(image_palm, Point(WIDTH_SMALL_MINUS, HEIGHT_SMALL_MINUS), Scalar(127));
	}

	//------------------------------------------------------------------------------------------------------------------------------

	Mat image_hand = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (BlobNew& blob : hand_splitter.primary_hand_blobs)
		for (Point& pt : blob.data)
			if (image_palm.ptr<uchar>(pt.y, pt.x)[0] != 127)
				image_hand.ptr<uchar>(pt.y, pt.x)[0] = 127;
			else
				image_hand.ptr<uchar>(pt.y, pt.x)[0] = 254;

	// GaussianBlur(image_hand, image_hand, Size(3, 3), 0, 0);
	// threshold(image_hand, image_hand, 170, 254, THRESH_BINARY);

	//------------------------------------------------------------------------------------------------------------------------------

	blob_detector_image_hand.compute(image_hand, 254, hand_splitter.x_min_result, hand_splitter.x_max_result,
									                  hand_splitter.y_min_result, hand_splitter.y_max_result, true);

	if (blob_detector_image_hand.blobs->size() == 0)
	{
		value_store.set_bool("reset", true);
		return false;
	}

	blob_detector_image_hand.sort_blobs_by_angle(pivot);

	if (secondary)
		return true;

	//--------------------------------------------------------------------------------------------------------------------------------

	const int dest_x = 80;
	const int dest_y = 40;
	dest_diff_x = pt_hand_anchor.x - dest_x;
	dest_diff_y = pt_hand_anchor.y - dest_y;

	pt_hand_anchor_rotated = rotate(pt_hand_anchor);

	Mat image_hand_rotated_raw = rotate_image(image_hand, -angle_final, pt_hand_anchor, 0);
	image_hand_rotated_raw = translate_image(image_hand_rotated_raw, dest_diff_x, dest_diff_y);

	Mat image_hand_rotated;
	int x_min_imtf;
	int x_max_imtf;
	int y_min_imtf;
	int y_max_imtf;
	threshold_get_bounds(image_hand_rotated_raw, image_hand_rotated, 250, x_min_imtf, x_max_imtf, y_min_imtf, y_max_imtf);

	blob_detector_image_hand_rotated.compute(image_hand_rotated, 254, x_min_imtf, x_max_imtf, y_min_imtf, y_max_imtf, true);

	if (blob_detector_image_hand_rotated.blobs->size() == 0)
	{
		value_store.set_bool("reset", true);
		return false;
	}

	blob_detector_image_hand_rotated.sort_blobs_by_x();

	//--------------------------------------------------------------------------------------------------------------------------------

	BlobNew* blob_index_unrotated = NULL;
	BlobNew* blob_index_rotated = NULL;
	BlobNew* blob_thumb_unrotated = NULL;
	BlobNew* blob_thumb_rotated = NULL;

	if (pose_name == "point")
	{
		BlobNew* blob_dist_min = blob_detector_image_hand_rotated.blob_max_size;
		Point pt_anchor0 = Point(pt_hand_anchor_rotated.x, HEIGHT_SMALL_MINUS);
		Point pt_anchor1 = Point(pt_hand_anchor_rotated.x + palm_radius, HEIGHT_SMALL_MINUS);

		for (BlobNew& blob : *blob_detector_image_hand_rotated.blobs)
		{
			float dist_min = get_distance(blob_dist_min->pt_y_max, pt_anchor0) + get_distance(blob_dist_min->pt_y_max, pt_anchor1);
			float dist = get_distance(blob.pt_y_max, pt_anchor0) + get_distance(blob.pt_y_max, pt_anchor1);

			if (dist < dist_min)
				blob_dist_min = &blob;
		}

		blob_index_unrotated = find_parent_blob_before_rotation(blob_dist_min);
		blob_index_rotated = blob_dist_min;
	}

	if (blob_index_rotated != NULL)
	{
		int y_max = -1;
		for (BlobNew& blob : *blob_detector_image_hand_rotated.blobs)
			if (blob.pt_y_min.x < blob_index_rotated->pt_y_min.x)
				if (blob.y_max > y_max)
					y_max = blob.y_max;

		if (y_max != -1)
		{
			BlobNew* blob_dist_min = NULL;
			float dist_min = 9999;
			Point pt_anchor = Point(0, y_max);

			for (BlobNew& blob : *blob_detector_image_hand_rotated.blobs)
				if (blob.pt_y_min.x < blob_index_rotated->pt_y_min.x)
				{
					float dist = blob.compute_min_dist(pt_anchor);
					if (dist < dist_min)
					{
						dist_min = dist;
						blob_dist_min = &blob;
					}
				}

			blob_thumb_rotated = blob_dist_min;
			blob_thumb_unrotated = find_parent_blob_before_rotation(blob_thumb_rotated);
		}
	}

	static Point pt_hand_anchor_static = pt_hand_anchor;

	if (blob_index_unrotated != NULL)
	{
		pt_index = blob_index_unrotated->pt_y_max;

		blob_index_unrotated->fill(image_hand, 127);
		blob_index_rotated->fill(image_hand_rotated, 127);
	}

	if (blob_thumb_unrotated != NULL && blob_thumb_unrotated->atlas_id != blob_index_unrotated->atlas_id)
	{
		pt_thumb = blob_thumb_unrotated->pt_y_max;

		blob_thumb_unrotated->fill(image_hand, 127);
		blob_thumb_rotated->fill(image_hand_rotated, 127);

		float thumb_ratio = get_distance(pt_thumb, pt_hand_anchor) / palm_radius;
		low_pass_filter.compute_if_larger(thumb_ratio, 0.5, "thumb_ratio");

		if (thumb_ratio < 2)
			pt_thumb = Point(-1, -1);
	}

	if (pose_name == "point" && name == "0" && blob_index_rotated != NULL)
	{
		vector<BlobNew> blob_vec = *blob_detector_image_hand_rotated.blobs;
		sort(blob_vec.begin(), blob_vec.end(), compare_blob_y_max());

		for (int i = 0; i < blob_vec.size(); ++i)
			if ((blob_index_rotated != NULL && blob_vec[i].atlas_id == blob_index_rotated->atlas_id) ||
				(blob_thumb_rotated != NULL && blob_vec[i].atlas_id == blob_thumb_rotated->atlas_id))
			{
				blob_vec.erase(blob_vec.begin() + i);
				--i;
			}

		if (blob_vec.size() > 0)
		{
			int y_reference = blob_vec[0].y_max;
			float y_diff = abs(blob_index_rotated->y_max - y_reference);
			low_pass_filter.compute_if_smaller(y_diff, 0.5, "y_diff");

			if (y_diff <= 5 && !record_pose)
			{
				pose_name = "";
				pinch_to_zoom = false;
			}
		}
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	BlobNew* blob_aux = NULL;

	{
		int index = 0;
		for (BlobNew& blob : *blob_detector_image_hand_rotated.blobs)
		{
			sort(blob.data.begin(), blob.data.end(), compare_point_y());
			Point pt_blob = blob.data[blob.data.size() * 0.7];

			if (blob.x_max < (pt_hand_anchor_rotated.x - palm_radius))
			{
				float angle_blob = get_angle(pt_hand_anchor_rotated, pt_blob, Point(pt_hand_anchor_rotated.x, 0));
				if (angle_blob < 150)
					if (blob_aux == NULL || blob.x_min < blob_aux->x_min)
						blob_aux = &blob;

				if (pt_blob.y < pt_hand_anchor_rotated.y)
					if (blob_aux == NULL || blob.x_min < blob_aux->x_min)
						blob_aux = &blob;
			}

			if (index == 0 && blob.y_min < pt_hand_anchor_rotated.y)
				if (blob_aux == NULL || blob.x_min < blob_aux->x_min)
					blob_aux = &blob;

			++index;
		}
	}

	if (blob_aux != NULL)
	{
		if (value_store.get_int("aux_counter") >= 1)
			value_store.set_bool("has_aux", true);

		value_store.set_int("aux_counter", value_store.get_int("aux_counter") + 1);
	}
	else
	{
		value_store.set_int("aux_counter", 0);
		value_store.set_bool("has_aux", false);
	}

	static float angle_static = 0;

	//--------------------------------------------------------------------------------------------------------------------------------

	if (name == "0")
	{
		vector<Point> convex_points;
		for (BlobNew& blob : *blob_detector_image_hand.blobs)
			convex_points.push_back(blob.pt_y_max);

		Point pt_x_min = Point(9999, 0);
		for (Point& pt : convex_points)
			if (pt.x < pt_x_min.x)
				pt_x_min = pt;

		float angle_raw = get_angle(pt_hand_anchor, pt_x_min, Point(0, pt_hand_anchor.y)) - 90;
		if (value_store.get_bool("has_aux") || pose_name == "point")
			angle_raw += 30;

		low_pass_filter.compute(angle_raw, 0.5, "angle_raw");

		vector<Point> convex_points_rotated;
		int x_max = 0;
		int y_max = 0;
		int x_min = 9999;
		int y_min = 9999;
		for (Point& pt : convex_points)
		{
			Point pt_rotated = rotate(pt, angle_raw);
			convex_points_rotated.push_back(pt_rotated);

			if (pt_rotated.x > x_max)
				x_max = pt_rotated.x;
			if (pt_rotated.y > y_max)
				y_max = pt_rotated.y;
			if (pt_rotated.x < x_min)
				x_min = pt_rotated.x;
			if (pt_rotated.y < y_min)
				y_min = pt_rotated.y;
		}

		Point pt_anchor = Point(x_max, HEIGHT_SMALL);

		static Point pt_reference_old = Point(WIDTH_SMALL, HEIGHT_SMALL);
		static float angle_reference_old = 0;
		pt_hand_anchor_rotated = rotate(pt_hand_anchor, angle_reference_old);

		float dist_min = 9999;
		Point pt_reference;
		for (Point& pt : convex_points_rotated)
		{
			Point pt_unrotated = unrotate(pt, angle_raw);
			Point pt_rotated = rotate(pt_unrotated, angle_reference_old);
			float angle = get_angle(pt_hand_anchor, pt_unrotated, Point(0, pt_hand_anchor.y)) - 90;

			float dist = (get_distance(pt, pt_anchor) * 1) +
					 	 (blob_index_unrotated == NULL ? 0 : (get_distance(pt_unrotated, blob_index_unrotated->pt_y_max) * 0.25)) +
						 (abs(angle_reference_old - angle) * 0.1) +
						 ((0 - (pt_rotated.y - pt_hand_anchor_rotated.y)) * 0.25);

			if (dist < dist_min)
			{
				dist_min = dist;
				pt_reference = pt;
			}
		}
		Point pt_reference_unrotated = unrotate(pt_reference, angle_raw);

		angle_reference_old = get_angle(pt_hand_anchor, pt_reference_unrotated, Point(0, pt_hand_anchor.y)) - 90;
		pt_reference_old = pt_reference;

		float angle_final_old = angle_final;
		float angle_final_new = angle_reference_old;
		angle_final_diff = abs(angle_final_new - angle_final_old);

		angle_final = angle_reference_old;
		if (angle_final > 0)
			angle_final = 0;

		angle_static = angle_final;
	}
	else
		angle_final = angle_static;

	//--------------------------------------------------------------------------------------------------------------------------------

	if (visualize && enable_imshow)
	{
		circle(image_hand_rotated, pt_hand_anchor_rotated, palm_radius, Scalar(127), 2);
		circle(image_hand, pt_hand_anchor, palm_radius, Scalar(64), 2);
		circle(image_hand, pt_palm, 10, Scalar(127), 1);

		imshow("image_hand" + name, image_hand);
		imshow("image_hand_rotated" + name, image_hand_rotated);
	}

	value_store.set_bool("reset", false);
	return true;
}

void MonoProcessorNew::sort_contour(vector<Point>& points, vector<Point>& points_sorted, Point& pivot)
{
	float dist_min = 9999;
	int index_dist_min;

	int index = 0;
	for (Point& pt : points)
	{
		float dist = get_distance(pt, pivot);
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

void MonoProcessorNew::compute_extension_line(Point pt_start, Point pt_end, const uchar length,
											  vector<Point>& line_points, const bool reverse)
{
	if (pt_start.y > pt_end.y)
	{
		Point pt_intersection;
		if (get_intersection_at_y(pt_end, pt_start, HEIGHT_SMALL, pt_intersection))
		{
			if (!reverse)
				bresenham_line(pt_start.x, pt_start.y, pt_intersection.x, pt_intersection.y, line_points, length);
			else
				bresenham_line(pt_end.x, pt_end.y, pt_intersection.x, pt_intersection.y, line_points, length);
		}
	}
	else if (pt_start.y < pt_end.y)
	{
		Point pt_intersection;
		if (get_intersection_at_y(pt_end, pt_start, 0, pt_intersection))
		{
			if (!reverse)
				bresenham_line(pt_start.x, pt_start.y, pt_intersection.x, pt_intersection.y, line_points, length);
			else
				bresenham_line(pt_end.x, pt_end.y, pt_intersection.x, pt_intersection.y, line_points, length);
		}
	}
	else if (pt_start.y == pt_end.y)
	{
		Point pt_intersection = Point(0, pt_start.y);
		if (pt_start.x < pt_end.x)
			pt_intersection.x = pt_start.x - length;
		else
			pt_intersection.x = pt_start.x + length;

		if (!reverse)
			bresenham_line(pt_start.x, pt_start.y, pt_intersection.x, pt_intersection.y, line_points, length);
		else
			bresenham_line(pt_end.x, pt_end.y, pt_intersection.x, pt_intersection.y, line_points, length);
	}
}

BlobNew* MonoProcessorNew::find_parent_blob_before_rotation(BlobNew* blob)
{
	Point pt_reference = blob->pt_y_max;
	Point pt_reference_unrotated = unrotate(pt_reference);

	BlobNew* pt_reference_unrotated_parent_blob = NULL;
	float min_dist = 9999;

	for (BlobNew& blob_current : *blob_detector_image_hand.blobs)
	{
		float current_dist = blob_current.compute_min_dist(pt_reference_unrotated);
		if (current_dist < min_dist)
		{
			min_dist = current_dist;
			pt_reference_unrotated_parent_blob = &blob_current;
		}
	}
	return pt_reference_unrotated_parent_blob;
}

Point MonoProcessorNew::rotate(Point pt, float angle, Point anchor_in)
{
	if (angle == 9999)
		angle = angle_final;

	if (anchor_in.x == 9999)
		anchor_in = pt_hand_anchor;

	pt = rotate_point(angle, pt, pt_hand_anchor);
	pt.x -= dest_diff_x;
	pt.y -= dest_diff_y;
	return pt;
}

Point MonoProcessorNew::unrotate(Point pt, float angle, Point anchor_in)
{
	if (angle == 9999)
		angle = angle_final;

	if (anchor_in.x == 9999)
		anchor_in = pt_hand_anchor;

	pt.x += dest_diff_x;
	pt.y += dest_diff_y;
	pt = rotate_point(-angle, pt, pt_hand_anchor);
	return pt;
}

Point MonoProcessorNew::find_pt_extremum(BlobNew* blob, Point pt_anchor)
{
	float dist_max = 0;
	Point pt_dist_max;
	for (Point& pt : blob->data)
	{
		float dist = get_distance(pt, pt_anchor);
		if (dist > dist_max)
		{
			dist_max = dist;
			pt_dist_max = pt;
		}
	}
	return pt_dist_max;
}
