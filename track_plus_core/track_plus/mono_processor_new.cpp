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

bool MonoProcessorNew::compute(HandSplitterNew& hand_splitter, const string name, bool visualize)
{
	pt_index = Point(-1, -1);
	pt_thumb = Point(-1, -1);

	LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");

	if (value_store.get_bool("reset", true))
		low_pass_filter->reset();

	//------------------------------------------------------------------------------------------------------------------------------

	if (hand_splitter.primary_hand_blobs.size() == 0)
	{
		value_store.set_bool("reset", true);
		return false;
	}

	Mat image_active_hand = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_skeleton_segmented = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_palm_segmented = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	Mat image_find_contours = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	float palm_radius = value_store.get_float("palm_radius", 1);
	Point pt_hand_anchor = value_store.get_point("pt_hand_anchor");

	Point2f pt_palm = Point2f(0, 0);
	int pt_palm_count = 0;
	const int y_threshold = pt_hand_anchor.y - palm_radius;

	for (BlobNew& blob : hand_splitter.primary_hand_blobs)
	{
		blob.fill(image_find_contours, 254);
		blob.fill(image_active_hand, 254);
		blob.fill(image_skeleton_segmented, 254);
		blob.fill(image_palm_segmented, 254);

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
							const float dist_current = get_distance(pt0, pt1);

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

	low_pass_filter->compute(palm_radius, 0.1, "palm_radius");
	value_store.set_float("palm_radius", palm_radius);
	value_store.set_point("pt_hand_anchor", pt_hand_anchor);

	//------------------------------------------------------------------------------------------------------------------------------

	Point pivot;
	static Point pt_intersection_hand_direction_stereo = Point(0, 0);

	{
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
						float dist0 = get_distance(pt0, pt_hand_anchor);
						float dist1 = get_distance(pt1, pt_hand_anchor);
						float dist2 = get_distance(pt2, pt_hand_anchor);

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
						float dist = get_distance(Point(pt_convex_indexed.x, pt_convex_indexed.y), pt_hand_anchor);
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

			Point seek_anchor = Point(hand_splitter.x_min_result, hand_splitter.y_max_result * 3);

			const int i_max = convex_points_indexed.size();
			for (int i = 0; i < i_max; ++i)
			{
				Point anchor0 = Point(anchor_points0[i].x, anchor_points0[i].y);
				Point anchor1 = Point(anchor_points1[i].x, anchor_points1[i].y);
				float dist0 = get_distance(anchor0, pt_hand_anchor);
				float dist1 = get_distance(anchor1, pt_hand_anchor);

				Point anchor;
				if (dist0 < dist1)
					anchor = anchor0;
				else
					anchor = anchor1;

				Point tip = Point(convex_points_indexed[i].x, convex_points_indexed[i].y);

				float current_dist = get_distance(tip, seek_anchor);
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
				// pt_intersection_hand_direction.x += pt_intersection_hand_direction.x - pt_hand_anchor.x;
				pt_intersection_hand_direction_stereo.x += (pt_intersection_hand_direction.x - pt_intersection_hand_direction_stereo.x) * 0.1;
				pt_intersection_hand_direction_stereo.y = HEIGHT_SMALL;
				get_intersection_at_y(pt_intersection_hand_direction_stereo, pt_hand_anchor, pt_hand_anchor.y - (palm_radius * 2), pivot);
			}

			break;
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	vector<Point> contour_sorted;
	sort_contour(contours[0], contour_sorted, pivot);

	if (contour_sorted.size() == 0)
	{
		value_store.set_bool("reset", true);
		return false;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	vector<Point> contour_approximated_unsorted;
	approxPolyDP(Mat(contours[0]), contour_approximated_unsorted, 1, false);

	vector<Point> contour_approximated;
	sort_contour(contour_approximated_unsorted, contour_approximated, pivot);

	{
		pose_estimation_points = contour_approximated;
		pose_estimation_points.push_back(pivot);

		stereo_matching_points = contour_sorted;
		stereo_matching_points.push_back(pivot);
	}

	contour_approximated.insert(contour_approximated.begin(), contour_sorted[0]);
	contour_approximated.push_back(contour_sorted[contour_sorted.size() - 1]);

	//------------------------------------------------------------------------------------------------------------------------------

	Mat image_palm = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	vector<Point3f> concave_points_indexed;
	vector<Point3f> convex_points_indexed;

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
					float dist0 = get_distance(pt0, pt_hand_anchor);
					float dist1 = get_distance(pt1, pt_hand_anchor);
					float dist2 = get_distance(pt2, pt_hand_anchor);

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
						float dist = get_distance(Point(pt_concave_indexed.x, pt_concave_indexed.y), pt_hand_anchor);
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
						float dist = get_distance(Point(pt_convex_indexed.x, pt_convex_indexed.y), pt_hand_anchor);
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

		int x_min_palm_lines = 9999;
		int x_max_palm_lines = 0;
		int x_min_palm_lines_index = 0;
		int x_max_palm_lines_index = 0;
		for (Point3f& pt : concave_points_indexed)
		{
			line(image_skeleton_segmented, Point(pt.x, pt.y), pivot, Scalar(0), 1);

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
		}

		break;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	float hand_angle = get_angle(pt_hand_anchor, pt_intersection_hand_direction_stereo, Point(0, pt_hand_anchor.y)) - 90;

	RotatedRect r_rect0 = RotatedRect(pt_hand_anchor, Size2f(500, 500), -hand_angle);
	Point2f vertices0[4];
	r_rect0.points(vertices0);

	int vertices_0_3_x = (vertices0[3].x - vertices0[2].x) / 2 + vertices0[2].x;
	int vertices_0_3_y = (vertices0[3].y - vertices0[2].y) / 2 + vertices0[2].y - (palm_radius * 1);
	int vertices_0_0_x = (vertices0[0].x - vertices0[1].x) / 2 + vertices0[1].x;
	int vertices_0_0_y = (vertices0[0].y - vertices0[1].y) / 2 + vertices0[1].y - (palm_radius * 1);

	line(image_palm, Point(vertices_0_3_x, vertices_0_3_y), Point(vertices_0_0_x, vertices_0_0_y), Scalar(254), 1);
	line(image_palm_segmented, Point(vertices_0_3_x, vertices_0_3_y), Point(vertices_0_0_x, vertices_0_0_y), Scalar(0), 1);

	RotatedRect r_rect1 = RotatedRect(pt_hand_anchor, Size2f(palm_radius * 2, 500), -hand_angle);
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

	circle(image_palm, pt_hand_anchor, palm_radius, Scalar(254), 1);
	circle(image_palm_segmented, pt_hand_anchor, palm_radius, Scalar(0), 1);

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
	}

	//------------------------------------------------------------------------------------------------------------------------------

	if (vertices_0_0_y < vertices_0_3_y)
		floodFill(image_palm, Point(0, HEIGHT_SMALL_MINUS), Scalar(127));
	else
		floodFill(image_palm, Point(WIDTH_SMALL_MINUS, HEIGHT_SMALL_MINUS), Scalar(127));

	//------------------------------------------------------------------------------------------------------------------------------

	BlobDetectorNew* blob_detector_image_palm_segmented = value_store.get_blob_detector("blob_detector_image_palm_segmented");
	blob_detector_image_palm_segmented->compute(image_palm_segmented, 254, hand_splitter.x_min_result, hand_splitter.x_max_result,
									                  					   hand_splitter.y_min_result, hand_splitter.y_max_result, true);

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

	vector<Point> convex_points_filtered;
	for (Point3f& pt : convex_points_indexed)
	{
		BlobNew* blob_min_dist = NULL;
		float min_dist = 9999;
		for (BlobNew& blob : *blob_detector_image_palm_segmented->blobs)
		{
			float dist = blob.compute_min_dist(Point(pt.x, pt.y));
			if (dist < min_dist)
			{
				min_dist = dist;
				blob_min_dist = &blob;
			}
		}
		if (blob_min_dist != NULL && blob_min_dist->active)
		{
			blob_min_dist->fill(image_palm_segmented, 127);
			convex_points_filtered.push_back(Point(pt.x, pt.y));
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------

	/*BlobDetectorNew* blob_detector_image_skeleton_segmented = value_store.get_blob_detector("blob_detector_image_skeleton_segmented");
	blob_detector_image_skeleton_segmented->compute(image_skeleton_segmented, 254,
													hand_splitter.x_min_result, hand_splitter.x_max_result,
									                hand_splitter.y_min_result, hand_splitter.y_max_result, true);

	for (Point& pt : convex_points_filtered)
	{
		BlobNew* blob_min_dist = NULL;
		float min_dist = 9999;
		for (BlobNew& blob : *blob_detector_image_skeleton_segmented->blobs)
		{
			float dist = blob.compute_min_dist(pt);
			if (dist < min_dist)
			{
				min_dist = dist;
				blob_min_dist = &blob;
			}
		}
		if (blob_min_dist != NULL)
			blob_min_dist->fill(image_skeleton_segmented, 127);
	}*/

	//------------------------------------------------------------------------------------------------------------------------------

	imshow("image_palm_segmented" + name, image_palm_segmented);
	// imshow("image_skeleton_segmented" + name, image_skeleton_segmented);

	value_store.set_bool("reset", false);
	return false;
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