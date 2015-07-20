/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

#include "pointer_mapper.h"

void PointerMapper::compute(HandResolver& hand_resolver, Reprojector& reprojector)
{
	active = false;

	compute_cursor_point(index_down, hand_resolver.pt_precise_index0, hand_resolver.pt_precise_index1,
						 pt_index, reprojector, pt_cursor_index, dist_cursor_index_plane, actuate_dist, "compute_index");

	compute_cursor_point(thumb_down, hand_resolver.pt_precise_thumb0, hand_resolver.pt_precise_thumb1,
						 pt_thumb, reprojector, pt_cursor_thumb, dist_cursor_thumb_plane, actuate_dist + 10, "compute_thumb");

	if (!index_down)
		thumb_down = false;

	compute_pinch_to_zoom(hand_resolver);
}

void PointerMapper::add_calibration_point(const uchar index)
{
	if (index == 0)
		pt_calib_vec0.push_back(pt_index);
	else if (index == 1)
		pt_calib_vec1.push_back(pt_index);
	else if (index == 2)
		pt_calib_vec2.push_back(pt_index);
	else if (index == 3)
		pt_calib_vec3.push_back(pt_index);
}

void PointerMapper::compute_calibration_points()
{
	vector<float> x_vec;
	vector<float> y_vec;
	vector<float> z_vec;

	float x_median;
	float y_median;
	float z_median;

	if (pt_calib_vec0.size() > 0 && pt_calib_vec0.size() > 1 && pt_calib_vec0.size() > 2 && pt_calib_vec0.size() > 3)
	{
		x_vec = vector<float>();
		y_vec = vector<float>();
		z_vec = vector<float>();

		for (Point3f& pt : pt_calib_vec0)
		{
			x_vec.push_back(pt.x);
			y_vec.push_back(pt.y);
			z_vec.push_back(pt.z);
		}

		sort(x_vec.begin(), x_vec.end());
		sort(y_vec.begin(), y_vec.end());
		sort(z_vec.begin(), z_vec.end());

		x_median = x_vec[x_vec.size() * 0.5];
		y_median = y_vec[y_vec.size() * 0.5];
		z_median = z_vec[z_vec.size() * 0.5];

		pt_calib0 = Point3f(x_median, y_median, z_median);

		x_vec = vector<float>();
		y_vec = vector<float>();
		z_vec = vector<float>();

		for (Point3f& pt : pt_calib_vec1)
		{
			x_vec.push_back(pt.x);
			y_vec.push_back(pt.y);
			z_vec.push_back(pt.z);
		}

		sort(x_vec.begin(), x_vec.end());
		sort(y_vec.begin(), y_vec.end());
		sort(z_vec.begin(), z_vec.end());

		x_median = x_vec[x_vec.size() * 0.5];
		y_median = y_vec[y_vec.size() * 0.5];
		z_median = z_vec[z_vec.size() * 0.5];

		pt_calib1 = Point3f(x_median, y_median, z_median);

		x_vec = vector<float>();
		y_vec = vector<float>();
		z_vec = vector<float>();

		for (Point3f& pt : pt_calib_vec2)
		{
			x_vec.push_back(pt.x);
			y_vec.push_back(pt.y);
			z_vec.push_back(pt.z);
		}

		sort(x_vec.begin(), x_vec.end());
		sort(y_vec.begin(), y_vec.end());
		sort(z_vec.begin(), z_vec.end());

		x_median = x_vec[x_vec.size() * 0.5];
		y_median = y_vec[y_vec.size() * 0.5];
		z_median = z_vec[z_vec.size() * 0.5];

		pt_calib2 = Point3f(x_median, y_median, z_median);

		x_vec = vector<float>();
		y_vec = vector<float>();
		z_vec = vector<float>();

		for (Point3f& pt : pt_calib_vec3)
		{
			x_vec.push_back(pt.x);
			y_vec.push_back(pt.y);
			z_vec.push_back(pt.z);
		}

		sort(x_vec.begin(), x_vec.end());
		sort(y_vec.begin(), y_vec.end());
		sort(z_vec.begin(), z_vec.end());

		x_median = x_vec[x_vec.size() * 0.5];
		y_median = y_vec[y_vec.size() * 0.5];
		z_median = z_vec[z_vec.size() * 0.5];

		pt_calib3 = Point3f(x_median, y_median, z_median);

		const float x0_plane = pt_calib0.x;
		const float y0_plane = pt_calib0.y;
		const float z0_plane = pt_calib0.z + 100;

	    const float x1_plane = pt_calib1.x;
	    const float y1_plane = pt_calib1.y;
	    const float z1_plane = pt_calib1.z + 100;

	    const float x2_plane = pt_calib2.x;
	    const float y2_plane = pt_calib2.y;
	    const float z2_plane = pt_calib2.z + 100;

	    const float x3_plane = pt_calib3.x;
	    const float y3_plane = pt_calib3.y;
	    const float z3_plane = pt_calib3.z + 100;

	    float angle00 = get_angle(Point(x0_plane, z0_plane), Point(x1_plane, z1_plane), Point(x2_plane, z2_plane));
	    float angle01 = get_angle(Point(x1_plane, z1_plane), Point(x0_plane, z0_plane), Point(x2_plane, z2_plane));
	    float angle02 = get_angle(Point(x2_plane, z2_plane), Point(x0_plane, z0_plane), Point(x1_plane, z1_plane));
	    float angle0_max = max(max(angle00, angle01), angle02);

	    float angle10 = get_angle(Point(x1_plane, z1_plane), Point(x2_plane, z2_plane), Point(x3_plane, z3_plane));
	    float angle11 = get_angle(Point(x2_plane, z2_plane), Point(x1_plane, z1_plane), Point(x3_plane, z3_plane));
	    float angle12 = get_angle(Point(x3_plane, z3_plane), Point(x1_plane, z1_plane), Point(x2_plane, z2_plane));
	    float angle1_max = max(max(angle10, angle11), angle12);

	    float angle20 = get_angle(Point(x2_plane, z2_plane), Point(x3_plane, z3_plane), Point(x0_plane, z0_plane));
	    float angle21 = get_angle(Point(x3_plane, z3_plane), Point(x2_plane, z2_plane), Point(x0_plane, z0_plane));
	    float angle22 = get_angle(Point(x0_plane, z0_plane), Point(x2_plane, z2_plane), Point(x3_plane, z3_plane));
	    float angle2_max = max(max(angle20, angle21), angle22);

	    float angle30 = get_angle(Point(x3_plane, z3_plane), Point(x0_plane, z0_plane), Point(x1_plane, z1_plane));
	    float angle31 = get_angle(Point(x0_plane, z0_plane), Point(x3_plane, z3_plane), Point(x1_plane, z1_plane));
	    float angle32 = get_angle(Point(x1_plane, z1_plane), Point(x3_plane, z3_plane), Point(x0_plane, z0_plane));
	    float angle3_max = max(max(angle30, angle31), angle32);

	    vector<float> angle_vec = { angle0_max, angle1_max, angle2_max, angle3_max };
	    sort(angle_vec.begin(), angle_vec.end());

	    float diff_min = 9999;
	    int index_diff_min0;
	    int index_diff_min1;
	    int index_diff_min2;

	    int index0 = 0;
	    for (float& angle0 : angle_vec)
	    {
	    	int index1 = 0;
	    	for (float& angle1 : angle_vec)
	    	{
	    		int index2 = 0;
	    		for (float& angle2 : angle_vec)
	    		{
	    			if (index0 != index1 && index1 != index2 && index0 != index2)
	    			{
	    				float diff0 = abs(angle0 - angle1);
	    				float diff1 = abs(angle1 - angle2);
	    				float diff2 = abs(angle0 - angle2);
	    				float diff_sum = diff0 + diff1 + diff2 + max(max(diff0, diff1), diff2);

	    				if (diff_sum < diff_min)
	    				{
	    					diff_min = diff_sum;
	    					index_diff_min0 = index0;
	    					index_diff_min1 = index1;
	    					index_diff_min2 = index2;
	    				}
	    			}
	    			++index2;
	    		}
	    		++index1;
	    	}
	    	++index0;
	    }

	    vector<float> angle_vec_filtered;

	    int index = 0;
	    for (float& angle : angle_vec)
	    {
	    	if (index == index_diff_min0 || index == index_diff_min1 || index == index_diff_min2)
	    		angle_vec_filtered.push_back(angle);

	    	++index;
	    }

	    sort(angle_vec_filtered.begin(), angle_vec_filtered.end());
	    float angle_selected = angle_vec_filtered[1];

	    COUT << "size is " << angle_vec_filtered.size() << endl;
	    COUT << index_diff_min0 << " " << index_diff_min1 << " " << index_diff_min2 << endl;

	    float configuration0[10] = { angle0_max, x0_plane, y0_plane, z0_plane,
	    									  	 x1_plane, y1_plane, z1_plane,
	    										 x2_plane, y2_plane, z2_plane };

	    float configuration1[10] = { angle1_max, x1_plane, y1_plane, z1_plane,
	    									  	 x2_plane, y2_plane, z2_plane,
	    										 x3_plane, y3_plane, z3_plane };

	    float configuration2[10] = { angle2_max, x2_plane, y2_plane, z2_plane,
	    									  	 x3_plane, y3_plane, z3_plane,
	    										 x0_plane, y0_plane, z0_plane };

	    float configuration3[10] = { angle3_max, x3_plane, y3_plane, z3_plane,
	    									  	 x0_plane, y0_plane, z0_plane,
	    										 x1_plane, y1_plane, z1_plane };

	    vector<float*> configutation_vec = { configuration0, configuration1, configuration2, configuration3 };

	    float x0_selected;
	    float y0_selected;
	    float z0_selected;

	    float x1_selected;
	    float y1_selected;
	    float z1_selected;

	    float x2_selected;
	    float y2_selected;
	    float z2_selected;

	    bool found = false;

		for (float* configuration : configutation_vec)
	    	if (configuration[0] == angle_selected)
	    	{
	    		x0_selected = configuration[1];
	    		y0_selected = configuration[2];
	    		z0_selected = configuration[3];

	    		x1_selected = configuration[4];
	    		y1_selected = configuration[5];
	    		z1_selected = configuration[6];

	    		x2_selected = configuration[7];
	    		y2_selected = configuration[8];
	    		z2_selected = configuration[9];

	    		found = true;
	    		break;
	    	}

    	plane = Plane(Point3f(x0_selected, y0_selected, z0_selected),
    				  Point3f(x1_selected, y1_selected, z1_selected),
    				  Point3f(x2_selected, y2_selected, z2_selected));

	    direction_plane = cross_product(Point3f(x0_selected - x1_selected, y0_selected - y1_selected, z0_selected - z1_selected),
	    								Point3f(x2_selected - x1_selected, y2_selected - y1_selected, z2_selected - z1_selected));

	    direction_plane.x = -direction_plane.x;
	    direction_plane.y = -direction_plane.y;
	    direction_plane.z = -direction_plane.z;

	    project_to_plane(pt_calib0, pt_calib0_projected, dist_calib0_plane);
		project_to_plane(pt_calib1, pt_calib1_projected, dist_calib1_plane);
		project_to_plane(pt_calib2, pt_calib2_projected, dist_calib2_plane);
		project_to_plane(pt_calib3, pt_calib3_projected, dist_calib3_plane);

		const float d0 = get_distance(pt_calib0_projected.x, pt_calib0_projected.y, pt_calib2_projected.x, pt_calib2_projected.y);
		const float d1 = get_distance(pt_calib1_projected.x, pt_calib1_projected.y, pt_calib3_projected.x, pt_calib3_projected.y);
		d_max = std::max(d0, d1);

		rect_warper.setSource(pt_calib0_projected.x, pt_calib0_projected.y, pt_calib1_projected.x, pt_calib1_projected.y,
							  pt_calib2_projected.x, pt_calib2_projected.y, pt_calib3_projected.x, pt_calib3_projected.y);

		rect_warper.setDestination(0, 0, 1000, 0, 1000, 1000, 0, 1000);

		calibrated = true;
	}
	else
		COUT << "not calibrated" << endl;
}

bool PointerMapper::project_to_plane(Point3f& pt, Point3f& result, float& dist_to_plane)
{
    Ray ray = Ray(pt, direction_plane);

	float t;
    if (ray.intersects(plane, t))
    {
    	const float x = pt.x + t * direction_plane.x;
    	const float y = pt.y + t * direction_plane.y;
    	const float z = pt.z + t * direction_plane.z;
        result = Point3f(x, y, z);
        dist_to_plane = get_distance(result, pt);
        
        return true;
    }

    return false;
}

float PointerMapper::compute_hit_dist(Point3f& pt)
{
	const float dist0 = get_distance(pt.x, pt.y, pt_calib0_projected.x, pt_calib0_projected.y);
	const float dist1 = get_distance(pt.x, pt.y, pt_calib1_projected.x, pt_calib1_projected.y);
	const float dist2 = get_distance(pt.x, pt.y, pt_calib2_projected.x, pt_calib2_projected.y);
	const float dist3 = get_distance(pt.x, pt.y, pt_calib3_projected.x, pt_calib3_projected.y);

	const float coeff0 = d_max / dist0;
	const float coeff1 = d_max / dist1;
	const float coeff2 = d_max / dist2;
	const float coeff3 = d_max / dist3;

	float result = (dist_calib0_plane * coeff0) + (dist_calib1_plane * coeff1) + (dist_calib2_plane * coeff2) + (dist_calib3_plane * coeff3);
	result /= (coeff0 + coeff1 + coeff2 + coeff3);
	return result;
}

void PointerMapper::compute_cursor_point(bool& target_down, Point2f& pt_target0, Point2f& pt_target1, Point3f& pt_target,
										 Reprojector& reprojector, Point2f& pt_cursor, float& dist_cursor_target_plane,
										 const float actuation_dist, string name)
{
	LowPassFilter* low_pass_filter = value_store.get_low_pass_filter("low_pass_filter" + name);

	if (do_reset)
	{
		do_reset = false;
		low_pass_filter->reset();
	}

	if (pt_target0.x != -1 && pt_target1.x != -1)
	{
		active = true;
		pt_target = reprojector.reproject_to_3d(pt_target0.x, pt_target0.y, pt_target1.x, pt_target1.y);

		if (calibrated)
		{
			Point3f pt_target_projected;
			float dist_target_plane;
			project_to_plane(pt_target, pt_target_projected, dist_target_plane);

			rect_warper.warp(pt_target_projected.x, pt_target_projected.y, pt_cursor.x, pt_cursor.y);

			if (value_store.has_point2f("pt_cursor" + name))
			{
                Point2f temp = value_store.get_point2f("pt_cursor" + name);
                float alpha = get_distance(pt_cursor, temp) / 1000;
	            if (alpha < 0.05)
	            	alpha = 0.05;
	            else if (alpha > 1)
	            	alpha = 1;

	            low_pass_filter->compute(alpha, 0.5, "alpha" + name);
	            low_pass_filter->compute(pt_cursor, alpha, "pt_cursor" + name);
	        }
	        value_store.set_point2f("pt_cursor" + name, pt_cursor);

			float hit_dist = compute_hit_dist(pt_target_projected);

			dist_cursor_target_plane = dist_target_plane - hit_dist;
			low_pass_filter->compute(dist_cursor_target_plane, 0.2, "dist_cursor_target_plane");

			float hit_dist_processed = hit_dist;
			float hit_dist_processed_old = value_store.get_float("hit_dist_processed_old" + name, hit_dist_processed); 
			hit_dist_processed += ((hit_dist_processed - hit_dist_processed_old) * 0.25);
			value_store.set_float("hit_dist_processed_old" + name, hit_dist_processed);

			float dist_cursor_target_plane_no_lowpass = dist_target_plane - hit_dist_processed;

			if (dist_cursor_target_plane_no_lowpass <= actuation_dist + 2)
				value_store.set_bool("actuated" + name, true);

			if (value_store.get_bool("actuated" + name))
			{
				target_down = true;
				
				if (dist_cursor_target_plane_no_lowpass > actuation_dist + 10)
				{
					target_down = false;
					value_store.set_bool("actuated" + name, false);
				}
			}
		}
	}
	else
		low_pass_filter->reset();
}

void PointerMapper::compute_pinch_to_zoom(HandResolver& hand_resolver)
{
	if (thumb_down && index_down)
		value_store.set_bool("thumb_actuated", true);
	if (/*dist_cursor_thumb_plane > 9999 || */!index_down)
		value_store.set_bool("thumb_actuated", false);

	bool has_thumb = (hand_resolver.pt_precise_thumb0.x != -1);

	const uchar has_thumb_vec_size_max = 3;
	int has_thumb_vec_index = value_store.get_int("has_thumb_vec_index");

	vector<int>* has_thumb_vec = value_store.get_int_vec("has_thumb_vec");
	if (has_thumb_vec->size() < has_thumb_vec_size_max)
	{
		has_thumb_vec->push_back(has_thumb);
		++has_thumb_vec_index;
	}
	else if (has_thumb_vec_index < has_thumb_vec_size_max)
	{
		(*has_thumb_vec)[has_thumb_vec_index] = has_thumb;
		++has_thumb_vec_index;
	}
	else
	{
		has_thumb_vec_index = 0;
		(*has_thumb_vec)[has_thumb_vec_index] = has_thumb;
		++has_thumb_vec_index;
	}

	value_store.set_int("has_thumb_vec_index", has_thumb_vec_index);

	bool has_thumb_old = true;
	for (int i = 0; i < has_thumb_vec->size(); ++i)
		if ((*has_thumb_vec)[i] == false)
		{
			has_thumb_old = false;
			break;
		}

	const bool zoom_out = has_thumb && has_thumb_old;

	if (value_store.get_bool("thumb_actuated") && !value_store.get_bool("thumb_actuated_old"))
	{
		Point2f pt_anchor = Point2f((pt_cursor_index.x + pt_cursor_thumb.x) / 2, (pt_cursor_index.y + pt_cursor_thumb.y) / 2);
		Point2f pt_diff = Point2f(pt_cursor_index.x - pt_anchor.x, pt_cursor_index.y - pt_anchor.y);

		value_store.set_point2f("pt_anchor", pt_anchor);
		value_store.set_point2f("pt_diff", pt_diff);
	}

	if (value_store.get_bool("thumb_actuated"))
	{
		Point2f pt_anchor = value_store.get_point2f("pt_anchor");
		Point2f pt_diff = value_store.get_point2f("pt_diff");

		const float diff_x = (pt_cursor_index.x - pt_anchor.x) / 2;
		const float diff_y = (pt_cursor_index.y - pt_anchor.y) / 2;

		pt_pinch_to_zoom_index = Point2f(pt_anchor.x + diff_x + pt_diff.x / 2, pt_anchor.y + diff_y + pt_diff.y / 2);
		pt_pinch_to_zoom_thumb = Point2f(pt_anchor.x - diff_x + pt_diff.x / 2, pt_anchor.y - diff_y + pt_diff.y / 2);

		pinch_to_zoom = true;
	}
	else if (pinch_to_zoom)
		pinch_to_zoom = false;

	value_store.set_bool("thumb_actuated_old", value_store.get_bool("thumb_actuated"));
}

void PointerMapper::reset()
{
	do_reset = true;
}