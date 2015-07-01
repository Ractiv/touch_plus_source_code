#pragma once

#include "plane.h"

class Ray
{
public:
	Point3f direction;
	Point3f position;

	Ray(Point3f& position_in, Point3f& direction_in);
	bool intersects(Plane& plane, float& result);
};