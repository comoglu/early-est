/***************************************************************************
 * statistics.c:
 *
 * TODO: add doc
 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 * modified: 2009.02.13
 ***************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "statistics.h"

#define INLINE inline

/** double comparison for qsort */

int compare_doubles(const void *a, const void *b) {
    const double *da = (const double *) a;
    const double *db = (const double *) b;

    return (*da > *db) - (*da < *db);
}

/** Returns a vector of size 101, where returned_vector[95] gives the
value of the 95th percentile, for example. Returned_vector[100] is always
the maximum value, and returned_vector[0] is always the min (regardless
of rounding rule).

\param data	a  double arrayr of data.
\param rounding This will either be 'u', 'd', or 'a'. Unless your data is
exactly a multiple of 101, some percentiles will be ambiguous. If 'u',
then round up (use the next highest value); if 'd' (or anything else),
round down to the next lowest value; if 'a', take the mean of the two nearest points. If 'u' or 'a', then you can say "5% or
more  of the sample is below returned_vector[5]"; if 'd' or 'a', then you can
say "5% or more of the sample is above returned_vector[5]".

Adapted from: Apophenia: http://apophenia.sourceforge.net/
Copyright (c) 2006--2007 by Ben Klemens.  Licensed under the modified GNU GPL v2; see COPYING and COPYING2.

 */

double *vector_percentiles(double *orig_data, int size, char rounding) {

    double *data = calloc(size, sizeof (double));
    memcpy(data, orig_data, size * sizeof (double));
    //qsort(data, size, sizeof (double), compare_doubles);  // 20110203 AJL - shellSort is faster
    shellSort(data, size, 1);

    double *pctiles = calloc(101, sizeof (double));
    int i, index;

    for (i = 0; i < 101; i++) {
        index = i * (size - 1) / 100.0;
        if (rounding == 'u' && index != i * (size - 1) / 100.0)
            index++; //index was rounded down, but should be rounded up.
        if (rounding == 'a' && index != i * (size - 1) / 100.0)
            pctiles[i] = (data[index] + data[index + 1]) / 2.0;
        else
            pctiles[i] = data[index];
    }

    free(data);

    return (pctiles);
}

double *vector_percentiles_weighted(double *data, double *weights, int length) {

    int n;

    double *pctiles = calloc(101, sizeof (double));

    if (length == 0) {
        return (NULL);
    }

    if (length == 1) {
        for (n = 0; n < 101; n++) {
            pctiles[n] = data[0];
        }
        return (pctiles);
    }

    double weightSum = 0.0;

    double** sorted = calloc(length, sizeof (double*));
    for (n = 0; n < length; n++) {
        sorted[n] = calloc(2, sizeof (double));
        sorted[n][0] = data[n];
        sorted[n][1] = weights[n];
        weightSum += weights[n];
    }


    shellSort2D(sorted, length, 1);

    // following from http://en.wikipedia.org/wiki/Percentile

    double last_percentile = 0.0;
    double current_wt_sum = sorted[0][1];
    double current_percentile = (100.0 / weightSum) * (current_wt_sum - sorted[0][1] / 2.0);
    n = 0;
    int ipercent;
    for (ipercent = 0; ipercent < 101; ipercent++) {

        double percentile = ipercent;

        while (n < length - 1 && current_percentile < percentile) {
            n++;
            last_percentile = current_percentile;
            current_wt_sum += sorted[n][1];
            current_percentile = (100.0 / weightSum) * (current_wt_sum - sorted[n][1] / 2.0);
        }
        if (n == 0) {
            pctiles[ipercent] = sorted[0][0];
            continue;
        } else if (n == length - 1) {
            if (percentile > current_percentile) {
                pctiles[ipercent] = sorted[n][0];
                continue;
            }
        }
        //System.out.println("weightSum, percentile, last_percentile, current_wt_sum  "
        //        + weightSum + " " + percentile + " " + last_percentile + " " + current_wt_sum);
        double lower = sorted[n - 1][0];
        double upper = sorted[n][0];
        double dif = (percentile - last_percentile) / (current_percentile - last_percentile);
        //System.out.println("upper, lower. dif  " + upper + " " + lower + " " + dif);
        pctiles[ipercent] = lower + dif * (upper - lower);

    }

    for (n = 0; n < length; n++) {
        free(sorted[n]);
    }
    free(sorted);

    return (pctiles);

}

/** Shellsort using Hibbard increments, sorts on a[n][0] values
 *
 *  Hibbard increments: 1, 3, 7, ... , (2**k - 1)
 *
 *  (from Weiss, Data structures and algorithms in Java, Addison-Wesley, 1999,  sec 7.4
 *
 */
//

/** integer power function */
int intPow(int a, int b) {

    int ipow = 1;
    int i;
    for (i = 0; i < b; i++) {
        ipow *= a;
    }

    return (ipow);
}
//

void shellSort(double* a, int length, int increasingSort) {

    int i;

    double tmp;

    // find largest Hibbard increment
    int index = 0;
    while (intPow(2, index + 1) - 1 < length) {
        index++;
    }
    // do shell sort
    for (; index >= 0; index--) {
        int gap = intPow(2, index) - 1;
        for (i = gap; i < length; i++) {
            tmp = a[i];
            int j = i;
            for (; j >= gap && tmp < a[j - gap]; j -= gap) {
                a[j] = a[j - gap];
            }
            a[j] = tmp;
        }
    }

    // reverse order for decreasing sort
    if (!increasingSort) {
        for (i = 0; i < length / 2; i++) {
            int iswap = length - i - 1;
            tmp = a[i];
            a[i] = a[iswap];
            a[iswap] = tmp;
        }
    }

}

void shellSort2D(double** a, int length, int increasingSort) {

    int i;

    double tmp[2];

    // find largest Hibbard increment
    int index = 0;
    while (intPow(2, index + 1) - 1 < length) {
        index++;
    }
    // do shell sort
    for (; index >= 0; index--) {
        int gap = intPow(2, index) - 1;
        for (i = gap; i < length; i++) {
            tmp[0] = a[i][0];
            tmp[1] = a[i][1];
            int j = i;
            for (; j >= gap && tmp[0] < a[j - gap][0]; j -= gap) {
                a[j][0] = a[j - gap][0];
                a[j][1] = a[j - gap][1];
            }
            a[j][0] = tmp[0];
            a[j][1] = tmp[1];
        }
    }

    // reverse order for decreasing sort
    if (!increasingSort) {
        for (i = 0; i < length / 2; i++) {
            int iswap = length - i - 1;
            tmp[0] = a[i][0];
            tmp[1] = a[i][1];
            a[i][0] = a[iswap][0];
            a[i][1] = a[iswap][1];
            a[iswap][0] = tmp[0];
            a[iswap][1] = tmp[1];
        }
    }

}


/** simple great-circle distance calculation on a sphere
 * returns distance in degrees
 *
 * Adapted from:
 *    Geographic Distance and Azimuth Calculations
 *    by Andy McGovern
 *    http://www.codeguru.com/algorithms/GeoCalc.html
 */

#define DE2RA (M_PI/180.0)
#define AVG_ERAD 6371.0
#define KM2DEG 90.0/10000.0

INLINE double GCDistance_stats(double lat1, double lon1, double lat2, double lon2) {
    double d;

    if (lat1 == lat2 && lon1 == lon2)
        return (0.0);

    lat1 *= DE2RA;
    lon1 *= DE2RA;
    lat2 *= DE2RA;
    lon2 *= DE2RA;

    d = sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(lon1 - lon2);
    return (AVG_ERAD * acos(d) * KM2DEG);

}

/**
 * Set weights based on clustering in a set of points on the sphere
 *
 * First calculates the mean Dm of the minimum distance between each station and all other stations,
 *    and an area Am = Dm^2.
 * Then accumulate weight for each station based on area (As = distance^2) "between" two stations,
 *    if As < Am, sum (1-As/Am), station weight then = 1/sum.
 *
 * The idea is that the weight of a station is a function of the number and distance^2 to all other
 *    stations that are closer than the cutoff distance Dm.  So clusters of stations are down-weighted,
 *    but distant stations do not affect each others weights.
 *
 */
double* setDistributionWeights(double* lat, double* lon, int size) {

    int n, m;

    double areaMinMean = 0.0;

    // find mean of minimum distances
    double distanceMinMean = getDistanceMinMean(lat, lon, size);
    areaMinMean = distanceMinMean * distanceMinMean;


    double* weights = calloc(size, sizeof (double));

    // set weights
    double weightSum = 0.0;
    double weight;
    double distance;
    double area;
    for (n = 0; n < size; n++) {
        double sum = 1.0; // 1.0 is weight of station n
        for (m = 0; m < size; m++) {
            if (m != n) {
                distance = GCDistance_stats(lat[n], lon[n], lat[m], lon[m]);
                area = distance * distance;
                if (area < areaMinMean) {
                    sum += 1.0 - (area / areaMinMean);
                }
            }
        }
        weight = 1.0 / sum;
        weights[n] = weight;
        weightSum += weight;
    }

    // normalize so sum = 1 (required for weighted variance formula)
    double norm = weightSum;
    //fprintf(stdout, "DEBUG: setDistributionWeights()->norm: %.2f\n", norm);
    for (n = 0; n < size; n++) {
        weights[n] = weights[n] / norm;
        //fprintf(stdout, "Sta: %.2f %.2f  wt: %.2f\n", lat[n], lon[n], weights[n]);
    }

    return (weights);

}

/**
 * Returns the mean of the minimum distance between each point on the sphere and all other points
 */
double getDistanceMinMean(double* lat, double* lon, int size) {

    int n, m;
    double distance;

    // find mean of minimum distances
    double distanceMinMean = 0.0;
    int icount = 0;
    for (n = 0; n < size; n++) {
        double distanceMin = FLT_MAX;
        for (m = 0; m < size; m++) {
            if (m != n) {
                distance = GCDistance_stats(lat[n], lon[n], lat[m], lon[m]);
                if (distance < distanceMin) {
                    distanceMin = distance;
                }
            }
        }
        distanceMinMean += distanceMin;
        icount++;
    }

    if (icount > 0)
        distanceMinMean /= (double) icount;

    return (distanceMinMean);

}

/**
 * Returns the mean of the maximum distance between each point on the sphere and all other points
 */
double getDistanceMaxMean(double* lat, double* lon, int size) {

    int n, m;
    double distance;

    // find mean of maximum distances
    double distanceMaxMean = 0.0;
    int icount = 0;
    for (n = 0; n < size; n++) {
        double distanceMax = -FLT_MAX;
        for (m = 0; m < size; m++) {
            if (m != n) {
                distance = GCDistance_stats(lat[n], lon[n], lat[m], lon[m]);
                if (distance > distanceMax) {
                    distanceMax = distance;
                }
            }
        }
        distanceMaxMean += distanceMax;
        icount++;
    }

    if (icount > 0)
        distanceMaxMean /= (double) icount;

    return (distanceMaxMean);

}

/** standard deviation of product of two independent normal distributions
 *
 * from: http://projecteuclid.org/DPubS?service=UI&version=1.0&verb=Display&handle=euclid.aoms/1177730442
 * The Probability Function of the Product of Two Normally Distributed Variables
 * Leo A. Aroian
 * Source: Ann. Math. Statist. Volume 18, Number 2 (1947), 265-271.
 *
 * TODO: is this at all correct?
 *
 */

double stdDevProductNormalDist(double exp1, double std1, double exp2, double std2) {

    double var1 = std1 * std1;
    double var2 = std2 * std2;
    return (sqrt(var1 * exp2 * exp2 + var2 * exp1 * exp1 + var1 * var2));

}

/** returns variance and mean of an array of double values, checking for invalid values
 *
 * // 20160809 AJL - modified to calculate mean first then var, to avoid numerical precision problems
 *
 */

double calcVarianceMean(double* darray, int nvalues, double *pmean) {

    double sum = 0.0;
    double sum_sqr = 0.0;

    int nused = 0;

    int i;
    for (i = 0; i < nvalues; i++) {
        nused++;
        sum += darray[i];
    }

    double variance = 0.0;

    if (nused < 1) {
        *pmean = 0.0;
        return (variance);
    } else if (nused == 1) {
        *pmean = sum;
        return (variance);
    }

    double mean = sum / (double) nused;

    for (i = 0; i < nvalues; i++) {
        sum_sqr += (darray[i] - mean) * (darray[i] - mean);
    }

    variance = (sum_sqr / (double) (nused - 1));

    *pmean = mean;
    return (variance);

}

/** returns variance and mean of an array of double values, checking for invalid values
 *
 * // 20160809 AJL - modified to calculate mean first then var, to avoid numerical precision problems
 *
 */

double calcVarianceMeanWeighted(double* darray, double* wtarray, int nvalues, double *pmean) {

    double sum = 0.0;
    double sum_sqr = 0.0;

    double wt_sum = 0.0;

    int i;
    for (i = 0; i < nvalues; i++) {
        wt_sum += wtarray[i];
        sum += darray[i] * wtarray[i];
    }

    double variance = 0.0;

    if (wt_sum < FLT_MIN) {
        *pmean = 0.0;
        return (variance);
    } else if (nvalues <= 1) {
        *pmean = sum;
        return (variance);
    }

    double mean = sum / wt_sum;

    for (i = 0; i < nvalues; i++) {
        sum_sqr += (darray[i] - mean) * (darray[i] - mean)  * wtarray[i];
    }

    variance = (sum_sqr / (wt_sum * ((double) (nvalues - 1) / (double) nvalues)));   // approximate (nused - 1) in denominator

    *pmean = mean;
    return (variance);

}

/** returns variance and mean of an array of double values, checking for invalid values
 *
 * // 20160809 AJL - modified to calculate mean first then var, to avoid numerical precision problems
 *
 */

double calcVarianceInvalid(double* darray, int nvalues, double value_invalid, double *pmean) {

    double sum = 0.0;
    double sum_sqr = 0.0;

    int nused = 0;

    int i;
    for (i = 0; i < nvalues; i++) {
        if (darray[i] == value_invalid)
            continue;
        nused++;
        sum += darray[i];
    }

    double variance = 0.0;

    if (nused < 1) {
        *pmean = 0.0;
        return (variance);
    } else if (nused == 1) {
        *pmean = sum;
        return (variance);
    }

    double mean = sum / (double) nused;

    for (i = 0; i < nvalues; i++) {
        if (darray[i] == value_invalid)
            continue;
        nused++;
        sum_sqr += (darray[i] - mean) * (darray[i] - mean);
    }

    variance = (sum_sqr / (double) (nused - 1));

    *pmean = mean;
    return (variance);

}

/** returns the cumulative distribution from -infinity to x for a normal distribution with specified mean and standard deviation
 *  http://en.wikipedia.org/wiki/Normal_distribution
 */

double cumulDistNormal(double x, double mean, double deviation) {

    if (deviation < DBL_MIN) {
        if (x < mean)
            return (0.0);
        else if (x > mean)
            return (1.0);
        else
            return (0.5);
    }

    return (0.5 * (1.0 + erf((x - mean) / (sqrt(2.0) * deviation))));

}

/** returns the normal distribution density for x with specified mean and standard deviation
 */

double densityNormal(double x, double mean, double deviation) {

    if (deviation < DBL_MIN) {
        return (0.0);
    }

    double arg = (x - mean) / deviation;
    return (exp(-arg * arg / 2.0) / (deviation * sqrt(2.0 * M_PI)));

}

/** returns the relative normal distribution density for x with specified mean and standard deviation
 *
 * value is relative to a value of 1.0 for x == mean
 */

double realtiveDensityNormal(double x, double mean, double deviation) {

    if (deviation < DBL_MIN) {
        return (0.0);
    }

    double arg = (x - mean) / deviation;
    return (exp(-arg * arg / 2.0));

}


#include "../statistics/student_t_table_1000_68.h"

/** Performs a simple linear regression of y_value vs. x_value for specified values
 *
 * returns number of valid values used in the regression
 *
 *      x_value, y_value : double arrays of N=nvalues of x,y data for regression, x_value[n] == DBL_MAX flags invalid data
 *      plinReg : LinRegression structure containing regression results
 */
int calcLinearRegression(double *x_value, double *y_value, int nvalues, LinRegression *plinReg) {

    int nvalid = 0;
    plinReg->nvalues = nvalid;
    plinReg->slope = -9.9;
    plinReg->intercept = 0.0;
    plinReg->correl_coeff = -1.0;
    plinReg->slope_dev = 0.0;
    plinReg->intercept_dev = 0.0;

    if (nvalues < 3) {
        return (-1);
    }

    double x_sum = 0.0;
    double y_sum = 0.0;
    double xy_sum = 0.0;
    double xx_sum = 0.0;
    double yy_sum = 0.0;

    double xval, yval;

    // loop over distance values and accumulate regression sums
    for (int narr = 0; narr < nvalues; narr++) {
        // check for valid a_ref value
        if (x_value[narr] > -DBL_MAX) {
            xval = x_value[narr];
            yval = y_value[narr];
            x_sum += xval;
            y_sum += yval;
            xy_sum += xval * yval;
            xx_sum += xval * xval;
            yy_sum += yval * yval;
            nvalid++;
        }
    }

    // calculate regression values (http://en.wikipedia.org/wiki/Simple_linear_regression)
    double slope, intercept;
    if (nvalid > 3) {
        double nv = (double) nvalid;
        // estimate of slope and intercept
        slope = (xy_sum - x_sum * y_sum / nv) / (xx_sum - x_sum * x_sum / nv);
        plinReg->slope = slope;
        intercept = (y_sum - slope * x_sum) / nv;
        //double intercept = (y_sum * xx_sum - x_sum * xy_sum) / (nv * xx_sum - x_sum * x_sum);
        plinReg->intercept = intercept;
        double correl_coeff = (xy_sum - x_sum * y_sum / nv) / sqrt((xx_sum - x_sum * x_sum / nv) * (yy_sum - y_sum * y_sum / nv));
        plinReg->correl_coeff = correl_coeff;
        // confidence interval
        double s_e_2 = (1.0 / (nv * (nv - 2))) * (nv * yy_sum - y_sum * y_sum - slope * slope * (nv * xx_sum - x_sum * x_sum));
        double s_slope = sqrt(nv * s_e_2 / (nv * xx_sum - x_sum * x_sum));
        int ndeg_freedom = nvalid - 2;
        if (ndeg_freedom > STUDENT_T_NUM_DEG_FREEDOM_MAX)
            ndeg_freedom = STUDENT_T_NUM_DEG_FREEDOM_MAX;
        double student_t_quantile = student_t_value[ndeg_freedom];
        //double student_t_quantile = 0.0;
        plinReg->slope_dev = student_t_quantile * s_slope;
    } else {
        return (-1);
    }

    plinReg->nvalues = nvalid;

    return (nvalid);

}

/** Performs a simple linear regression of y_value vs. x_value without intercept (regression through the origin) for specified values
 *
 * returns number of valid values used in the regression
 *
 *      x_value, y_value : double arrays of N=nvalues of x,y data for regression, x_value[n] == DBL_MAX flags invalid data
 *      plinReg : LinRegression structure containing regression results
 */
int calcLinearRegressionInterceptZero(double *x_value, double *y_value, int nvalues, LinRegression *plinReg) {

    int nvalid = 0;
    plinReg->nvalues = nvalid;
    plinReg->slope = -9.9;
    plinReg->intercept = 0.0;
    plinReg->correl_coeff = -1.0;
    plinReg->slope_dev = 0.0;
    plinReg->intercept_dev = 0.0;

    if (nvalues < 3) {
        return (-1);
    }

    double xy_sum = 0.0;
    double xx_sum = 0.0;
    double yy_sum = 0.0;

    double xval, yval;

    // loop over distance values and accumulate regression sums
    for (int narr = 0; narr < nvalues; narr++) {
        // check for valid a_ref value
        if (x_value[narr] > -DBL_MAX) {
            xval = x_value[narr];
            yval = y_value[narr];
            xy_sum += xval * yval;
            xx_sum += xval * xval;
            yy_sum += yval * yval;
            nvalid++;
        }
    }

    // calculate regression values
    //   http://en.wikipedia.org/wiki/Simple_linear_regression
    //   http://courses.washington.edu/qsci483/Lectures/20.pdf
    double slope, intercept = 0.0;
    if (nvalid > 3) {
        double nv = (double) nvalid;
        // estimate of slope and intercept
        slope = xy_sum / xx_sum;
        plinReg->slope = slope;
        plinReg->intercept = intercept; // = 0.0
        double correl_coeff = xy_sum / sqrt(xx_sum * yy_sum);
        plinReg->correl_coeff = correl_coeff;
        // confidence interval
        double s_e_2 = (1.0 / (nv - 1)) * (yy_sum - slope * slope * xx_sum);
        double s_slope = sqrt(s_e_2 / xx_sum);
        int ndeg_freedom = nvalid - 1;
        if (ndeg_freedom > STUDENT_T_NUM_DEG_FREEDOM_MAX)
            ndeg_freedom = STUDENT_T_NUM_DEG_FREEDOM_MAX;
        double student_t_quantile = student_t_value[ndeg_freedom];
        //double student_t_quantile = 0.0;
        plinReg->slope_dev = student_t_quantile * s_slope;
    } else {
        return (-1);
    }

    plinReg->nvalues = nvalid;

    return (nvalid);

}

/** Performs a simple, weighted linear regression of y_value vs. x_value without intercept (regression through the origin) for specified values
 *
 * returns number of valid values used in the regression
 *
 *      x_value, y_value : double arrays of N=nvalues of x,y data for regression, x_value[n] == DBL_MAX flags invalid data
 *      plinReg : LinRegression structure containing regression results
 */
double calcWeightedLinearRegressionInterceptZero(double *x_value, double *y_value, double *weight, int nvalues, LinRegression *plinReg) {

    int nvalid = 0;
    plinReg->nvalues = nvalid;
    plinReg->slope = -9.9;
    plinReg->intercept = 0.0;
    plinReg->correl_coeff = -1.0;
    plinReg->slope_dev = 0.0;
    plinReg->intercept_dev = 0.0;

    if (nvalues < 3) {
        return (-1);
    }

    double xy_sum = 0.0;
    double xx_sum = 0.0;
    double yy_sum = 0.0;
    double weight_sum = 0.0;

    double xval, yval;

    // loop over distance values and accumulate regression sums
    for (int narr = 0; narr < nvalues; narr++) {
        // check for valid a_ref value
        if (x_value[narr] > -DBL_MAX) {
            xval = x_value[narr] * weight[narr];
            yval = y_value[narr] * weight[narr];
            xy_sum += xval * yval;
            xx_sum += xval * xval;
            yy_sum += yval * yval;
            nvalid++;
            weight_sum += weight[narr];
        }
    }

    // calculate regression values
    //   http://en.wikipedia.org/wiki/Simple_linear_regression
    //   http://courses.washington.edu/qsci483/Lectures/20.pdf
    double slope, intercept = 0.0;
    if (weight_sum > 3.0) {
        //double nv = (double) nvalid;
        // estimate of slope and intercept
        slope = xy_sum / xx_sum;
        plinReg->slope = slope;
        plinReg->intercept = intercept; // = 0.0
        double correl_coeff = xy_sum / sqrt(xx_sum * yy_sum);
        plinReg->correl_coeff = correl_coeff;
        // confidence interval
        double s_e_2 = (1.0 / (weight_sum - 1.0)) * (yy_sum - slope * slope * xx_sum);
        double s_slope = sqrt(s_e_2 / xx_sum);
        int ndeg_freedom = (int) weight_sum - 1;
        if (ndeg_freedom > STUDENT_T_NUM_DEG_FREEDOM_MAX)
            ndeg_freedom = STUDENT_T_NUM_DEG_FREEDOM_MAX;
        double student_t_quantile = student_t_value[ndeg_freedom];
        //double student_t_quantile = 0.0;
        plinReg->slope_dev = student_t_quantile * s_slope;
    } else {
        return (-1);
    }

    plinReg->nvalues = nvalid;
    plinReg->weight_sum = weight_sum;

    return (weight_sum);

}
