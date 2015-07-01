#pragma once

#include <opencv2/opencv.hpp>
#include "globals.h"
#include "math_plus.h"

using namespace std;
using namespace cv;

class BlobNew
{
public:
	Mat image_atlas;

	ushort atlas_id;

	vector<Point> data;
	vector<Point> tips;

	vector<int> index_vec;

	vector<vector<Point>> extension_vecs;

	vector<Point> convex_points;

	vector<string> names;

	int x_min = 9999;
	int x_max = 0;
	int y_min = 9999;
	int y_max = 0;
	int width;
	int height;
	int area;
	int count = 0;
	int x;
	int y;
	int index;
	int id = -1;

	int* id_ptr = NULL;

	bool active = true;
	bool merge = false;
	bool selected = false;

	Point pt_y_min = Point(0, 9999);
	Point pt_y_max = Point(0, 0);
	Point pt_x_min = Point(9999, 0);
	Point pt_x_max = Point(0, 0);

	BlobNew* matching_blob = NULL;

	string name = "";

	BlobNew();
	BlobNew(Mat& image_atlas_in, const ushort atlas_id_in);

	void add(const int i, const int j);
	void compute();
	int compute_overlap(BlobNew& blob_in);
	int compute_overlap(BlobNew& blob_in, const int x_diff_in, const int y_diff_in, const int dilate_num);
	float compute_min_dist(Point pt_in);
	Point compute_median_point();
	void fill(Mat& image_in, const uchar gray_in);
};