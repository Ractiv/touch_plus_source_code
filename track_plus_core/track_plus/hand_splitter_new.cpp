#include "hand_splitter_new.h"

bool HandSplitterNew::compute(ForegroundExtractorNew& foreground_extractor, MotionProcessorNew& motion_processor, const string name)
{
	motion_processor_ptr = &motion_processor;

	if (foreground_extractor.blob_detector.blobs->size() == 0)
		return false;

	Mat image_in = foreground_extractor.image_foreground;

	if (image_in.cols == 0)
		return false;

	Mat image_small;
	resize(image_in, image_small, Size(WIDTH_MIN, HEIGHT_MIN), 0, 0, INTER_LINEAR);
	threshold(image_small, image_small, 0, 254, THRESH_BINARY);

	Mat image_dilated;
	dilate(image_small, image_dilated, Mat(), Point(-1, -1), 1);

	Mat image_histogram;
	int x_min_imhg;
	int x_max_imhg;
	int y_min_imhg;
	int y_max_imhg;
	histogram_builder.compute_horizontal(image_dilated, image_histogram, 19, x_min_imhg, x_max_imhg, y_min_imhg, y_max_imhg);

	blob_detector_image_histogram.compute(image_histogram, 254, x_min_imhg, x_max_imhg, y_min_imhg, y_max_imhg, true);

	if (blob_detector_image_histogram.blobs->size() == 0)
		return false;

	blob_detector_image_histogram.sort_blobs_by_count();

	for (BlobNew& blob : *(blob_detector_image_histogram.blobs))
	{
		blob.width *= 2;
		blob.height *= 2;
		blob.x *= 2;
		blob.y *= 2;
		blob.x_min *= 2;
		blob.x_max *= 2;
		blob.y_min *= 2;
		blob.y_max *= 2;
	}

	blob_tracker.compute(*blob_detector_image_histogram.blobs);

	x_middle_median = motion_processor.x_separator_middle_median;

	if (count_left_vec.size() < 100)
	{
		//getting count_left_median and count_right_median
		int count_left = 0;
		int count_right = 0;

		for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
			for (Point& pt : blob.data)
				if (pt.x < x_middle_median)
					++count_left;
				else
					++count_right;

		count_left_vec.push_back(count_left);
		count_right_vec.push_back(count_right);
		sort(count_left_vec.begin(), count_left_vec.end());
		sort(count_right_vec.begin(), count_right_vec.end());
		count_left_median = count_left_vec[count_left_vec.size() / 2];
		count_right_median = count_right_vec[count_right_vec.size() / 2];
	}

	bool dont_place_x_middle = false;

	if (blob_detector_image_histogram.blobs->size() >= 2)
	{
		const float width0 = (*blob_detector_image_histogram.blobs)[0].width;
		const float height0 = (*blob_detector_image_histogram.blobs)[0].height;
		const float width1 = (*blob_detector_image_histogram.blobs)[1].width;
		const float height1 = (*blob_detector_image_histogram.blobs)[1].height;

		const float height_min = min(height0, height1);
		const float height_max = max(height0, height1);

		const float wh_ratio0 = width0 / height0;
		const float wh_ratio1 = width1 / height1;
		const float min_max_ratio = height_min / height_max;

		const bool bool0 = min_max_ratio > 0.7 && height_min > HEIGHT_SMALL / 4 && wh_ratio0 < 1.5 && wh_ratio1 < 1.5;

		int count_left = 0;
		int count_right = 0;

		for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
			for (Point& pt : blob.data)
				if (pt.x < motion_processor.x_separator_middle)
					++count_left;
				else
					++count_right;

		const float count_left_min = min(count_left, count_left_median);
		const float count_left_max = max(count_left, count_left_median);
		const float count_right_min = min(count_right, count_right_median);
		const float count_right_max = max(count_right, count_right_median);

		const bool bool1 = count_left_min / count_left_max > 0.7 && count_right_min / count_right_median > 0.7;

		if (bool0 || bool1)
		{
			BlobNew* blob_left;
			BlobNew* blob_right;

			if ((*blob_detector_image_histogram.blobs)[0].x < (*blob_detector_image_histogram.blobs)[1].x)
			{
				blob_left = &((*blob_detector_image_histogram.blobs)[0]);
				blob_right = &((*blob_detector_image_histogram.blobs)[1]);
			}
			else
			{
				blob_left = &((*blob_detector_image_histogram.blobs)[1]);
				blob_right = &((*blob_detector_image_histogram.blobs)[0]);
			}

			if (blob_left->x_max < x_middle_median && blob_right->x_min > x_middle_median)
			{
				id0_new = (*blob_detector_image_histogram.blobs)[0].id;
				id1_new = (*blob_detector_image_histogram.blobs)[1].id;
				motion_processor.x_separator_middle = (blob_left->x_max + blob_right->x_min) / 2;
				merged = false;
				dont_place_x_middle = true;
			}
		}
	}

	BlobNew* blob0 = find_blob_by_id(id0);
	BlobNew* blob1 = find_blob_by_id(id1);

	if (blob0 == NULL && blob1 == NULL)
	{
		id0 = id0_new;
		id1 = id1_new;

		blob0 = find_blob_by_id(id0);
		blob1 = find_blob_by_id(id1);
	}

	if (blob_tracker.merge)
	{
		for (BlobNew& blob : *blob_detector_image_histogram.blobs)
			if (blob.merge)
			{
				bool found0 = false;
				bool found1 = false;

				for (int index_a : blob.index_vec)
				{
					bool break_now = false;
					for (int index_b : index_vec0)
						if (index_a == index_b)
						{
							found0 = true;
							break_now = true;
							break;
						}
					if (break_now)
						break;
				}
				for (int index_a : blob.index_vec)
				{
					bool break_now = false;
					for (int index_b : index_vec1)
						if (index_a == index_b)
						{
							found1 = true;
							break_now = true;
							break;
						}
					if (break_now)
						break;
				}
				if (found0 && found1)
				{
					merged = true;
					break;
				}
			}
	}

	if (blob0 != NULL)
		index_vec0 = blob0->index_vec;
	if (blob1 != NULL)
		index_vec1 = blob1->index_vec;

	bool place_x_middle = false;

	if (blob0 == NULL && blob1 == NULL)
	{
		merged = false;

		if (!dont_place_x_middle)
			for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
			{
				bool break_now = false;

				for (Point& pt : blob.data)
				{
					place_x_middle = true;
					break_now = true;
					break;
				}

				if (break_now)
					break;
			}
	}

	if (!merged)
	{
		const int x_middle_old = motion_processor.x_separator_middle;

		BlobNew* blob_left = NULL;
		BlobNew* blob_right = NULL;
		if (blob0 != NULL && blob1 != NULL)
		{
			if (blob0->x > blob1->x)
			{
				blob_right = blob0;
				blob_left = blob1;
			}
			else
			{
				blob_right = blob1;
				blob_left = blob0;
			}

			move_x_middle(image_histogram, motion_processor, (blob_left->x_max + blob_right->x_min) / 2);
		}

		else if (blob0 != NULL)
			move_x_middle(image_histogram, motion_processor, blob0->x);
		else if (blob1 != NULL)
			move_x_middle(image_histogram, motion_processor, blob1->x);

		//shift x_middle using hit test
		const int x_middle_half = motion_processor.x_separator_middle / 2;

		for (BlobNew& blob : *blob_detector_image_histogram.blobs)
		{
			bool hit = false;

			for (int j = 0; j < HEIGHT_MIN; ++j)
				if (blob.image_atlas.ptr<ushort>(j, x_middle_half)[0] == blob.atlas_id)
				{
					if (blob0 != NULL && blob1 == NULL)
					{
						id0 = blob.id;
						blob0 = find_blob_by_id(id0);
					}
					else if (blob1 != NULL && blob0 == NULL)
					{
						id1 = blob.id;
						blob1 = find_blob_by_id(id1);
					}
					hit = true;
					break;	
				}

			if (hit)
			{
				if (place_x_middle)
				{
					vector<BlobNew*> foreground_blobs;

					for (BlobNew& blob_foreground : *foreground_extractor.blob_detector.blobs)
						for (Point pt : blob.data)
							if (blob_foreground.image_atlas.ptr<ushort>(pt.y * 2, pt.x * 2)[0] == blob_foreground.atlas_id)
							{
								foreground_blobs.push_back(&blob_foreground);
								break;
							}

					int y_max = 0;
					int y_min = 9999;

					for (BlobNew*& blob_foreground : foreground_blobs)
						for (Point& pt : blob_foreground->data)
						{
							if (pt.y > y_max)
								y_max = pt.y;
							if (pt.y < y_min)
								y_min = pt.y;
						}
							
					const int y_cap = (y_max - y_min) * 0.25 + y_min;
					vector<int> x_vec;

					for (BlobNew*& blob_foreground : foreground_blobs)
						for (Point& pt : blob_foreground->data)
							if (pt.y < y_cap)
								x_vec.push_back(pt.x);

					if (x_vec.size() > 0)
					{
						sort(x_vec.begin(), x_vec.end());
						const int x_pivot = x_vec[x_vec.size() / 2];

						if (x_pivot > x_middle_median)
							motion_processor.x_separator_middle = blob.x_min;
						else
							motion_processor.x_separator_middle = blob.x_max;
					}

					id0 = blob.id;
				}
				else
				{
					BlobNew* blob_in_history;
					if (blob_tracker.find_in_history(blob.id, 1, blob_in_history))
						if (x_middle_old < blob_in_history->x)
							motion_processor.x_separator_middle = blob.x_min;
						else
							motion_processor.x_separator_middle = blob.x_max;
				}

				break;
			}
		}

		//set values for split detection
		x_min_on_merge = 9999;
		x_max_on_merge = 0;
		for (BlobNew& blob : *blob_detector_image_histogram.blobs)
		{
			if (blob.x_min < x_min_on_merge)
				x_min_on_merge = blob.x_min;
			if (blob.x_max > x_max_on_merge)
				x_max_on_merge = blob.x_max;
		}

		if (blob_left != NULL && blob_right != NULL)
		{
			count_left_on_merge = blob_left->count;
			count_right_on_merge = blob_right->count;
		}
	}
	else if (blob_tracker.split && blob0 != NULL && blob1 != NULL)
	{
		BlobNew* blob_left = NULL;
		BlobNew* blob_right = NULL;
		if (blob0->x > blob1->x)
		{
			blob_right = blob0;
			blob_left = blob1;
		}
		else
		{
			blob_right = blob1;
			blob_left = blob0;
		}

		if (blob_left->x_max < x_middle_median && blob_right->x_min > x_middle_median && (id0 != id0_new || id1 != id1_new))
			merged = false;

		if (merged)
		{
			const int count_left_on_split = blob_left->count;
			const int count_right_on_split = blob_right->count;

			if (count_left_on_split > (count_left_on_merge / 2) && count_right_on_split > (count_right_on_merge / 2))
			{
				int x_min_on_split = 9999;
				int x_max_on_split = 0;
				for (BlobNew& blob : *blob_detector_image_histogram.blobs)
				{
					if (blob.x_min < x_min_on_split)
						x_min_on_split = blob.x_min;
					if (blob.x_max > x_max_on_split)
						x_max_on_split = blob.x_max;
				}
				const int x_min_diff = abs(x_min_on_split - x_min_on_merge);
				const int x_max_diff = abs(x_max_on_split - x_max_on_merge);

				if (min(x_min_diff, x_max_diff) < 10)
					merged = false;
			}
		}
	}

	if ((blob0 == NULL || blob1 == NULL) && (blob0 != NULL || blob1 != NULL))
	{
		BlobNew* blob_tracking = NULL;
		if (blob0 != NULL)
			blob_tracking = blob0;
		else if (blob1 != NULL)
			blob_tracking = blob1;

		BlobNew* blob_selected = NULL;
		if (blob_tracking != NULL)
		{
			for (BlobNew& blob : *blob_detector_image_histogram.blobs)
			{
				const float height_min = min(blob.height, blob_tracking->height);
				const float height_max = max(blob.height, blob_tracking->height);

				if (height_min / height_max > 0.7 && blob.id != blob_tracking->id)
				{
					bool hit = false;

					for (int j = 0; j < HEIGHT_MIN; ++j)
						if (blob.image_atlas.ptr<ushort>(j, x_middle_median)[0] == blob.atlas_id)
						{
							hit = true;
							break;
						}

					if (!hit)
						blob_selected = &blob;
				}
			}
		}
		if (blob_selected != NULL)
		{
			if (blob0 == NULL)
			{
				id0 = blob_selected->id;
				blob0 = find_blob_by_id(id0);
			}
			else if (blob1 == NULL)
			{
				id1 = blob_selected->id;
				blob1 = find_blob_by_index(id1);
			}
		}
	}

	if (blob0 == NULL && blob1 == NULL)
		move_x_middle(image_histogram, motion_processor, x_middle_median);

	primary_hand_blobs = vector<BlobNew>();
	x_min_result = 9999;
	x_max_result = 0;
	y_min_result = 9999;
	y_max_result = 0;

	for (BlobNew& blob : *foreground_extractor.blob_detector.blobs)
		if (blob.active)
			if (blob.x > motion_processor.x_separator_middle)
			{
				primary_hand_blobs.push_back(blob);

				if (blob.x_min < x_min_result)
					x_min_result = blob.x_min;
				if (blob.x_max > x_max_result)
					x_max_result = blob.x_max;
				if (blob.y_min < y_min_result)
					y_min_result = blob.y_min;
				if (blob.y_max > y_max_result)
					y_max_result = blob.y_max;
			}

	if (false)
	{
		for (BlobNew& blob : *blob_detector_image_histogram.blobs)
		{
			putText(image_histogram, to_string(blob.id), cvPoint(blob.x / 2, blob.y / 2 < 20 ? 20 : blob.y / 2), 
		    	    FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(127, 127, 127), 1, CV_AA);

			/*int y_add = 30;
			for (int index : blob.index_vec)
			{
				putText(image_histogram, to_string(index), cvPoint(blob.x / 2, blob.y / 2 + y_add), 
		    	    	FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(100, 100, 100), 1, CV_AA);
				
				y_add += 10;
			}*/
		}
		line(image_in, Point(x_middle_median, 0), Point(x_middle_median, HEIGHT_SMALL), Scalar(127), 2);
		imshow("image_in" + name, image_in);
		// imshow("image_histogram" + name, image_histogram);
	}

	if (primary_hand_blobs.size() > 0)
		return true;
	else
		return false;
}

BlobNew* HandSplitterNew::find_blob_by_id(const int id_in)
{
	for (BlobNew& blob : *blob_detector_image_histogram.blobs)
		if (blob.id == id_in)
			return &blob;

	return NULL;
}

BlobNew* HandSplitterNew::find_blob_by_index(const int index_in)
{
	for (BlobNew& blob : *blob_detector_image_histogram.blobs)
		for (int index : blob.index_vec)
			if (index == index_in)
				return &blob;

	return NULL;
}

void HandSplitterNew::move_x_middle(Mat& image_histogram, MotionProcessorNew& motion_processor, const int x_in)
{
	int x0 = motion_processor.x_separator_middle / 2;
	const int x1 = x_in / 2;
	const char increment = x1 > x0 ? 1 : -1;

	while (x0 != x1)
	{
		for (int j = 0; j < HEIGHT_MIN; ++j)
			if (image_histogram.ptr<uchar>(j, x0)[0] > 0)
			{
				motion_processor.x_separator_middle = (x0 - increment) * 2;
				return;
			}
		x0 += increment;
	}
	motion_processor.x_separator_middle = x_in;
}