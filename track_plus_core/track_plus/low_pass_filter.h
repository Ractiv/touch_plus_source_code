#pragma once

#include <unordered_map>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class LowPassFilter
{
public:
	unordered_map<string, float> value_map;

	void compute(float& value, const float alpha, const string name);
	void compute_if_smaller(float& value, const float alpha, const string name);
	void compute_if_smaller(uchar& value, const float alpha, const string name);
	void compute_if_larger(float& value, const float alpha, const string name);
	void compute_if_larger(uchar& value, const float alpha, const string name);
	void compute(int& value, const float alpha, const string name);
	void compute(uchar& value, const float alpha, const string name);
	void compute(Point& value, const float alpha, const string name);
	void compute(Point2f& value, const float alpha, const string name);
	void compute(Point3f& value, const float alpha, const string name);
	void reset();
};