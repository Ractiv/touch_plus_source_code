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

#include "string_functions.h"

vector<string> split_string(const string str_in, const string str_char)
{
	vector<string> result;
	string str = "";

	const int i_max = str_in.size();
	for (int i = 0; i < i_max; ++i)
	{
		string char_current = "";
		char_current += str_in.at(i);

		if (char_current != str_char)
			str += str_in.at(i);
		else
		{
			result.push_back(str);
			str = "";
		}
	}
	result.push_back(str);
	return result;		
}

bool string_has_line_break(string str_in)
{
	for (char& c : str_in)
	{
		string c_string = "";
		c_string += c;
		if (c_string == "\n")
			return true;
	}

	return false;
}