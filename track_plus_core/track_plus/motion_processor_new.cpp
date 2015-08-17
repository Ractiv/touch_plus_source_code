#include "motion_processor_new.h"

bool MotionProcessorNew::compute(Mat& image_in,             Mat& image_raw,  const int y_ref, float pitch,
								 bool construct_background, string name,     bool visualize)
{
	if (mode == "tool")
		return false;

	LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter");

	if (compute_background_static == false && construct_background == true)
	{
		value_store.set_bool("image_background_set", false);
		value_store.set_bool("result", false);

		gray_threshold_left = 9999;
		gray_threshold_right = 9999;
		diff_threshold = 9999;

		left_moving = false;
		right_moving = false;
		both_moving = false;

		low_pass_filter->reset();
	}

	compute_background_static = construct_background;

	left_moving = false;
	right_moving = false;
	both_moving = false;

	static float alpha = 1;

	//------------------------------------------------------------------------------------------------------------------------

	Mat image_background_unbiased = value_store.get_mat("image_background_unbiased", true);
	Mat image_subtraction_unbiased = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	uchar diff_max_unbiased = 0;
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < y_ref; ++j)
		{
			uchar diff = abs(image_in.ptr<uchar>(j, i)[0] - image_background_unbiased.ptr<uchar>(j, i)[0]);
			if (diff > diff_max_unbiased)
				diff_max_unbiased = diff;

			image_subtraction_unbiased.ptr<uchar>(j, i)[0] = diff;
		}
	threshold(image_subtraction_unbiased, image_subtraction_unbiased, diff_max_unbiased * 0.1, 254, THRESH_BINARY);

	value_store.set_mat("image_background_unbiased", image_in);

	//------------------------------------------------------------------------------------------------------------------------

	int entropy_left = 0;
	int entropy_right = 0;

	BlobDetectorNew* blob_detector_image_subtraction_unbiased = value_store.get_blob_detector("blob_detector_image_subtraction_unbiased");
	blob_detector_image_subtraction_unbiased->compute(image_subtraction_unbiased, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

	for (BlobNew& blob : *blob_detector_image_subtraction_unbiased->blobs)
		if (blob.x < x_separator_middle)
			entropy_left += blob.count;
		else
			entropy_right += blob.count;

	int entropy_left_biased = 1;
	int entropy_right_biased = 1;

	BlobDetectorNew* blob_detector_image_subtraction = value_store.get_blob_detector("blob_detector_image_subtraction");
	for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
		if (blob.x < x_separator_middle)
			entropy_left_biased += blob.count;
		else
			entropy_right_biased += blob.count;

	//------------------------------------------------------------------------------------------------------------------------

	if (entropy_left + entropy_right < 4000)
	{
		int x_min = blob_detector_image_subtraction_unbiased->x_min_result;
		int x_max = blob_detector_image_subtraction_unbiased->x_max_result;

		float x_seed_vec0_max = value_store.get_float("x_seed_vec0_max");
		float x_seed_vec1_min = value_store.get_float("x_seed_vec1_min");

		int intensity_array[WIDTH_SMALL] { 0 };
		for (BlobNew& blob : *blob_detector_image_subtraction_unbiased->blobs)
			for (Point& pt : blob.data)
				++intensity_array[pt.x];

		vector<Point> hist_pt_vec;
		for (int i = 0; i < WIDTH_SMALL; ++i)
		{
			int j = intensity_array[i];
			low_pass_filter->compute(j, 0.5, "histogram_j");

			if (j < 10)
				continue;

			for (int j_current = 0; j_current < j; ++j_current)
				hist_pt_vec.push_back(Point(i, j_current));
		}

		Point seed0 = Point(x_min, 0);
		Point seed1 = Point(x_max, 0);

		vector<Point> seed_vec0;
		vector<Point> seed_vec1;

		while (true)
		{
			seed_vec0.clear();
			seed_vec1.clear();

			for (Point& pt : hist_pt_vec)
			{
				float dist0 = get_distance(pt, seed0);
				float dist1 = get_distance(pt, seed1);

				if (dist0 < dist1)
					seed_vec0.push_back(pt);
				else
					seed_vec1.push_back(pt);
			}

			if (seed_vec0.size() == 0 || seed_vec1.size() == 0)
				break;

			Point seed0_new = Point(0, 0);
			for (Point& pt : seed_vec0)
			{
				seed0_new.x += pt.x;
				seed0_new.y += pt.y;
			}
			seed0_new.x /= seed_vec0.size();
			seed0_new.y /= seed_vec0.size();

			Point seed1_new = Point(0, 0);
			for (Point& pt : seed_vec1)
			{
				seed1_new.x += pt.x;
				seed1_new.y += pt.y;
			}
			seed1_new.x /= seed_vec1.size();
			seed1_new.y /= seed_vec1.size();

			if (seed0 == seed0_new && seed1 == seed1_new)
				break;

			seed0 = seed0_new;
			seed1 = seed1_new;
		}

		if (seed_vec0.size() > 0 && seed_vec1.size() > 0)
		{
			x_seed_vec0_max = seed_vec0[seed_vec0.size() - 1].x;
			x_seed_vec1_min = seed_vec1[0].x;

			low_pass_filter->compute_if_smaller(x_seed_vec0_max, 0.5, "x_seed_vec0_max");
			low_pass_filter->compute_if_larger(x_seed_vec1_min, 0.5, "x_seed_vec1_min");

			value_store.set_float("x_seed_vec0_max", x_seed_vec0_max);
			value_store.set_float("x_seed_vec1_min", x_seed_vec1_min);

			value_store.set_bool("x_min_max_set", true);

			#if 0
			{
				Mat image_histogram = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

				for (Point& pt : seed_vec0)
					line(image_histogram, pt, Point(pt.x, 0), Scalar(127), 1);

				for (Point& pt : seed_vec1)
					line(image_histogram, pt, Point(pt.x, 0), Scalar(254), 1);

				circle(image_histogram, seed0, 5, Scalar(64), -1);
				circle(image_histogram, seed1, 5, Scalar(64), -1);

				line(image_histogram, Point(x_seed_vec0_max, 0), Point(x_seed_vec0_max, 9999), Scalar(64), 1);
				line(image_histogram, Point(x_seed_vec1_min, 0), Point(x_seed_vec1_min, 9999), Scalar(64), 1);

				imshow("image_histogramasd" + name, image_histogram);
			}
			#endif

			float subject_width0 = x_seed_vec0_max - x_min;
			float subject_width1 = x_max - x_seed_vec1_min;
			float subject_width_max = max(subject_width0, subject_width1);
			float gap_width = x_seed_vec1_min - x_seed_vec0_max;
			float total_width = x_max - x_min;
			float gap_ratio = gap_width / subject_width_max;

			both_moving = (total_width > 80 && gap_ratio > 0.3);
		}

		static bool both_moving_0_set = false;
		static bool both_moving_1_set = false;

		static int x_separator_middle0;
		static int x_separator_middle1;

		static string motion_processor_primary_name = "";

		if (both_moving)
		{
			//------------------------------------------------------------------------------------------------------------------------

			vector<int>* x_separator_middle_vec = value_store.get_int_vec("x_separator_middle_vec");
			if (x_separator_middle_vec->size() < 1000)
			{
				x_separator_middle_vec->push_back((x_seed_vec1_min + x_seed_vec0_max) / 2);
				sort(x_separator_middle_vec->begin(), x_separator_middle_vec->end());
			}
			x_separator_middle = (*x_separator_middle_vec)[x_separator_middle_vec->size() / 2];

			if (name == "0")
				x_separator_middle0 = x_separator_middle;
			if (name == "1")
				x_separator_middle1 = x_separator_middle;

			//------------------------------------------------------------------------------------------------------------------------

			if (both_moving_0_set == false || both_moving_1_set == false)
			{
				if (name == "0")
					both_moving_0_set = true;
				if (name == "1")
					both_moving_1_set = true;

				both_moving = false;
			}
			else
			{
				if (motion_processor_primary_name == "")
				{
					int x_diff0 = abs(WIDTH_SMALL / 2 - x_separator_middle0);
					int x_diff1 = abs(WIDTH_SMALL / 2 - x_separator_middle1);

					if (x_diff0 < x_diff1)
						motion_processor_primary_name = "0";
					else
						motion_processor_primary_name = "1";
				}

				left_moving = true;
				right_moving = true;
			}
		}
		else if (entropy_left > entropy_threshold || entropy_right > entropy_threshold)
		{
			if (entropy_left > entropy_right)
				left_moving = true;
			else
				right_moving = true;
		}

		//------------------------------------------------------------------------------------------------------------------------

		if (both_moving_0_set && both_moving_1_set)
		{
			Mat image_background = value_store.get_mat("image_background", true);
			Mat image_subtraction = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

			uchar diff_max = 0;
			for (int i = 0; i < WIDTH_SMALL; ++i)
				for (int j = 0; j < y_ref; ++j)
				{
					uchar diff = abs(image_in.ptr<uchar>(j, i)[0] - image_background.ptr<uchar>(j, i)[0]);
					if (diff > diff_max)
						diff_max = diff;

					image_subtraction.ptr<uchar>(j, i)[0] = diff;
				}
			threshold(image_subtraction, image_subtraction, diff_max * 0.1, 254, THRESH_BINARY);

			//------------------------------------------------------------------------------------------------------------------------

			if (entropy_right > entropy_threshold && entropy_right < 2000 && (both_moving || entropy_left < entropy_right))
				if (entropy_right / entropy_right_biased == 0)
					for (int i = x_separator_middle; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							image_background.ptr<uchar>(j, i)[0] += (image_in.ptr<uchar>(j, i)[0] - image_background.ptr<uchar>(j, i)[0]) * alpha;

			if (entropy_left > entropy_threshold && entropy_left < 2000 && (both_moving || entropy_left > entropy_right))
				if (entropy_left / entropy_left_biased == 0)
					for (int i = 0; i < x_separator_middle; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							image_background.ptr<uchar>(j, i)[0] += (image_in.ptr<uchar>(j, i)[0] - image_background.ptr<uchar>(j, i)[0]) * alpha;

			value_store.set_mat("image_background", image_background);

			//------------------------------------------------------------------------------------------------------------------------

			blob_detector_image_subtraction->compute(image_subtraction, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

			if ((left_moving || right_moving) && value_store.get_bool("x_min_max_set"))
			{
				if (both_moving)
				{
					x_separator_left = blob_detector_image_subtraction->x_min_result;
					x_separator_right = blob_detector_image_subtraction->x_max_result;
				}
				else if (left_moving)
					x_separator_left = blob_detector_image_subtraction->x_min_result;
				else if (right_moving)
					x_separator_right = blob_detector_image_subtraction->x_max_result;
			}

			//------------------------------------------------------------------------------------------------------------------------

			int intensity_array0[HEIGHT_SMALL] { 0 };
			for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
				for (Point& pt : blob.data)
					++intensity_array0[pt.y];

			int y_min = 9999;
			int y_max = 0;

			vector<Point> hist_pt_vec0;
			for (int i = 0; i < HEIGHT_SMALL; ++i)
			{
				int j = intensity_array0[i];
				low_pass_filter->compute(j, 0.5, "histogram_j");

				if (j < 10)
					continue;

				for (int j_current = 0; j_current < j; ++j_current)
					hist_pt_vec0.push_back(Point(j_current, i));

				if (i < y_min)
					y_min = i;
				if (i > y_max)
					y_max = i;
			}

			Mat image_histogram = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
			for (Point& pt : hist_pt_vec0)
				line(image_histogram, pt, Point(0, pt.y), Scalar(254), 1);

			BlobDetectorNew* blob_detector_image_histogram = value_store.get_blob_detector("blob_detector_image_histogram");
			blob_detector_image_histogram->compute(image_histogram, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

			if (both_moving)
			{
				y_separator_down = blob_detector_image_histogram->blob_max_size->y_max;
				value_store.set_int("y_separator_down_left", y_separator_down);
				value_store.set_int("y_separator_down_right", y_separator_down);
			}
			else if (left_moving)
			{
				int y_separator_down_left = blob_detector_image_histogram->blob_max_size->y_max;
				int y_separator_down_right = value_store.get_int("y_separator_down_right", 0);
				value_store.set_int("y_separator_down_left", y_separator_down_left);

				if (y_separator_down_left > y_separator_down_right)
					y_separator_down = y_separator_down_left;
			}
			else if (right_moving)
			{
				int y_separator_down_right = blob_detector_image_histogram->blob_max_size->y_max;
				int y_separator_down_left = value_store.get_int("y_separator_down_left", 0);
				value_store.set_int("y_separator_down_right", y_separator_down_right);

				if (y_separator_down_right > y_separator_down_left)
					y_separator_down = y_separator_down_right;
			}

			//------------------------------------------------------------------------------------------------------------------------

			if (left_moving || right_moving)
			{
				int blobs_y_min = 9999;
				for (BlobNew& blob : *blob_detector_image_subtraction->blobs)
					if (blob.y_min < blobs_y_min)
						blobs_y_min = blob.y_min;

				y_separator_up = blobs_y_min + 5;
			}

			//------------------------------------------------------------------------------------------------------------------------

			if ((((left_moving || right_moving) && value_store.get_bool("result", false)) || both_moving) && construct_background)
			{
				//triangle fill
				if (y_separator_up > 0)
				{
					Point pt_center = Point(x_separator_middle, y_separator_up);
					Mat image_flood_fill = Mat::zeros(pt_center.y + 1, WIDTH_SMALL, CV_8UC1);
					line(image_flood_fill, pt_center, Point(x_separator_left, 0), Scalar(254), 1);
					line(image_flood_fill, pt_center, Point(x_separator_right, 0), Scalar(254), 1);
					floodFill(image_flood_fill, Point(pt_center.x, 0), Scalar(254));

					const int j_max = image_flood_fill.rows;
					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < j_max; ++j)
							if (image_flood_fill.ptr<uchar>(j, i)[0] > 0)
								fill_image_background_static(i, j, image_in);
				}

				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = y_separator_down; j < HEIGHT_SMALL; ++j)
						fill_image_background_static(i, j, image_in);

				//------------------------------------------------------------------------------------------------------------------------

				static float gray_threshold_left_stereo = 9999;
				static float gray_threshold_right_stereo = 9999;

				if (name == "0")
				{
					vector<uchar> gray_vec_left;
					vector<uchar> gray_vec_right;

					for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
						if (blob.active)
                        {
							if (blob.x < x_separator_middle)
							{
								for (Point pt : blob.data)
								{
									uchar gray = std::max(image_in.ptr<uchar>(pt.y, pt.x)[0], image_background.ptr<uchar>(pt.y, pt.x)[0]);
									gray_vec_left.push_back(gray);
								}
							}
							else
							{
								for (Point pt : blob.data)
								{
									uchar gray = std::max(image_in.ptr<uchar>(pt.y, pt.x)[0], image_background.ptr<uchar>(pt.y, pt.x)[0]);
									gray_vec_right.push_back(gray);
								}	
							}
                        }

					if (gray_vec_left.size() > 0 && left_moving)
					{
						sort(gray_vec_left.begin(), gray_vec_left.end());
						float gray_median_left = gray_vec_left[gray_vec_left.size() * 0.5];

						gray_threshold_left = gray_median_left - 20;
						low_pass_filter->compute(gray_threshold_left, 0.1, "gray_threshold_left");
						gray_threshold_left_stereo = gray_threshold_left;
					}

					if (gray_vec_right.size() > 0 && right_moving)
					{
						sort(gray_vec_right.begin(), gray_vec_right.end());
						float gray_median_right = gray_vec_right[gray_vec_right.size() * 0.5];

						gray_threshold_right = gray_median_right - 20;
						low_pass_filter->compute(gray_threshold_right, 0.1, "gray_threshold_right");
						gray_threshold_right_stereo = gray_threshold_right;
					}
				}
				else
				{
					gray_threshold_left = gray_threshold_left_stereo;
					gray_threshold_right = gray_threshold_right_stereo;
				}

				//------------------------------------------------------------------------------------------------------------------------

				Mat image_in_thresholded = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (i < x_separator_middle && image_in.ptr<uchar>(j, i)[0] > gray_threshold_left)
							image_in_thresholded.ptr<uchar>(j, i)[0] = 254;
						else if (i > x_separator_middle && image_in.ptr<uchar>(j, i)[0] > gray_threshold_right)
							image_in_thresholded.ptr<uchar>(j, i)[0] = 254;

				//------------------------------------------------------------------------------------------------------------------------

				uchar static_diff_max = 255;
				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (image_in_thresholded.ptr<uchar>(j, i)[0] > 0)
							if (image_background_static.ptr<uchar>(j, i)[0] < 255)
							{
								const uchar diff = abs(image_in.ptr<uchar>(j, i)[0] - image_background_static.ptr<uchar>(j, i)[0]);
								if (diff > static_diff_max)
									static_diff_max = diff;
							}

				static float diff_threshold_stereo;
				if (name == "0")
				{
					diff_threshold = static_diff_max * 0.3;
					diff_threshold_stereo = diff_threshold;
				}
				else
					diff_threshold = diff_threshold_stereo;

				//------------------------------------------------------------------------------------------------------------------------

				Mat image_canny;
				Canny(image_raw, image_canny, 50, 50, 3);

				for (int i = 0; i < WIDTH_SMALL; ++i)
					for (int j = 0; j < HEIGHT_SMALL; ++j)
						if (image_canny.ptr<uchar>(j, i)[0] > 0)
							image_in_thresholded.ptr<uchar>(j, i)[0] = 0;

				for (BlobNew& blob : *(blob_detector_image_subtraction->blobs))
					for (Point& pt : blob.data)
						if (image_in_thresholded.ptr<uchar>(pt.y, pt.x)[0] == 254)
							floodFill(image_in_thresholded, pt, Scalar(127));

				dilate(image_in_thresholded, image_in_thresholded, Mat(), Point(-1, -1), 3);

				//------------------------------------------------------------------------------------------------------------------------

				BlobDetectorNew* blob_detector_image_in_thresholded = value_store.get_blob_detector("blob_detector_image_in_thresholded");
				blob_detector_image_in_thresholded->compute(image_in_thresholded, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

				for (BlobNew& blob : *blob_detector_image_in_thresholded->blobs)
					if (blob.x < x_separator_left || blob.x > x_separator_right)
						for (Point& pt : blob.data)
							fill_image_background_static(pt.x, pt.y, image_in);

				//------------------------------------------------------------------------------------------------------------------------

				const int x_separator_middle_const = x_separator_middle;

				if (gray_threshold_left != 9999)
					for (int i = 0; i < x_separator_middle; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_in_thresholded.ptr<uchar>(j, i)[0] == 0)
								fill_image_background_static(i, j, image_in);

				if (gray_threshold_right != 9999)
					for (int i = x_separator_middle; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_in_thresholded.ptr<uchar>(j, i)[0] == 0)
								fill_image_background_static(i, j, image_in);

				//------------------------------------------------------------------------------------------------------------------------

				if (both_moving && entropy_left + entropy_right > 1000)
				{
					float hole_count_left = 0;
					float hole_count_right = 0;

					for (int i = 0; i < WIDTH_SMALL; ++i)
						for (int j = 0; j < HEIGHT_SMALL; ++j)
							if (image_background_static.ptr<uchar>(j, i)[0] == 255)
							{
								if (i < x_separator_middle)
									++hole_count_left;
								else
									++hole_count_right;
							}

					if (hole_count_right / (entropy_right + 0.1) < 1 && hole_count_left / (entropy_left + 0.1) < 1)
					{
						alpha = 0.5;
						value_store.set_bool("result", true);

						//------------------------------------------------------------------------------------------------------------------------

						float width_bottom = x_separator_right - x_separator_left + 10;
						float width_top = width_bottom - power(pitch, 0.006834535, 2.199475);

						int x_middle_bottom = x_separator_middle;
						int x_middle_top = (x_middle_bottom - (WIDTH_SMALL / 2)) * 0.5 + (WIDTH_SMALL / 2);

						int x_top_left = x_middle_top - (width_top / 2);
						int x_top_right = x_middle_top + (width_top / 2);
						int x_bottom_left = x_middle_bottom - (width_bottom / 2);
						int x_bottom_right = x_middle_bottom + (width_bottom / 2);

						Point pt_top_left = Point(x_top_left, y_separator_up);
						Point pt_top_right = Point(x_top_right, y_separator_up);
						Point pt_bottom_left = Point(x_bottom_left, y_separator_down);
						Point pt_bottom_right = Point(x_bottom_right, y_separator_down);

						Point pt_intersection0;
						Point pt_intersection1;
						Point pt_intersection2;
						Point pt_intersection3;
						bool b0 = get_intersection_at_y(pt_top_left, pt_bottom_left, 0, pt_intersection0);
						bool b1 = get_intersection_at_y(pt_top_left, pt_bottom_left, HEIGHT_SMALL_MINUS, pt_intersection1);
						bool b2 = get_intersection_at_y(pt_top_right, pt_bottom_right, 0, pt_intersection2);
						bool b3 = get_intersection_at_y(pt_top_right, pt_bottom_right, HEIGHT_SMALL_MINUS, pt_intersection3);

						if (b0 && b1 && b2 && b3)
						{
							Mat image_borders = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

							line(image_borders, pt_intersection0, pt_intersection1, Scalar(254), 1);
							line(image_borders, pt_intersection2, pt_intersection3, Scalar(254), 1);

							BlobDetectorNew* blob_detector_image_borders = value_store.get_blob_detector("blob_detector_image_borders");

							bool b4 = false;
							if (pt_intersection0.x > 0)
							{
								floodFill(image_borders, Point(0, 0), Scalar(254));
								b4 = true;
							}
							else if (pt_intersection1.x > 0)
							{
								floodFill(image_borders, Point(0, HEIGHT_SMALL_MINUS), Scalar(254));
								b4 = true;
							}

							bool b5 = false;
							if (pt_intersection2.x < WIDTH_SMALL_MINUS)
							{
								floodFill(image_borders, Point(WIDTH_SMALL_MINUS, 0), Scalar(254));
								b5 = true;
							}
							else if (pt_intersection3.x < WIDTH_SMALL_MINUS)
							{
								floodFill(image_borders, Point(WIDTH_SMALL_MINUS, HEIGHT_SMALL_MINUS), Scalar(254));
								b5 = true;
							}

							if (b4 && b5)
								for (int i = 0; i < WIDTH_SMALL; ++i)
									for (int j = 0; j < HEIGHT_SMALL; ++j)
										if (image_borders.ptr<uchar>(j, i)[0] > 0)
											fill_image_background_static(i, j, image_in);
						}
					}
				}
			}

			//------------------------------------------------------------------------------------------------------------------------

			if (enable_imshow && visualize)
			{
				Mat image_visualization = image_background_static.clone();
				line(image_visualization, Point(x_separator_left, 0), Point(x_separator_left, 999), Scalar(254), 1);
				line(image_visualization, Point(x_separator_right, 0), Point(x_separator_right, 999), Scalar(254), 1);
				line(image_visualization, Point(x_separator_middle, 0), Point(x_separator_middle, 999), Scalar(254), 1);
				line(image_visualization, Point(0, y_separator_down), Point(999, y_separator_down), Scalar(254), 1);
				line(image_visualization, Point(0, y_separator_up), Point(999, y_separator_up), Scalar(254), 1);

				imshow("image_visualization" + name, image_visualization);
				// imshow("image_subtraction" + name, image_subtraction);
			}
		}
	}

	return value_store.get_bool("result", false);
}

inline void MotionProcessorNew::fill_image_background_static(const int x, const int y, Mat& image_in)
{
	if (!compute_background_static)
		return;

	uchar* pix_ptr = &(image_background_static.ptr<uchar>(y, x)[0]);

	if (*pix_ptr == 255)
		*pix_ptr = image_in.ptr<uchar>(y, x)[0];
	else
		*pix_ptr = *pix_ptr + ((image_in.ptr<uchar>(y, x)[0] - *pix_ptr) * 0.1);
}