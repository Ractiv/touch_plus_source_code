#pragma once

#include <opencv/cv.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <iostream>
#include <windows.h>


#include "..\CurveFitting\curve_fitting.h"

using namespace std;
using namespace cv;

enum ScanOrientation
{
	HORIZONTAL = 0,
	VERTICAL = 1
};

int main(int argc, char* argv[]);

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

class CWarper
{
public: 
	CWarper(void);
	void setSource(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3);
	void setDestination(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3);
	void warp(double srcX, double srcY, double &dstX, double &dstY);


private:
	double srcX[4];
	double srcY[4];
	double dstX[4];
	double dstY[4];
	double srcMat[16];
	double dstMat[16];
	double warpMat[16];
	bool   dirty;

	void computeSquareToQuad(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double mat[]);
	void computeQuadToSquare(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double mat[]);
	void multMats(double srcMat[], double dstMat[], double resMat[]);
	void setIdentity(void);
	void warp(double mat[], double srcX, double srcY, double &dstX, double &dstY);
	void computeWarp(void);
};