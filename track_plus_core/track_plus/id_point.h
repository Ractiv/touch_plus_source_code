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

#pragma once
#include "opencv2/opencv.hpp"

using namespace std;

struct IDPoint
{
	int id;
	int x;
	int y;

	Point pt;
	Point pt_predict = Point(-1, -1);

	IDPoint(int id_in, Point pt_in)
	{
		id = id_in;
		pt = pt_in;
		x = pt.x;
		y = pt.y;
	}
};