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
vector<vector<Point>> vertex_points_collection;
vector<string> names_collection;

vector<Point> PoseEstimator::points_dist_min;
vector<Point> PoseEstimator::labels_dist_min;
vector<Point> PoseEstimator::vertex_points_dist_min;

string PoseEstimator::pose_name = "";
string PoseEstimator::target_pose_name = "";

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
			vector<Point> vertex_points;
			for (String& str : label_data)
			{
				vector<string> str_pair = split_string(str, "!");
				Point label_indexes_parsed = Point(atoi(str_pair[0].c_str()), atoi(str_pair[1].c_str()));
				labels.push_back(label_indexes_parsed);

				Point pt_y_max = Point(-1, -1);
				for (int i = label_indexes_parsed.x; i <= label_indexes_parsed.y; ++i)
				{
					Point pt = points[i];
					if (pt.y > pt_y_max.y)
						pt_y_max = pt;
				}
				if (pt_y_max.y == -1)
					vertex_points.push_back(Point(9999, 9999));
				else
					vertex_points.push_back(pt_y_max);
			}
			labels_collection.push_back(labels);
			vertex_points_collection.push_back(vertex_points);
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

void PoseEstimator::compute(vector<Point>& points_in, vector<Point>& vertex_points_in, string name)
{
	points_current = points_in;

	string pose_name_dist_min = "";
	float dist_min = 9999;

	int index = -1;
	for (vector<Point>& points : points_collection)
	{
		++index;

		Mat cost_mat = compute_cost_mat(points_current, points, false);
		float dist = compute_dtw(cost_mat);

		int skipped_count = 0;
		vector<float> dist_vertex_vec;
		if (vertex_points_in.size() > 0)
		{
			int index_vertex = -1;
			for (Point& pt_vertex_in : vertex_points_in)
			{
				++index_vertex;
				Point pt_vertex_matching = vertex_points_collection[index][index_vertex];

				if (pt_vertex_matching.x == 9999 || pt_vertex_in.x == 9999)
				{
					++skipped_count;
					continue;
				}

				dist_vertex_vec.push_back(get_distance(pt_vertex_in, pt_vertex_matching, false));
			}
		}
		if (dist_vertex_vec.size() > 0)
		{
			sort(dist_vertex_vec.begin(), dist_vertex_vec.end());
			float dist_vertex = dist_vertex_vec[dist_vertex_vec.size() - 1];
			float dist_vertex_median = dist_vertex_vec[dist_vertex_vec.size() / 2];

			for (int i = 0; i < skipped_count; ++i)
				dist_vertex += dist_vertex_median;

			dist += dist_vertex;
		}

		if (dist < dist_min)
		{
			dist_min = dist;
			PoseEstimator::points_dist_min = points;
			pose_name_dist_min = names_collection[index];

			if (labels_collection.size() > index)
			{
				PoseEstimator::labels_dist_min = labels_collection[index];
				PoseEstimator::vertex_points_dist_min = vertex_points_collection[index];
			}
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

	if (dist_min != 9999 && show)
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
		{
			int index = -1;
			for (Point& pt : vertex_points_dist_min)
			{
				++index;
				if (pt.x == 9999)
					continue;

				// circle(image_dist_min, pt, 3, colors[index], 1);

				if (vertex_points_in.size() == 0)
					continue;

				Point pt_matching = vertex_points_in[index];
				if (pt_matching.x == 9999)
					continue;

				circle(image_dist_min, pt_matching, 3, colors[index], 1);
				line(image_dist_min, pt, pt_matching, colors[index], 1);
			}
		}

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