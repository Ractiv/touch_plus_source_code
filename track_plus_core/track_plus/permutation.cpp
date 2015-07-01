#include "permutation.h"

int int_array[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
unordered_map<string, bool> checker;
extern vector<vector<int>> permutations = vector<vector<int>>();

void swap(int x, int y)
{
    int temp = int_array[x];
    int_array[x] = int_array[y];
    int_array[y] = temp;
}

void push_result(int size)
{
	string key = "";
	for (int i = 0; i < size; ++i)
    	key += to_string(int_array[i]);

    if (checker.count(key) == 0)
    {
    	checker[key] = true;

    	permutations.push_back(vector<int>());
	    for (int i = 0; i < size; ++i)
	        permutations[permutations.size() - 1].push_back(int_array[i]);
	}
}

void permute(int k, int size)
{
    if (k == 0)
        push_result(size);
    else
        for (int i = k - 1; i >= 0; --i)
        {
            swap(i, k - 1);
            permute(k - 1, size);
            swap(i, k - 1);
        }
}

void compute_permutations(int k, int size)
{
	checker.clear();
	permutations.clear();
	permute(k, size);
}