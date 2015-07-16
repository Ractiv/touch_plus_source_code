#include "stereo_processor.h"

bool StereoProcessor::compute(MonoProcessorNew& mono_processor0, MonoProcessorNew& mono_processor1,
							  MotionProcessorNew& motion_processor0, MotionProcessorNew& motion_processor1)
{
	int x_diff = motion_processor0.x_separator_middle_median - motion_processor1.x_separator_middle_median;
	int y_diff = motion_processor0.y_separator_motion_down_median - motion_processor1.y_separator_motion_down_median;

	vector<OverlappingBlobPair> overlapping_blob_pair_vec;

	int index0 = 0;
	int index1 = 0;

	for (BlobNew& blob0 : *mono_processor0.blob_detector_image_hand.blobs)
	{
		index1 = 0;
		for (BlobNew& blob1 :  *mono_processor1.blob_detector_image_hand.blobs)
		{
			int overlap_count = blob0.compute_overlap(blob1, -x_diff, -y_diff, 10);
			int y_diff_current = blob0.y_max - blob1.y_max;

			overlap_count += 50 / (abs(y_diff_current - y_diff) + 1);

			overlapping_blob_pair_vec.push_back(OverlappingBlobPair(&blob0, &blob1, overlap_count, index0, index1));
			++index1;
		}
		++index0;
	}

	sort(overlapping_blob_pair_vec.begin(), overlapping_blob_pair_vec.end(), compare_overlap_count());

	bool checker0[100];
	bool checker1[100];

	matches.clear();

	for (OverlappingBlobPair& pair : overlapping_blob_pair_vec)
	{
		bool occupied0 = checker0[pair.index0] == true;
		bool occupied1 = checker1[pair.index1] == true;

		if (!occupied0 && !occupied1)
		{
			checker0[pair.index0] = true;
			checker1[pair.index1] = true;

			matches.push_back(pair);
		}
	}

	if (matches.size() == 0)
		return false;

	Mat image_visualization_stereo_processor = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	for (OverlappingBlobPair& pair : matches)
	{
		pair.blob0->fill(image_visualization_stereo_processor, 254);
		pair.blob1->fill(image_visualization_stereo_processor, 127);
	}

	for (OverlappingBlobPair& pair : matches)
		line(image_visualization_stereo_processor, pair.blob0->pt_y_max, pair.blob1->pt_y_max, Scalar(64), 2);

	imshow("image_visualization_stereo_processor", image_visualization_stereo_processor);
	return true;
}