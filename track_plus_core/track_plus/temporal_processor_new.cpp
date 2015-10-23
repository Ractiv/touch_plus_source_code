#include "temporal_processor_new.h"

void TemporalProcessor::compute(StereoProcessor& stereo_processor)
{
	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC3);
	for (Point3f& pt : stereo_processor.pt3d_vec)
		circle(image_visualization, Point(pt.x + 320, pt.y + 240), pow(1000 / (pt.z + 1), 2), Scalar(254, 254, 254), 2);

	imshow("image_visualization", image_visualization);
}