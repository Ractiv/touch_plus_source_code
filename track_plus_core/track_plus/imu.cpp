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

#include "imu.h"
#include "globals.h"

Point3f IMU::compute_azimuth(float x_accel, float y_accel, float z_accel)
{
	float roll_current = -atan2((float)y_accel, sqrt(((float)x_accel * (float)x_accel) + ((float)z_accel * (float)z_accel))) * 180 / CV_PI;
	float pitch_current = atan2((float)x_accel, sqrt(((float)y_accel * (float)y_accel) + ((float)z_accel * (float)z_accel))) * 180 / CV_PI;
 	float yaw_current = 0;

 	return Point3f(roll_current, pitch_current, yaw_current);
}

void IMU::compute(float x_accel, float y_accel, float z_accel)
{
	Point3f heading = compute_azimuth(x_accel, y_accel, z_accel);
	roll = heading.x;
	pitch = heading.y;
	yaw = heading.z;

	low_pass_filter.compute(roll, 0.01, "roll");
	low_pass_filter.compute(pitch, 0.01, "pitch");
	low_pass_filter.compute(yaw, 0.01, "yaw");
}