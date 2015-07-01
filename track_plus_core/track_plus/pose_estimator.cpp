#include "pose_estimator.h"

void PoseEstimator::init()
{
	if (!directory_exists(pose_database_path))
		create_directory(pose_database_path);

	load();

	COUT << "pose estimator initialized" << endl;
}

void PoseEstimator::compute(vector<Point>& points_in)
{
	points_current = points_in;

	string pose_name_dist_min = "";
	float dist_min = FLT_MAX;

	int index = 0;
	for (vector<Point>& points : points_collection)
	{
		Mat cost_mat = compute_cost_mat(points_current, points);
		float dist = compute_dtw(cost_mat);

		if (dist < dist_min)
		{
			dist_min = dist;
			pose_name_dist_min = names_collection[index];
		}
		++index;
	}

	if (record_pose && ((target_pose_name != "" && pose_name_dist_min != target_pose_name)/* || dist_min > 200*/))
	{
		save(target_pose_name);
		COUT << pose_name_dist_min << "->" << target_pose_name << " " << dist_min << endl;
	}

	string pose_name_temp;
	accumulate_pose(pose_name_dist_min, 5, pose_name_temp);

	if (pose_name_temp == "point")
		pose_name = pose_name_temp;

	if (show)
		COUT << pose_name_temp << endl;
}

Mat PoseEstimator::compute_cost_mat(vector<Point>& vec0, vector<Point>& vec1)
{
	Mat cost_mat = Mat(vec1.size(), vec0.size(), CV_32FC1);

	const int vec0_size = vec0.size();
	const int vec1_size = vec1.size();
	for (int i = 0; i < vec0_size; ++i)
		for (int j = 0; j < vec1_size; ++j)
			cost_mat.ptr<float>(j, i)[0] = get_distance(vec0[i], vec1[j]);

	normalize(cost_mat, cost_mat, 0, 10000, NORM_MINMAX);

	return cost_mat;
}

float PoseEstimator::compute_dtw(Mat& cost_mat)
{
	const int i_max = cost_mat.cols;
	const int j_max = cost_mat.rows;
	
	for (int i = 0; i < i_max; ++i)
		for (int j = 0; j < j_max; ++j)
		{
			float val0;
			if (i - 1 < 0)
				val0 = FLT_MAX;
			else
				val0 = cost_mat.ptr<float>(j, i - 1)[0];

			float val1;
			if (i - 1 < 0 || j - i < 0)
				val1 = FLT_MAX;
			else
				val1 = cost_mat.ptr<float>(j - 1, i - 1)[0];

			float val2;
			if (j - 1 < 0)
				val2 = FLT_MAX;
			else
				val2 = cost_mat.ptr<float>(j - 1, i)[0];

			float val_min = std::min(std::min(val0, val1), val2);

			if (val_min == FLT_MAX)
				continue;
			
			cost_mat.ptr<float>(j, i)[0] += val_min;
		}

	return cost_mat.ptr<float>(j_max - 1, i_max - 1)[0];
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

	const string path = pose_database_path + "\\" + name + to_string(name_count) + "." + extension;
	write_string_to_file(path, data);

	points_collection.push_back(points_current);
	names_collection.push_back(name);

	COUT << "pose data saved: " << name << endl;
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
			const string path = pose_database_path + "\\" + name_current;
			vector<string> data = read_text_file(path);

			vector<Point> points;
			for (string& str : data)
			{
				vector<String> str_pair = split_string(str, "!");
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
	}
}