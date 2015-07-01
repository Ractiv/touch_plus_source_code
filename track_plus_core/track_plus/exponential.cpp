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

#include "curve_fitting.h"

// WD, 16 Sept 2014: based on http:////web.iitd.ac.in//~pmvs//courses//mel705//curvefitting.pdf
// y = C * exp(A^x)

// t = independent value,  y = response value (dependent value)
int CCurveFitting::exponential_fitting(double* x, int t_len, double* y, double* a_out, double* c_out)
{
	//determine how many points used
	int n = t_len;

	// Solve a first order linear curve fit Ax=b
	//[n       sum(X)	  [B	  =  [sum(Y)
	// sum(X)  sum(X^2)]   A]         sum(X*Y)]
	// where: Y=ln(y), X=x, and finally C=exp(B)

	Mat matA(2, 2, CV_64F);
	Mat matx(2, 1, CV_64F);
	Mat matb(2, 1, CV_64F);

	double sumX=0.0, sumX2=0.0, sumY=0.0, sumXY=0.0, B;
	bool bNonSingular;

	for (int i = 0; i < n; i++)
	{
		sumX += (x[i]);
		sumX2 += (pow(x[i], 2.0));
		sumY += (log(y[i]));
		sumXY += (x[i] * log(y[i]));
	}
	matA.ptr<double>(0, 0)[0] = (double)n;
	matA.ptr<double>(0, 1)[0] = sumX;
	matA.ptr<double>(1, 0)[0] = sumX;
	matA.ptr<double>(1, 1)[0] = sumX2;

	matb.ptr<double>(0, 0)[0] = sumY;
	matb.ptr<double>(1, 0)[0] = sumXY;

	bNonSingular = solve(matA, matb, matx, DECOMP_LU);

	*a_out = matx.ptr<double>(1, 0)[0];
	B = matx.ptr<double>(0, 0)[0];
	*c_out = exp(B);

	return 0;
}



