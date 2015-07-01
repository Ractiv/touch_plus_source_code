#pragma once

#include "string_functions.h"
#include "contour_functions.h"
#include "filesystem.h"
#include "math_plus.h"
#include "mat_functions.h"
#include "opencv2\opencv.hpp"
#include <unordered_map>

using namespace cv;
using namespace std;

class PoseEstimator
{
public:
	bool show = false;

	vector<Point> points_current;
	vector<vector<Point>> points_collection;
	vector<string> names_collection;

	void init();
	void compute(vector<Point>& points_in);
	Mat compute_cost_mat(vector<Point>& vec0, vector<Point>& vec1);
	float compute_dtw(Mat& cost_mat);
	bool accumulate_pose(const string name_in, const int count_max, string& name_out);
	void save(const string name);
	void load();
};