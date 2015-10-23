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

#include "value_store.h"

class ValueAccumulator
{
public:
	bool ready = false;

	unordered_map<string, bool> ready_map;
	unordered_map<string, float> max_val_map;
	unordered_map<string, float> min_val_map;

	ValueStore value_store; 

	float compute(float& val, string name, int size_limit, float val_default, float ratio, bool stop_when_ready, bool overwrite = true);
	float compute_max(float& val, string name);
	float compute_min(float& val, string name);
};