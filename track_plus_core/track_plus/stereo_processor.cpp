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

#include "stereo_processor.h"

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

struct BlobPair
{
	BlobNew* blob0;
	BlobNew* blob1;
	float angle_diff;

	BlobPair(BlobNew* _blob0, BlobNew* _blob1, float _angle_diff)
	{
		blob0 = _blob0;
		blob1 = _blob1;
		angle_diff = _angle_diff;
	};
};

struct compare_blob_pair_angle_diff
{
	bool operator() (const BlobPair& blob_pair0, const BlobPair& blob_pair1)
	{
		return (blob_pair0.angle_diff < blob_pair1.angle_diff);
	}
};

bool StereoProcessor::compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1,
							  Reprojector& reprojector,          PointerMapper& pointer_mapper)
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
		}

		++index;
	}

	if (angle_vec.size() == 0)
		return false;

	const float angle_median = angle_vec[angle_vec.size() / 2];

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

	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

	vector<BlobPair> blob_pair_vec;

	index = 0;
	for (BlobNew& blob : *blob_vec0)
	{
		int score_board[10] { 0 };
		for (Point4f& pt0 : points0)
			if (pt0.w == index)
			{
				Point4f pt1 = points1[pt0.z];
				++score_board[(int)pt1.w];
			}
		int max_score = -1;
		int max_score_index;
		for (int a = 0; a < 10; ++a)
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

			if (angle_diff < 20)
				blob_pair_vec.push_back(BlobPair(blob0, blob1, angle_diff));
		}

		++index;
	}

	sort(blob_pair_vec.begin(), blob_pair_vec.end(), compare_blob_pair_angle_diff());

	unordered_map<string, uchar> checker0;
	unordered_map<string, uchar> checker1;

	vector<BlobPair> blob_pair_vec_filtered;
	for (BlobPair& pair : blob_pair_vec)
	{
		string key0 = to_string(pair.blob0->pt_y_max.x) + "," + to_string(pair.blob0->pt_y_max.y);
		string key1 = to_string(pair.blob1->pt_y_max.x) + "," + to_string(pair.blob1->pt_y_max.y);

		if (checker0.count(key0) == 0 && checker1.count(key1) == 0)
			blob_pair_vec_filtered.push_back(pair);

		checker0[key0] = 1;
		checker1[key1] = 1;
	}

	for (BlobPair& pair : blob_pair_vec_filtered)
	{
		BlobNew* blob0 = pair.blob0;
		BlobNew* blob1 = pair.blob1;

		blob0->fill(image_visualization, 254);
		for (Point& pt : blob1->data)
			image_visualization.ptr<uchar>(pt.y, pt.x + 100)[0] = 254;

		line(image_visualization, blob0->pt_y_max, Point(blob1->pt_y_max.x + 100, blob1->pt_y_max.y), Scalar(127), 1);
	}

	imshow("image_visualizationlkdsfhkjdhf", image_visualization);

	//------------------------------------------------------------------------------------------------------------------------------

	return false;
}