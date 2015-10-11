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

#include "stereo_processor_dtw.h"
#include "dtw.h"

struct Point4f
{
	float x;
	float y;
	float z;
	float w;

	Point4f(float _x, float _y, float _z, float _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	};
};

struct BlobPairAngleDiff
{
	BlobNew* blob0;
	BlobNew* blob1;
	float angle_diff;

	BlobPairAngleDiff(BlobNew* _blob0, BlobNew* _blob1, float _angle_diff)
	{
		blob0 = _blob0;
		blob1 = _blob1;
		angle_diff = _angle_diff;
	};
};

struct compare_blob_pair_angle_diff
{
	bool operator() (const BlobPairAngleDiff& blob_pair0, const BlobPairAngleDiff& blob_pair1)
	{
		return (blob_pair0.angle_diff < blob_pair1.angle_diff);
	}
};

bool StereoProcessorDTW::compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1, PointResolver& point_resolver,
							     PointerMapper& pointer_mapper,     Mat image0,                       Mat image1)
{
	vector<Point>* vec0 = &mono_processor0.stereo_matching_points;
	vector<Point>* vec1 = &mono_processor1.stereo_matching_points;

	vector<BlobNew>* blob_vec0 = &mono_processor0.fingertip_blobs;
	vector<BlobNew>* blob_vec1 = &mono_processor1.fingertip_blobs;

	Mat cost_mat = compute_cost_mat(*vec0, *vec1, true);
	vector<Point> indexes = compute_dtw_indexes(cost_mat);

	vector<Point4f> points0; //x and y are point positions, z is DTW matched index in points1, w is index of touching blob
	vector<Point4f> points1;

	vector<float> angle_vec;
	vector<int> y_diff_vec;
	vector<int> x_diff_vec;

	int index = 0;
	for (Point& index_pair : indexes)
	{
		Point pt0 = (*vec0)[index_pair.x];
		Point pt1 = (*vec1)[index_pair.y];

		points0.push_back(Point4f(pt0.x, pt0.y, index, -1));
		points1.push_back(Point4f(pt1.x, pt1.y, index, -1));

		if (angle_vec.size() < 1000)
		{
			float angle = get_angle(pt0, pt1, Point(pt0.x, 0));
			if (pt0.x < pt1.x)
				angle = 360 - angle;

			angle_vec.push_back(angle);
			y_diff_vec.push_back(pt0.y - pt1.y);
			x_diff_vec.push_back(pt0.x - pt1.x);
		}

		++index;
	}

	if (angle_vec.size() == 0)
		return false;

	const float angle_median = angle_vec[angle_vec.size() / 2];
	const int y_diff_median = y_diff_vec[y_diff_vec.size() / 2];
	const int x_diff_median = x_diff_vec[x_diff_vec.size() / 2];

	//------------------------------------------------------------------------------------------------------------------------------

	index = 0;
	for (BlobNew& blob : *blob_vec0)
	{
		for (Point4f& pt : points0)
			if (blob.image_atlas.ptr<ushort>(pt.y, pt.x)[0] == blob.atlas_id)
				pt.w = index;

		++index;
	}

	index = 0;
	for (BlobNew& blob : *blob_vec1)
	{
		for (Point4f& pt : points1)
			if (blob.image_atlas.ptr<ushort>(pt.y, pt.x)[0] == blob.atlas_id)
				pt.w = index;

		++index;
	}

	//------------------------------------------------------------------------------------------------------------------------------

	vector<BlobPairAngleDiff> blob_pair_vec;

	index = 0;
	for (BlobNew& blob : *blob_vec0)
	{
		int score_board[100] { 0 };
		for (Point4f& pt0 : points0)
			if (pt0.w == index)
			{
				Point4f pt1 = points1[pt0.z];
				++score_board[(int)pt1.w];
			}
		int max_score = -1;
		int max_score_index;
		for (int a = 0; a < 100; ++a)
			if (score_board[a] > max_score)
			{
				max_score = score_board[a];
				max_score_index = a;
			}

		if (max_score != -1)
		{
			BlobNew* blob0 = &blob;
			BlobNew* blob1 = &(*blob_vec1)[max_score_index];

			float angle = get_angle(blob0->pt_y_max, blob1->pt_y_max, Point(blob0->pt_y_max.x, 0));
			if (blob0->pt_y_max.x < blob1->pt_y_max.x)
				angle = 360 - angle;

			float angle_diff = abs(angle - angle_median);
			blob_pair_vec.push_back(BlobPairAngleDiff(blob0, blob1, angle_diff));
		}

		++index;
	}

	sort(blob_pair_vec.begin(), blob_pair_vec.end(), compare_blob_pair_angle_diff());

	//------------------------------------------------------------------------------------------------------------------------------

	unordered_map<string, uchar> checker0;
	unordered_map<string, uchar> checker1;

	vector<BlobPairAngleDiff> blob_pair_vec_filtered;
	for (BlobPairAngleDiff& pair : blob_pair_vec)
	{
		string key0 = to_string(pair.blob0->pt_y_max.x) + "," + to_string(pair.blob0->pt_y_max.y);
		string key1 = to_string(pair.blob1->pt_y_max.x) + "," + to_string(pair.blob1->pt_y_max.y);

		if (checker0.count(key0) == 0 && checker1.count(key1) == 0)
			blob_pair_vec_filtered.push_back(pair);

		checker0[key0] = 1;
		checker1[key1] = 1;
	}

	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

	Point pt_resolved_pivot0 = point_resolver.reprojector->remap_point(mono_processor0.pt_palm, 0, 4);
	Point pt_resolved_pivot1 = point_resolver.reprojector->remap_point(mono_processor1.pt_palm, 1, 4);

	for (BlobPairAngleDiff& pair : blob_pair_vec_filtered)
	{
		BlobNew* blob0 = pair.blob0;
		BlobNew* blob1 = pair.blob1;

		Point2f pt_resolved0 = point_resolver.compute(blob0->pt_tip, image0, 0);
		Point2f pt_resolved1 = point_resolver.compute(blob1->pt_tip, image1, 1);

#if 0
		circle(image_visualization, pt_resolved0, 5, Scalar(127), 2);
		circle(image_visualization, pt_resolved1, 5, Scalar(254), 2);
		circle(image_visualization, pt_resolved_pivot0, 10, Scalar(127), 2);
		circle(image_visualization, pt_resolved_pivot1, 10, Scalar(254), 2);
#endif

#if 1
		if (pt_resolved0.x != 9999 && pt_resolved1.x != 9999)
		{
			Point3f pt3d = point_resolver.reprojector->reproject_to_3d(pt_resolved0.x, pt_resolved0.y, pt_resolved1.x, pt_resolved1.y);
			circle(image_visualization, Point(320 + pt3d.x, 240 + pt3d.y), pow(1000 / pt3d.z, 2), Scalar(127), 1);
		}
#endif
	}

	imshow("image_visualizationadsfasdfasdf", image_visualization);

	//------------------------------------------------------------------------------------------------------------------------------

	algo_name_vec.push_back(algo_name);
	return true;
}