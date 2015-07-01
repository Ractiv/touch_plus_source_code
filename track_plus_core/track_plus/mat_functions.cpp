#include "mat_functions.h"

LowPassFilter mat_functions_low_pass_filter;

void threshold_get_bounds(Mat& image_in, Mat& image_out, const int threshold_val, int& x_min, int& x_max, int& y_min, int& y_max)
{
	Mat image_out_temp = Mat::zeros(image_in.size(), CV_8UC1);

	const int i_max = image_in.cols;
	const int j_max = image_in.rows;

	x_min = 9999;
	x_max = 0;
	y_min = 9999;
	y_max = 0;

	for (int i = 0; i < i_max; ++i)
		for (int j = 0; j < j_max; ++j)
			if (image_in.ptr<uchar>(j, i)[0] > threshold_val)
			{
				image_out_temp.ptr<uchar>(j, i)[0] = 254;

				if (i < x_min)
					x_min = i;
				if (i > x_max)
					x_max = i;
				if (j < y_min)
					y_min = j;
				if (j > y_max)
					y_max = j;
			}

	if (x_min == 9999)
		x_min = x_max = y_min = y_max = 0;

	image_out = image_out_temp;
}

Mat rotate_image(const Mat& image_in, const float angle, const Point origin, const int border)
{
    Mat bordered_source;
    copyMakeBorder(image_in, bordered_source, border, border, border, border, BORDER_CONSTANT,Scalar());
    Point2f src_center(origin.x, origin.y);
    Mat rot_mat = getRotationMatrix2D(src_center, angle, 1.0);
    Mat dst;
    warpAffine(bordered_source, dst, rot_mat, bordered_source.size());  
    return dst;
}

Mat translate_image(Mat& image_in, const int x_diff, const int y_diff)
{
	Mat image_result = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
			if (image_in.ptr<uchar>(j, i)[0] > 0)
			{
				const int i_new = i - x_diff;
				const int j_new = j - y_diff;

				if (i_new < 0 || i_new > WIDTH_SMALL_MINUS || j_new < 0 || j_new > HEIGHT_SMALL_MINUS)
					continue;

				image_result.ptr<uchar>(j_new, i_new)[0] = image_in.ptr<uchar>(j, i)[0];
			}

	return image_result;
}

Mat resize_image(Mat& image_in, const float scale)
{
	Mat image_resized;
	resize(image_in, image_resized, Size(image_in.cols * scale, image_in.rows * scale), 0, 0, INTER_LINEAR);

	Mat image_new = Mat::zeros(image_in.rows, image_in.cols, CV_8UC1);

	const int x_offset = (image_new.cols - image_resized.cols) / 2;
	const int y_offset = (image_new.rows - image_resized.rows) / 2;

	const int i_max = image_resized.cols;
	const int j_max = image_resized.rows;
	for (int i = 0; i < i_max; ++i)
	{
		const int i_new = i + x_offset;
		if (i_new >= image_in.cols || i_new < 0)
			continue;

		for (int j = 0; j < j_max; ++j)
		{
			const int j_new = j + y_offset;
			if (j_new >= image_in.rows || j_new < 0)
				continue;

			image_new.ptr<uchar>(j_new, i_new)[0] = image_resized.ptr<uchar>(j, i)[0];
		}
	}

	return image_new;
}

void distance_transform(Mat& image_in, float& dist_min, float& dist_max, Point& pt_dist_min, Point& pt_dist_max)
{
	Mat image_find_contours = image_in.clone();

	vector<Vec4i> hiearchy;
	vector<vector<Point>> contours;
	findContours(image_find_contours, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	dist_min = 9999;
	dist_max = 0;

	const int image_width_const = image_in.cols;
	const int image_height_const = image_in.rows;

	for (int i = 0; i < image_width_const; ++i)
		for (int j = 0; j < image_height_const; ++j)
			if (image_in.ptr<uchar>(j, i)[0] > 0)
			{
				float dist_min_current = 9999;
				for (vector<Point>& contour : contours)
					for (Point& pt : contour)
					{
						const float dist_current = get_distance(i, j, pt.x, pt.y);
						if (dist_current < dist_min_current)
							dist_min_current = dist_current;
					}

				if (dist_min_current < dist_min)
				{
					dist_min = dist_min_current;
					pt_dist_min = Point(i, j);
				}

				if (dist_min_current > dist_max)
				{
					dist_max = dist_min_current;
					pt_dist_max = Point(i, j);
				}
			}
}

void compute_channel_diff_image(Mat& image_in, Mat& image_out, bool normalize, string name)
{
	const int image_width_const = image_in.cols;
	const int image_height_const = image_in.rows;

	image_out = Mat(image_height_const, image_width_const, CV_8UC1);

	uchar gray_min = 9999;
	uchar gray_max = 0;
	for (int i = 0; i < image_width_const; ++i)
		for (int j = 0; j < image_height_const; ++j)
		{
			int diff0 = image_in.ptr<uchar>(j, i)[0] - image_in.ptr<uchar>(j, i)[1];
			int diff1 = image_in.ptr<uchar>(j, i)[2] - image_in.ptr<uchar>(j, i)[1];

			if (diff0 < 0)
				diff0 = 0;
			if (diff1 < 0)
				diff1 = 0;

			const uchar gray = min(diff0, diff1);
			if (gray < gray_min)
				gray_min = gray;
			if (gray > gray_max)
				gray_max = gray;

			image_out.ptr<uchar>(j, i)[0] = gray;
		}

	mat_functions_low_pass_filter.compute(gray_min, 0.1, "gray_min" + name);
	mat_functions_low_pass_filter.compute(gray_max, 0.1, "gray_max" + name);

	if (normalize)
		for (int i = 0; i < image_width_const; ++i)
			for (int j = 0; j < image_height_const; ++j)
			{
				int gray = map_val(image_out.ptr<uchar>(j, i)[0], gray_min, gray_max, 0, 254);
				if (gray > 254)
					gray = 254;
				if (gray < 0)
					gray = 0;

				image_out.ptr<uchar>(j, i)[0] = gray;
			}

	rectangle(image_out, Rect(0, 0, image_in.cols, image_in.rows), Scalar(0), 2);
}

void compute_max_image(Mat& image_in, Mat& image_out)
{
	const int image_width_const = image_in.cols;
	const int image_height_const = image_in.rows;

	for (int i = 0; i < image_width_const; ++i)
		for (int j = 0; j < image_height_const; ++j)
			image_out.ptr<uchar>(j, i)[0] = max(image_in.ptr<uchar>(j, i)[0],
										    max(image_in.ptr<uchar>(j, i)[1], image_in.ptr<uchar>(j, i)[2]));
}

void print_mat_type(Mat& image_in)
{
	int type = image_in.type();

	string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth)
	{
		case CV_8U:  r = "8U"; break;
		case CV_8S:  r = "8S"; break;
		case CV_16U: r = "16U"; break;
		case CV_16S: r = "16S"; break;
		case CV_32S: r = "32S"; break;
		case CV_32F: r = "32F"; break;
		case CV_64F: r = "64F"; break;
		default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	COUT << r << endl;
}