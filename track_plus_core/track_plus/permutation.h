#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

extern vector<vector<int>> permutations;

void swap(int x, int y);
void printArray(int size);
void permute(int k, int size);
void compute_permutations(int k, int size);