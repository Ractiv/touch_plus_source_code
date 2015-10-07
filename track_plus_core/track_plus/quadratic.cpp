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

#include <iostream>
#include "curve_fitting.h"
#include "console_log.h"

using namespace std;
 
// t = independent value,  y = response value (dependent value)
int CCurveFitting::quadratic_fitting(double* t, int t_len, double* y, double* a_out, double* b_out, double* c_out)
{
        //determine how many points used
        int size = t_len;
       
        //Allocate memory for data[size][2]
        //data[3][0] -> x4, data[3][1] -> y4
        double **data;
        data = new double*[size];
        for(int i=0; i < size; i++)
                data[i] = new double[2];
 
        // sumx3 = (x0)^3 + (x1)^3 + ... + (x{size})^3
        double sumx4 = 0, sumx3 = 0, sumx2 = 0, sumx1 = 0, det;
       
        // Allocate memory for X^T
        double **transpose;
        transpose = new double*[3];
        for(int i=0; i < 3; i++)
                transpose[i] = new double[size];
 
        //Allocate memory for (X^T *X)^-1 * X^T
        double **stuff;
        stuff = new double*[3];
        for(int i=0; i < 3; i++)
                stuff[i] = new double[size];
 
        // y = a[0]x^2 + a[1]x + a[2]
        double a[3] = {0};

        for (int i=0; i < size; i++)
        {
            data[i][0] = *(t + i);
            data[i][1] = *(y + i);
        }
 
        //input data, do some precalculations at the same time to avoid making more loops
        for (int i=0; i < size; i++)
        {
                for (int j=0; j < 2; j++)
                {
                        //cin >> data[i][j];
                        //these computations only happen with x values
                        if(j==0)
                        {
                                //develops the sum values needed for inverse
                                sumx4 += data[i][j]*data[i][j]*data[i][j]*data[i][j];
                                sumx3 += data[i][j]*data[i][j]*data[i][j];
                                sumx2 += data[i][j]*data[i][j];
                                sumx1 += data[i][j];
                               
                                //develops transpose matrix
                                transpose[2][i] = 1;
                                transpose[1][i] = data[i][j];
                                transpose[0][i] = data[i][j]*data[i][j];
                        }
                }
        }
       
//After solving all the math
        //determinate
        det = (sumx4*sumx2*size) + (sumx3*sumx1*sumx2) + (sumx2*sumx3*sumx1) - (sumx2*sumx2*sumx2) - (sumx1*sumx1*sumx4) - (size*sumx3*sumx3);
 
        //precalculated the inverse matrix to avoid numerical methods which take time or lose accuracy, NOTE: does not include division of determinate
        double inverse[3][3] = {
                {size*sumx2 - sumx1*sumx1, -(size*sumx3 - sumx1*sumx2), sumx1*sumx3-sumx2*sumx2},
                {-(size*sumx3-sumx2*sumx1), size*sumx4-sumx2*sumx2, -(sumx1*sumx4-sumx3*sumx2)},
                {sumx1*sumx3 - sumx2*sumx2, -(sumx1*sumx4 - sumx2*sumx3), sumx2*sumx4 - sumx3*sumx3}
        };
 
        //This is matrix multiplication for this particular pair of matrices
        for (int i=0; i < 3; i++)
        {
                for (int j=0; j < size; j++)
                {
                        stuff[i][j] = inverse[i][0]*transpose[0][j] + inverse[i][1]*transpose[1][j] + inverse[i][2]*transpose[2][j];
                }
        }
 
        //This is the final matrix multiplication that outputs a 1x3 matrix with our curve parameters
        for (int i=0; i < 3; i++)
        {
                for (int j=0; j < size; j++)
                {
                        a[i] += stuff[i][j]*data[j][1];
                }
                //dont forget to divide by determinate
                a[i] /= det;
        }
 
       // console_log("a: " + to_string(a[0]) + " b: " + to_string(a[1]) + " c: " + to_string(a[2]));
        *a_out = a[0];
        *b_out = a[1];
        *c_out = a[2];

		// WD: 22 Sep 014: add deletion to prevent memory leak
		// --->>>
		for (int i = size; i>0; --i)
		{
			if (data[i - 1])	
				delete[] data[i - 1];
		}
		delete[] data;

		for (int i = 3; i>0; --i)
		{
			if (transpose[i - 1])
				delete[] transpose[i - 1];
		}
		delete[] transpose;

		for (int i = 3; i>0; --i)
		{
			if (stuff[i - 1])
				delete[] stuff[i - 1];
		}
		delete[] stuff;
		// <<<---

        return 0;
}