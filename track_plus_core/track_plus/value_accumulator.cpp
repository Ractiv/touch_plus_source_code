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