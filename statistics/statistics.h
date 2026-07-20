/***************************************************************************
 * statistics.h:
 *
 * TODO: add doc
 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 * modified: 2009.02.13
 ***************************************************************************/


// linear regression relation =====================================================================================
// regression values (http://en.wikipedia.org/wiki/Simple_linear_regression))
typedef struct {
    int nvalues; // number of data points used for regression
    double weight_sum; // sum of weights of points used for regression
    double slope; // slope of regression
    double intercept; // y-intercept of regression
    double correl_coeff; // square of sample correlation coefficient
    double slope_dev; // 1 std deviation of slope estimate
    double intercept_dev; // 1 std deviation of constant estimate
}
LinRegression;



int compare_doubles(const void *a, const void *b);
double* vector_percentiles(double *data, int size, char rounding);
double *vector_percentiles_weighted(double *data, double *weights, int length);
void shellSort(double* a, int length, int increasingSort);
void shellSort2D(double** a, int length, int increasingSort);
double GCDistance_stats(double lat1, double lon1, double lat2, double lon2);
double getDistanceMinMean(double* lat, double* lon, int size);
double getDistanceMaxMean(double* lat, double* lon, int size);
double* setDistributionWeights(double* lat, double* lon, int size);
double stdDevProductNormalDist(double exp1, double std1, double exp2, double std2);
double calcVarianceMean(double* darray, int nvalues, double *pmean);
double calcVarianceMeanWeighted(double* darray, double* wtarray, int nvalues, double *pmean);
double calcVarianceInvalid(double* darray, int nvalues, double value_invalid, double *pmean);
double cumulDistNormal(double x, double mean, double deviation);
double densityNormal(double x, double mean, double deviation);
double realtiveDensityNormal(double x, double mean, double deviation);
int calcLinearRegression(double *x_value, double *y_value, int nvalues, LinRegression *plinReg);
int calcLinearRegressionInterceptZero(double *x_value, double *y_value, int nvalues, LinRegression *plinReg);
double calcWeightedLinearRegressionInterceptZero(double *x_value, double *y_value, double *weight, int nvalues, LinRegression *plinReg);
