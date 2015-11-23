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

#include "pose_estimator.h"
#include "console_log.h"

vector<Point> points_current;
vector<vector<Point>> points_collection;
vector<vector<Point>> labels_collection;
vector<string> names_collection;

vector<Point> PoseEstimator::points_dist_min;
vector<Point> PoseEstimator::labels_dist_min;

string PoseEstimator::pose_name = "";
string PoseEstimator::target_pose_name = "";

struct decreasing
{
	bool operator() (float& f0, float& f1)
	{
		return f0 > f1;
	}
};

bool accumulate_pose(const string name_in, const int count_max, string& name_out)
{
	bool result = false;

	static int count = 0;
	static map<const string, int> val_map;

	if (count == count_max)
	{
		int val_max = 0;
		string name_val_max;
		for (pair<const string, int>& pair : val_map)
		{
			int val = pair.second;
			if (val > val_max)
			{
				val_max = val;
				name_val_max = pair.first;
			}
		}
		name_out = name_val_max;
		count = 0;
		val_map.clear();
		result = true;
	}

	if (!val_map.count(name_in))
		val_map[name_in] = 1;
	else
		++val_map[name_in];

	++count;

	return result;
}

void save(const string name)
{
	const string extension = "nrocinunerrad";

	vector<string> file_name_vec = list_files_in_directory(pose_database_path);
	int name_count = 0;

	for (string& name_current : file_name_vec)
	{
		vector<string> name_extension_vec = split_string(name_current, ".");
		if (name_extension_vec.size() <= 1)
			continue;

		if (name_extension_vec[1] != extension)
			continue;

		string name_without_num = "";
		for (char& c : name_current)
		{
			string char_str = "";
			char_str += c;

			if (char_str != "0" && atoi(char_str.c_str()) == 0)
				name_without_num += c;
			else
				break;
		}

		if (name_without_num == name)
		{
			string num_without_name = "";
			for (char& c : name_current)
			{
				string char_str = "";
				char_str += c;

				if (char_str == "0" || atoi(char_str.c_str()) != 0)
					num_without_name += c;
				else
					continue;
			}

			int num = atoi(num_without_name.c_str());
			if (num > name_count)
				name_count = num;
		}
	}
	name_count += 1;

	string data = "";
	for (Point& pt : points_current)
		data += to_string(pt.x) + "!" + to_string(pt.y) + "\n";

	data.pop_back();

	const string path = pose_database_path + slash + name + to_string(name_count) + "." + extension;
	write_string_to_file(path, data);

	points_collection.push_back(points_current);
	names_collection.push_back(name);

	cout << "pose data saved: " + name << endl;
}

void load()
{
	vector<string> file_name_vec = list_files_in_directory(pose_database_path);

	for (string& name_current : file_name_vec)
	{
		string name_without_num = "";

		for(char& c : name_current)
		{
			string char_str = "";
			char_str += c;

			if (char_str != "0" && atoi(char_str.c_str()) == 0)
				name_without_num += c;
			else
				break;
		}

		vector<string> name_extension_vec = split_string(name_current, ".");

		if (name_extension_vec.size() > 1 && name_extension_vec[1] == "nrocinunerrad")
		{
			const string path = pose_database_path + slash + name_current;
			vector<string> data = read_text_file(path);

			vector<Point> points;
			for (string& str : data)
			{
				vector<string> str_pair = split_string(str, "!");
				int x = atoi(str_pair[0].c_str());
				int y = atoi(str_pair[1].c_str());
				points.push_back(Point(x, y));
			}

			string pose_name_loaded = name_extension_vec[0];

			int num_count = 0;
			for (char& c : pose_name_loaded)
			{
				string char_str = "";
				char_str += c;
				if (atoi(char_str.c_str()) != 0 || char_str == "0")
					++num_count;
			}

			for (int i = 0; i < num_count; ++i)
				pose_name_loaded.pop_back();

			points_collection.push_back(points);
			names_collection.push_back(pose_name_loaded);

			const string label_path = pose_database_path + slash + "labels" + slash + name_current;
			vector<string> label_data = read_text_file(label_path);

			vector<Point> labels;
			for (String& str : label_data)
			{
				vector<string> str_pair = split_string(str, "!");
				labels.push_back(Point(atoi(str_pair[0].c_str()), atoi(str_pair[1].c_str())));
			}
			labels_collection.push_back(labels);
		}
		else if (name_extension_vec.size() > 1 && name_extension_vec[1] == "png")
		{
			const string path = pose_database_path + slash + name_current;
			Mat image_loaded = imread(path);
		}
	}
}

void PoseEstimator::init()
{
	if (!directory_exists(pose_database_path))
		create_directory(pose_database_path);

	load();

	console_log("pose estimator initialized");
}

float compute_dtw_dist(vector<Point>& new_points, vector<Point>& database_points)
{
	Mat cost_mat = compute_cost_mat(new_points, database_points, false);
	return compute_dtw(cost_mat);
}

void compute_optimized()
{
	const int divisions = 5;
	const int generations = 10;
	const int initial_seed_num = 10;
	const int min_seed_num = 1;
	const int max_seed_num = 10;

	const int items_in_each_division = points_collection.size() / divisions;
	const int points_collection_size = points_collection.size();

	int total_iterations = 0;

	bool checker[1000] { 0 };
	unordered_map<int, float> division_dist_min_checker;
	unordered_map<float, int> dist_min_division_checker;
	unordered_map<float, bool> dist_min_checker;
	unordered_map<float, int> dist_min_index_checker;

	float dist_min_selected = 9999;
	vector<Point> database_points_selected;

	for (int i = 0; i < generations; ++i)
	{
		for (int a = 0; a < (points_collection_size - 1); a += items_in_each_division)
		{
			int index_min = a;
			int index_max = a + items_in_each_division;

			vector<int> seed_indexes;
			for (int b = index_min; b <= index_max; ++b)
				seed_indexes.push_back(b);

			if (seed_indexes.size() == 0)
				continue;

			int seed_num = initial_seed_num;
			if (i > 0)
			{
				float dist_min = division_dist_min_checker[a];
				int index_division = dist_min_index_checker[dist_min];
				seed_num = map_val(index_division, 0, divisions - 1, min_seed_num, max_seed_num);
			}

			for (int b = 0; b < seed_num; ++b)
			{
				int k = get_random(0, seed_indexes.size() - 1);
				int seed_index = seed_indexes[k];

				if (checker[seed_index] == true)
					continue;

				checker[seed_index] = true;

				vector<Point> database_points = points_collection[seed_index];
				float dist = compute_dtw_dist(points_current, database_points);

				if (dist < dist_min_selected)
				{
					dist_min_selected = dist;
					database_points_selected = database_points;
				}

				++total_iterations;

				float dist_min = division_dist_min_checker.count(a) > 0 ? division_dist_min_checker[a] : 9999;
				if (dist >= dist_min)
					continue;

				dist_min = dist;
				while (dist_min_checker.count(dist_min) > 0)
					dist_min += 0.001;

				dist_min_checker[dist_min] = true;
				division_dist_min_checker[a] = dist_min;
				dist_min_division_checker[dist_min] = a;
			}
		}

		vector<float> dist_min_vec;
		for (int a = 0; a < (points_collection_size - 1); a += items_in_each_division)
			dist_min_vec.push_back(division_dist_min_checker[a]);

		std::sort(dist_min_vec.begin(), dist_min_vec.end(), decreasing());

		int index = -1;
		for (float& dist_min : dist_min_vec)
		{
			++index;
			dist_min_index_checker[dist_min] = index;
		}
	}

	Mat image_haha = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	draw_contour(database_points_selected, image_haha, 254, 1, 0);
	imshow("image_haha", image_haha);

	waitKey(1);

	cout << total_iterations << endl;
}

void PoseEstimator::compute(vector<Point>& points_in)
{
	points_current = points_in;
	// compute_optimized();

	string pose_name_dist_min = "";
	float dist_min = 9999;

	int index = -1;
	for (vector<Point>& points : points_collection)
	{
		++index;

		Mat cost_mat = compute_cost_mat(points_current, points, false);
		float dist = compute_dtw(cost_mat);

		if (dist < dist_min)
		{
			dist_min = dist;
			PoseEstimator::points_dist_min = points;
			pose_name_dist_min = names_collection[index];

			if (labels_collection.size() > index)
				PoseEstimator::labels_dist_min = labels_collection[index];
		}
	}

	bool boolean0 = record_pose;
	bool boolean1 = target_pose_name != "";
	bool boolean2 = points_current.size() > 500;

	if ((boolean0 && boolean1 && boolean2) || force_record_pose)
	{
		force_record_pose = false;
		save(target_pose_name);
		cout << pose_name_dist_min << "->" << target_pose_name << " " << to_string(dist_min) << endl;
	}

	if (dist_min != 9999)
	{
		Mat image_dist_min = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC3);

		static vector<Scalar> colors;
		if (colors.size() == 0)
		{
			colors.push_back(Scalar(255, 0, 0));
			colors.push_back(Scalar(0, 153, 0));
			colors.push_back(Scalar(0, 0, 255));
			colors.push_back(Scalar(153, 0, 102));
			colors.push_back(Scalar(102, 102, 102));
		}

		int label_indexes[1000];
		int label_indexes_count = 0;

		{
			int label_index = -1;
			for (Point& pt : PoseEstimator::labels_dist_min)
			{
				++label_index;
				for (int i = pt.x; i <= pt.y; ++i)
				{
					label_indexes[i] = label_index;
					++label_indexes_count;
				}
			}
		}
		{
			int index = -1;
			Point pt_old = Point(-1, -1);
			for (Point& pt : PoseEstimator::points_dist_min)
			{
				++index;
				if (index >= label_indexes_count)
					continue;

				int label_index = label_indexes[index];

				if (pt_old.x != -1)
					line(image_dist_min, pt, pt_old, colors[label_index], 1);

				pt_old = pt;
			}
		}

		if (show)
			imshow("image_dist_min", image_dist_min);
	}

	string pose_name_temp;
	accumulate_pose(pose_name_dist_min, 10, pose_name_temp);

	if (pose_name_temp != "")
		pose_name = pose_name_temp;

	if (show)
		cout << pose_name_temp << endl;

	Mat image_current = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	Point pt_old = Point(-1, -1);
	for (Point& pt : points_current)
	{
		if (pt_old.x != -1)
			line(image_current, pt, pt_old, Scalar(254), 1);

		pt_old = pt;
	}

	if (show)
	{
		imshow("image_current", image_current);
		waitKey(1);
	}
}