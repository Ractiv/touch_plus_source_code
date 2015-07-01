#include <iostream>    
#include "lmcurve.h"
#include <stdio.h>
#include <math.h>
#include "curve_fitting.h"

using namespace std;


// 4: F(x) = ((A-D)/(1+((x/C)^B))) + D
double f( double t, const double *p )
{
	// WD, 9 Sep 2014: to anticipate if -1.#IND is detected
	// --->>>
    // return ((p[0] - p[3]) / (1 + pow((t / p[2]), p[1]))) + p[3];

	double tempPow;
	tempPow = pow((t / p[2]), p[1]);
	if (tempPow != tempPow)
		tempPow = 0.0;

	return (((p[0] - p[3]) / (1 + tempPow)) + p[3]);
	// <<<---
}


double CCurveFitting::VMAX(double* d, int count)
{
    double max = d[0];
    for (size_t i = 1; i < count; i++)
    {
        max = d[i] > max ? d[i] : max;
    }
    return max;
}


double CCurveFitting::VMIN(double* d, int count)
{
    double min = d[0];
    for (size_t i = 1; i < count; i++)
    {
        min = d[i] < min ? d[i] : min;
    }
    return min;
}

 
double CCurveFitting::VAVERAGE(double* independentArray, double* resposne, int count, double midValue)
{
    double diff = abs(resposne[0] - midValue);
    double vaverage = resposne[0];
    int index = 0;
    for (size_t i = 0; i < count; i++)
    {
        vaverage = abs(resposne[i] - midValue) < diff ? resposne[i] : vaverage;
        diff = resposne[i] - midValue;
        index = i;
    }

    return independentArray[index];
}


double CCurveFitting::VSLOPE(double* x, double* y, int count)
{
    return (y[count - 1] - y[0]) / (x[count] - x[0]);
}


int CCurveFitting::curve_fitting4(double* t, int t_len, double* y, double* a_out, double* b_out, double* c_out, double* d_out)
{
    /* data points: a slightly distorted standard parabola */
    int m = t_len;

    // for test.
    //double t[10] = { 233, 202, 181, 164, 151, 142, 134, 127, 122, 118 };
    //double y[10] = { 10, 12, 14, 16, 18, 20, 22, 24, 16, 18 };


    // prefer a initial guess for A, B, C, D
    // A = guess it with the minimum value of the Response variable
    // B = slope of the line between first and last point. The slope is given by ¦¤y/¦¤x
    // C = approximately the dose whose response is nearest to the mid response 
    // D = guess it with the maximum value of the Response variable

	double par[4]; // WD, 9 Sep 2014: no need to initialize... = { 118, -10, 20, 202 }; /* really bad starting value */
    par[0] = VMIN(y, t_len);
    par[3] = VMAX(y, t_len);
    par[1] = VSLOPE(t, y, t_len);
    par[2] = VAVERAGE(t, y, t_len, (par[3] - par[0]) / 2);


    lm_control_struct control = lm_control_double;
    lm_status_struct status;
    control.verbosity = 9;

    //printf( "Fitting ...\n" );
    // now the call to lmfit
	lmcurve(4, par, m, t, y, f, &control, &status);
	
	// WD, 9 Sep 2014: to show the result for curvefitting4
	COUT << endl << "a: " << par[0] << " b: " << par[1] << " c: " << par[2] << " d: " << par[3] << endl;
    *a_out = par[0];
    *b_out = par[1];
    *c_out = par[2];
    *d_out = par[3];

    return 0;
}


// WD, 9 Sep 2014: the output result of this function is different from that of readerFit.com, version used is different?
// F(x) = D+(A-D)/((1+(x/C)^B)^E)    from http://www.mathworks.com/matlabcentral/fileexchange/38043-five-parameters-logistic-regression-there-and-back-again
// F(x) = A + (D/(1+(X/C)^B)^E)      from readerFit, why different? TODO (peter)
double f5(double t, const double *p)
{
	// WD, 9 Sep 2014: to anticipate if -1.#IND is detected
	// --->>>
    ////return p[3] + (p[0] - p[3]) / pow((1 + pow((t/p[2]), p[1])), p[4]);
    //return p[0] +  p[3] / pow((1 + pow((t/p[2]), p[1])), p[4]);

	double tempPow, tempPow2;

	tempPow = pow((t / p[2]), p[1]);
	if (tempPow != tempPow)
		tempPow = 0.0;

	tempPow2 = pow((1 + tempPow), p[4]);
	if (tempPow2 < 1.0E-100)
		return 1.0E100;   // WD, 9 Sep 2014: should explore why get tempPow2's precision very large. Just return a very huge number for now to avoid -1.#IND

	return (p[0] + p[3] / tempPow2);
	// <<<---
}


int CCurveFitting::curve_fitting5(double* t, int t_len, double* y, double* a_out, double* b_out, double* c_out, double* d_out, double* e_out)
{
    /* data points: a slightly distorted standard parabola */
    int m = t_len;

    // fot test
    //double t[10] = { 233, 202, 181, 164, 151, 142, 134, 127, 122, 118 };
    //double y[10] = { 10, 12, 14, 16, 18, 20, 22, 24, 16, 18 };

    // prefer a initial guess for A, B, C, D
    // A = guess it with the minimum value of the Response variable
    // B = slope of the line between first and last point. The slope is given by ¦¤y/¦¤x
    // C = approximately the dose whose response is nearest to the mid response 
    // D = guess it with the maximum value of the Response variable
    // E = Asymmetry factor, guess it with no asymmetry (E=1).

    double par[5];// = { 118, -10, 20, 202, 1 }; /* really bad starting value */
    par[0] = VMIN(y, t_len);
    par[3] = VMAX(y, t_len);
    par[1] = VSLOPE(t, y, t_len);
    par[2] = VAVERAGE(t, y, t_len, (par[3] - par[0]) / 2);
    par[4] = 1;

    lm_control_struct control = lm_control_double;
    lm_status_struct status;
    control.verbosity = 9;

    //printf( "Fitting ...\n" );
    // now the call to lmfit
	lmcurve(5, par, m, t, y, f5, &control, &status);
        
	// WD, 9 Sep 2014: to show the result for curvefitting5
	COUT << endl << "a: " << par[0] << " b: " << par[1] << " c: " << par[2] << " d: " << par[3] << " e: " << par[4] << endl;
    *a_out = par[0];
    *b_out = par[1];
    *c_out = par[2];
    *d_out = par[3];
    *e_out = par[4];

    return 0;
}
