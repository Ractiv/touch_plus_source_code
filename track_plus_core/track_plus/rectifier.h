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

using namespace std;
using namespace cv;

enum ScanOrientation
{
	HORIZONTAL = 0,
	VERTICAL = 1
};

// int main(int argc, char* argv[]);

class CRectifier
{
public:
	Mat  Init();
	void FindChessboardCorners(Mat &image);
	void Draw_ChessboardCorners_OrderIndicatorLines(Mat &image);

	vector<Point2f> whiteCircles, greyCircles;

	void FindCornersPointsSets(int scanOrientation);
	void DrawCircles(int scanOrientation, Mat &image);
	void GetNewCornersWith4Param(int scanOrientation);
	void DrawNew4ParamCircles(int scanOrientation, Mat image);
	void GetNewCornersWithQuad(int scanOrientation);
	void DrawNewQuadCircles(int scanOrientation, Mat image);


	Mat  Init(string fileName, int width, int height);
	vector<vector<Point2f>> InitExtendedCorners(int orgCol, int orgRow, int extLCol, int extRCol, int extTRow, int extBRow);
	void FindCornersPointsSets(int extLCol, int extTRow);
	void FindCornersPointsSets2(int extLCol, int extTRow, int extBRow);
	void GetNewCornersWithQuad(int extCol, int extTRow, int extBRow);
	void GetNewCornersWithQuad2(int extLCol, int extRCol);
	void Draw_ExtCorners(Mat &image);
	void Draw_Intermediate(Mat image);
	//void Transform(Mat image, int extLCol, int extRCol, int extTRow, int extBRow);
	void Transform2(Mat image, int extLCol, int extRCol, int extTRow, int extBRow, string fileName);
	void GetMaxExtendedCorners(Mat image, int &extLCol, int &extRCol, int &extTRow, int &extBRow);
	void CleanUp(void);

	vector<vector<Point2f>> extCorners;  // WD: after checkerboard is extended
	
private:
//	Mat imageGray;
	Size patternSize;
	Scalar grayIntensity;
	vector<Point2f> corners; //this will be filled by the detected centers
//	vector<vector<Point2f>> listScanHorizontal, listScanVertical;
//	vector<vector<vector<double>>> vvvHorResultRowCol, vvvVerResultRowCol;
	vector<vector<double>> horizontalScanEquations, verticalScanEquations;
	vector<vector<Point2f>> horizontalCorners, verticalCorners;
	vector<vector<Point2f>> new4ParamHorizontalCorners, new4ParamVerticalCorners;
	vector<vector<Point2f>> newQuadHorizontalCorners, newQuadVerticalCorners;

	void SetPatternSize(int width, int height);     //number of centers
	Point2f getPoint(int row, int col);
	vector<double> FindEquations(int scanOrientation, vector<Point2f> Corners);


	vector<double> FindQuadEquations(int scanOrientation, vector<Point2f> Corners);

	vector<vector<Point2f>> maxExtCorners;
};

void compute_rectification_data(string image_file_name_without_extension);