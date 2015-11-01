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

void PoseEstimator::init()
{
	if (!directory_exists(pose_database_path))
		create_directory(pose_database_path);

	load();

	console_log("pose estimator initialized");
}

void PoseEstimator::compute(vector<Point>& points_in)
{
	points_current = points_in;

	string pose_name_dist_min = "";
	vector<Point> points_dist_min;
	float dist_min = FLT_MAX;

	int index = 0;
	for (vector<Point>& points : points_collection)
	{
		// float dist = matchShapes(points_current, points, CV_CONTOURS_MATCH_I1, 0);
		Mat cost_mat = compute_cost_mat(points_current, points, false);
		float dist = compute_dtw(cost_mat);

		if (dist < dist_min)
		{
			dist_min = dist;
			points_dist_min = points;
			pose_name_dist_min = names_collection[index];
		}

		++index;
	}

	if (record_pose && target_pose_name != "" && (pose_name_dist_min != target_pose_name/* || dist_min > 1000*/))
	{
		save(target_pose_name);
		console_log(pose_name_dist_min + "->" + target_pose_name + " " + to_string(dist_min));
	}

	if (dist_min != FLT_MAX)
	{
		Mat image_dist_min = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

		Point pt_old = Point(-1, -1);
		for (Point& pt : points_dist_min)
		{
			if (pt_old.x != -1)
				line(image_dist_min, pt, pt_old, Scalar(254), 1);

			pt_old = pt;
		}

		imshow("image_dist_min", image_dist_min);
	}

	string pose_name_temp;
	accumulate_pose(pose_name_dist_min, 5, pose_name_temp);

	if (pose_name_temp != "")
		pose_name = pose_name_temp;

	if (show)
		console_log(pose_name_temp);

	Mat image_current = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	Point pt_old = Point(-1, -1);
	for (Point& pt : points_current)
	{
		if (pt_old.x != -1)
			line(image_current, pt, pt_old, Scalar(254), 1);

		pt_old = pt;
	}

	imshow("image_current", image_current);
	waitKey(1);
}

bool PoseEstimator::accumulate_pose(const string name_in, const int count_max, string& name_out)
{
	if (count_max > 2)
	{
		static int count = 0;
		static map<const string, int> val_map;

		if (count < count_max)
		{
			if (!val_map.count(name_in))
				val_map[name_in] = 0;
			else
				++val_map[name_in];

			++count;
			return false;
		}
		else
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
			return true;
		}
	}
	else
	{
		name_out = name_in;
		return true;
	}
}

void PoseEstimator::save(const string name)
{
	const string extension = "nrocinunerrad";

	vector<string> file_name_vec = list_files_in_directory(pose_database_path);
	int name_count = 0;

	for (string& name_current : file_name_vec)
	{
		if (split_string(name_current, ".")[1] != extension)
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
			++name_count;
	}

	string data = "";
	for (Point& pt : points_current)
		data += to_string(pt.x) + "!" + to_string(pt.y) + "\n";

	data.pop_back();

	const string path = pose_database_path + slash + name + to_string(name_count) + ".";
	write_string_to_file(path + extension, data);

	points_collection.push_back(points_current);
	names_collection.push_back(name);

	console_log("pose data saved: " + name);
}

void PoseEstimator::load()
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
		}
		else if (name_extension_vec.size() > 1 && name_extension_vec[1] == "png")
		{
			const string path = pose_database_path + slash + name_current;
			Mat image_loaded = imread(path);
		}
	}
}