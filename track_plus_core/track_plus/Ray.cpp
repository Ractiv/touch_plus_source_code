#include "ray.h"
#include <iostream>

using namespace std;

Ray::Ray(Point3f& position_in, Point3f& direction_in)
{
	direction = direction_in;
	position = position_in;
}

bool Ray::intersects(Plane& plane, float& result)
{
    float den = dot_product(direction, plane.normal);

    if (abs(den) == 0)
        return false;

    result = (-plane.d - dot_product(plane.normal, position)) / den;
    return true;
}