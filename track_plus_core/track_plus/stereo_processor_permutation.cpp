#include "stereo_processor_permutation.h"
#include "permutation.h"
#include "contour_functions.h"

struct StereoPair
{
	vector<int> index_vec_large;
	vector<int> index_vec_small;

	vector<Point> pt_vec_large_sorted;
	vector<Point> pt_vec_small_sorted;

	vector<BlobNew*> blob_vec_large_sorted;
	vector<BlobNew*> blob_vec_small_sorted;

	void compute(vector<Point>* pt_vec_large, vector<Point>* pt_vec_small, vector<BlobNew>& blob_vec_large, vector<BlobNew>& blob_vec_small)
	{
		for (int index : index_vec_large)
		{
			pt_vec_large_sorted.push_back((*pt_vec_large)[index]);
			blob_vec_large_sorted.push_back(&blob_vec_large[index]);
		}

		for (int index : index_vec_small)
		{
			pt_vec_small_sorted.push_back((*pt_vec_small)[index]);
			blob_vec_small_sorted.push_back(&blob_vec_small[index]);
		}
	};

	void push_large_index(int index)
	{
		index_vec_large.push_back(index);
	};

	void push_small_index(int index)
	{
		index_vec_small.push_back(index);
	};
};

struct MonoData
{
	vector<Point>* array;
	MonoProcessorNew* mono_processor;
	Mat image;
	uchar side;

	MonoData() { }

	MonoData(vector<Point>* _array, MonoProcessorNew* _mono_processor, Mat _image, uchar _side)
	{
		array = _array;
		mono_processor = _mono_processor;
		image = _image;
		side = _side;
	}
};

void compute_stereo_permutation(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1,
								PointResolver& point_resolver, PointerMapper& pointer_mapper, Mat& image0, Mat& image1)
{
	MonoData mono_data_large;
	MonoData mono_data_small;
	MonoData mono_data0;
	MonoData mono_data1;
	bool side_flipped;
	if (mono_processor0.fingertip_points.size() > mono_processor1.fingertip_points.size())
	{
		mono_data_large = MonoData(&mono_processor0.fingertip_points, &mono_processor0, image0, 0);
		mono_data_small = MonoData(&mono_processor1.fingertip_points, &mono_processor1, image1, 1);
		mono_data0 = mono_data_large;
		mono_data1 = mono_data_small;
		side_flipped = false;
	}
	else
	{
		mono_data_small = MonoData(&mono_processor0.fingertip_points, &mono_processor0, image0, 0);
		mono_data_large = MonoData(&mono_processor1.fingertip_points, &mono_processor1, image1, 1);
		mono_data1 = mono_data_large;
		mono_data0 = mono_data_small;
		side_flipped = true;
	}

	const int large_array_size = mono_data_large.array->size() > 5 ? 5 : mono_data_large.array->size();
	const int small_array_size = mono_data_small.array->size() > 5 ? 5 : mono_data_small.array->size();

	compute_permutations(large_array_size, small_array_size);

	float dist_sigma_min = 9999;
	StereoPair stereo_pair_dist_sigma_min;

	Point pt_y_max_large = get_y_max_point(*mono_data_large.array);
	Point pt_y_max_small = get_y_max_point(*mono_data_small.array);
	int alignment_y_diff = pt_y_max_large.y - pt_y_max_small.y;
	int alignment_x_diff = mono_data_large.mono_processor->pt_alignment.x - mono_data_small.mono_processor->pt_alignment.x;

	for (vector<int>& rows : permutations)
	{
		float dist_sigma = 0;
		StereoPair stereo_pair;

		int small_array_index = 0;
		for (int large_array_index : rows)
		{
			stereo_pair.push_large_index(large_array_index);
			stereo_pair.push_small_index(small_array_index);

			Point pt_small_array = (*mono_data_small.array)[small_array_index];
			Point pt_large_array = (*mono_data_large.array)[large_array_index];

			pt_small_array.x += alignment_x_diff;
			pt_small_array.y += alignment_y_diff;

			float dist = get_distance(pt_small_array, pt_large_array, false);
			dist_sigma += dist;

			++small_array_index;
		}
		if (dist_sigma < dist_sigma_min)
		{
			dist_sigma_min = dist_sigma;
			stereo_pair_dist_sigma_min = stereo_pair;
		}
	}

	stereo_pair_dist_sigma_min.compute(mono_data_large.array,                           mono_data_small.array,
									   mono_data_large.mono_processor->fingertip_blobs, mono_data_small.mono_processor->fingertip_blobs);

	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

	Point pt_resolved_pivot0 = point_resolver.reprojector->remap_point(mono_data0.mono_processor->pt_palm, mono_data0.side, 4);
	Point pt_resolved_pivot1 = point_resolver.reprojector->remap_point(mono_data1.mono_processor->pt_palm, mono_data1.side, 4);

	for (int i = 0; i < stereo_pair_dist_sigma_min.pt_vec_large_sorted.size(); ++i)
	{
		BlobNew* blob_large = stereo_pair_dist_sigma_min.blob_vec_large_sorted[i];
		BlobNew* blob_small = stereo_pair_dist_sigma_min.blob_vec_small_sorted[i];
		BlobNew* blob0 = side_flipped ? blob_small : blob_large;
		BlobNew* blob1 = side_flipped ? blob_large : blob_small;

		Point2f pt_resolved0 = point_resolver.compute(blob0->pt_tip, mono_data0.image, mono_data0.side);
		Point2f pt_resolved1 = point_resolver.compute(blob1->pt_tip, mono_data1.image, mono_data1.side);

#if 0
		circle(image_visualization, pt_resolved0, 5, Scalar(127), 2);
		circle(image_visualization, pt_resolved1, 5, Scalar(254), 2);
		circle(image_visualization, pt_resolved_pivot0, 10, Scalar(127), 2);
		circle(image_visualization, pt_resolved_pivot1, 10, Scalar(254), 2);
#endif

#if 1
		if (pt_resolved0.x != 9999 && pt_resolved1.x != 9999)
		{
			Point3f pt3d = point_resolver.reprojector->reproject_to_3d(pt_resolved0.x, pt_resolved0.y,
																	   pt_resolved1.x, pt_resolved1.y);
			
			circle(image_visualization, Point(320 + pt3d.x, 240 + pt3d.y), pow(1000 / pt3d.z, 2), Scalar(127), 1);
		}
#endif
	}
	imshow("image_visualizationiuhuewli", image_visualization);
}