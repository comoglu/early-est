/***************************************************************************
 * fmamp_utils.c
 *
 * Utility functions for fault plane analysis.
 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 * created: 2014.02.19
 ***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "../vector/vector.h"
#include "../alomax_matrix/alomax_matrix.h"
#include "fmamp.h"
#include "fmamp_utils.h"
#include "../statistics/statistics.h"



#ifndef D2R
#define D2R (M_PI / 180.0)
#endif
#ifndef R2D
#define R2D (180.0 / M_PI)
#endif



// 20160513 AJL - following from "gmt_macros.h"
// make a dummy sincos with sin and cos

void sincos_DUMMY(double a, double *s, double *c) {
    *s = sin(a);
    *c = cos(a);
}
// Macros for degree-based trig
#define sind(x) sin((x) * D2R)
//#define sindf(x) sinf((x) * D2R)
#define cosd(x) cos((x) * D2R)
//#define cosdf(x) cosf((x) * D2R)
//#define tand(x) tan((x) * D2R)
//#define tandf(x) tanf((x) * D2R)
#define sincosd(x,s,c) sincos_DUMMY((x) * D2R,s,c)
#define asind(x) (asin(x) * R2D)
//#define acosd(x) (acos(x) * R2D)
//#define atand(x) (atan(x) * R2D)
#define atan2d(y,x) (atan2(y,x) * R2D)
//#define atan2df(y,x) (atan2f(y,x) * R2D)
// Safe versions of the degree-based trig
//#define d_acosd(x) (fabs(x) >= 1.0 ? ((x) < 0.0 ? 180.0 : 0.0) : acosd(x))
//#define d_asind(x) (fabs(x) >= 1.0 ? copysign (90.0, (x)) : asind(x))
#define d_atan2d(y,x) ((x) == 0.0 && (y) == 0.0 ? 0.0 : atan2d(y,x))


#define cPI (4. * atan(1.))     // PI
#define DEG2RAD (cPI / 180.0)

/** gets fault normal vector,fnorm, and slip
 * adapted from: HASH1.2 util_subs.f:subroutine FPCOOR(strike,dip,rake,fnorm,slip,idir)
 * vector, slip, from (strike,dip,rake) or vice versa.
 *   idir = 1 compute fnorm,slip
 *   idir = c compute strike,dip,rake
 * Reference:  Aki and Richards, p. 115
 *   uses (x,y,z) coordinate system with x=north, y=east, z=down
 */
void strikeRakeDip2faultNormSlip(double *pstrike, double *pdip, double *prake, double *fnorm, double *slip, int idir) {


    double strike = *pstrike;
    double dip = *pdip;
    double rake = *prake;

    double degrad = 1.0 / DEG2RAD;
    double pi = cPI;

    double phi, del, lam, a, clam, slam;
    phi = strike / degrad;
    del = dip / degrad;
    lam = rake / degrad;

    if (idir == 1) {
        fnorm[0] = -sin(del) * sin(phi);
        fnorm[1] = sin(del) * cos(phi);
        fnorm[2] = -cos(del);
        slip[0] = cos(lam) * cos(phi) + cos(del) * sin(lam) * sin(phi);
        slip[1] = cos(lam) * sin(phi) - cos(del) * sin(lam) * cos(phi);
        slip[2] = -sin(lam) * sin(del);
    } else {
        if ((1.0 - fabs(fnorm[2])) <= 1.0e-7) {
            fprintf(stderr, "WARNING: in strikeRakeDip2faultNormSlip(): horizontal fault strike undefined.\n");
            del = 0.0;
            phi = atan2(-slip[0], slip[1]);
            clam = cos(phi) * slip[0] + sin(phi) * slip[1];
            slam = sin(phi) * slip[0] - cos(phi) * slip[1];
            lam = atan2(slam, clam);
        } else {
            phi = atan2(-fnorm[0], fnorm[1]);
            a = sqrt(fnorm[0] * fnorm[0] + fnorm[1] * fnorm[1]);
            del = atan2(a, -fnorm[2]);
            clam = cos(phi) * slip[0] + sin(phi) * slip[1];
            slam = -slip[2] / sin(del);
            lam = atan2(slam, clam);
            if (del > (0.5 * pi)) {
                del = pi - del;
                phi = phi + pi;
                lam = -lam;
            }
        }
        strike = phi * degrad;
        if (strike < 0.0) strike = strike + 360.0;
        dip = del * degrad;
        rake = lam * degrad;
        if (rake <= -180.0) rake = rake + 360.0;
        if (rake > 180.0) rake = rake - 360.0;

        *pstrike = strike;
        *pdip = dip;
        *prake = rake;
    }

    return;

}

/** simple great-circle distance calculation on a sphere
 * returns distance in degrees
 *
 * Adapted from:
 *    Geographic Distance and Azimuth Calculations
 *    by Andy McGovern
 *    http://www.codeguru.com/algorithms/GeoCalc.html
 */

double GCDistance_local(double latA, double lonA, double latB, double lonB) {

    double d;

    if (latA == latB && lonA == lonB)
        return (0.0);

    latA *= DE2RA;
    lonA *= DE2RA;
    latB *= DE2RA;
    lonB *= DE2RA;

    d = sin(latA) * sin(latB) + cos(latA) * cos(latB) * cos(lonA - lonB);
    // 20120928 AJL return (AVG_ERAD * acos(d) * KM2DEG);
    return (acos(d) * RA2DE);

}

/**
 * Returns the mean of a set of fm axes on the sphere
 */
struct AXIS getMeanAxis(struct AXIS *axis, int naxes, struct AXIS ref_axis) {


    double ref_str = ref_axis.str;
    double ref_dip = ref_axis.dip;

    //printf("DEBUG: getMeanAxis:naxes: %d\n", naxes);
    double dist1, dist2;

    int n;
    double str1, dip1;
    double str_mean = 0.0;
    double dip_mean = 0.0;

    // find mean of distances
    int icount = 0;
    for (n = 0; n < naxes; n++) {
        str1 = (axis + n)->str;
        dip1 = (axis + n)->dip;
        //if (icount % 100000 == 0)
        //    printf("DEBUG: getMeanAxis:distance_mean: %f/%f %f/%f %f icount %d\n", dip1, str1, dip2, str2, distance_mean, icount);
        // check upper and lower hemisphere positions for axis, use closer of two to ref axis
        dist1 = GCDistance_local(ref_dip, ref_str, dip1, str1);
        dist2 = GCDistance_local(ref_dip, ref_str, -dip1, str1 - 180.0);
        if (dist1 < dist2) {
            str_mean += str1;
            dip_mean += dip1;
        } else {
            str1 = str1 - 180.0;
            if (str1 < 180.0)
                str1 += 360.0;
            str_mean += str1;
            dip_mean += -dip1;
        }
        icount++;
    }

    if (icount > 0) {
        str_mean /= (double) (icount);
        dip_mean /= (double) (icount);
    }
    //printf("DEBUG: getMeanAxis:distance_mean: %f icount %d\n", distance_mean, icount);

    struct AXIS axis_mean;
    axis_mean.str = str_mean;
    axis_mean.dip = dip_mean;

    return (axis_mean);

}

/**
 * Returns the first-motion P and T axis with minimum mean distance on the focal sphere to the other P and T axes in a set of mechanisms
 *
 * For each distance, checks points of intersection of each other axis on opposite sides of focal-sphere
 * and used minium distance of these two points.  Avoids ambiguities and instability due to angular wrap on the focal sphere
 *
 */
void getMinMeanDistanceAxis(
        struct AXIS *axis_P, struct AXIS *axis_T, struct AXIS *axis_N, double *prob, int naxes, // set of mechanism axes to process
        double* pdistance_mean_min_P, double* pdistance_mean_min_T, // minimum mean distance of P and T axes
        struct AXIS *pmin_mean_axis_P, struct AXIS *pmin_mean_axis_T, struct AXIS *pmin_mean_axis_N) // minimum mean distance P and T axes
{

    //printf("DEBUG: getMinMeanDistanceAxis:naxes: %d\n", naxes);

    double distance_mean_min = DBL_MAX;
    int n_distance_mean_min = -1;

    double dist1, dist2;
    double distance_mean_P, distance_mean_T;
    double distance_mean_P_weighted, distance_mean_T_weighted;

    int n, m;
    double strP1, dipP1, strP2, dipP2;
    double strT1, dipT1, strT2, dipT2;
    double weight1, weight2, weight;
    double weight_sum;
    int icount;

    int ndist_mean = 0;
    *pdistance_mean_min_P = 0.0;
    *pdistance_mean_min_T = 0.0;

    // find mean of distances
    for (n = 0; n < naxes; n++) {
        icount = 0;
        weight_sum = 0.0;
        distance_mean_P = 0.0;
        distance_mean_T = 0.0;
        distance_mean_P_weighted = 0.0;
        distance_mean_T_weighted = 0.0;
        strP1 = (axis_P + n)->str;
        dipP1 = (axis_P + n)->dip;
        strT1 = (axis_T + n)->str;
        dipT1 = (axis_T + n)->dip;
        weight1 = prob[n]; // weight each distance by probability of corresponding mechanism solution
        for (m = 0; m < naxes; m++) {
            if (m != n) {
                strP2 = (axis_P + m)->str;
                dipP2 = (axis_P + m)->dip;
                strT2 = (axis_T + m)->str;
                dipT2 = (axis_T + m)->dip;
                weight2 = prob[m]; // weight each distance by probability of corresponding mechanism solution
                weight = weight1 * weight2;
                //weight = 1.0; // TEST!
                //if (icount % 100000 == 0)
                //    printf("DEBUG: getMinMeanDistanceAxis:distance_mean: %f/%f %f/%f %f icount %d\n", dip1, str1, dip2, str2, distance_mean, icount);
                // check upper and lower hemisphere positions for second axis, use closer of two
                dist1 = GCDistance_local(dipP1, strP1, dipP2, strP2);
                if (dist1 <= 90.0) {
                    distance_mean_P += dist1;
                    distance_mean_P_weighted += dist1 * weight;
                } else {
                    dist2 = GCDistance_local(dipP1, strP1, -dipP2, strP2 - 180.0);
                    distance_mean_P += dist2;
                    distance_mean_P_weighted += dist2 * weight;
                }
                dist1 = GCDistance_local(dipT1, strT1, dipT2, strT2);
                if (dist1 <= 90.0) {
                    distance_mean_T += dist1;
                    distance_mean_T_weighted += dist1 * weight;
                } else {
                    dist2 = GCDistance_local(dipT1, strT1, -dipT2, strT2 - 180.0);
                    distance_mean_T += dist2;
                    distance_mean_T_weighted += dist2 * weight;
                }
                weight_sum += weight;
                icount++;
            }
        }
        if (weight_sum > 0.0 && icount > 0) {
            // mean distance min P and T is not weighted - gives better representation of axes (solution pdf) spread since ~importance sampled
            *pdistance_mean_min_P += distance_mean_P / (double) icount;
            *pdistance_mean_min_T += distance_mean_T / (double) icount;
            ndist_mean++;
            // check if weighted distance mean is smallest - weighting keeps mean axis close to maximum likelhood point of solution pdf
            distance_mean_P_weighted /= weight_sum;
            distance_mean_T_weighted /= weight_sum;
            if (distance_mean_P_weighted + distance_mean_T_weighted < distance_mean_min) {
                distance_mean_min = distance_mean_P_weighted + distance_mean_T_weighted;
                /* 20160916 AJL - moved outside smallest distance mean test to avoid instability in mean distances
                // mean distance min P and T is not weighted - gives better representation of axes (solution pdf) spread since ~importance sampled
                 *pdistance_mean_min_P = distance_mean_P / (double) icount;
                 *pdistance_mean_min_T = distance_mean_T / (double) icount;*/
                n_distance_mean_min = n;
            }
        }
    }
    if (ndist_mean > 0) {
        *pdistance_mean_min_P /= (double) ndist_mean;
        *pdistance_mean_min_T /= (double) ndist_mean;
    }
    //printf("DEBUG: getMinMeanDistanceAxis:distance_mean_min: P:%f T:%f weight_sum %lf\n", *pdistance_mean_min_P, *pdistance_mean_min_T, weight_sum);

    if (n_distance_mean_min >= 0) {
        *pmin_mean_axis_P = *(axis_P + n_distance_mean_min);
        *pmin_mean_axis_T = *(axis_T + n_distance_mean_min);
        *pmin_mean_axis_N = *(axis_N + n_distance_mean_min);
    }

}

/**
 * Returns the first-motion P and T axis with minimum mean distance on the focal sphere to the other P and T axes in a set of mechanisms
 *
 * For each distance, checks points of intersection of each other axis on opposite sides of focal-sphere
 * and used minium distance of these two points.  Avoids ambiguities and instability due to angular wrap on the focal sphere
 *
 */
void getMinMeanDistanceAxis_OLD(
        struct AXIS *axis_P, struct AXIS *axis_T, struct AXIS *axis_N, double *prob, int naxes, // set of mechanism axes to process
        double* pdistance_mean_min_P, double* pdistance_mean_min_T, // minimum mean distance of P and T axes
        struct AXIS *pmin_mean_axis_P, struct AXIS *pmin_mean_axis_T, struct AXIS *pmin_mean_axis_N) // minimum mean distance P and T axes
{

    //printf("DEBUG: getMinMeanDistanceAxis:naxes: %d\n", naxes);

    double distance_mean_min = DBL_MAX;
    int n_distance_mean_min = -1;

    double dist1, dist2;
    double distance_mean_P, distance_mean_T;
    double distance_mean_P_weighted, distance_mean_T_weighted;

    int n, m;
    double strP1, dipP1, strP2, dipP2;
    double strT1, dipT1, strT2, dipT2;
    double weight1, weight2, weight;
    double weight_sum;
    int icount;

    // find mean of distances
    for (n = 0; n < naxes; n++) {
        icount = 0;
        weight_sum = 0.0;
        distance_mean_P = 0.0;
        distance_mean_T = 0.0;
        distance_mean_P_weighted = 0.0;
        distance_mean_T_weighted = 0.0;
        strP1 = (axis_P + n)->str;
        dipP1 = (axis_P + n)->dip;
        strT1 = (axis_T + n)->str;
        dipT1 = (axis_T + n)->dip;
        weight1 = prob[n]; // weight each distance by probability of corresponding mechanism solution
        for (m = 0; m < naxes; m++) {
            if (m != n) {
                strP2 = (axis_P + m)->str;
                dipP2 = (axis_P + m)->dip;
                strT2 = (axis_T + m)->str;
                dipT2 = (axis_T + m)->dip;
                weight2 = prob[m]; // weight each distance by probability of corresponding mechanism solution
                weight = weight1 * weight2;
                //weight = 1.0; // TEST!
                //if (icount % 100000 == 0)
                //    printf("DEBUG: getMinMeanDistanceAxis:distance_mean: %f/%f %f/%f %f icount %d\n", dip1, str1, dip2, str2, distance_mean, icount);
                // check upper and lower hemisphere positions for second axis, use closer of two
                dist1 = GCDistance_local(dipP1, strP1, dipP2, strP2);
                if (dist1 <= 90.0) {
                    distance_mean_P += dist1;
                    distance_mean_P_weighted += dist1 * weight;
                } else {
                    dist2 = GCDistance_local(dipP1, strP1, -dipP2, strP2 - 180.0);
                    distance_mean_P += dist2;
                    distance_mean_P_weighted += dist2 * weight;
                }
                dist1 = GCDistance_local(dipT1, strT1, dipT2, strT2);
                if (dist1 <= 90.0) {
                    distance_mean_T += dist1;
                    distance_mean_T_weighted += dist1 * weight;
                } else {
                    dist2 = GCDistance_local(dipT1, strT1, -dipT2, strT2 - 180.0);
                    distance_mean_T += dist2;
                    distance_mean_T_weighted += dist2 * weight;
                }
                weight_sum += weight;
                icount++;
            }
        }
        if (weight_sum > 0.0 && icount > 0) {
            distance_mean_P_weighted /= weight_sum;
            distance_mean_T_weighted /= weight_sum;
            // check if weighted distance mean is smallest - weighting keeps mean axis close to maximum likelhood point of solution pdf
            if (distance_mean_P_weighted + distance_mean_T_weighted < distance_mean_min) {
                distance_mean_min = distance_mean_P_weighted + distance_mean_T_weighted;
                // mean distance min P and T is not weighted - gives better representation of axes (solution pdf) spread since ~importance sampled
                *pdistance_mean_min_P = distance_mean_P / (double) icount;
                *pdistance_mean_min_T = distance_mean_T / (double) icount;
                n_distance_mean_min = n;
            }
        }
    }
    //printf("DEBUG: getMinMeanDistanceAxis:distance_mean_min: P:%f T:%f weight_sum %lf\n", *pdistance_mean_min_P, *pdistance_mean_min_T, weight_sum);

    if (n_distance_mean_min >= 0) {
        *pmin_mean_axis_P = *(axis_P + n_distance_mean_min);
        *pmin_mean_axis_T = *(axis_T + n_distance_mean_min);
        *pmin_mean_axis_N = *(axis_N + n_distance_mean_min);
    }

}

/**
 * Returns the mean of the distance between axes on the sphere
 */
double getMeanDistance(struct AXIS *axis, int naxes, int check_upper_and_lower) {

    //printf("DEBUG: getMeanDistance:naxes: %d\n", naxes);
    double dist1, dist2, distance_mean = 0.0;

    int n, m;
    double str1, dip1, str2, dip2;

    // find mean of distances
    int icount = 0;
    for (n = 0; n < naxes; n++) {
        str1 = (axis + n)->str;
        dip1 = (axis + n)->dip;
        for (m = 0; m < n; m++) {
            str2 = (axis + m)->str;
            dip2 = (axis + m)->dip;
            //if (icount % 100000 == 0)
            //    printf("DEBUG: getMeanDistance:distance_mean: %f/%f %f/%f %f icount %d\n", dip1, str1, dip2, str2, distance_mean, icount);
            // check upper and lower hemisphere positions for second axis, use closer of two
            dist1 = GCDistance_local(dip1, str1, dip2, str2);
            dist2 = 1.0e9;
            if (check_upper_and_lower) {
                dist2 = GCDistance_local(dip1, str1, -dip2, str2 - 180.0);
            }
            distance_mean += dist1 < dist2 ? dist1 : dist2;
            icount++;
        }
    }
    if (icount > 0)
        distance_mean /= (double) (icount);
    //printf("DEBUG: getMeanDistance:distance_mean: %f icount %d\n", distance_mean, icount);

    return (distance_mean);

}

/**
 * Returns the generalized discrepancy D(N) for a set of points on the sphere
 *
 * from: Examining the Distribution of Sampling Point Sets on Sphere for Monte Carlo Image Rendering
 *       A. A. Penzov, I. T. Dimov, N. M. Mitev, G. I. Sirakova and L. Szirmay-Kalos
 *       AIP Conference Paper
 *
 * The generalized discrepancy D(N) characterizes "how well the point set {p1,...,pN} is equi-distributed on the sphere".
 * The lower the D(N) is, the more uniformly distributed the sampling pattern is, in general lim D(N) = 0 as N->infinity
 *
 * points_on_sphere - array of npoints arrays of size 2: [azimuth on sphere (0-360), inclination on sphere (-90 -> 90)]
 */
double getUniformityOfDistributionOnSphere(double *points_on_sphere[2], int npoints) {

    //printf("DEBUG: getUniformityDistributionOnSphere:npoints: %d\n", npoints);

    if (npoints < 1) {
        return (1.0);
    }

    int n, m;
    double x1, y1, z1;
    double x2, y2, z2;
    double az1, inc1, sin1;
    double az2, inc2, sin2;
    double dot_product;
    double value, temp;
    double sum = 0.0;
    for (n = 0; n < npoints; n++) {
        az1 = DE2RA * points_on_sphere[n][0];
        inc1 = DE2RA * points_on_sphere[n][1];
        sin1 = sin(inc1);
        x1 = cos(az1) * sin1;
        y1 = sin(az1) * sin1;
        z1 = cos(inc1);
        for (m = 0; m < npoints; m++) {
            az2 = DE2RA * points_on_sphere[m][0];
            inc2 = DE2RA * points_on_sphere[m][1];
            sin2 = sin(inc2);
            x2 = cos(az2) * sin2;
            y2 = sin(az2) * sin2;
            z2 = cos(inc2);
            dot_product = x1 * x2 + y1 * y2 + z1 * z2;
            temp = 1.0 - dot_product;
            if (temp < 0.0) {
                temp = DBL_MIN; // if dot_product == 1.0, 1.0 - dot_product may be negative
            }
            value = 1.0 - 2.0 * log(1.0 + sqrt(temp / 2.0));
            sum += value;
            //if (icount % 100000 == 0)
            //printf("DEBUG: (%.1f,%1f) (%.1f,%1f)  dot_product %f  value %f  sum %f\n", az1, inc1, az2, inc2, dot_product, value, sum);
        }
    }

    //double generalized_discrepancy = (1.0 / (2.0 * (double) npoints * sqrt(M_PI))) * sqrt(sum);
    double generalized_discrepancy = (1.0 / (double) npoints) * sqrt(sum);

    //printf("DEBUG: getUniformityDistributionOnSphere:generalized_discrepancy: %f npoints %d\n", generalized_discrepancy, npoints);

    return (generalized_discrepancy);

}
/**
 * Returns the fraction of focal sphere covered for a set of points on the sphere
 *
 * points_on_sphere - array of npoints arrays of size 2: [azimuth on sphere (0-360), inclination on sphere (-90 -> 90)]
 */
#define AZ_STEP_0 30.0
#define INC_STEP 30.0

double fractionFocalSphereCovered(double *points_on_sphere[2], int npoints) {

    //printf("DEBUG: fractionFocalSphereCovered:npoints: %d\n", npoints);

    if (npoints < 1) {
        return (1.0);
    }

    int num_az, naz, ninc, n;
    double inclination_min, inclination_max;
    double azimuth_min, azimuth_max, az_step, area_factor;
    double az, inc, d_area;
    double sum_total = 0.0;
    double sum_covered = 0.0;
    int num_inc = (int) ((180.0 + INC_STEP * 0.001) / INC_STEP);
    inclination_min = -90.0;
    for (ninc = 0; ninc < num_inc; ninc++) {
        inclination_max = inclination_min + INC_STEP;
        az_step = AZ_STEP_0 / cos((inclination_min + INC_STEP / 2.0) * DE2RA);
        num_az = (int) ((360.0 - az_step * 0.001) / az_step);
        if (num_az < 1) {
            num_az = 1;
        }
        az_step = 360.0 / (double) num_az;
        area_factor = az_step * cos((inclination_min + INC_STEP / 2.0) * DE2RA);
        d_area = area_factor * az_step * INC_STEP;
        azimuth_min = 0.0;
        for (naz = 0; naz < num_az; naz++) {
            azimuth_max = azimuth_min + az_step;
            sum_total += d_area;
            for (n = 0; n < npoints; n++) {
                az = points_on_sphere[n][0];
                if (az >= azimuth_min && az < azimuth_max) {
                    inc = points_on_sphere[n][1];
                    if (inc >= inclination_min && inc < inclination_max) {
                        sum_covered += d_area;
                        break; // only count first point found
                    }

                }
            }
            //printf("DEBUG: fractionFocalSphereCovered: (%.1f,%1f) (%.1f,%1f)  sum_covered %.2f  sum_total %.2f\n", inclination_min, inclination_max, azimuth_min, azimuth_max, sum_covered, sum_total);
            azimuth_min += az_step;
        }
        inclination_min += INC_STEP;
    }

    //double generalized_discrepancy = (1.0 / (2.0 * (double) npoints * sqrt(M_PI))) * sqrt(sum);
    double fractionCovered = sum_covered / sum_total;

    //printf("DEBUG: fractionFocalSphereCovered: fractionCovered: %f npoints %d\n", fractionCovered, npoints);

    return (fractionCovered);

}

/** computed_rake2
 *      from GMT -> meca/utilmeca.c
 */

double computed_rake2(double str1, double dip1, double str2, double dip2, double fault) {
    /*
       Compute rake in the second nodal plane when strike and dip
       for first and second nodal plane are given with a double
       characterizing the fault :
                                              +1. inverse fault
                                              -1. normal fault.
       Angles are in degrees.

       Genevieve Patau */

    double rake2, sinrake2, sd, cd, ss, cs;

    sincosd(str1 - str2, &ss, &cs);

    sd = sind(dip1);
    cd = cosd(dip2);
    if (fabs(dip2 - 90.0) < EPSIL)
        sinrake2 = fault * cd;
    else
        sinrake2 = -fault * sd * cs / cd;

    rake2 = d_atan2d(sinrake2, -fault * sd * ss);

    return (rake2);
}

/** null_axis_dip
 *      from GMT -> meca/utilmeca.c
 */

double null_axis_dip(double str1, double dip1, double str2, double dip2) {
    /*
       compute null axis dip when strike and dip are given
       for each nodal plane.  Angles are in degrees.

       Genevieve Patau
     */

    double den;

    den = asind(sind(dip1) * sind(dip2) * sind(str1 - str2));
    if (den < 0.) den = -den;
    return (den);
}

/** null_axis_strike
 *      from GMT -> meca/utilmeca.c
 */

double null_axis_strike(double str1, double dip1, double str2, double dip2) {
    /*
       Compute null axis strike when strike and dip are given
       for each nodal plane.   Angles are in degrees.

       Genevieve Patau
     */

    double phn, cosphn, sinphn, sd1, cd1, sd2, cd2, ss1, cs1, ss2, cs2;

    sincosd(dip1, &sd1, &cd1);
    sincosd(dip2, &sd2, &cd2);
    sincosd(str1, &ss1, &cs1);
    sincosd(str2, &ss2, &cs2);

    cosphn = sd1 * cs1 * cd2 - sd2 * cs2 * cd1;
    sinphn = sd1 * ss1 * cd2 - sd2 * ss2 * cd1;
    if (sind(str1 - str2) < 0.0) {
        cosphn = -cosphn;
        sinphn = -sinphn;
    }
    phn = d_atan2d(sinphn, cosphn);
    if (phn < 0.0) phn += 360.0;
    return (phn);
}

/** axes to double couple
 *      from GMT -> meca/utilmeca.c
 */
void axe2dc(struct AXIS T, struct AXIS P, struct nodal_plane *NP1, struct nodal_plane * NP2) {
    /*
      Calculate double couple from principal axes.
      Angles are in degrees.

      Genevieve Patau, 16 juin 1997
     */

    double p1, d1, p2, d2;
    double cdp, sdp, cdt, sdt, cpt, spt, cpp, spp;
    double amz, amy, amx, im;

    sincosd(P.dip, &sdp, &cdp);
    sincosd(P.str, &spp, &cpp);
    sincosd(T.dip, &sdt, &cdt);
    sincosd(T.str, &spt, &cpt);

    cpt *= cdt;
    spt *= cdt;
    cpp *= cdp;
    spp *= cdp;

    amz = sdt + sdp;
    amx = spt + spp;
    amy = cpt + cpp;
    d1 = atan2d(hypot(amx, amy), amz);
    p1 = atan2d(amy, -amx);
    if (d1 > 90.0) {
        d1 = 180.0 - d1;
        p1 -= 180.0;
    }
    if (p1 < 0.0) p1 += 360.0;

    amz = sdt - sdp;
    amx = spt - spp;
    amy = cpt - cpp;
    d2 = atan2d(hypot(amx, amy), amz);
    p2 = atan2d(amy, -amx);
    if (d2 > 90.0) {
        d2 = 180.0 - d2;
        p2 -= 180.0;
    }
    if (p2 < 0.0) p2 += 360.0;

    NP1->dip = d1;
    NP1->str = p1;
    NP2->dip = d2;
    NP2->str = p2;

    im = 1;
    if (P.dip > T.dip) im = -1;
    NP1->rake = computed_rake2(NP2->str, NP2->dip, NP1->str, NP1->dip, im);
    NP2->rake = computed_rake2(NP1->str, NP1->dip, NP2->str, NP2->dip, im);
}

/** double couple to axes
 *      from GMT -> meca/utilmeca.c
 */

void dc2axe(st_me meca, struct AXIS *T, struct AXIS *N, struct AXIS * P) {
    /*
    From FORTRAN routines of Anne Deschamps :
    compute azimuth and plungement of P-T axis
    from nodal plane strikes, dips and rakes.
     */

    double cd1, sd1, cd2, sd2, cp1, sp1, cp2, sp2;
    double amz, amx, amy, dx, px, dy, py;

    cd1 = cosd(meca.NP1.dip) * M_SQRT2;
    sd1 = sind(meca.NP1.dip) * M_SQRT2;
    cd2 = cosd(meca.NP2.dip) * M_SQRT2;
    sd2 = sind(meca.NP2.dip) * M_SQRT2;
    cp1 = -cosd(meca.NP1.str) * sd1;
    sp1 = sind(meca.NP1.str) * sd1;
    cp2 = -cosd(meca.NP2.str) * sd2;
    sp2 = sind(meca.NP2.str) * sd2;

    amz = -(cd1 + cd2);
    amx = -(sp1 + sp2);
    amy = cp1 + cp2;
    dx = atan2d(hypot(amx, amy), amz) - 90.0;
    px = atan2d(amy, -amx);
    if (px < 0.0) px += 360.0;
    if (dx < EPSIL) {
        if (px > 90.0 && px < 180.0) px += 180.0;
        if (px >= 180.0 && px < 270.0) px -= 180.0;
    }

    amz = cd1 - cd2;
    amx = sp1 - sp2;
    amy = -cp1 + cp2;
    dy = atan2d(hypot(amx, amy), -fabs(amz)) - 90.0;
    py = atan2d(amy, -amx);
    if (amz > 0.0) py -= 180.0;
    if (py < 0.0) py += 360.0;
    if (dy < EPSIL) {
        if (py > 90.0 && py < 180.0) py += 180.0;
        if (py >= 180.0 && py < 270.0) py -= 180.0;
    }

    if (meca.NP1.rake > 0.0) {
        P->dip = dy;
        P->str = py;
        T->dip = dx;
        T->str = px;
    } else {
        P->dip = dx;
        P->str = px;
        T->dip = dy;
        T->str = py;
    }

    N->str = null_axis_strike(T->str, T->dip, P->str, P->dip);
    N->dip = null_axis_dip(T->str, T->dip, P->str, P->dip);
}

void crossProductAxes(struct AXIS *paxis1, struct AXIS *paxis2, struct AXIS *paxis3) {

    double x1 = cos(paxis1->dip * D2R) * cos(paxis1->str * D2R);
    double y1 = cos(paxis1->dip * D2R) * sin(paxis1->str * D2R);
    double z1 = sin(paxis1->dip * D2R);
    double x2 = cos(paxis2->dip * D2R) * cos(paxis2->str * D2R);
    double y2 = cos(paxis2->dip * D2R) * sin(paxis2->str * D2R);
    double z2 = sin(paxis2->dip * D2R);

    double x3, y3, z3;
    cross_product_3d(x1, y1, z1, x2, y2, z2, &x3, &y3, &z3);

    paxis3->dip = asin(z3) * R2D;
    paxis3->str = atan2(y3, x3) * R2D;

    if (paxis3->str < 0.0) {
        paxis3->str += 360.0;
    }
    if (paxis3->dip < 0.0) {
        paxis3->dip *= -1.0;
        paxis3->str += 180.0;
        if (paxis3->str >= 360.0) {
            paxis3->str -= 360.0;
        }
    }


}

/** function to calculate the mean distances between the P and T axes of a set of fault-plane samples
 *      and find the fault-plane with P and T axis pair at minimum distance to all other P or T axes, respectively.
 *
 * x - strike
 * y - dip
 * z - rake
 *
 */
void CalcMeanDistancePTandMinDistMechanism(
        float* fdata, int nSamples,
        double *pdistance_mean_min_P, double *pdistance_mean_min_T,
        double *pmin_dist_strike, double *pmin_dist_dip, double *pmin_dist_rake,
        struct AXIS *pmin_dist_P_axis, struct AXIS *pmin_dist_T_axis, struct AXIS *pmin_dist_N_axis) {


    st_me meca;
    struct AXIS T[nSamples], N[nSamples], P[nSamples];
    double prob[nSamples];
    double strike, dip, rake;
    double fp1_norm[3], fp1_slip[3];
    double strike_np2, dip_np2, rake_np2;
    int nsamp, ipos;
    ipos = 0;
    for (nsamp = 0; nsamp < nSamples; nsamp++) {
        strike = fdata[ipos++];
        dip = fdata[ipos++];
        rake = fdata[ipos++];
        prob[nsamp] = exp(fdata[ipos]);
        ipos++; // fdata value is in 4th position
        strikeRakeDip2faultNormSlip(&strike, &dip, &rake, fp1_norm, fp1_slip, 1);
        strikeRakeDip2faultNormSlip(&strike_np2, &dip_np2, &rake_np2, fp1_slip, fp1_norm, -1);
        meca.NP1.str = strike;
        meca.NP1.dip = dip;
        meca.NP1.rake = rake;
        meca.NP2.str = strike_np2;
        meca.NP2.dip = dip_np2;
        meca.NP2.rake = rake_np2;
        dc2axe(meca, T + nsamp, N + nsamp, P + nsamp);
        //if (nsamp % 100 == 0)
        //    printf("DEBUG: CalcMeanDistancePT:distance_mean: %f/%f %f/%f nsamp %d\n",
        //        (P + nsamp)->str, (P + nsamp)->dip, (T + nsamp)->str, (T + nsamp)->dip, nsamp);
    }

    // get minimum mean distance P, T and N axes
    struct AXIS min_dist_P_axis, min_dist_T_axis, min_dist_N_axis;
    getMinMeanDistanceAxis(P, T, N, prob, nSamples, pdistance_mean_min_P, pdistance_mean_min_T, &min_dist_P_axis, &min_dist_T_axis, &min_dist_N_axis);

    // calculate N axis
    crossProductAxes(&min_dist_P_axis, &min_dist_T_axis, &min_dist_N_axis);

    // set min dist mechanism
    st_me meca_min_dist;
    axe2dc(min_dist_T_axis, min_dist_P_axis, &meca_min_dist.NP1, &meca_min_dist.NP2);
    *pmin_dist_strike = meca_min_dist.NP1.str;
    *pmin_dist_dip = meca_min_dist.NP1.dip;
    *pmin_dist_rake = meca_min_dist.NP1.rake;
    *pmin_dist_P_axis = min_dist_P_axis;
    *pmin_dist_T_axis = min_dist_T_axis;
    *pmin_dist_N_axis = min_dist_N_axis;

}

/** function to calculate the mean distances  between the P and T axes of a set of fault-plane samples
 *
 * x - strike
 * y - dip
 * z - rake
 *
 */
void CalcMeanDistancePTandMeanMechanism(float* fdata, int nSamples, double *pmean_dist_P, double *pmean_dist_T,
        double ref_strike, double ref_dip, double ref_rake, double *pmean_strike, double *pmean_dip, double *pmean_rake) {


    // get mean distances for P and T axes
    st_me meca;
    struct AXIS T[nSamples], N[nSamples], P[nSamples];
    double strike, dip, rake;
    double fp1_norm[3], fp1_slip[3];
    double strike_np2, dip_np2, rake_np2;
    int nsamp, ipos;
    ipos = 0;
    for (nsamp = 0; nsamp < nSamples; nsamp++) {
        strike = fdata[ipos++];
        dip = fdata[ipos++];
        rake = fdata[ipos++];
        ipos++; // fdata value is in 4th position
        strikeRakeDip2faultNormSlip(&strike, &dip, &rake, fp1_norm, fp1_slip, 1);
        strikeRakeDip2faultNormSlip(&strike_np2, &dip_np2, &rake_np2, fp1_slip, fp1_norm, -1);
        meca.NP1.str = strike;
        meca.NP1.dip = dip;
        meca.NP1.rake = rake;
        meca.NP2.str = strike_np2;
        meca.NP2.dip = dip_np2;
        meca.NP2.rake = rake_np2;
        dc2axe(meca, T + nsamp, N + nsamp, P + nsamp);
        //if (nsamp % 100 == 0)
        //    printf("DEBUG: CalcMeanDistancePT:distance_mean: %f/%f %f/%f nsamp %d\n",
        //        (P + nsamp)->str, (P + nsamp)->dip, (T + nsamp)->str, (T + nsamp)->dip, nsamp);
    }
    int check_upper_and_lower = 1;
    *pmean_dist_P = getMeanDistance(P, nSamples, check_upper_and_lower);
    *pmean_dist_T = getMeanDistance(T, nSamples, check_upper_and_lower);


    // get mean mechanism
    strikeRakeDip2faultNormSlip(&ref_strike, &ref_dip, &ref_rake, fp1_norm, fp1_slip, 1);
    strikeRakeDip2faultNormSlip(&strike_np2, &dip_np2, &rake_np2, fp1_slip, fp1_norm, -1);
    st_me meca_ref;
    meca_ref.NP1.str = ref_strike;
    meca_ref.NP1.dip = ref_dip;
    meca_ref.NP1.rake = ref_rake;
    meca_ref.NP2.str = strike_np2;
    meca_ref.NP2.dip = dip_np2;
    meca_ref.NP2.rake = rake_np2;
    struct AXIS T_ref, N_ref, P_ref;
    dc2axe(meca_ref, &T_ref, &N_ref, &P_ref);
    struct AXIS mean_P = getMeanAxis(P, nSamples, P_ref);
    struct AXIS mean_T = getMeanAxis(T, nSamples, T_ref);
    st_me meca_mean;
    axe2dc(mean_T, mean_P, &meca_mean.NP1, &meca_mean.NP2);
    *pmean_strike = meca_mean.NP1.str;
    *pmean_dip = meca_mean.NP1.dip;
    *pmean_rake = meca_mean.NP1.rake;

}



//
// WARNING: the following functions are not correct!
// They do not properly account for strike/dip/rake wrap around
//

void correct_fm_angles(double *x, double *y, double *z, double xReference, double yReference, double zReference) {

    if (*x - xReference > 180.0)
        *x -= 360.0;
    else if (*x - xReference < -180.0)
        *x += 360.0;

    if (*y - yReference > 45.0)
        *y -= 90.0;
    else if (*y - yReference < -45.0)
        *y += 90.0;

    if (*z - zReference > 90.0)
        *z -= 180.0;
    else if (*z - zReference < -90.0)
        *z += 180.0;

}

/** function to calculate the expectation (mean) of a set of samples
 *
 * x - strike
 * y - dip
 * z - rake
 *
 * corrects for:
 *   x wrap of 360 deg
 *
 */

Vect3D CalcExpectationSamplesFM(float* fdata, int nSamples, double xReference, double yReference, double zReference) {

    int nsamp, ipos;

    double x, y, z;
    Vect3D expect = {0.0, 0.0, 0.0};

    ipos = 0;
    for (nsamp = 0; nsamp < nSamples; nsamp++) {
        x = fdata[ipos++];
        y = fdata[ipos++];
        z = fdata[ipos++];
        ipos++; // fdata value is in 4th position

        correct_fm_angles(&x, &y, &z, xReference, yReference, zReference);

        expect.x += x;
        expect.y += y;
        expect.z += z;
    }

    expect.x /= (double) nSamples;
    expect.y /= (double) nSamples;
    expect.z /= (double) nSamples;

    return (expect);
}

/** function to calculate the covariance of a set of samples
 * x - strike
 * y - dip
 * z - rake
 *
 * corrects for:
 *   x wrap of 360 deg
 */

Mtrx3D CalcCovarianceSamplesFM(float* fdata, int nSamples, Vect3D * pexpect) {

    int nsamp, ipos;

    double x, y, z; //, prob;
    Mtrx3D cov = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    double xReference = pexpect->x;
    double yReference = pexpect->y;
    double zReference = pexpect->z;

    /* calculate covariance following eq. (6-12), T & V, 1982 */

    ipos = 0;
    for (nsamp = 0; nsamp < nSamples; nsamp++) {
        x = fdata[ipos++];
        if (x - xReference > 180.0)
            x -= 360.0;
        else if (x - xReference < -180.0)
            x += 360.0;
        y = fdata[ipos++];
        z = fdata[ipos++];
        ipos++; //prob = fdata[ipos++];

        correct_fm_angles(&x, &y, &z, xReference, yReference, zReference);

        cov.xx += x * x;
        cov.xy += x * y;
        cov.xz += x * z;

        cov.yy += y * y;
        cov.yz += y * z;

        cov.zz += z * z;

    }

    cov.xx = cov.xx / (double) nSamples - pexpect->x * pexpect->x;
    cov.xy = cov.xy / (double) nSamples - pexpect->x * pexpect->y;
    cov.xz = cov.xz / (double) nSamples - pexpect->x * pexpect->z;

    cov.yx = cov.xy;
    cov.yy = cov.yy / (double) nSamples - pexpect->y * pexpect->y;
    cov.yz = cov.yz / (double) nSamples - pexpect->y * pexpect->z;

    cov.zx = cov.xz;
    cov.zy = cov.yz;
    cov.zz = cov.zz / (double) nSamples - pexpect->z * pexpect->z;


    return (cov);
}



/*** function to calculate radiation amplitude */

/* A & R figs 4.20 & 5.5 */

double calc_rad(double mech_phi, double mech_del, double mech_lam, double ray_vert_ang, double ray_horiz_ang, char orig_wave) {

    double radamp = 1.0;

    /* calculate radiation pattern (from Aki & Richards eqs. 4.84 - 4.86) */

    if (orig_wave == 'P') {
        radamp = cos(mech_lam) * sin(mech_del) * pow(sin(ray_vert_ang), 2.0)
                * sin(2.0 * (ray_horiz_ang - mech_phi))
                - cos(mech_lam) * cos(mech_del) * sin(2.0 * ray_vert_ang)
                * cos(ray_horiz_ang - mech_phi)
                + sin(mech_lam) * sin(2.0 * mech_del)
                * (pow(cos(ray_vert_ang), 2.0) - pow(sin(ray_vert_ang)
                * sin(ray_horiz_ang - mech_phi), 2.0))
                + sin(mech_lam) * cos(2.0 * mech_del) * sin(2.0 * ray_vert_ang)
                * sin(ray_horiz_ang - mech_phi)
                ;
    } else if (orig_wave == 'V') {
        radamp = sin(mech_lam) * cos(2.0 * mech_del) * cos(2.0 * ray_vert_ang)
                * sin(ray_horiz_ang - mech_phi)
                - cos(mech_lam) * cos(mech_del) * cos(2.0 * ray_vert_ang)
                * cos(ray_horiz_ang - mech_phi)
                + 0.5 * cos(mech_lam) * sin(mech_del)
                * sin(2.0 * ray_vert_ang) * sin(2.0 * (ray_horiz_ang - mech_phi))
                - 0.5 * sin(mech_lam) * sin(2.0 * mech_del) * sin(2.0 * ray_vert_ang)
                * (1.0 + pow(sin(ray_horiz_ang - mech_phi), 2.0))
                ;
        radamp *= -1.0;
        /* mult by -1 since A & R change dir of + in figs 4.20 & 5.5 */
    } else if (orig_wave == 'H') {
        radamp = cos(mech_lam) * cos(mech_del) * cos(ray_vert_ang)
                * sin(ray_horiz_ang - mech_phi)
                + cos(mech_lam) * sin(mech_del) * sin(ray_vert_ang)
                * cos(2.0 * (ray_horiz_ang - mech_phi))
                + sin(mech_lam) * cos(2.0 * mech_del)
                * cos(ray_vert_ang) * cos(ray_horiz_ang - mech_phi)
                - 0.5 * sin(mech_lam) * sin(2.0 * mech_del) * sin(ray_vert_ang)
                * sin(2.0 * (ray_horiz_ang - mech_phi))
                ;
    } else {
        fprintf(stderr, "ERROR: in calc_rad(): unrecognized original wave type: %c", orig_wave);
    }

    return (radamp);

}

/** evaulates eq. 31 from
 * 2009__Walsh_et__A_Bayesian_approach_to_determining_and_parametrizing_earthquake_focal_mechanisms__GJI.pdf
 *
 * inconsistent_polarity_rate = pi^prime_p0   e.g. 0.2
 * amplitude_noise_rate = sigma_a0   e.g. 1/6
 *
 * returns probability = pi_i
 *
 */


double WalshEtAl2009_polarity_probability(double inconsistent_polarity_rate, double amplitude_noise_rate, double rad_amp) {

    double probabilty = inconsistent_polarity_rate + (1.0 - 2.0 * inconsistent_polarity_rate)
            * cumulDistNormal(rad_amp / amplitude_noise_rate, 0.0, 1.0);

    return (probabilty);

}

double WalshEtAl2009_polarity_probability_modif(double inconsistent_polarity_rate, double weight) {

    double probabilty = inconsistent_polarity_rate + (1.0 - 2.0 * inconsistent_polarity_rate) * weight;

    return (probabilty);

}
