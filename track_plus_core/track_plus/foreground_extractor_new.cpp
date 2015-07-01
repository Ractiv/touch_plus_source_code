#include "foreground_extractor_new.h"

void ForegroundExtractorNew::init()
{
	blob_detector = BlobDetectorNew();
	image_foreground = Mat();
}

bool ForegroundExtractorNew::compute(Mat& image_in, Mat& image_smoothed_in,
									 MotionProcessorNew& motion_processor, const string name, const bool visualize)
{
	Mat image_foreground_smoothed = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			const uchar gray_current = image_smoothed_in.ptr<uchar>(j, i)[0];

			if (gray_current > motion_processor.gray_threshold)
			{
				if (motion_processor.image_background_static.ptr<uchar>(j, i)[0] == 255)
					image_foreground_smoothed.ptr<uchar>(j, i)[0] = 254;
				else
					image_foreground_smoothed.ptr<uchar>(j, i)[0] =
						abs(gray_current - motion_processor.image_background_static.ptr<uchar>(j, i)[0]);
			}
		}

	threshold(image_foreground_smoothed, image_foreground_smoothed, motion_processor.diff_threshold, 254, THRESH_BINARY);
	GaussianBlur(image_foreground_smoothed, image_foreground_smoothed, Size(1, 9), 0, 0);
	threshold(image_foreground_smoothed, image_foreground_smoothed, 100, 254, THRESH_BINARY);

	const uchar gray_threshold_new = motion_processor.gray_threshold * 1;
	const uchar diff_threshold_new = motion_processor.diff_threshold * 1;

	Mat image_foreground_regular = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
		{
			const uchar gray_current = image_in.ptr<uchar>(j, i)[0];

			if (gray_current > gray_threshold_new)
			{
				if (motion_processor.image_background_static.ptr<uchar>(j, i)[0] == 255)
					image_foreground_regular.ptr<uchar>(j, i)[0] = 254;
				else
					image_foreground_regular.ptr<uchar>(j, i)[0] =
						abs(gray_current - motion_processor.image_background_static.ptr<uchar>(j, i)[0]);
			}
		}

	threshold(image_foreground_regular, image_foreground_regular, diff_threshold_new, 254, THRESH_BINARY);

	image_foreground = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	for (int i = 0; i < WIDTH_SMALL; ++i)
		for (int j = 0; j < HEIGHT_SMALL; ++j)
			if (image_foreground_smoothed.ptr<uchar>(j, i)[0] > 0 && image_foreground_regular.ptr<uchar>(j, i)[0] > 0)
				image_foreground.ptr<uchar>(j, i)[0] = 254;

	blob_detector.compute(image_foreground, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, false);
	for (BlobNew& blob : *blob_detector.blobs)
		if (blob.count < motion_processor.noise_size ||
			blob.width <= 3 || blob.height <= 3 ||
			blob.y > motion_processor.y_separator_motion_down_median ||
			blob.x_max < motion_processor.x_separator_motion_left_median ||
			blob.x_min > motion_processor.x_separator_motion_right_median)
		{
			blob.active = false;
			blob.fill(image_foreground, 0);
		}

	if (visualize && enable_imshow)
		imshow("image_foreground" + name, image_foreground);

	return true;
}