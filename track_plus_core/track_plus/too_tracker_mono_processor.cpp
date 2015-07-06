#include "tool_tracker_mono_processor.h"

void ToolTrackerMonoProcessor::compute(Mat& image_in, const string name)
{
	static Scalar Colors[] = { Scalar(255, 0, 0),
		                       Scalar(0, 255, 0),
		                       Scalar(0, 0, 255),
							   Scalar(255, 255, 0),
	                           Scalar(0, 255, 255),
							   Scalar(255, 0, 255),
							   Scalar(255, 127, 255),
							   Scalar(127, 0, 255),
							   Scalar(127, 0, 127) };

	static CTracker tracker = CTracker(0.2, 0.5, 9999, 0, 10);

	static ValueStore value_store;

	Mat image_thresholded;
	threshold(image_in, image_thresholded, 150, 254, THRESH_BINARY);

	BlobDetectorNew* blob_detector = value_store.get_blob_detector("blob_detector");
	blob_detector->compute(image_thresholded, 254, 0, WIDTH_SMALL, 0, HEIGHT_SMALL, true);

	static int track_id_max = -1;

	Mat image_visualization;
	cvtColor(image_thresholded, image_visualization, CV_GRAY2BGR);

	vector<Point2f> points_to_track;
	for (BlobNew& blob : *(blob_detector->blobs))
		points_to_track.push_back(Point2f(blob.x, blob.y));

	if(points_to_track.size() > 0)
	{
		tracker.Update(points_to_track);

		for (int i = 0; i < tracker.tracks.size(); ++i)
		{
			int track_id = tracker.tracks[i]->track_id;
			if (track_id > track_id_max)
				track_id_max = track_id;

			if (tracker.tracks[i]->trace.size() > 1)
				for(int j = 0; j < tracker.tracks[i]->trace.size() - 1; ++j)
					line(image_visualization,
						 tracker.tracks[i]->trace[j],
					  	 tracker.tracks[i]->trace[j+1],
					 	 Colors[tracker.tracks[i]->track_id % 9],
						 2,
						 CV_AA);
		}
	}

	imshow("image_visualization" + name, image_visualization);	
}