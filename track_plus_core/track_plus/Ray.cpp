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

    if (den <= 0)
        return false;

    result = (-plane.d - dot_product(plane.normal, position)) / den;
    return true;
}