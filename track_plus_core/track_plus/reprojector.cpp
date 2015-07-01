#include "reprojector.h"

float Reprojector::compute_depth(float disparity_in)
{
	return ((a_out - d_out) / (1 + pow(disparity_in / c_out, b_out))) + d_out;
}

Point2f Reprojector::compute_plane_size(float depth)
{
    float width = FOV_WIDTH * (depth / FOV_DEPTH);
    float height = width / 4 * 3;
	return Point2f(width, height);
}

Point3f Reprojector::reproject_to_3d(float pt0_x, float pt0_y, float pt1_x, float pt1_y)
{
	float disparity_val = abs(pt1_x - pt0_x);
	float depth = compute_depth(disparity_val);

	Point2f plane_size = compute_plane_size(depth);

	float half_plane_width = plane_size.x / 2;
	float halfPlaneHeight = plane_size.y / 2;

	float real_x = map_val(pt0_x, 0, 640, -half_plane_width,  half_plane_width);
	float real_y = map_val(pt0_y, 0, 480, -halfPlaneHeight,   halfPlaneHeight);

	return Point3f(real_x * 10, real_y * 10, depth * 10);
}

void Reprojector::load(IPC& ipc)
{
	bool serial_first_non_zero = false;
	string serial = "";

	for (int i = 4; i < 10; ++i)
	{
		string str_temp = "";
		str_temp += serial_number[i];

		if (!serial_first_non_zero && str_temp != "0")
			serial_first_non_zero = true;

		if (serial_first_non_zero)
			serial += serial_number[i];
	}

	bool has_complete_calib_data = false;

	if (directory_exists(data_path_current_module))
		if (file_exists(data_path_current_module + "\\0.jpg"))
			if (file_exists(data_path_current_module + "\\1.jpg"))
				if (file_exists(data_path_current_module + "\\stereoCalibData.txt"))
					if (file_exists(data_path_current_module + "\\rect0.txt"))
						if (file_exists(data_path_current_module + "\\rect1.txt"))
							has_complete_calib_data = true;

	if (!has_complete_calib_data)
	{
		static bool block_thread = true;
		ipc.send_message("menu_plus", "show window", "");
		ipc.get_response("menu_plus", "show download", "", [](const string message_body)
		{
			COUT << "unblock" << endl;
			block_thread = false;
		});
		
		while (block_thread)
		{
			ipc.update();
			Sleep(100);
		}

		CreateDirectory(data_path.c_str(), NULL);
		CreateDirectory(data_path_current_module.c_str(), NULL);

		copy_file(executable_path + "\\downloader.exe", data_path_current_module + "\\downloader.exe");
		copy_file(executable_path + "\\rectifier.exe", data_path_current_module + "\\rectifier.exe");
		copy_file(executable_path + "\\opencv_core249.dll", data_path_current_module + "\\opencv_core249.dll");
		copy_file(executable_path + "\\opencv_highgui249.dll", data_path_current_module + "\\opencv_highgui249.dll");
		copy_file(executable_path + "\\opencv_imgproc249.dll", data_path_current_module + "\\opencv_imgproc249.dll");
		copy_file(executable_path + "\\opencv_calib3d249.dll", data_path_current_module + "\\opencv_calib3d249.dll");
		copy_file(executable_path + "\\opencv_flann249.dll", data_path_current_module + "\\opencv_flann249.dll");
		copy_file(executable_path + "\\opencv_features2d249.dll", data_path_current_module + "\\opencv_features2d249.dll");

		string param0 = "http://d2i9bzz66ghms6.cloudfront.net/data/" + serial + "/0.jpg";
		string param1 = data_path_current_module + "\\0.jpg";

		string* serial_ptr = &serial;
		IPC* ipc_ptr = &ipc;
		ipc.get_response("menu_plus", "download", param0 + "`" + param1, [serial_ptr, ipc_ptr](const string message_body)
		{
			if (message_body == "false")
				ipc_ptr->send_message("daemon_plus", "exit", "");
			else
			{
				string param0 = "http://d2i9bzz66ghms6.cloudfront.net/data/" + *serial_ptr + "/1.jpg";
				string param1 = data_path_current_module + "\\1.jpg";

				ipc_ptr->get_response("menu_plus", "download", param0 + "`" + param1, [serial_ptr, ipc_ptr](const string message_body)
				{
					if (message_body == "false")
						ipc_ptr->send_message("daemon_plus", "exit", "");
					else
					{
						string param0 = "http://d2i9bzz66ghms6.cloudfront.net/data/" + *serial_ptr + "/stereoCalibData.txt";
						string param1 = data_path_current_module + "\\stereoCalibData.txt";

						ipc_ptr->get_response("menu_plus", "download", param0 + "`" + param1, [serial_ptr, ipc_ptr](const string message_body)
						{
							if (message_body == "false")
								ipc_ptr->send_message("daemon_plus", "exit", "");
							else
							{
								system(("cd " + cmd_quote + data_path_current_module + cmd_quote + "&& rectifier.exe").c_str());

								bool has_complete_calib_data = false;
								if (directory_exists(data_path_current_module))
									if (file_exists(data_path_current_module + "\\0.jpg"))
										if (file_exists(data_path_current_module + "\\1.jpg"))
											if (file_exists(data_path_current_module + "\\stereoCalibData.txt"))
												if (file_exists(data_path_current_module + "\\rect0.txt"))
													if (file_exists(data_path_current_module + "\\rect1.txt"))
														has_complete_calib_data = true;

								if (has_complete_calib_data)
									block_thread = false;
								else
									ipc_ptr->send_message("menu_plus", "download failed", "");
							}
						});
					}
				});
			}
		});

		block_thread = true;

		while (block_thread)
		{
			ipc.update();
			Sleep(100);
		}
	}

	ifstream file_stereo_calib_data(data_path_current_module + "\\stereoCalibData.txt");

	bool is_number_new = false;
	bool is_number_old = false;

	int block_count = 0;
	int block[4];

	vector<Point> disparity_data;

	string str_num_temp = "";
	string disparities_string = "";
	
	while (getline(file_stereo_calib_data, disparities_string))
	{
		const int i_max = disparities_string.length();
		for (int i = 0; i < i_max; ++i)
		{
			string str_temp = "";
			str_temp += disparities_string[i];

			if (str_temp != "," && str_temp != ";")
				is_number_new = true;
			else
				is_number_new = false;

			if (is_number_new)
			{
				if (!is_number_old)
					str_num_temp = str_temp;
				else
					str_num_temp += str_temp;
			}
			else if (is_number_old)
			{
				block[block_count] = stoi(str_num_temp);
				++block_count;
			}

			if (block_count == 3)
			{
				bool found = false;

				for (int a = 0; a < disparity_data.size(); ++a)
				{
					if (disparity_data[a].x == block[0])
					{
						found = true;
						disparity_data[a].y = (disparity_data[a].y + abs(block[1] - block[2])) / 2;
					}
					else if (disparity_data[a].y == abs(block[1] - block[2]))
					{
						found = true;
						disparity_data[a].x = min(disparity_data[a].x, block[0]);
					}
				}
				if (!found)
					disparity_data.push_back(Point(block[0], abs(block[1] - block[2])));

				block_count = 0;
			}

			is_number_old = is_number_new;
		}
	}
	sort(disparity_data.begin(), disparity_data.end(), compare_point_x());

	double *t, *y;

	t = new double[disparity_data.size()];
	y = new double[disparity_data.size()];

	for (unsigned int a = 0; a < disparity_data.size(); a++)
	{
		t[a] = (double)(disparity_data[a].y);
		y[a] = (double)(disparity_data[a].x);
	}
	CCurveFitting cf;
	cf.curve_fitting4(t, disparity_data.size(), y, &a_out, &b_out, &c_out, &d_out);

	delete []t;
	delete []y;

	ifstream file0(data_path_current_module + "\\rect0.txt");
	is_number_new = false;
	is_number_old = false;
	block_count = 0;

	rect_mat0 = new Point*[640];
	for (int i = 0; i < 640; ++i)
		rect_mat0[i] = new Point[480];

	string rect0_string = "";
	while (getline(file0, rect0_string))
	{
		const int i_max = rect0_string.length();
		for (int i = 0; i < i_max; ++i)
		{
			string str_temp = "";
			str_temp += rect0_string[i];

			if (str_temp != " " && str_temp != "," && str_temp != ";")
				is_number_new = true;
			else
				is_number_new = false;

			if (is_number_new)
			{
				if (!is_number_old)
					str_num_temp = str_temp;
				else
					str_num_temp += str_temp;
			}
			else if (is_number_old)
			{
				block[block_count] = stoi(str_num_temp);
				++block_count;
			}
			if (block_count == 4)
			{
				rect_mat0[block[0]][block[1]] = Point(block[2], block[3]);
				block_count = 0;
			}
			is_number_old = is_number_new;
		}
	}
	ifstream file1(data_path_current_module + "\\rect1.txt");
	is_number_new = false;
	is_number_old = false;
	block_count = 0;

	rect_mat1 = new Point*[640];
	for (int i = 0; i < 640; ++i)
		rect_mat1[i] = new Point[480];

	string rect1_string = "";
	while (getline(file1, rect1_string))
	{
		const int i_max = rect1_string.length();
		for (int i = 0; i < i_max; ++i)
		{
			string str_temp = "";
			str_temp += rect1_string[i];

			if (str_temp != " " && str_temp != "," && str_temp != ";")
				is_number_new = true;
			else
				is_number_new = false;

			if (is_number_new)
			{
				if (!is_number_old)
					str_num_temp = str_temp;
				else
					str_num_temp += str_temp;
			}
			else if (is_number_old)
			{
				block[block_count] = stoi(str_num_temp);
				++block_count;
			}
			if (block_count == 4)
			{
				rect_mat1[block[0]][block[1]] = Point(block[2], block[3]);
				block_count = 0;
			}
			is_number_old = is_number_new;
		}
	}
}

Mat Reprojector::remap(Mat* const image_in, const uchar side, const bool interpolate)
{
	const int image_width_const = image_in->cols;
	const int image_height_const = image_in->rows;

	const int scale = 640 / image_in->cols;

	Mat image_out = Mat(image_in->size(), CV_8UC1, Scalar(254));
	Point** rect_mat = NULL;

	if (side == 0)
		rect_mat = rect_mat0;
	else
		rect_mat = rect_mat1;

	Point pt_reprojected;
	uchar gray_reprojected;

	for (int i = 0; i < image_width_const; ++i)
		for (int j = 0; j < image_height_const; ++j)
		{
			pt_reprojected = rect_mat[i * scale][j * scale];
			gray_reprojected = image_in->ptr<uchar>(j, i)[0];

			if (gray_reprojected == 254)
				gray_reprojected = 253;

			image_out.ptr<uchar>(pt_reprojected.y / scale, pt_reprojected.x / scale)[0] = gray_reprojected;
		}

	const int image_width_const_minus = image_width_const - 1;
	const int image_height_const_minus = image_height_const - 1;

	uchar pix0, pix1, pix2, pix3, pix4, pix5, pix6, pix7;
	int pix_mean;
	uchar pix_count;

	if (interpolate)
		for (int i = 1; i < image_width_const_minus; ++i)
			for (int j = 1; j < image_height_const_minus; ++j)
				if (image_out.ptr<uchar>(j, i)[0] == 254)
				{
					pix_mean = 0;
					pix_count = 0;

					pix0 = image_out.ptr<uchar>(j - 1, i - 1)[0];
					pix1 = image_out.ptr<uchar>(j - 1, i)[0];
					pix2 = image_out.ptr<uchar>(j - 1, i + 1)[0];
					pix3 = image_out.ptr<uchar>(j, i - 1)[0];
					pix4 = image_out.ptr<uchar>(j, i + 1)[0];
					pix5 = image_out.ptr<uchar>(j + 1, i - 1)[0];
					pix6 = image_out.ptr<uchar>(j + 1, i)[0];
					pix7 = image_out.ptr<uchar>(j + 1, i + 1)[0];

					if (pix0 == 255 || pix1 == 255 || pix2 == 255 || pix3 == 255 || pix4 == 255 || pix5 == 255 || pix6 == 255 || pix7 == 255)
						continue;
					
					if (pix0 < 254)
					{
						pix_mean += pix0;
						++pix_count;
					}
					if (pix1 < 254)
					{
						pix_mean += pix1;
						++pix_count;
					}
					if (pix2 < 254)
					{
						pix_mean += pix2;
						++pix_count;
					}
					if (pix3 < 254)
					{
						pix_mean += pix3;
						++pix_count;
					}
					if (pix4 < 254)
					{
						pix_mean += pix4;
						++pix_count;
					}
					if (pix5 < 254)
					{
						pix_mean += pix5;
						++pix_count;
					}
					if (pix6 < 254)
					{
						pix_mean += pix6;
						++pix_count;
					}
					if (pix7 < 254)
					{
						pix_mean += pix7;
						++pix_count;
					}
					pix_mean /= pix_count;
					image_out.ptr<uchar>(j, i)[0] = pix_mean;
				}

	return image_out;
}

Mat Reprojector::remap(Mat* const image_in, const int x_offset, const int y_offset, const uchar side, Point& pt_offset)
{
	const int image_width_const = image_in->cols;
	const int image_height_const = image_in->rows;

	Mat image_out = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);
	Point** rect_mat = NULL;

	if (side == 0)
		rect_mat = rect_mat0;
	else
		rect_mat = rect_mat1;

	Point pt_reprojected;
	uchar gray_reprojected;

	int x_min = 9999;
	int x_max = 0;
	int y_min = 9999;
	int y_max = 0;

	for (int i = 0; i < image_width_const; ++i)
	{
		const int i_x_offset = i + x_offset;
		for (int j = 0; j < image_height_const; ++j)
		{
			pt_reprojected = rect_mat[i_x_offset][j + y_offset];

			if (pt_reprojected.x < x_min)
				x_min = pt_reprojected.x;
			if (pt_reprojected.x > x_max)
				x_max = pt_reprojected.x;
			if (pt_reprojected.y < y_min)
				y_min = pt_reprojected.y;
			if (pt_reprojected.y > y_max)
				y_max = pt_reprojected.y;

			gray_reprojected = image_in->ptr<uchar>(j, i)[0];
			image_out.ptr<uchar>(pt_reprojected.y,  pt_reprojected.x)[0] = gray_reprojected;
		}
	}

	pt_offset = Point(x_min, y_min);
	image_out = image_out(Rect(x_min, y_min, x_max - x_min, y_max - y_min));
	return image_out;
}

Point Reprojector::remap_point(Point& pt_in, const uchar side, const uchar scale)
{
	Point** rect_mat = NULL;

	if (side == 0)
		rect_mat = rect_mat0;
	else
		rect_mat = rect_mat1;

	return rect_mat[pt_in.x * scale][pt_in.y * scale];
}

Mat Reprojector::compute_gray_image(Mat* const image)
{
	Mat image_gray = Mat(image->size(), CV_8UC1);

	const int image_width_const = image->cols;
	const int image_height_const = image->rows;

	for (int i = 0; i < image_width_const; ++i)
		for (int j = 0; j < image_height_const; ++j)
		{
			int val = max(image->ptr<uchar>(j, i)[0], max(image->ptr<uchar>(j, i)[1], image->ptr<uchar>(j, i)[2]));
			val = pow(val, 2) / 20;
			if (val > 255)
				val = 255;

			image_gray.ptr<uchar>(j, i)[0] = val;
		}

	return image_gray;
}