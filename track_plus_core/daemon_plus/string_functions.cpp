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

#ifdef _WIN32
string to_string(wstring ws)
{
	setlocale(LC_ALL, "");
	const locale locale("");
	typedef codecvt<wchar_t, char, mbstate_t> converter_type;
	const converter_type& converter = use_facet<converter_type>(locale);
	vector<char> to(ws.length() * converter.max_length());
	mbstate_t state;
	const wchar_t* from_next;
	char* to_next;

	const converter_type::result result =
		converter.out(state, ws.data(), ws.data() + ws.length(), from_next, &to[0], &to[0] + to.size(), to_next);

	if (result == converter_type::ok || result == converter_type::noconv)
	{
		const string s(&to[0], to_next);
		return s;
	}

	return 0;
}

const wchar_t* to_wchar_t_array(string str)
{
	std::wstring widestr = std::wstring(str.begin(), str.end());
	const wchar_t* widecstr = widestr.c_str();
	return widecstr;
}
#endif