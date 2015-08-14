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