#include "stereo_processor_overlap.h"

struct BlobPairOverlap
{
	BlobNew* blob0;
	BlobNew* blob1;
	int overlap;

	BlobPairOverlap(BlobNew* _blob0, BlobNew* _blob1, int _overlap)
	{
		blob0 = _blob0;
		blob1 = _blob1;
		overlap = _overlap;
	};
};

struct compare_blob_pair_overlap
{
	bool operator() (const BlobPairOverlap& blob_pair0, const BlobPairOverlap& blob_pair1)
	{
		return (blob_pair0.overlap < blob_pair1.overlap);
	}
};

void compute_stereo_overlap(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1,
							PointResolver& point_resolver, PointerMapper& pointer_mapper, Mat& image0, Mat& image1)
{
	Point pt_y_max0 = get_y_max_point(mono_processor0.fingertip_points);
	Point pt_y_max1 = get_y_max_point(mono_processor1.fingertip_points);
	int alignment_y_diff = pt_y_max0.y - pt_y_max1.y;
	int alignment_x_diff = mono_processor0.pt_alignment.x - mono_processor1.pt_alignment.x;

	vector<BlobPairOverlap> blob_pair_vec;
	for (BlobNew& blob0 : mono_processor0.fingertip_blobs)
		for (BlobNew& blob1 : mono_processor1.fingertip_blobs)
		{
			int overlap = blob0.compute_overlap(blob1, alignment_x_diff, alignment_y_diff, 5);
			blob_pair_vec.push_back(BlobPairOverlap(&blob0, &blob1, overlap));
		}
	sort(blob_pair_vec.begin(), blob_pair_vec.end(), compare_blob_pair_overlap());
}