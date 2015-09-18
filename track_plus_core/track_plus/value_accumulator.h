#pragma once

#include "value_store.h"

class ValueAccumulator
{
public:
	bool ready = false;

	unordered_map<string, bool> ready_map;

	ValueStore value_store; 

	void compute(float& val, string name, int size_limit, float val_default, float ratio);
};