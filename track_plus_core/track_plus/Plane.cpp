#include "plane.h"

Plane::Plane(){}

Plane::Plane(Point3f a, Point3f b, Point3f c)
{
	Point3f ab = Point3f(b.x - a.x, b.y - a.y, b.z - a.z);
	Point3f ac = Point3f(c.x - a.x, c.y - a.y, c.z - a.z);

	Point3f cross = cross_product(ab, ac);
	normal = normalize(cross);
	d = -(dot_product(normal, a));
}