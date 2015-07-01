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

#include <opencv/cv.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

double f(double t, const double *p);
double f5(double t, const double *p);  // WD, 9 Sep 2014: the output result of this function is different from that of readerFit.com, version used is different?

class CCurveFitting
{   // Based on http:////readerfit.com//
public:
	// four -parameters-logistic-regression
	// t = independent value,  y = response value (dependent value)
	int curve_fitting4(double* t, int t_len, double* y, double* a_out, double* b_out, double* c_out, double* d_out);
	
	// five-parameters-logistic-regression
	int curve_fitting5(double* t, int t_len, double* y, double* a_out, double* b_out, double* c_out, double* d_out, double* e_out);

	int quadratic_fitting(double* t, int t_len, double* y, double* a_out, double* b_out, double* c_out);

	//WD, 16 Sept 2014: based on http:////web.iitd.ac.in//~pmvs//courses//mel705//curvefitting.pdf
	int exponential_fitting(double* t, int t_len, double* y, double* a_out, double* b_out); 

private:
	double VMAX(double* d, int count);
	double VMIN(double* d, int count);
	double VAVERAGE(double* independentArray, double* resposne, int count, double midValue);
	double VSLOPE(double* x, double* y, int count);

};