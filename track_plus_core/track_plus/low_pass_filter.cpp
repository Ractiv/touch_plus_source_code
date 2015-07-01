#include "low_pass_filter.h"

void LowPassFilter::compute(float& value, const float alpha, const string name)
{
	float value_old;
	if (value_map.count(name) > 0)
		value_old = value_map[name];
	else
		value_old = value;

	const float value_new = value;

	if (!isnormal(value) && value != 0)
	{
		value = value_old;
		return;
	}

	float result = value_old + ((value_new - value_old) * alpha);

	value_map[name] = result;
	value = result;
}

void LowPassFilter::compute_if_smaller(float& value, const float alpha, const string name)
{
	float value_old;
	if (value_map.count(name) > 0)
		value_old = value_map[name];
	else
		value_old = value;

	const float value_new = value;

	if (!isnormal(value) && value != 0)
	{
		value = value_old;
		return;
	}

	float result;
	if (value_new < value_old)
		result = value_old + ((value_new - value_old) * alpha);
	else
		result = value_new;

	value_map[name] = result;
	value = result;
}

void LowPassFilter::compute_if_smaller(uchar& value, const float alpha, const string name)
{
	float value_old;
	if (value_map.count(name) > 0)
		value_old = value_map[name];
	else
		value_old = value;

	const float value_new = value;

	if (!isnormal((float)value) && value != 0)
	{
		value = value_old;
		return;
	}

	float result;
	if (value_new < value_old)
		result = value_old + ((value_new - value_old) * alpha);
	else
		result = value_new;

	value_map[name] = result;
	value = result;
}

void LowPassFilter::compute_if_larger(float& value, const float alpha, const string name)
{
	float value_old;
	if (value_map.count(name) > 0)
		value_old = value_map[name];
	else
		value_old = value;

	const float value_new = value;

	if (!isnormal(value) && value != 0)
	{
		value = value_old;
		return;
	}

	float result;
	if (value_new > value_old)
		result = value_old + ((value_new - value_old) * alpha);
	else
		result = value_new;

	value_map[name] = result;
	value = result;
}

void LowPassFilter::compute_if_larger(uchar& value, const float alpha, const string name)
{
	float value_old;
	if (value_map.count(name) > 0)
		value_old = value_map[name];
	else
		value_old = value;

	const float value_new = value;

	if (!isnormal((float)value) && value != 0)
	{
		value = value_old;
		return;
	}

	float result;
	if (value_new > value_old)
		result = value_old + ((value_new - value_old) * alpha);
	else
		result = value_new;

	value_map[name] = result;
	value = result;
}

void LowPassFilter::compute(int& value, const float alpha, const string name)
{
	float value_old;
	if (value_map.count(name) > 0)
		value_old = value_map[name];
	else
		value_old = value;

	const float value_new = value;

	if (!isnormal((float)value) && value != 0)
	{
		value = value_old;
		return;
	}

	float result = value_old + ((value_new - value_old) * alpha);

	value_map[name] = result;
	value = result;
}

void LowPassFilter::compute(uchar& value, const float alpha, const string name)
{
	float value_old;
	if (value_map.count(name) > 0)
		value_old = value_map[name];
	else
		value_old = value;

	const float value_new = value;

	if (!isnormal((float)value) && value != 0)
	{
		value = value_old;
		return;
	}

	float result = value_old + ((value_new - value_old) * alpha);

	value_map[name] = result;
	value = result;
}

void LowPassFilter::compute(Point& value, const float alpha, const string name)
{
	const string name_x = name + "x";
	const string name_y = name + "y";

	float value_old_x;
	if (value_map.count(name_x) > 0)
		value_old_x = value_map[name_x];
	else
		value_old_x = value.x;

	const float value_new_x = value.x;

	float value_old_y;
	if (value_map.count(name_y) > 0)
		value_old_y = value_map[name_y];
	else
		value_old_y = value.y;

	const float value_new_y = value.y;

	if ((!isnormal((float)value.x) && value.x != 0) || (!isnormal((float)value.y) && value.y != 0))
	{
		value.x = value_old_x;
		value.y = value_old_y;
		return;
	}

	float result_x = value_old_x + ((value_new_x - value_old_x) * alpha);
	float result_y = value_old_y + ((value_new_y - value_old_y) * alpha);

	value_map[name_x] = result_x;
	value_map[name_y] = result_y;

	value.x = result_x;
	value.y = result_y;
}

void LowPassFilter::compute(Point2f& value, const float alpha, const string name)
{
	const string name_x = name + "x";
	const string name_y = name + "y";

	float value_old_x;
	if (value_map.count(name_x) > 0)
		value_old_x = value_map[name_x];
	else
		value_old_x = value.x;

	const float value_new_x = value.x;

	float value_old_y;
	if (value_map.count(name_y) > 0)
		value_old_y = value_map[name_y];
	else
		value_old_y = value.y;

	const float value_new_y = value.y;

	if ((!isnormal((float)value.x) && value.x != 0) || (!isnormal((float)value.y) && value.y != 0))
	{
		value.x = value_old_x;
		value.y = value_old_y;
		return;
	}

	float result_x = value_old_x + ((value_new_x - value_old_x) * alpha);
	float result_y = value_old_y + ((value_new_y - value_old_y) * alpha);

	value_map[name_x] = result_x;
	value_map[name_y] = result_y;

	value.x = result_x;
	value.y = result_y;
}

void LowPassFilter::compute(Point3f& value, const float alpha, const string name)
{
	const string name_x = name + "x";
	const string name_y = name + "y";
	const string name_z = name + "z";

	float value_old_x;
	if (value_map.count(name_x) > 0)
		value_old_x = value_map[name_x];
	else
		value_old_x = value.x;

	const float value_new_x = value.x;

	float value_old_y;
	if (value_map.count(name_y) > 0)
		value_old_y = value_map[name_y];
	else
		value_old_y = value.y;

	const float value_new_y = value.y;

	float value_old_z;
	if (value_map.count(name_z) > 0)
		value_old_z = value_map[name_z];
	else
		value_old_z = value.z;

	const float value_new_z = value.z;

	if ((!isnormal((float)value.x) && value.x != 0) || (!isnormal((float)value.y) && value.y != 0) ||
		(!isnormal((float)value.z) && value.z != 0))
	{
		value.x = value_old_x;
		value.y = value_old_y;
		value.z = value_old_z;
		return;
	}

	float result_x = value_old_x + ((value_new_x - value_old_x) * alpha);
	float result_y = value_old_y + ((value_new_y - value_old_y) * alpha);
	float result_z = value_old_z + ((value_new_z - value_old_z) * alpha);

	value_map[name_x] = result_x;
	value_map[name_y] = result_y;
	value_map[name_z] = result_z;

	value.x = result_x;
	value.y = result_y;
	value.z = result_z;
}

void LowPassFilter::reset()
{
	value_map = unordered_map<string, float>();
}