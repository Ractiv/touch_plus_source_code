#include "surface_computer.h"

void SurfaceComputer::init(Mat& image0)
{
	// Mat image_bright = image0;
	Mat image_bright = Mat::zeros(image0.size(), CV_8UC3);

	for (int i = 0; i < WIDTH_LARGE; ++i)
		for (int j = 0; j < HEIGHT_LARGE; ++j)
		{
			int b = image0.ptr<uchar>(j, i)[0] + 100;
			int g = image0.ptr<uchar>(j, i)[1] + 100;
			int r = image0.ptr<uchar>(j, i)[2] + 100;

			if (b > 255)
				b = 255;
			if (g > 255)
				g = 255;
			if (r > 255)
				r = 255;

			image_bright.ptr<uchar>(j, i)[0] = b;
			image_bright.ptr<uchar>(j, i)[1] = g;
			image_bright.ptr<uchar>(j, i)[2] = r;
		}

    Mat image_canny;
    Canny(image_bright, image_canny, 20, 60, 3);

    y_reflection = HEIGHT_LARGE;

    int intensity_array[HEIGHT_LARGE];
    for (int j = 0; j < HEIGHT_LARGE; ++j)
    {
        int intensity = 0;
        for (int i = 0; i < WIDTH_LARGE; ++i)
            if (image_canny.ptr<uchar>(j, i)[0] > 0)
                ++intensity;
        
        intensity_array[j] = intensity;
    }

    Mat image_histogram = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

    vector<Point3f> concave_pt_index_vec;
    vector<Point3f> convex_pt_index_vec;

    Point pt_old = Point(-1, -1);
    Point pt_old_old = Point(-1, -1);

    int index = 0;
    for (int j = 0; j < HEIGHT_LARGE; ++j)
    {
        int i = intensity_array[j];
        low_pass_filter.compute(i, 0.1, "i");

        Point pt = Point(i, j);

        if (pt_old.x != -1)
        {
            line(image_histogram, pt, pt_old, Scalar(254), 1);

            if (pt_old_old.x != -1)
            {
                if (pt_old.x < pt_old_old.x && pt_old.x < pt.x)
                    concave_pt_index_vec.push_back(Point3f(pt_old.x, pt_old.y, index - 1));

                if (pt_old.x > pt_old_old.x && pt_old.x > pt.x)
                    convex_pt_index_vec.push_back(Point3f(pt_old.x, pt_old.y, index - 1));
            }
            pt_old_old = pt_old;
        }
        pt_old = pt;

        ++index;
    }

    int x_diff_sum_max = -1;
    for (Point3f& concave_pt_index : concave_pt_index_vec)
    {
        int before_x_max = -1;
        int after_x_max = -1;
        for (Point3f& convex_pt_index : convex_pt_index_vec)
        {
            if (convex_pt_index.z < concave_pt_index.z)
            {
                if (convex_pt_index.x > before_x_max)
                    before_x_max = convex_pt_index.x;
            }
            else
            {
                if (convex_pt_index.x > after_x_max)
                    after_x_max = convex_pt_index.x;
            }
        }
        if (before_x_max != -1 && after_x_max != -1)
        {
            int x_diff_before = before_x_max - concave_pt_index.x;
            int x_diff_after = after_x_max - concave_pt_index.x;
            int x_diff_sum = (x_diff_before * x_diff_after) + (concave_pt_index.z * 2);

            if (x_diff_sum > x_diff_sum_max)
            {
                x_diff_sum_max = x_diff_sum;
                y_reflection = concave_pt_index.z;
            }
        }
    }
    y_reflection /= 4;

 //    vector<Vec4i> lines(10000);
	// HoughLinesP(image_canny, lines, 1, CV_PI / 180, 20, 50, 10);
 //    for (size_t i = 0; i < lines.size(); ++i)
 //    {
 //        Vec4i l = lines[i];
 //        Point pt0 = Point(l[0], l[1]);
 //        Point pt1 = Point(l[2], l[3]);

 //        if (abs(pt0.y - pt1.y) <= 5 && pt0.y < y_reflection)
 //        	line(image_canny, pt0, pt1, Scalar(127), 3);
 //    }
}