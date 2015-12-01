#include "stereo_processor_new.h"

void StereoProcessorNew::compute(SCOPA& scopa0, SCOPA& scopa1,
									PointResolver& point_resolver, PointerMapper& pointer_mapper, Mat& image0, Mat& image1, bool visualize)
{
	static const int blob_vec_cache_num = 3;
	static int blob_vec_cached_count = -1;
	static bool begin_action = false;

	++blob_vec_cached_count;
	if (blob_vec_cached_count == blob_vec_cache_num)
	{
		blob_vec_cached_count = 0;
		begin_action = true;
	}

	vector<vector<BlobNew>> blob_vec_pair_new(2);
	blob_vec_pair_new[0] = scopa0.fingertip_blobs;
	blob_vec_pair_new[1] = scopa1.fingertip_blobs;

	static vector<vector<BlobNew>> blob_vec_cache[blob_vec_cache_num];
	blob_vec_cache[blob_vec_cached_count] = blob_vec_pair_new;

	if (!begin_action)
		return;

	int index_cache = blob_vec_cached_count - (blob_vec_cache_num - 1);
	if (index_cache < 0)
		index_cache = blob_vec_cache_num + index_cache;

	vector<vector<BlobNew>> blob_vecs_current = blob_vec_cache[index_cache];
	vector<vector<BlobNew>> blob_vecs_latest = blob_vec_cache[blob_vec_cached_count];

	Mat image_visualization = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	for (BlobNew& blob : blob_vecs_current[0])
		blob.fill(image_visualization, 254);

	for (BlobNew& blob : blob_vecs_current[1])
		blob.fill(image_visualization, 254);

	imshow("image_visualizationasdgkljh", image_visualization);
}