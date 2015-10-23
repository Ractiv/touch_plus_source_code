#include "temporal_processor.h"
#include "c_tracker.h"

struct Trace2D
{
	float x;
	float y;
	float z;
	int id;
	int size;
	int size_total;
	Point2f point;
	vector<Point2f> points;
	float dist;
	float confidence;

	Trace2D() {}

	Trace2D(Point2f& point_in, int id_in, vector<Point2f>& points_in, float dist_in, int size_total_in, float confidence_in, float z_in)
	{
		x = point_in.x;
		y = point_in.y;
		point = point_in;
		id = id_in;
		points = points_in;
		dist = dist_in;
		size = points_in.size();
		size_total = size_total_in;
		confidence = confidence_in;
		z = z_in;
	}
};

Scalar Colors[] = { Scalar(255, 0, 0),
					Scalar(0, 255, 0),
					Scalar(0, 0, 255),
					Scalar(255, 255, 0),
					Scalar(0, 255, 255),
					Scalar(255, 0, 255),
					Scalar(255, 127, 255),
					Scalar(127, 0, 255),
					Scalar(127, 0, 127) };

CTracker tracker = CTracker(0.1, 0.5, 9999, 0, 10);

const int trace_frames_back_num = 2;
const int trace_frames_count_max = 1000;

int trace_frames_count = -1;
int trace_frames_count_total = -1;
vector<Trace2D> trace_frames[trace_frames_count_max];

vector<Trace2D> get_trace_frame(int frame)
{
	if (frame < 0)
		frame = trace_frames_count_max - frame;

	return trace_frames[frame];
}

void TemporalProcessor::compute(StereoProcessor& stereo_processor)
{
	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC3);

	vector<Point2f> points_to_track;
	unordered_map<string, float> confidence_map;
	unordered_map<string, float> z_map;

	int index = 0;
	for (Point3f& pt3d : stereo_processor.pt3d_vec)
	{
		Point position = Point(320 + pt3d.x, 240 + pt3d.y);

		string key = to_string((int)position.x) + "!" + to_string((int)position.y);
		confidence_map[key] = stereo_processor.confidence_vec[index];
		z_map[key] = pt3d.z;

		points_to_track.push_back(position);
		++index;
	}

	if (points_to_track.size() == 0)
		return;

	tracker.Update(points_to_track);

	vector<Trace2D> trace_frame;
	for (int i = 0; i < tracker.tracks.size(); ++i)
	{
		int track_id = tracker.tracks[i]->track_id;

		if (tracker.tracks[i]->trace.size() > 1)
		{
			const int trace_size = tracker.tracks[i]->trace.size();
			float distance = get_distance(tracker.tracks[i]->trace[trace_size - 1], tracker.tracks[i]->trace[trace_size - 2], true);
			tracker.tracks[i]->distance_travelled += distance;
			++tracker.tracks[i]->size_total;
		}

		string key = to_string((int)tracker.tracks[i]->raw.x) + "!" + to_string((int)tracker.tracks[i]->raw.y);
		Trace2D trace = Trace2D(tracker.tracks[i]->raw,
                                tracker.tracks[i]->track_id,
                                tracker.tracks[i]->trace,
                                tracker.tracks[i]->distance_travelled,
                                tracker.tracks[i]->size_total,
                                confidence_map[key],
                                z_map[key]);

		trace_frame.push_back(trace);
	}

	++trace_frames_count;
	++trace_frames_count_total;
	if (trace_frames_count == trace_frames_count_max)
		trace_frames_count = 0;

	trace_frames[trace_frames_count] = trace_frame;

	if (trace_frames_count < trace_frames_back_num && trace_frames_count == trace_frames_count_total)
		return;

	vector<Trace2D> trace_frame_time_shift = get_trace_frame(trace_frames_count - trace_frames_back_num);

	for (Trace2D trace : trace_frame_time_shift)
	{
		bool found = false;
		for (Trace2D trace_latest : trace_frame)
			if (trace_latest.id == trace.id)
			{
				found = true;
				break;
			}

		if (found /*|| trace.confidence > 0.5*/)
		{
			Point2f pt_old = Point2f(-1, -1);
			for (Point2f& pt_new : trace.points)
			{
				if (pt_old.x != -1 && pt_old.y != -1)
					line(image_visualization, pt_old, pt_new, Colors[trace.id % 9], 2, CV_AA);
				
				pt_old = pt_new;
			}

			circle(image_visualization, trace.point, pow(1000 / (trace.z + 1), 2), Colors[trace.id % 9], 2);
			put_text(to_string(trace.id), image_visualization, trace.point.x, trace.point.y);
		}
	}

	// for (Trace2D& trace : frame_cached.trace_vec)
	// {
	// 	if (trace.x != 0 && trace.y != HEIGHT_SMALL && (trace.dist / trace.size_total) > 1)
	// 	{
			
	// 	}
	// }

	/*++pt3d_vec_frame_real;

	++pt3d_vec_frame;
	if (pt3d_vec_frame == pt3d_vec_timeline_length)
		pt3d_vec_frame = 0;

	pt3d_vec_timeline[pt3d_vec_frame] = stereo_processor.pt3d_vec;

	if (pt3d_vec_frame == pt3d_vec_frame_real && pt3d_vec_frame < frames_back)
		return;

	Mat image_visualization = Mat::zeros(HEIGHT_LARGE, WIDTH_LARGE, CV_8UC1);

	for (int i = 0; i <= frames_back; ++i)
	{
		vector<Point3f> pt3d_vec = get_pt3d_vec_at_frame(pt3d_vec_frame - i);
		for (Point3f& pt3d : pt3d_vec)
		{
			// circle(image_visualization, Point(320 + pt3d.x, 240 + pt3d.y), 5, Scalar(254), -1);
			circle(image_visualization, Point(320 + pt3d.x, 240 + pt3d.y), pow(1000 / (pt3d.z + 1), 2), Scalar(254), 1);
		}
	}*/

	imshow("image_visualizationdsladfhpi", image_visualization);
}