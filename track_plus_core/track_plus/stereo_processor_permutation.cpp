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

void stereo_processor_permutation_compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1,
										  PointResolver& point_resolver, PointerMapper& pointer_mapper, Mat& image0, Mat& image1)
{
	vector<Point>* large_array;
	vector<Point>* small_array;
	MonoProcessorNew* mono_processor_large;
	MonoProcessorNew* mono_processor_small;
	if (mono_processor0.fingertip_points.size() > mono_processor1.fingertip_points.size())
	{
		large_array = &mono_processor0.fingertip_points;
		small_array = &mono_processor1.fingertip_points;
		mono_processor_large = &mono_processor0;
		mono_processor_small = &mono_processor1;
	}
	else
	{
		large_array = &mono_processor1.fingertip_points;
		small_array = &mono_processor0.fingertip_points;
		mono_processor_large = &mono_processor1;
		mono_processor_small = &mono_processor0;
	}

	const int large_array_size = large_array->size() > 5 ? 5 : large_array->size();
	const int small_array_size = small_array->size() > 5 ? 5 : small_array->size();

	compute_permutations(large_array_size, small_array_size);

	float dist_sigma_min = 9999;
	StereoPair stereo_pair_dist_sigma_min;

	Point pt_y_max_large = get_y_max_point(*large_array);
	Point pt_y_max_small = get_y_max_point(*small_array);
	int alignment_y_diff = pt_y_max_large.y - pt_y_max_small.y;
	int alignment_x_diff = mono_processor_large->pt_alignment.x - mono_processor_small->pt_alignment.x;

	for (vector<int>& rows : permutations)
	{
		float dist_sigma = 0;
		StereoPair stereo_pair;

		int small_array_index = 0;
		for (int large_array_index : rows)
		{
			stereo_pair.push_large_index(large_array_index);
			stereo_pair.push_small_index(small_array_index);

			Point pt_small_array = (*small_array)[small_array_index];
			Point pt_large_array = (*large_array)[large_array_index];

			pt_small_array.x += alignment_x_diff;
			pt_small_array.y += alignment_y_diff;

			float dist = get_distance(pt_small_array, pt_large_array);
			dist_sigma += dist;

			++small_array_index;
		}
		if (dist_sigma < dist_sigma_min)
		{
			dist_sigma_min = dist_sigma;
			stereo_pair_dist_sigma_min = stereo_pair;
		}
	}

	stereo_pair_dist_sigma_min.compute(large_array, small_array, mono_processor_large->fingertip_blobs, mono_processor_small->fingertip_blobs);

	Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	for (int i = 0; i < stereo_pair_dist_sigma_min.pt_vec_large_sorted.size(); ++i)
	{
		Point pt_large = stereo_pair_dist_sigma_min.pt_vec_large_sorted[i];
		Point pt_small = stereo_pair_dist_sigma_min.pt_vec_small_sorted[i];

		BlobNew* blob_large = stereo_pair_dist_sigma_min.blob_vec_large_sorted[i];
		BlobNew* blob_small = stereo_pair_dist_sigma_min.blob_vec_small_sorted[i];

		blob_large->fill(image_visualization, 80);
		blob_small->fill(image_visualization, 100);

		circle(image_visualization, pt_large, 3, Scalar(127), -1);
		circle(image_visualization, pt_small, 3, Scalar(254), 1);
		line(image_visualization, pt_large, pt_small, Scalar(127), 1);
	}

	imshow("image_visualizationiuhuewli", image_visualization);
}