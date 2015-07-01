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