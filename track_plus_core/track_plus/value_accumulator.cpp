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

void ValueAccumulator::compute(float& val, string name, int size_limit, float val_default, float ratio)
{
	const int size_threshold = 20;

	vector<float>* float_vec = value_store.get_float_vec(name);
	if (float_vec->size() < size_limit)
	{
		if (float_vec->size() == 0)
			ready_map[name] = false;

		float_vec->push_back(val);

		if (float_vec->size() > size_threshold)
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