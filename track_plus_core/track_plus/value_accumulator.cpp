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

#include "value_accumulator.h"

const int value_accumulator_size_threshold = 20;

void ValueAccumulator::compute(float& val, string name, int size_limit, float val_default, float ratio, bool stop_when_ready)
{
	vector<float>* float_vec = value_store.get_float_vec(name);
	if (float_vec->size() < size_limit && !(stop_when_ready && ready))
	{
		if (float_vec->size() == 0)
			ready_map[name] = false;

		float_vec->push_back(val);

		if (float_vec->size() > value_accumulator_size_threshold)
			ready_map[name] = true;

		bool all_ready = true;
		unordered_map<string, bool>::iterator it;
		for (it = ready_map.begin(); it != ready_map.end(); ++it)
		{
			bool is_ready = (bool)it->second;
			if (!is_ready)
				all_ready = false;
		}

		ready = all_ready;
	}

	if (ready)
	{
		sort(float_vec->begin(), float_vec->end());
		val = (*float_vec)[float_vec->size() * ratio];
	}
	else
		val = val_default;
}

float ValueAccumulator::compute_max(float& val, string name)
{
	float val_max = FLT_MIN;
	if (max_val_map.count(name) > 0)
		val_max = max_val_map[name];

	if (val > val_max)
		val_max = val;

	max_val_map[name] = val_max;
	return val_max;
}

float ValueAccumulator::compute_min(float& val, string name)
{
	float val_min = FLT_MAX;
	if (min_val_map.count(name) > 0)
		val_min = min_val_map[name];

	if (val < val_min)
		val_min = val;

	min_val_map[name] = val_min;
	return val_min;
}