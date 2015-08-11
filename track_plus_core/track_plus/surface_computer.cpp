#include "surface_computer.h"

void SurfaceComputer::init(Mat& image_in)
{
    Mat image_flipped;
    flip(image_in, image_flipped, 0);

    Mat image0 = image_flipped(Rect(0, 0, 640, 480));
    Mat image1 = image_flipped(Rect(640, 0, 640, 480));

    Mat image_to_canny = image0;

    Mat image_canny;
    Canny(image_to_canny, image_canny, 20, 60, 3);

    int intensity_array[HEIGHT_LARGE];
    for (int j = 0; j < HEIGHT_LARGE; ++j)
    {
        int intensity = 0;
        for (int i = 0; i < WIDTH_LARGE; ++i)
            if (image_canny.ptr<uchar>(j, i)[0] > 0)
                ++intensity;
        
        intensity_array[j] = intensity;
    }

    // Mat image_histogram = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

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
            // line(image_histogram, pt, pt_old, Scalar(254), 1);

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

    int y_reflection = HEIGHT_LARGE;

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

    // line(image_histogram, Point(0, y_reflection), Point(999, y_reflection), Scalar(254), 1);
    // imshow("image_histogram", image_histogram);

    // Mat image_visualization = image_canny.clone();

    pt_start_vec.clear();
    pt_end_vec.clear();

    vector<int> horizontal_y_vec;

    vector<Vec4i> lines;
    HoughLinesP(image_canny, lines, 1, CV_PI/180, 50, 50, 10 );
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        Point pt0 = Point(l[0], l[1]);
        Point pt1 = Point(l[2], l[3]);

        if ((pt0.y + pt1.y) / 2 < y_reflection)
        {
            float slope = (float)abs(pt0.y - pt1.y) / ((float)abs(pt0.x - pt1.x) + 0.1);

            if (pt0.y == pt1.y)
            {
            	bool found = false;
            	for (int& horizontal_y : horizontal_y_vec)
            		if (abs(horizontal_y - pt0.y) < 15)
            		{
            			found = true;
            			break;
            		}

            	if (!found)
            	{
	                pt_start_vec.push_back(pt0);
	                pt_end_vec.push_back(pt1);
	                horizontal_y_vec.push_back(pt0.y);
	    			// line(image_visualization, pt0, pt1, Scalar(127), 2);
	            }
            }
            else if (pt0.x < pt1.x && ((pt0.x + pt1.x) / 2) < (WIDTH_LARGE / 2) && pt0.y > pt1.y && slope > 0.5)
            {
                pt_start_vec.push_back(pt0);
                pt_end_vec.push_back(pt1);
    			// line(image_visualization, pt0, pt1, Scalar(127), 2);
            }
            else if (pt0.x < pt1.x && ((pt0.x + pt1.x) / 2) > (WIDTH_LARGE / 2) && pt0.y < pt1.y && slope > 0.5)
            {
                pt_start_vec.push_back(pt0);
                pt_end_vec.push_back(pt1);
    			// line(image_visualization, pt0, pt1, Scalar(127), 2);
            }
        }
    }

    // imshow("image_visualization_asdasd", image_visualization);
}

void SurfaceComputer::compute(Mat& image_in)
{
	const int scan_count = 5;
	vector<int> gray_diff_vec;

	for (int i = 0; i < pt_start_vec.size(); ++i)
	{
		Point pt_start = pt_start_vec[i];
		Point pt_end = pt_end_vec[i];

		// pt_start.x /= 4;
		// pt_start.y /= 4;
		// pt_end.x /= 4;
		// pt_end.y /= 4;

		// if (pt_start.x - scan_count < 0 || pt_start.x + scan_count > WIDTH_SMALL_MINUS ||
		// 	pt_end.x - scan_count < 0 || pt_end.x + scan_count > WIDTH_SMALL_MINUS)
		// 		continue;

		// int gray00 = image_in.ptr<uchar>(pt_start.y, pt_start.x - scan_count)[0];
		// int gray01 = image_in.ptr<uchar>(pt_start.y, pt_start.x + scan_count)[0];
		// int gray_diff0 = abs(gray00 - gray01);

		// int gray10 = image_in.ptr<uchar>(pt_end.y, pt_end.x - scan_count)[0];
		// int gray11 = image_in.ptr<uchar>(pt_end.y, pt_end.x + scan_count)[0];
		// int gray_diff1 = abs(gray10 - gray11);

		// int gray_diff_max = max(gray_diff0, gray_diff1);
		// gray_diff_vec.push_back(gray_diff_max);

		line(image_in, pt_start, pt_end, Scalar(255), 2);
	}

	sort(gray_diff_vec.begin(), gray_diff_vec.end());

	imshow("image_in_asdasda", image_in);
	waitKey(0);
}