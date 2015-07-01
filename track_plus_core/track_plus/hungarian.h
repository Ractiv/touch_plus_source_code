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

#include <vector>
#include <iostream>
#include <limits>
#include <time.h>
#include "globals.h"

using namespace std;

class AssignmentProblemSolver
{
private:
	// Computes the optimal assignment (minimum overall costs) using Munkres algorithm.
	void assignmentoptimal(int *assignment, float *cost, float *distMatrix, int nOfRows, int nOfColumns);
	void buildassignmentvector(int *assignment, bool *starMatrix, int nOfRows, int nOfColumns);
	void computeassignmentcost(int *assignment, float *cost, float *distMatrix, int nOfRows);

	void step2a(int *assignment, float *distMatrix, bool *starMatrix, bool *newStarMatrix, bool *primeMatrix,
				bool *coveredColumns, bool *coveredRows, int nOfRows, int nOfColumns, int minDim);

	void step2b(int *assignment, float *distMatrix, bool *starMatrix, bool *newStarMatrix, bool *primeMatrix,
				bool *coveredColumns, bool *coveredRows, int nOfRows, int nOfColumns, int minDim);

	void step3 (int *assignment, float *distMatrix, bool *starMatrix, bool *newStarMatrix, bool *primeMatrix,
				bool *coveredColumns, bool *coveredRows, int nOfRows, int nOfColumns, int minDim);

	void step4 (int *assignment, float *distMatrix, bool *starMatrix, bool *newStarMatrix, bool *primeMatrix,
				bool *coveredColumns, bool *coveredRows, int nOfRows, int nOfColumns, int minDim, int row, int col);

	void step5 (int *assignment, float *distMatrix, bool *starMatrix, bool *newStarMatrix, bool *primeMatrix,
				bool *coveredColumns, bool *coveredRows, int nOfRows, int nOfColumns, int minDim);
	
	// Computes a suboptimal solution. Good for cases with many forbidden assignments.
	void assignmentsuboptimal1(int *assignment, float *cost, float *distMatrixIn, int nOfRows, int nOfColumns);
	// Computes a suboptimal solution. Good for cases with many forbidden assignments.
	void assignmentsuboptimal2(int *assignment, float *cost, float *distMatrixIn, int nOfRows, int nOfColumns);

public:
	enum TMethod { optimal, many_forbidden_assignments, without_forbidden_assignments };
	float Solve(vector<vector<float>>& DistMatrix,vector<int>& Assignment,TMethod Method=optimal);
};