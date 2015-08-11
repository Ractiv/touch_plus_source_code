#include "surface_computer.h"

void SurfaceComputer::init(Mat& image_in)
{
    Mat image_flipped;
    flip(image_in, image_flipped, 0);

    Mat image0 = image_flipped(Rect(0, 0, 640, 480));

    Mat image_canny;
    Canny(image0, image_canny, 20, 60, 3);

    int y_reflection = HEIGHT_LARGE;

    {
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
 	   // imshow("image_histogram_koiausd", image_histogram);
	}

    vector<Vec4i> lines;
	HoughLinesP(image_canny, lines, 1, CV_PI / 180, 20, 50, 20);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        Vec4i l = lines[i];
        Point pt0 = Point(l[0], l[1]);
        Point pt1 = Point(l[2], l[3]);

        if (pt0.y == pt1.y && pt0.y < y_reflection)
        	line(image_canny, pt0, pt1, Scalar(127), 3);
    }

	int x_min = 9999;
	int x_max = 0;

	{
		Mat image_histogram = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

		int y_max = 0;
	    for (int i = 0; i < WIDTH_LARGE; ++i)
	    {
	    	int intensity = 0;

	    	int j_old = 0;
	    	int j_max = y_reflection;
	    	for (int j = 0; j < j_max; ++j)
	    		if (image_canny.ptr<uchar>(j, i)[0] > 0)
	    		{
	    			intensity += 5 / (j - j_old + 1);
	    			j_old = j;
	    		}

	        low_pass_filter.compute(intensity, 0.1, "intensity");
	    	line(image_histogram, Point(i, 0), Point(i, intensity * 2), Scalar(254), 1);

	    	if (intensity > y_max)
	    		y_max = intensity;
	    }

	    const int j = y_max;
	   	const int i_max = WIDTH_LARGE - 10;
	    for (int i = 10; i < i_max; ++i)
	    	if (image_histogram.ptr<uchar>(j, i)[0] > 0)
	    	{
	    		if (i < x_min)
	    			x_min = i;
	    		if (i > x_max)
	    			x_max = i;
	    	}

	    line(image_histogram, Point(0, j), Point(999, j), Scalar(127), 3);
	    imshow("image_histogram_fjfkdfh", image_histogram);
	}

    Mat image_visualization = image_canny.clone();

    COUT << x_min << " " << x_max << endl;

	line(image_visualization, Point(x_min, 0), Point(x_min, 999), Scalar(127), 3);
	line(image_visualization, Point(x_max, 0), Point(x_max, 999), Scalar(127), 3);

    /*vector<Point> pt_start_vec;
    vector<Point> pt_end_vec;
    vector<int> horizontal_y_vec;

    int x_keyboard = 0;
    int x_keyboard_count = 0;

    vector<Vec4i> lines;
    HoughLinesP(image_canny, lines, 1, CV_PI / 180, 30, 10, 20);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        Vec4i l = lines[i];
        Point pt0 = Point(l[0], l[1]);
        Point pt1 = Point(l[2], l[3]);

        if ((pt0.y + pt1.y) / 2 < y_reflection || true)
        {
            float slope = (float)abs(pt0.y - pt1.y) / ((float)abs(pt0.x - pt1.x) + 0.1);

            if (slope > 2)
	    		line(image_visualization, pt0, pt1, Scalar(127), 3);

       //      if (pt0.y == pt1.y)
       //      {
       //      	bool found = false;
       //      	for (int& horizontal_y : horizontal_y_vec)
       //      		if (abs(horizontal_y - pt0.y) < 15)
       //      		{
       //      			found = true;
       //      			break;
       //      		}

       //      	if (!found)
       //      	{
	      //           // pt_start_vec.push_back(pt0);
	      //           // pt_end_vec.push_back(pt1);
	      //           horizontal_y_vec.push_back(pt0.y);
	      //       }

	      //       int weight = pow(abs(pt0.x - pt1.x), 2);

	      //      	x_keyboard += pt0.x * weight;
	      //      	x_keyboard_count += weight;

	      //      	x_keyboard += pt1.x * weight;
	      //      	x_keyboard_count += weight;

	    		// line(image_visualization, pt0, pt1, Scalar(127), 3);
       //      }
       //      else if (pt0.x < pt1.x && ((pt0.x + pt1.x) / 2) < (WIDTH_LARGE / 2) && pt0.y > pt1.y && slope > 0.5)
       //      {
       //          pt_start_vec.push_back(pt0);
       //          pt_end_vec.push_back(pt1);
    			// // line(image_visualization, pt0, pt1, Scalar(127), 2);
       //      }
       //      else if (pt0.x < pt1.x && ((pt0.x + pt1.x) / 2) > (WIDTH_LARGE / 2) && pt0.y < pt1.y && slope > 0.5)
       //      {
       //          pt_start_vec.push_back(pt0);
       //          pt_end_vec.push_back(pt1);
    			// // line(image_visualization, pt0, pt1, Scalar(127), 2);
       //      }
        }
    }*/

   /* x_keyboard /= x_keyboard_count;
    line(image_visualization, Point(x_keyboard, 0), Point(x_keyboard, 999), Scalar(127), 3);*/

    // for (int i = 0; i < pt_start_vec.size(); ++i)
    // {
    // 	Point pt_start = pt_start_vec[i];
    // 	Point pt_end = pt_end_vec[i];

    // 	// line(image_visualization, pt_start, pt_end, Scalar(127), 3);

    // 	Point pt_intersection0;
    // 	Point pt_intersection1;
    // 	bool b0 = get_intersection_at_y(pt_start, pt_end, 0, pt_intersection0);
    // 	bool b1 = get_intersection_at_y(pt_start, pt_end, y_reflection, pt_intersection1);

    // 	if (b0 && b1)
    // 		line(image_visualization, pt_intersection0, pt_intersection1, Scalar(127), 3);
    // }

    line(image_visualization, Point(0, y_reflection), Point(999, y_reflection), Scalar(127), 3);

    imshow("image_visualization_asdasd", image_visualization);
    imshow("image0_asdasd", image0);
    waitKey(0);
}