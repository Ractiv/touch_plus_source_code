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

// Rectifier.cpp : Defines the entry point for the console application.
//

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include <iostream>

#include "rectifier.h"
#include "warper.h"
#include "curve_fitting.h"
#include "console_log.h"

void CRectifier::SetPatternSize(int width, int height)	{
	patternSize.width  = width;
	patternSize.height = height;
}

Point2f CRectifier::getPoint(int row, int col) {
	int index = col * patternSize.width + row;    
	return corners[index];
}

void CRectifier::FindCornersPointsSets(int scanOrientation)
{
	int nSize1, nSize2;
	Point2f ptStart, ptMiddle, ptEnd;

	if (scanOrientation == HORIZONTAL)   {
		nSize1 = patternSize.height;
		nSize2 = patternSize.width;
	}
	else    {    // VERTICAL
		nSize1 = patternSize.width;
		nSize2 = patternSize.height;
	}

	for (int n = 0; n < nSize1; n++) {
		vector<Point2f> pointList, pointList0, pointList1;

		if (scanOrientation == HORIZONTAL)	{
			ptStart = getPoint(0, n);
			ptMiddle = getPoint(nSize2 / 2, n);
			ptEnd = getPoint(nSize2 - 1, n);
		}
		else    {   // VERTICAL
			ptStart = getPoint(n, 0);
			ptMiddle = getPoint(n, nSize2 / 2);
			ptEnd = getPoint(n, nSize2 - 1);
		}

		Point2f ptOld(9999.0, 9999.0);
		float diffMax = 0.0;
		unsigned int indexInflection = 0;

		for (int m = 0; m < nSize2; m++) {
			Point2f ptCurrent;
			if (scanOrientation == HORIZONTAL)
				ptCurrent = getPoint(m, n);
			else    // VERTICAL
				ptCurrent = getPoint(n, m);

			if (ptOld.x != 9999.0 && ptOld.y != 9999.0) {
				float diff;
				if (scanOrientation == HORIZONTAL)
					diff = abs(ptCurrent.x - ptOld.x);
				else    // VERTICAL
					diff = abs(ptCurrent.y - ptOld.y);

				if (diff > diffMax) {
					diffMax = diff;
					indexInflection = m;
				}
			}
			pointList.push_back(ptCurrent);
			ptOld = ptCurrent;
		}

		for (unsigned int m = 0; m < pointList.size(); m++) {
			Point2f pt = pointList[m];
			if (m < indexInflection)	{
				pointList0.push_back(pt);
				whiteCircles.push_back(pt);	// WD, 16 Sep 2014: to store all "white circles"
			}
			if (m > indexInflection)	{
				pointList1.push_back(pt);
				greyCircles.push_back(pt);	// WD, 16 Sep 2014: to store all "grey circles"
			}
			if (m == indexInflection)	{
				//if (scanOrientation == HORIZONTAL)
				//{
				//	if (n != 11)  // WD: 16 Sep 2014
				//	{
				//		pointList0.push_back(pt);
				//		whiteCircles.push_back(pt);
				//	}
				//	if (n != 12)	// WD: 16 Sep 2014
				//	{
				//		pointList1.push_back(pt);
				//		greyCircles.push_back(pt);
				//	}
				//}
				//else
				//{
					pointList0.push_back(pt);
					whiteCircles.push_back(pt);
					pointList1.push_back(pt);
					greyCircles.push_back(pt);
				//}
			}
		}
		reverse(pointList0.begin(), pointList0.end());  // WD, 16 Sep 2014: order of position vector is always from middle position to the end

		// WD, 16 Sep 2014: originally from XL
		// --->>>
		//vector<Point2f> list0, list1;
		//for (unsigned int a = 1; a < pointList0.size(); a++) {
		//	float xDiff = pointList0[a].x - pointList0[a-1].x;
		//	float yDiff = pointList0[a].y - pointList0[a-1].y;
		//	list0.push_back(Point2f(xDiff, yDiff));
		//}
		//for (unsigned int a = 1; a < pointList1.size(); a++) {
		//	float xDiff = pointList1[a].x - pointList1[a-1].x;
		//	float yDiff = pointList1[a].y - pointList1[a-1].y;
		//	list1.push_back(Point2f(xDiff, yDiff));
		//}
		//if (scanOrientation == HORIZONTAL)   {
		//	//listScanHorizontal.Add(new List<PointF>[2] { list0, list1 });
		//	listScanHorizontal.push_back(list0);
		//	listScanHorizontal.push_back(list1);
		//}
		//else   {
		//	//listScanVertical.Add(new List<PointF>[2] { list0, list1 });
		//	listScanVertical.push_back(list0);
		//	listScanVertical.push_back(list1);
		//}
		// <<<---


		// Get equation of each set of points and store them as vector
		vector<double> equation;

		equation = FindEquations(scanOrientation, pointList0);
		if (scanOrientation == HORIZONTAL)
			horizontalScanEquations.push_back(equation);
		else    // VERTICAL
			verticalScanEquations.push_back(equation);

		equation = FindEquations(scanOrientation, pointList1);
		if (scanOrientation == HORIZONTAL)
			horizontalScanEquations.push_back(equation);
		else    // VERTICAL
			verticalScanEquations.push_back(equation);

		// Keep original corners set that forms each segment
		if (scanOrientation == HORIZONTAL)	{
			horizontalCorners.push_back(pointList0);
			horizontalCorners.push_back(pointList1);
		}
		else {  // VERTICAL
			verticalCorners.push_back(pointList0);
			verticalCorners.push_back(pointList1);
		}
	}
}

void CRectifier::DrawCircles(int scanOrientation, Mat &image)
{
	// White circles
	grayIntensity = 255;
	for (unsigned int a = 0; a < whiteCircles.size(); a++)
		circle(image, whiteCircles[a], 5, grayIntensity, 2);

	// Grey circles
	grayIntensity = 127;
	for (unsigned int a = 0; a < greyCircles.size(); a++)
		circle(image, greyCircles[a], 5, grayIntensity, 2);

	////OmniViewer.Show(ref imageGray, "imageGray");
	//if (scanOrientation == HORIZONTAL)
	//	imshow("Image Scan Horizontal", image);
	//else   // VERTICAL
	//	imshow("Image Scan Vertical", image);

	//cvWaitKey(0);
}

void CRectifier::DrawNew4ParamCircles(int scanOrientation, Mat image)
{
	if (scanOrientation == HORIZONTAL)
	{
		for (unsigned int a = 0; a < new4ParamHorizontalCorners.size(); a++)
		{
			for (unsigned int b = 0; b < new4ParamHorizontalCorners[a].size(); b++)
			{
				Point2f pt(new4ParamHorizontalCorners[a][b].x, new4ParamHorizontalCorners[a][b].y);
				if (a % 2 == 0)   	
					grayIntensity = 255; 
				else                
					grayIntensity = 127;   
				circle(image, pt, 5, grayIntensity, 2);
			}
		}
		imshow("Image Scan Horizontal", image);
		console_log("Finished drawing horizontal scanning");
	}
	else               // VERTICAL
	{
		for (unsigned int a = 0; a < new4ParamVerticalCorners.size(); a++)
		{
			for (unsigned int b = 0; b < new4ParamVerticalCorners[a].size(); b++)
			{
				Point2f pt(new4ParamVerticalCorners[a][b].x, new4ParamVerticalCorners[a][b].y);
				if (a % 2 == 0)   	// red circles on grey scale
					grayIntensity = 255;
				else                // green circles on grey scale
					grayIntensity = 127; 
				circle(image, pt, 5, grayIntensity, 2);
			}
		}
		imshow("Image Scan Vertical", image);
		console_log("Finished drawing vertical scanning");
	}
	cvWaitKey(0);
}

void CRectifier::DrawNewQuadCircles(int scanOrientation, Mat image)
{
	if (scanOrientation == HORIZONTAL)
	{
		for (unsigned int a = 0; a < newQuadHorizontalCorners.size(); a++)
		{
			for (unsigned int b = 0; b < newQuadHorizontalCorners[a].size(); b++)
			{
				Point2f pt(newQuadHorizontalCorners[a][b].x, newQuadHorizontalCorners[a][b].y);
				if (a % 2 == 0)   	
					grayIntensity = 255;
				else                
					grayIntensity = 127;
				circle(image, pt, 5, grayIntensity, 2);
			}
		}
		imshow("Image Scan Horizontal", image);
		console_log("Finished drawing horizontal scanning");
	}
	else               // VERTICAL
	{
		for (unsigned int a = 0; a < newQuadVerticalCorners.size(); a++)
		{
			for (unsigned int b = 0; b < newQuadVerticalCorners[a].size(); b++)
			{
				Point2f pt(newQuadVerticalCorners[a][b].x, newQuadVerticalCorners[a][b].y);
				if (a % 2 == 0)   	// red circles on grey scale
					grayIntensity = 255;
				else                // green circles on grey scale
					grayIntensity = 127;
				circle(image, pt, 5, grayIntensity, 2);
			}
		}
		imshow("Image Scan Vertical", image);
		console_log("Finished drawing vertical scanning");
	}
	cvWaitKey(0);
}

vector<double> CRectifier::FindEquations(int scanOrientation, vector<Point2f> Corners)
{
	double *t, *y;
	double a_out, b_out, c_out, d_out, e_out;
	vector<double> vResultLine;
	CCurveFitting cf;
	int ret;

	t = new double[Corners.size()];
	y = new double[Corners.size()];

	for (unsigned int a = 0; a < Corners.size(); a++)	{
		if (scanOrientation == HORIZONTAL)   {
			t[a] = (double)(Corners[a].x);
			y[a] = (double)(Corners[a].y);
		}
		else    {
			t[a] = (double)(Corners[a].y);
			y[a] = (double)(Corners[a].x);
		}
	}
	ret = cf.curve_fitting4(t, (int)(Corners.size()), y, &a_out, &b_out, &c_out, &d_out);
	vResultLine.push_back(a_out);
	vResultLine.push_back(b_out);
	vResultLine.push_back(c_out);
	vResultLine.push_back(d_out);
	ret = cf.curve_fitting5(t, (int)(Corners.size()), y, &a_out, &b_out, &c_out, &d_out, &e_out);
	vResultLine.push_back(a_out);
	vResultLine.push_back(b_out);
	vResultLine.push_back(c_out);
	vResultLine.push_back(d_out);
	vResultLine.push_back(e_out);
	ret = cf.quadratic_fitting(t, (int)(Corners.size()), y, &a_out, &b_out, &c_out);
	vResultLine.push_back(a_out);
	vResultLine.push_back(b_out);
	vResultLine.push_back(c_out);

	delete []t;
	delete []y;

	return vResultLine;
}

Mat CRectifier::Init()
{
	Mat image = imread("01.jpg", CV_LOAD_IMAGE_GRAYSCALE); //source image
	SetPatternSize(21, 13);
	return image;
}

void CRectifier::FindChessboardCorners(Mat &image)
{
	bool patternfound = findChessboardCorners(image, patternSize, corners, CALIB_CB_ADAPTIVE_THRESH);
	if (corners.empty())
		console_log("Unable to detect corners");

	cornerSubPix(image, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
}

void CRectifier::Draw_ChessboardCorners_OrderIndicatorLines(Mat &image)
{
	grayIntensity = 127;
	circle(image, corners[0], 3, grayIntensity, 1);
	for (unsigned int i = 1; i < corners.size(); i++)	{
		line(image, corners[i - 1], corners[i], grayIntensity, 1);
		circle(image, corners[i], 3, grayIntensity, 1);
	}
	//imshow("Init", image);
	//cvWaitKey(0);
}

//4-Params: F(x) = ((A - D) / (1 + ((x / C) ^ B))) + D
void CRectifier::GetNewCornersWith4Param(int scanOrientation)
{
	double A, B, C, D;
	float x, y;

	if (scanOrientation == HORIZONTAL)
	{
		for (unsigned int eq = 0; eq < horizontalScanEquations.size(); eq++)
		{
			vector<Point2f> pointList;
			// For 4 Param, only need the first 4 numbers 
			A = horizontalScanEquations[eq][0];
			B = horizontalScanEquations[eq][1];
			C = horizontalScanEquations[eq][2];
			D = horizontalScanEquations[eq][3];
			for (int unsigned idx = 0; idx < horizontalCorners[eq].size(); idx++)
			{
				x = horizontalCorners[eq][idx].x;
				double tempPow = pow((x / C), B);
				if (tempPow != tempPow)
					tempPow = 0.0;
				y = (float)((A - D) / (1 + tempPow) + D);  //y = (float)((A - D) / (1 + pow((x / C), B)) + D);
				Point2f pt(x, y);
				pointList.push_back(pt);
			}
			// Add 5 more extrapolating points outside the checker board
			for (int n = 0; n < 5; n++)
			{
				if (eq % 2 == 0)   // white circles  
					x -= 30.0;
				else               // grey circles
					x += 30.0;
				double tempPow = pow((x / C), B);
				if (tempPow != tempPow)
					tempPow = 0.0;
				y = (float)((A - D) / (1 + tempPow) + D);  //y = (float)((A - D) / (1 + pow((x / C), B)) + D);
				Point2f pt(x, y);
				pointList.push_back(pt);
			}
			new4ParamHorizontalCorners.push_back(pointList);
		}
	}
	else     // VERTICAL
	{
		for (unsigned int eq = 0; eq < verticalScanEquations.size(); eq++)
		{
			vector<Point2f> pointList;
			// For 4 Param, only need the first 4 numbers 
			A = verticalScanEquations[eq][0];
			B = verticalScanEquations[eq][1];
			C = verticalScanEquations[eq][2];
			D = verticalScanEquations[eq][3];
			for (int unsigned idx = 0; idx < verticalCorners[eq].size(); idx++)
			{
				x = verticalCorners[eq][idx].y;
				y = (float)((A - D) / (1 + pow((x / C), B)) + D);
				Point2f pt(y, x);
				pointList.push_back(pt);
			}
			// Add 5 more extrapolating points outside the checker board
			for (int n = 0; n < 5; n++)
			{
				if (eq % 2 == 0)   // white circles  
					x -= 30.0;
				else    // grey circles
					x += 30.0;
				y = (float)((A - D) / (1 + pow((x / C), B)) + D);
				Point2f pt(y, x);
				pointList.push_back(pt);
			}
			new4ParamVerticalCorners.push_back(pointList);
		}
	}
}

//Quadratic: F(x) = Ax^2 + Bx + C
void CRectifier::GetNewCornersWithQuad(int scanOrientation)
{
	double A, B, C;
	float x, y;

	if (scanOrientation == HORIZONTAL)
	{
		for (unsigned int eq = 0; eq < horizontalScanEquations.size(); eq++)
		{
			vector<Point2f> pointList;
			// For Quadratic, only need the last 3 numbers 
			A = horizontalScanEquations[eq][9];
			B = horizontalScanEquations[eq][10];
			C = horizontalScanEquations[eq][11];
			for (int unsigned idx = 0; idx < horizontalCorners[eq].size(); idx++)
			{
				x = horizontalCorners[eq][idx].x;
				y = (float)( (A*pow(x,2)) + (B*x) + C );
				Point2f pt(x, y);
				pointList.push_back(pt);
			}
			// Add 5 more extrapolating points outside the checker board.
			for (int n = 0; n < 5; n++)
			{
				//// METHOD 1: using quadratic equation and the previous three x values.
				//// x=at^2+bt+c
				//Mat matA(3, 3, CV_64F);
				//Mat matb(3, 1, CV_64F);
				//Mat matx(3, 1, CV_64F);
				//Mat maty(1, 1, CV_64F);
				//unsigned int idx = pointList.size() - 1;
				//// At t=idx-2: x[idx-2] = a*(idx-2)^2 + b*(idx-2) + c
				//// At t=idx-1: x[idx-1] = a*(idx-1)^2 + b*(idx-1) + c
				//// At t=idx:   x[idx]   = a*(idx)^2   + b*(idx)   + c
				//// b = Ax, where A = matrix of scalar of a,b,c and matrix b is the values of x[idx-2], x[idx-1], x[idx];
				//matA.at<double>(0, 0) = (double)(pow(idx - 2, 2)); matA.at<double>(0, 1) = (double)(idx - 2); matA.at<double>(0, 2) = 1.0;
				//matA.at<double>(1, 0) = (double)(pow(idx - 1, 2)); matA.at<double>(1, 1) = (double)(idx - 1); matA.at<double>(1, 2) = 1.0;
				//matA.at<double>(2, 0) = (double)(pow(idx, 2));     matA.at<double>(2, 1) = (double)(idx);     matA.at<double>(2, 2) = 1.0;
				//matb.at<double>(0, 0) = (double)(pointList[idx - 2].x);
				//matb.at<double>(1, 0) = (double)(pointList[idx - 1].x);
				//matb.at<double>(2, 0) = (double)(pointList[idx].x);
				//// x = (A^(-1))b
				////matx = (matA.inv(DECOMP_CHOLESKY)) * matb;
				//bool bNonSingular = solve(matA, matb, matx, DECOMP_LU);
				//// y = a*((idx+1)^2) + b*(idx+1) + c
				//maty = ((matx.at<double>(0, 0)) * (pow(idx + 1, 2))) + ((matx.at<double>(1, 0)) * (idx + 1)) + (matx.at<double>(2, 0));
				//x = (float)(maty.at<double>(0, 0));

				//Mat matLast5(1, 5, CV_64F);
				//for (int m = 0; m < 5; m++)
				//	matLast5.at<double>(0, m) = (double)(pointList[pointList.size() - 5 + m].x);
				//maty = matLast5 * matx;

				//x = (float)(maty.at<double>(0, 0));


				//// METHOD 2: Implementation of http:////practicalcryptography.com//miscellaneous//machine-learning//linear-prediction-tutorial//
				//Mat matA(pointList.size() - 5, 5, CV_64F);
				//Mat matb(pointList.size() - 5, 1, CV_64F);
				//Mat matx(5, 1, CV_64F);
				//Mat maty(1, 1, CV_64F);
				//for (unsigned int m = 0; m < pointList.size() - 5; m++)
				//{
				//	matA.at<double>(m, 0) = (double)(pointList[m + 0].x);   matA.at<double>(m, 1) = (double)(pointList[m + 1].x); matA.at<double>(m, 2) = (double)(pointList[m + 2].x); matA.at<double>(m, 3) = (double)(pointList[m + 3].x); matA.at<double>(m, 4) = (double)(pointList[m + 4].x);
				//	matb.at<double>(m, 0) = (double)(pointList[m + 5].x);
				//}

				//if (pointList.size() == 10)  //(matA.cols == matA.rows)
				//{
				//	////x = ((A'A)^-1).A'.b
				//	//Mat matAT = matA.t();
				//	//Mat matATA = matAT * matA;
				//	//Mat matATAinv = matATA.inv(DECOMP_LU);
				//	//Mat tempMat = matATAinv * matAT;
				//	//matx = tempMat * matb;

				//	//Mat matAinv = matA.inv(DECOMP_SVD);
				//	//matx = matAinv * matb;

				//	bool bNonSingular = solve(matA, matb, matx, DECOMP_SVD);
				//}
				//else
				//{
				//	Mat matAinv = matA.inv(DECOMP_SVD);
				//	matx = matAinv * matb;
				//}

				//Mat matLast5(1, 5, CV_64F);
				//for (int m = 0; m < 5; m++)
				//	matLast5.at<double>(0, m) = (double)(pointList[pointList.size()-5+m].x);
				//maty = matLast5 * matx;

				//x = (float)(maty.at<double>(0, 0));


				////METHOD 1B: Using quadratic equation from readerfit.com, F(t) = At^2 + Bt + C
				//double *t2, *y2;
				//double a_out, b_out, c_out;
				//CCurveFitting cf;
				//int ret;

				//t2 = new double[pointList.size()-1];
				//y2 = new double[pointList.size()-1];

				//for (unsigned int a = 1; a < pointList.size(); a++)	{
				//	if (scanOrientation == HORIZONTAL)   {
				//		t2[a-1] = (double)(a);
				//		y2[a-1] = (double)(abs(pointList[a].x - pointList[a - 1].x));  // diff of x as y
				//	}
				//	else    {   // VERTICAL
				//		t2[a-1] = (double)(a);
				//		y2[a-1] = (double)(abs(pointList[a].y - pointList[a - 1].y));
				//	}
				//}
				//ret = cf.quadratic_fitting(t2, (int)(pointList.size()-1), y2, &a_out, &b_out, &c_out);
				////F(t) = (a_out*(t ^ 2)) + (b_out * t) + C
				//double diffx = a_out * (double)(pow(pointList.size(), 2)) + (double)(b_out * pointList.size()) + c_out;
				//if (eq % 2 == 0)   // white circles
				//	x = (pointList[pointList.size() - 1].x) - (float)(diffx);  // x = last x - diff x
				//else			  // grey circles
				//	x = (pointList[pointList.size() - 1].x) + (float)(diffx);  // x = last x + diff x 

				//delete[] t2;
				//delete[] y2;


				// METHOD 3: using exponential equation from http:////web.iitd.ac.in//~pmvs//courses//mel705//curvefitting.pdf
				// y = C * exp(A^x), to get predicted subsequent x values
				double *t2, *y2;
				double a_out, c_out;
				CCurveFitting cf;
				int ret;

				t2 = new double[pointList.size() - 1];
				y2 = new double[pointList.size() - 1];

				for (unsigned int a = 1; a < pointList.size(); a++)	{
					t2[a - 1] = (double)(a);
					y2[a - 1] = (double)(abs(pointList[a].x - pointList[a - 1].x));  // diff of x as y
				}
				ret = cf.exponential_fitting(t2, (int)(pointList.size() - 1), y2, &a_out, &c_out);
				//F(t) = y = C * exp(A^t)   
				double diffx = c_out * (exp(a_out * pointList.size()));
				if (eq % 2 == 0)   // white circles
					x = (pointList[pointList.size() - 1].x) - (float)(diffx);  // x = last x - diff x
				else			  // grey circles
					x = (pointList[pointList.size() - 1].x) + (float)(diffx);  // x = last x + diff x 

				delete[] t2;
				delete[] y2;



				y = (float)((A*pow(x, 2)) + (B*x) + C);
				Point2f pt(x, y);
				pointList.push_back(pt);
			}
			newQuadHorizontalCorners.push_back(pointList);
		}
	}
	else     // VERTICAL
	{
		for (unsigned int eq = 0; eq < verticalScanEquations.size(); eq++)
		{
			vector<Point2f> pointList;
			// For Quadratic, only need the last 3 numbers 
			A = verticalScanEquations[eq][9];
			B = verticalScanEquations[eq][10];
			C = verticalScanEquations[eq][11];
			for (int unsigned idx = 0; idx < verticalCorners[eq].size(); idx++)
			{
				x = verticalCorners[eq][idx].y;
				y = (float)((A*pow(x, 2)) + (B*x) + C);
				Point2f pt(y, x);
				pointList.push_back(pt);
			}
			// Add 5 more extrapolating points outside the checker board
			for (int n = 0; n < 5; n++)
			{
				//if (eq % 2 == 0)   // white circles  
				//	x -= 30.0;
				//else    // grey circles
				//	x += 30.0;

				// METHOD 3: using exponential equation from http:////web.iitd.ac.in//~pmvs//courses//mel705//curvefitting.pdf
				// y = C * exp(A^x), to get predicted subsequent x values
				double *t2, *y2;
				double a_out, c_out;
				CCurveFitting cf;
				int ret;

				t2 = new double[pointList.size() - 1];
				y2 = new double[pointList.size() - 1];

				for (unsigned int a = 1; a < pointList.size(); a++)	{
					t2[a - 1] = (double)(a);
					y2[a - 1] = (double)(abs(pointList[a].y - pointList[a - 1].y));  // diff of y as y
				}
				ret = cf.exponential_fitting(t2, (int)(pointList.size() - 1), y2, &a_out, &c_out);
				//F(t) = y = C * exp(A^t)   
				double diffx = c_out * (exp(a_out * pointList.size()));
				if (eq % 2 == 0)   // white circles
					x = (pointList[pointList.size() - 1].y) - (float)(diffx);  // x = last x - diff x
				else			  // grey circles
					x = (pointList[pointList.size() - 1].y) + (float)(diffx);  // x = last x + diff x 

				delete[] t2;
				delete[] y2;


				y = (float)((A*pow(x, 2)) + (B*x) + C);
				Point2f pt(y, x);
				pointList.push_back(pt);
			}
			newQuadVerticalCorners.push_back(pointList);
		}
	}
}

Mat CRectifier::Init( string fileName, int width, int height )
{
	Mat image = imread(fileName, CV_LOAD_IMAGE_GRAYSCALE); //source image
	//console_log((int)(to_string(image.at<uchar>(0, 0))));
	//console_log((int)(to_string(image.at<uchar>(479, 639))));
	
	//if (image.rows != 480 && image.cols != 640)
	//{
	//	Size size(640, 480);
	//	resize(image, image, size);
	//}
	
	SetPatternSize(width, height);
	return image;
}

void CRectifier::FindCornersPointsSets(int extLCol, int extTRow)
{
	int nSize1, nSize2;
	Point2f ptStart, ptMiddle, ptEnd;

	nSize1 = patternSize.width;
	nSize2 = patternSize.height;

	for (int n = 0; n < nSize1; n++) {
		vector<Point2f> pointList, pointList0, pointList1;

		ptStart = getPoint(n, 0);				
		ptMiddle = getPoint(n, nSize2 / 2);			
		ptEnd = getPoint(n, nSize2 - 1);				

		Point2f ptOld(9999.0, 9999.0);
		float diffMax = 0.0;
		unsigned int indexInflection = 0;

		for (int m = 0; m < nSize2; m++) {
			Point2f ptCurrent;
			ptCurrent = getPoint(n, m); 

			if (ptOld.x != 9999.0 && ptOld.y != 9999.0) {
				float diff;
				diff = abs(ptCurrent.y - ptOld.y);

				if (diff > diffMax) {
					diffMax = diff;
					indexInflection = m;
				}
			}
			pointList.push_back(ptCurrent);
			extCorners[m + extTRow][n + extLCol] = ptCurrent;

			ptOld = ptCurrent;
		}

		for (unsigned int m = 0; m < pointList.size(); m++) {
			Point2f pt = pointList[m];
			if (m < indexInflection)	{
				pointList0.push_back(pt);
			}
			if (m > indexInflection)	{
				pointList1.push_back(pt);
			}
			if (m == indexInflection)	{
				pointList0.push_back(pt);
				pointList1.push_back(pt);
				//greyCircles.push_back(pt);
			}
		}
		reverse(pointList0.begin(), pointList0.end());  // WD, 16 Sep 2014: order of position vector is always from middle position to the end

		// Get equation of each set of points and store them as vector
		vector<double> equation;

		equation = FindQuadEquations(VERTICAL, pointList0);
		verticalScanEquations.push_back(equation);

		equation = FindQuadEquations(VERTICAL, pointList1);
		verticalScanEquations.push_back(equation);

		// Keep original corners set that forms each segment
		verticalCorners.push_back(pointList0);
		verticalCorners.push_back(pointList1);
	}
}


void CRectifier::FindCornersPointsSets2(int extLCol, int extTRow, int extBRow)
{
	int nSize1, nSize2;
	Point2f ptStart, ptMiddle, ptEnd;

	nSize1 = patternSize.height;
	nSize2 = patternSize.width;

	for (int n = 0; n < (extTRow + extBRow + nSize1); n++) {
		vector<Point2f> pointList, pointList0, pointList1;

		ptStart  = extCorners[n][extLCol];
		ptMiddle = extCorners[n][extLCol + (nSize2 / 2)];
		ptEnd    = extCorners[n][extLCol + nSize2 - 1];

		Point2f ptOld(9999.0, 9999.0);
		float diffMax = 0.0;
		unsigned int indexInflection = 0;

		for (int m = 0; m < nSize2; m++) {
			Point2f ptCurrent;
			ptCurrent = extCorners[n][extLCol + m];
			
			if (ptOld.x != 9999.0 && ptOld.y != 9999.0) {
				float diff;
				diff = abs(ptCurrent.x - ptOld.x);

				if (diff > diffMax) {
					diffMax = diff;
					indexInflection = m;
				}
			}
			pointList.push_back(ptCurrent);

			ptOld = ptCurrent;
		}

		for (unsigned int m = 0; m < pointList.size(); m++) {
			Point2f pt = pointList[m];
			if (m < indexInflection)	{
				pointList0.push_back(pt);
//				whiteCircles.push_back(pt);	// WD, 16 Sep 2014: to store all "white circles"
			}
			if (m > indexInflection)	{
				pointList1.push_back(pt);
//				greyCircles.push_back(pt);	// WD, 16 Sep 2014: to store all "grey circles"
			}
			if (m == indexInflection)	{
				pointList0.push_back(pt);
//				whiteCircles.push_back(pt);
				pointList1.push_back(pt);
//				greyCircles.push_back(pt);
			}
		}
		reverse(pointList0.begin(), pointList0.end());  // WD, 16 Sep 2014: order of position vector is always from middle position to the end
		
		// Get equation of each set of points and store them as vector
		vector<double> equation;

		equation = FindQuadEquations(HORIZONTAL, pointList0);
		horizontalScanEquations.push_back(equation);

		equation = FindQuadEquations(HORIZONTAL, pointList1);
		horizontalScanEquations.push_back(equation);


		// Keep original corners set that forms each segment
		horizontalCorners.push_back(pointList0);
		horizontalCorners.push_back(pointList1);
	}
}

vector<vector<Point2f>> CRectifier::InitExtendedCorners(int orgCol, int orgRow, int extLCol, int extRCol, int extTRow, int extBRow)
{
	Point2f pt(0.0, 0.0); // WD: init all (x,y) = (0.0, 0.0)
	vector<vector<Point2f>> vvExtCorners;

	for (int row = 0; row < (orgRow + (extTRow+extBRow)); row++)
	{
		vector<Point2f> rowCorners;

		for (int col = 0; col < (orgCol + (extLCol + extRCol)); col++)
			rowCorners.push_back(pt);
		vvExtCorners.push_back(rowCorners);
	}
	return vvExtCorners;
}

vector<double> CRectifier::FindQuadEquations(int scanOrientation, vector<Point2f> Corners)
{
	double *t, *y;
	double a_out, b_out, c_out;
	vector<double> vResultLine;
	CCurveFitting cf;
	int ret;

	t = new double[Corners.size()];
	y = new double[Corners.size()];

	for (unsigned int a = 0; a < Corners.size(); a++)	{
		if (scanOrientation == HORIZONTAL)   {
			t[a] = (double)(Corners[a].x);
			y[a] = (double)(Corners[a].y);
		}
		else    {
			t[a] = (double)(Corners[a].y);
			y[a] = (double)(Corners[a].x);
		}
	}
	ret = cf.quadratic_fitting(t, (int)(Corners.size()), y, &a_out, &b_out, &c_out);
	vResultLine.push_back(a_out);
	vResultLine.push_back(b_out);
	vResultLine.push_back(c_out);

	delete[]t;
	delete[]y;

	return vResultLine;
}

//Quadratic: F(x) = Ax^2 + Bx + C
void CRectifier::GetNewCornersWithQuad(int extLCol, int extTRow, int extBRow)
{
	double A, B, C;
	float x, y;

	for (unsigned int eq = 0; eq < verticalScanEquations.size(); eq++)
	{
		vector<Point2f> pointList;
		// For Quadratic, only need the last 3 numbers 
		A = verticalScanEquations[eq][0];
		B = verticalScanEquations[eq][1];
		C = verticalScanEquations[eq][2];
		for (int unsigned idx = 0; idx < verticalCorners[eq].size(); idx++)
		{
			x = verticalCorners[eq][idx].y;
			y = (float)((A*pow(x, 2)) + (B*x) + C);
			Point2f pt(y, x);
			pointList.push_back(pt);
		}
		// Add extrapolating points to top and bottom parts
		if (eq % 2 == 0)    // half top corners
		{
			for (int n = 0; n < extTRow; n++)
			{
				// Using exponential equation from http:////web.iitd.ac.in//~pmvs//courses//mel705//curvefitting.pdf
				// y = C * exp(A^x), to get predicted subsequent x values
				double *t2, *y2;
				double a_out, c_out;
				CCurveFitting cf;
				int ret;

				t2 = new double[pointList.size() - 1];
				y2 = new double[pointList.size() - 1];

				for (unsigned int a = 1; a < pointList.size(); a++)	{
					t2[a - 1] = (double)(a);
					y2[a - 1] = (double)(abs(pointList[a].y - pointList[a - 1].y));  // diff of y as y
				}
				ret = cf.exponential_fitting(t2, (int)(pointList.size() - 1), y2, &a_out, &c_out);
				//F(t) = y = C * exp(A^t)   
				double diffx = c_out * (exp(a_out * pointList.size()));
				x = (pointList[pointList.size() - 1].y) - (float)(diffx);  // x = last x - diff x

				delete[] t2;
				delete[] y2;

				y = (float)((A*pow(x, 2)) + (B*x) + C);
				Point2f pt(y, x);
				pointList.push_back(pt);
				extCorners[extTRow - n - 1][extLCol + (eq / 2)] = pt;
			}
//			newQuadVerticalCorners.push_back(pointList);
		}
		else   // half bottom corners
		{
			for (int n = 0; n < extBRow; n++)
			{
				// Using exponential equation from http:////web.iitd.ac.in//~pmvs//courses//mel705//curvefitting.pdf
				// y = C * exp(A^x), to get predicted subsequent x values
				double *t2, *y2;
				double a_out, c_out;
				CCurveFitting cf;
				int ret;

				t2 = new double[pointList.size() - 1];
				y2 = new double[pointList.size() - 1];

				for (unsigned int a = 1; a < pointList.size(); a++)	{
					t2[a - 1] = (double)(a);
					y2[a - 1] = (double)(abs(pointList[a].y - pointList[a - 1].y));  // diff of y as y
				}
				ret = cf.exponential_fitting(t2, (int)(pointList.size() - 1), y2, &a_out, &c_out);
				//F(t) = y = C * exp(A^t)   
				double diffx = c_out * (exp(a_out * pointList.size()));

				x = (pointList[pointList.size() - 1].y) + (float)(diffx);  // x = last x + diff x 

				delete[] t2;
				delete[] y2;

				y = (float)((A*pow(x, 2)) + (B*x) + C);
				Point2f pt(y, x);
				pointList.push_back(pt);

				extCorners[extTRow + patternSize.height + n][extLCol + (eq / 2)] = pt;
			}
//			newQuadVerticalCorners.push_back(pointList);
		}
	}
}

void CRectifier::GetNewCornersWithQuad2(int extLCol, int extRCol)
{
	double A, B, C;
	float x, y;

	for (unsigned int eq = 0; eq < horizontalScanEquations.size(); eq++)
	{
		vector<Point2f> pointList;
		// For Quadratic, only need the last 3 numbers 
		A = horizontalScanEquations[eq][0];
		B = horizontalScanEquations[eq][1];
		C = horizontalScanEquations[eq][2];
		for (int unsigned idx = 0; idx < horizontalCorners[eq].size(); idx++)
		{
			x = horizontalCorners[eq][idx].x;
			y = (float)((A*pow(x, 2)) + (B*x) + C);
			Point2f pt(x, y);
			pointList.push_back(pt);
		}
		// Add extrapolating corners to the left and right sides
		if (eq % 2 == 0)   // half left-hand corners
		{
			// Add 5 more extrapolating points outside the checker board.
			for (int n = 0; n < extLCol; n++)
			{
				// METHOD 3: using exponential equation from http:////web.iitd.ac.in//~pmvs//courses//mel705//curvefitting.pdf
				// y = C * exp(A^x), to get predicted subsequent x values
				double *t2, *y2;
				double a_out, c_out;
				CCurveFitting cf;
				int ret;

				t2 = new double[pointList.size() - 1];
				y2 = new double[pointList.size() - 1];

				for (unsigned int a = 1; a < pointList.size(); a++)	{
					t2[a - 1] = (double)(a);
					y2[a - 1] = (double)(abs(pointList[a].x - pointList[a - 1].x));  // diff of x as y
				}
				ret = cf.exponential_fitting(t2, (int)(pointList.size() - 1), y2, &a_out, &c_out);
				//F(t) = y = C * exp(A^t)   
				double diffx = c_out * (exp(a_out * pointList.size()));

				x = (pointList[pointList.size() - 1].x) - (float)(diffx);  // x = last x - diff x

				delete[] t2;
				delete[] y2;

				y = (float)((A*pow(x, 2)) + (B*x) + C);
				Point2f pt(x, y);
				pointList.push_back(pt);

				extCorners[eq / 2][extLCol - n - 1] = pt;

			}
//			newQuadHorizontalCorners.push_back(pointList);
		}
		else    // half right-hand corners
		{
			// Add 5 more extrapolating points outside the checker board.
			for (int n = 0; n < extRCol; n++)
			{
				// METHOD 3: using exponential equation from http:////web.iitd.ac.in//~pmvs//courses//mel705//curvefitting.pdf
				// y = C * exp(A^x), to get predicted subsequent x values
				double *t2, *y2;
				double a_out, c_out;
				CCurveFitting cf;
				int ret;

				t2 = new double[pointList.size() - 1];
				y2 = new double[pointList.size() - 1];

				for (unsigned int a = 1; a < pointList.size(); a++)	{
					t2[a - 1] = (double)(a);
					y2[a - 1] = (double)(abs(pointList[a].x - pointList[a - 1].x));  // diff of x as y
				}
				ret = cf.exponential_fitting(t2, (int)(pointList.size() - 1), y2, &a_out, &c_out);
				//F(t) = y = C * exp(A^t)   
				double diffx = c_out * (exp(a_out * pointList.size()));

				x = (pointList[pointList.size() - 1].x) + (float)(diffx);  // x = last x + diff x 

				delete[] t2;
				delete[] y2;

				y = (float)((A*pow(x, 2)) + (B*x) + C);
				Point2f pt(x, y);
				pointList.push_back(pt);

				extCorners[eq / 2][extLCol + patternSize.width + n] = pt;
			}
//			newQuadHorizontalCorners.push_back(pointList);
		}
	}
}

void CRectifier::Draw_ExtCorners(Mat &image)
{
	for (unsigned int row = 0; row < extCorners.size(); row++)
	{
		for (unsigned int col = 0; col < extCorners[row].size(); col++)
		{
			Point2f pt(extCorners[row][col].x, extCorners[row][col].y);
			grayIntensity = 255;
			circle(image, pt, 5, grayIntensity, 2);
		}
	}
//	imshow("Extended checkerboard", image);
//	console_log("Finished drawing");

//	cvWaitKey(0);
}

void CRectifier::Draw_Intermediate(Mat image)
{
	for (unsigned int row = 0; row < extCorners.size(); row++)
	{
		for (unsigned int col = 0; col < extCorners[row].size(); col++)
		{
			Point2f pt(extCorners[row][col].x, extCorners[row][col].y);
			grayIntensity = 255;
			circle(image, pt, 5, grayIntensity, 2);
		}
	}
	imshow("Intermediate checkerboard", image);
	console_log("Finished drawing");

	cvWaitKey(0);
}

//void CRectifier::Transform(Mat image, int extLCol, int extRCol, int extTRow, int extBRow)
//{
//	// Input Quadilateral or Image plane coordinates
//	Point2f inputQuad[4];
//	
//	// Output Quadilateral or World plane coordinates
//	Point2f outputQuad[4];
//
//	// Lambda Matrix
//	Mat lambda(2, 4, CV_32FC1);  // Mat lambda;
//	lambda = Mat::zeros(image.rows, image.cols, image.type());
//	// Output Image;
//	//Mat aux;
//	Mat newImage(image.rows, image.cols, CV_8UC1);
//	//Mat newImage = Mat::zeros(image.rows, image.cols, CV_8UC1); 
//
//	// Set the lambda matrix the same type and size as input
//	//lambda = Mat::zeros(image.rows, image.cols, image.type());
//
//	//// The 4 points that select quadilateral on the input , from top-left in clockwise order
//	//// These four pts are the sides of the rect box used as inputIMAGE
//	//inputQuad[0] = Point2f(extCorners[0][0].x, extCorners[0][0].y);
//	//inputQuad[1] = Point2f(extCorners[0][extCorners[0].size() - 1].x, extCorners[0][extCorners[0].size() - 1].y);
//	//inputQuad[2] = Point2f(extCorners[extCorners.size() - 1][extCorners[0].size() - 1].x, extCorners[extCorners.size() - 1][extCorners[0].size() - 1].y);
//	//inputQuad[3] = Point2f(extCorners[extCorners.size() - 1][0].x, extCorners[extCorners.size() - 1][0].y);
//	//// The 4 points where the mapping is to be done , from top-left in clockwise order
//	//outputQuad[0] = Point2f(0.0, 0.0);
//	//outputQuad[1] = Point2f(float(image.cols - 1), 0.0);
//	//outputQuad[2] = Point2f(float(image.cols - 1), float(image.rows - 1));
//	//outputQuad[3] = Point2f(0.0, float(image.rows - 1));
//
//	//// Get the Perspective Transform Matrix i.e. lambda
//	//lambda = getPerspectiveTransform(inputQuad, outputQuad);
//	//// Apply the Perspective Transform just found to the src image
//	//warpPerspective(image, output, lambda, output.size());
//
//
//	float cellWidth, cellHeight;
//
////	(image.rows % (patternSize.height + extTRow + extBRow) == 0) ? \
////		cellHeight = image.rows / (patternSize.height + extTRow + extBRow) : \
////		cellHeight = floor(image.rows / (patternSize.height + extTRow + extBRow));
////	(image.cols % (patternSize.width + extLCol + extRCol) == 0) ? \
////		cellWidth = image.cols / (patternSize.width + extLCol + extRCol) : \
////		cellWidth = floor(image.cols / (patternSize.width + extLCol + extRCol));
//
//	cellHeight = image.rows / (patternSize.height + extTRow + extBRow);
//	cellWidth = image.cols / (patternSize.width + extLCol + extRCol);
//
//	for (int row = 1; row < (patternSize.height + extTRow + extBRow); row++)
////	for (int row = 1; row < 5; row++)
//	{
//		for (int col = 1; col < (patternSize.width + extLCol + extRCol); col++)
//		{
//			// The 4 points that select quadilateral on the input , from top-left in clockwise order
//			// These four pts are the sides of the rect box used as inputIMAGE
//			//inputQuad[0] = Point2f( extCorners[row - 1][col - 1].x,       extCorners[row - 1][col - 1].y);
//			//inputQuad[1] = Point2f((extCorners[row - 1][col    ].x) - 1,  extCorners[row - 1][col    ].y);
//			//inputQuad[2] = Point2f((extCorners[row    ][col    ].x) - 1, (extCorners[row    ][col    ].y) - 1);
//			//inputQuad[3] = Point2f( extCorners[row    ][col - 1].x,      (extCorners[row    ][col - 1].y) - 1);
//
//			//inputQuad[0] = Point2f(extCorners[row - 1][col - 1].x, extCorners[row - 1][col - 1].y);
//			//inputQuad[1] = Point2f((extCorners[row - 1][col].x),   extCorners[row - 1][col].y);
//			//inputQuad[2] = Point2f((extCorners[row][col].x),      (extCorners[row][col].y));
//			//inputQuad[3] = Point2f(extCorners[row][col - 1].x,    (extCorners[row][col - 1].y));
//
//			vector<Point2f> inputQuadV;
//			inputQuadV.push_back(Point2f(extCorners[row - 1][col - 1].x, extCorners[row - 1][col - 1].y));
//			inputQuadV.push_back(Point2f((extCorners[row - 1][col].x), extCorners[row - 1][col].y));
//			inputQuadV.push_back(Point2f((extCorners[row][col].x), (extCorners[row][col].y)));
//			inputQuadV.push_back(Point2f(extCorners[row][col - 1].x, (extCorners[row][col - 1].y)));
//
//
//			// The 4 points where the mapping is to be done , from top-left in clockwise order
//			//outputQuad[0] = Point2f(float((col-1)*cellWidth), float((row-1)*cellHeight));
//			//outputQuad[1] = Point2f(float(col*cellWidth-1),   float((row-1)*cellHeight));
//			//outputQuad[2] = Point2f(float(col*cellWidth-1),   float(row*cellHeight-1));
//			//outputQuad[3] = Point2f(float((col-1)*cellWidth), float(row*cellHeight-1));
//
//			//outputQuad[0] = Point2f(float((col - 1)*cellWidth), float((row - 1)*cellHeight));
//			//outputQuad[1] = Point2f(float(col*cellWidth),       float((row - 1)*cellHeight));
//			//outputQuad[2] = Point2f(float(col*cellWidth),       float(row*cellHeight));
//			//outputQuad[3] = Point2f(float((col - 1)*cellWidth), float(row*cellHeight));
//
//			vector<Point2f> outputQuadV;
//			outputQuadV.push_back(Point2f(float((col - 1)*cellWidth), float((row - 1)*cellHeight)));
//			outputQuadV.push_back(Point2f(float(col*cellWidth), float((row - 1)*cellHeight)));
//			outputQuadV.push_back(Point2f(float(col*cellWidth), float(row*cellHeight)));
//			outputQuadV.push_back(Point2f(float((col - 1)*cellWidth), float(row*cellHeight)));
//
//			// Get the Perspective Transform Matrix i.e. lambda
//			lambda = getPerspectiveTransform(inputQuadV, outputQuadV);
//			// Apply the Perspective Transform just found to the src image
//			Rect rect(int(extCorners[row - 1][col - 1].x), int(extCorners[row - 1][col - 1].y), int(extCorners[row - 1][col].x - extCorners[row - 1][col - 1].x), int(extCorners[row][col - 1].y - extCorners[row - 1][col - 1].y));
//			Mat output(int(cellHeight), int(cellWidth), CV_8UC1);
//			vector<Point2f> outputV;
//
//			//Mat tempMat(inputQuad);
//			warpPerspective(inputQuadV, outputV, lambda, Size(outputV.size())); //warpPerspective(image(rect), output, lambda, output.size());
//			
//			//// Get the Perspective Transform Matrix i.e. lambda
//			//lambda = getAffineTransform(inputQuad, outputQuad);
//			//// Apply the Perspective Transform just found to the src image
//			//Rect rect(int(extCorners[row - 1][col - 1].x), int(extCorners[row - 1][col - 1].y), int(extCorners[row - 1][col].x - extCorners[row - 1][col - 1].x), int(extCorners[row][col - 1].y - extCorners[row - 1][col - 1].y));
//			//Mat output(int(cellHeight), int(cellWidth), CV_8UC1);
//			//warpAffine(image(rect), output, lambda, output.size());
//
//			imshow("Input", inputQuadV); //imshow("Input", image(rect));
//			imshow("Output", output);
//			cvWaitKey(0);
//
//			//Mat aux = newImage.colRange((col-1)*cellWidth, col*cellWidth).rowRange((row-1)*cellHeight, row*cellHeight); // pointing to desired submatrix
//			//output.copyTo(aux);
//
//			// http:////stackoverflow.com//questions//25808790//opencv-is-matvectorpoint2f-yelding-a-wrong-matrix-header
//		}
//	}
//
//	   
//
//	//Display input and output
//	imshow("Input", image);
//	imshow("New Image", newImage);
//
//	cvWaitKey(0);
//}

int compare(const void * a, const void * b)
{
	return (*(int*)a - *(int*)b);
}

void CRectifier::Transform2(Mat image, int extLCol, int extRCol, int extTRow, int extBRow, string fileName)
{
	FILE *file1;
	string txtFileName ("rect");
	txtFileName = txtFileName + fileName[0] + ".txt";
	file1 = fopen(txtFileName.c_str(), "w+");

	double cellHeight = double(image.rows) / double(patternSize.height + extTRow + extBRow - 1);
	double cellWidth = double(image.cols) / double(patternSize.width + extLCol + extRCol - 1);

	double c1x, c1y, c2x, c2y, c3x, c3y, c4x, c4y;			//c1 c2 c3 c4 are distorted corners
	double cf1x, cf1y, cf2x, cf2y, cf3x, cf3y, cf4x, cf4y;  //cf1 cf2 cf3 cf4 are rectified corners
	float  resultx, resulty;								// warped points
	Mat matWarped(image.rows, image.cols, CV_8UC1);
	matWarped = Mat(image.rows, image.cols, CV_8UC1, Scalar(255));

	for (int row = 1; row < (patternSize.height + extTRow + extBRow); row++)
	{
		for (int col = 1; col < (patternSize.width + extLCol + extRCol); col++)
		{
			CWarper warper;

			c1x = double(maxExtCorners[row - 1][col - 1].x); c1y = double(maxExtCorners[row - 1][col - 1].y);
			c2x = double(maxExtCorners[row - 1][col].x);     c2y = double(maxExtCorners[row - 1][col].y);
			c3x = double(maxExtCorners[row][col].x);     	 c3y = double(maxExtCorners[row][col].y);
			c4x = double(maxExtCorners[row][col - 1].x);	 c4y = double(maxExtCorners[row][col - 1].y);
			warper.setSource(c1x, c1y, c2x, c2y, c3x, c3y, c4x, c4y);

			cf1x = (col - 1)*cellWidth; cf1y = (row - 1)*cellHeight;
			cf2x = col*cellWidth;       cf2y = (row - 1)*cellHeight;
			cf3x = col*cellWidth;       cf3y = row*cellHeight;
			cf4x = (col - 1)*cellWidth; cf4y = row*cellHeight;
			warper.setDestination(cf1x, cf1y, cf2x, cf2y, cf3x, cf3y, cf4x, cf4y);

			//x0 y0 are coordinates of point to be rectified, and resutx resulty are outputs of rectified points
			// WD, 30 Sep 2014: for showing the image
			// --->>>
			//for (double y0 = c1y; y0 < c4y; y0=y0+0.1)
			//{
			//	if (y0 < 0 || y0 >= image.rows)
			//		continue;
			//	for (double x0 = c1x; x0 < c2x; x0=x0+0.1)
			//	{
			//		if (x0 < 0 || x0 >= image.cols)
			//			continue;
			//		warper.warp(x0, y0, resultx, resulty);
			//		resultx <= 0.0 ? resultx = 0.0 : resultx = resultx;
			//		resulty <= 0.0 ? resulty = 0.0 : resulty = resulty;
			//		resultx >= (image.cols - 0.1) ? resultx = (image.cols - 0.1) : resultx = resultx;
			//		resulty >= (image.rows - 0.1) ? resulty = (image.rows - 0.1) : resulty = resulty;
			//		matWarped.at<uchar>(resulty, resultx) = image.at<uchar>(y0, x0);
			//	}
			//}
			// <<<---

			// WD, 30 Sep 2014: for storing the pixel pairs into file
			// --->>>
			for (int y0 = int(round(c1y)); y0 < int(round(c4y)); y0++)
			{
				if (y0 < 0 || y0 >= image.rows)
					continue;

				for (int x0 = int(round(c1x)); x0 < int(round(c2x)); x0++)
				{
					if (x0 < 0 || x0 >= image.cols)
						continue;

					warper.warp(double(x0), double(y0), resultx, resulty);
					resultx <= 0.0 ? resultx = 0.0 : resultx = resultx;
					resulty <= 0.0 ? resulty = 0.0 : resulty = resulty;
					resultx >= (image.cols - 1.0) ? resultx = (image.cols - 1.0) : resultx = resultx;
					resulty >= (image.rows - 1.0) ? resulty = (image.rows - 1.0) : resulty = resulty;
					matWarped.at<uchar>(int(round(resulty)), int(round(resultx))) = image.at<uchar>(y0, x0);

					fprintf(file1, "%3d, %3d, %3d, %3d; ", x0, y0, int(round(resultx)), int(round(resulty)));
					
				}
				fprintf(file1, "\n");
			}
		}
	}

	//// Effort to eliminate white lines
	//// Way 1: replace white pixel with next non-white pixel
	//for (int row = 0; row < image.rows; row++)
	//{
	//	for (int col = 0; col < image.cols; col++)
	//	{
	//		if (int(matWarped.at<uchar>(row, col)) == 255)
	//		{
	//			bool modified = false;
	//			for (int crow = row; crow < image.rows; crow++)
	//			{
	//				for (int ccol = col + 1; ccol < image.cols; ccol++)
	//				{
	//					if (int(matWarped.at<uchar>(crow, ccol)) < 255) // non-white
	//					{
	//						matWarped.at<uchar>(row, col) = matWarped.at<uchar>(crow, ccol);
	//						modified = true;
	//						break;
	//					}
	//				}
	//				if (modified)	break;
	//			}
	//		}
	//	}
	//}

	//// Effort to eliminate white lines
	//// Way 2: replace white pixel with the median of 4 surrounding pixel values
	//int intArr[4], medianValue;
	//for (int row = 1; row < image.rows - 1; row++)
	//{
	//	for (int col = 1; col < image.cols - 1; col++)
	//	{
	//		if (int(matWarped.at<uchar>(row, col)) == 255)
	//		{
	//			intArr[0] = int(matWarped.at<uchar>(row - 1, col));  // top
	//			intArr[1] = int(matWarped.at<uchar>(row, col + 1));  // right
	//			intArr[2] = int(matWarped.at<uchar>(row + 1, col));	 // bottom
	//			intArr[3] = int(matWarped.at<uchar>(row, col - 1));	 // left
	//			qsort(intArr, 4, sizeof(int), compare);

	//			medianValue = int((intArr[1] + intArr[2]) / 2);
	//			matWarped.at<uchar>(row, col) = (uchar)(medianValue);
	//		}
	//	}
	//}

	fclose(file1);
//	imshow("Input", image);
//	imshow("Output", matWarped);
//	cvWaitKey(0);
}

void CRectifier::GetMaxExtendedCorners(Mat image, int &extLCol, int &extRCol, int &extTRow, int &extBRow)
{
	int maxLeft, maxRight, maxTop, maxBottom;
	bool bFound;

	// Check top
	bFound = false;
	maxTop = extTRow;
	for (int row = extTRow-1; row >= 0; row--)
	{
		for (int col = 0; col < extCorners[row].size(); col++)
		{
			if (extCorners[row][col].y < 0.0)	{
				maxTop = extTRow - row - 1;
				bFound = true;
				break;
			}
		}
		if (bFound)
			break;
	}

	// Check bottom
	bFound = false;
	maxBottom = extBRow;
	for (int row = (extTRow + patternSize.height); row < (extTRow + patternSize.height + extBRow); row++)
	{
		for (int col = 0; col < extCorners[row].size(); col++)
		{
			if (extCorners[row][col].y > float(image.rows - 1))	{
				maxBottom = row - (extTRow + patternSize.height);
				bFound = true;
				break;
			}
		}
		if (bFound)
			break;
	}

	// Check left
	bFound = false;
	maxLeft = extLCol;
	for (int row = 0; row < extCorners.size(); row++)
	{
		for (int col = (extLCol-1); col >= 0; col--)
		{
			if (extCorners[row][col].x < 0.0)	{
				maxLeft = extLCol - col - 1;
				bFound = true;
				break;
			}
		}
		if (bFound)
			break;
	}

	// Check right
	bFound = false;
	maxRight = extRCol;
	for (int row = 0; row < extCorners.size(); row++)
	{
		for (int col = (extLCol + patternSize.width); col < (extLCol + patternSize.width + extRCol); col++)
		{
			if (extCorners[row][col].x > float(image.cols - 1))	{
				maxRight = col- (extLCol + patternSize.width);
				bFound = true;
				break;
			}
		}
		if (bFound)
			break;
	}

	//Init maxExtCorners
	maxExtCorners = InitExtendedCorners(patternSize.width, patternSize.height, maxLeft, maxRight, maxTop, maxBottom);

	// Form new corners
	int r = -1, c;
	for (int row = (extTRow - maxTop); row < (extTRow + patternSize.height + maxBottom); row++)
	{
		r++;
		c = -1;
		for (int col = (extLCol - maxLeft); col < (extLCol + patternSize.width + maxRight); col++)
		{
			c++;
			maxExtCorners[r][c] = extCorners[row][col];
		}
	}

	extTRow = maxTop;
	extBRow = maxBottom;
	extLCol = maxLeft;
	extRCol = maxRight;
}

void CRectifier::CleanUp()
{
	extCorners.clear();
	horizontalScanEquations.clear();
	verticalScanEquations.clear();
	horizontalCorners.clear();
	verticalCorners.clear();
	//new4ParamHorizontalCorners.clear();
	//new4ParamVerticalCorners.clear();
	//newQuadHorizontalCorners.clear();
	//newQuadVerticalCorners.clear();
	maxExtCorners.clear();
}

/*int main(int argc, char* argv[])
{
	CRectifier Rect1;
//	Mat imageWithoutCircles;
	int orgRow = 13, orgCol = 21;
//	int extTRow = 3, extBRow = 4-1, extLCol = 4-1,  extRCol = 5;
	int extTRow, extBRow, extLCol, extRCol;


//	Mat image = Rect1.Init();
//	Rect1.FindChessboardCorners(image);
//	Rect1.Draw_ChessboardCorners_OrderIndicatorLines(image);
////	image.copyTo(imageWithoutCircles);

//	// Horizontal scanning
//	Rect1.FindCornersPointsSets(HORIZONTAL);
////	Rect1.DrawCircles(HORIZONTAL, image);
//	//Rect1.GetNewCornersWith4Param(HORIZONTAL);
//	//Rect1.DrawNew4ParamCircles(HORIZONTAL, image);
//	Rect1.GetNewCornersWithQuad(HORIZONTAL);
//	Rect1.DrawNewQuadCircles(HORIZONTAL, image);
//
//
//
//	imageWithoutCircles.copyTo(image);
//	// These vectors are cleared for drawing VERTICAL scanning
//	Rect1.whiteCircles.clear();
//	Rect1.greyCircles.clear();

//	// Vertical scanning
//	Rect1.FindCornersPointsSets(VERTICAL);
////	Rect1.DrawCircles(VERTICAL, image);
//	//Rect1.GetNewCornersWith4Param(VERTICAL);
//	//Rect1.DrawNew4ParamCircles(VERTICAL, image);
//	Rect1.GetNewCornersWithQuad(VERTICAL);
////	Rect1.DrawNewQuadCircles(VERTICAL, image);


	for (int i = 0; i < 2; i++)
	{
		Rect1.CleanUp();

		ostringstream ss;
		ss << i;
		string strFileName(ss.str());
		strFileName = strFileName + ".jpg";

		Mat image = Rect1.Init(strFileName, orgCol, orgRow);
		Rect1.FindChessboardCorners(image);
	//	Rect1.Draw_ChessboardCorners_OrderIndicatorLines(image);

		extTRow = 5; extBRow = 5; extLCol = 5; extRCol = 5;
		Rect1.extCorners = Rect1.InitExtendedCorners(orgCol, orgRow, extLCol, extRCol, extTRow, extBRow);
		Rect1.FindCornersPointsSets(extLCol, extTRow);
		Rect1.GetNewCornersWithQuad(extLCol, extTRow, extBRow);
	//	Rect1.Draw_Intermediate(image);
		Rect1.FindCornersPointsSets2(extLCol, extTRow, extBRow);
		Rect1.GetNewCornersWithQuad2(extLCol, extRCol);
	//	Rect1.Draw_ExtCorners(image);

		Rect1.GetMaxExtendedCorners(image, extLCol, extRCol, extTRow, extBRow);
		Rect1.Transform2(image, extLCol, extRCol, extTRow, extBRow, strFileName);
	}

	return 0;
}*/

CRectifier Rect1;

int orgRow = 13, orgCol = 21;
int extTRow, extBRow, extLCol, extRCol;

void compute_rectification_data(string image_file_name_without_extension)
{
	Rect1.CleanUp();

	Mat image = Rect1.Init(image_file_name_without_extension + ".jpg", orgCol, orgRow);
	Rect1.FindChessboardCorners(image);

	extTRow = 5; extBRow = 5; extLCol = 5; extRCol = 5;
	Rect1.extCorners = Rect1.InitExtendedCorners(orgCol, orgRow, extLCol, extRCol, extTRow, extBRow);
	Rect1.FindCornersPointsSets(extLCol, extTRow);
	Rect1.GetNewCornersWithQuad(extLCol, extTRow, extBRow);
	Rect1.FindCornersPointsSets2(extLCol, extTRow, extBRow);
	Rect1.GetNewCornersWithQuad2(extLCol, extRCol);

	Rect1.GetMaxExtendedCorners(image, extLCol, extRCol, extTRow, extBRow);
	Rect1.Transform2(image, extLCol, extRCol, extTRow, extBRow, image_file_name_without_extension);
}