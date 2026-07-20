/***************************************************************************
 * fmamp_invert.c
 *
 * Performs probabilistic, OctTree search for focal mechanism using P first motion and amplitude data.
 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 * created: 2014.02.18
 ***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>


#define EXTERN_MODE

#include "fmamp.h"
#include "../statistics/statistics.h"


// forward function declarations
double octtreeFaultPlaneInversion(
        Pick *ppick_list, int num_picks,
        int max_num_nodes, double min_node_size_deg, double initial_grid_step,
        int min_number_observations, int critical_number_observations, int use_first_motion_polarities, int use_amplitudes,
        FaultPlaneSolution *pbest_fault_plane_solution,
        float **pscatter_sample, int *p_n_alloc_scatter_sample, int *pn_scatter_sample,
        int verbose
        );

/***************************************************************************
 * read hypocenter and picks input files:
 *
 * Returns 0 on success and -1 otherwise.
 ***************************************************************************/
double fmamp_invert(Hypocenter *phypocenter, int max_num_nodes, double min_node_size_deg, double initial_grid_step,
        int min_number_observations, int critical_number_observations, int use_first_motion_polarities, int use_amplitudes,
        FaultPlaneSolution *pfault_plane_solution, float **pscatter_sample, int *p_n_alloc_scatter_sample, int *pn_scatter_sample,
        int verbose) {

    if (verbose) {
        printf("Info: Processing event: %4.2d.%2.2d.%2.2d-%2.2d.%2.2d.%06.3f w/ %d arrivals\n",
                phypocenter->year, phypocenter->month, phypocenter->day, phypocenter->hour, phypocenter->min, phypocenter->dec_sec,
                phypocenter->pick_list_size);
    }

    FaultPlaneSolution best_fault_plane_solution;
    best_fault_plane_solution.nobs_used_polarity = 0;
    best_fault_plane_solution.nobs_used_amplitude = 0;
    best_fault_plane_solution.probability = -1.0;
    best_fault_plane_solution.nreadings = 0;
    best_fault_plane_solution.stat_dist_ratio = -1.0;

    double best_prob = octtreeFaultPlaneInversion(
            phypocenter->pick_list, phypocenter->pick_list_size,
            max_num_nodes, min_node_size_deg, initial_grid_step,
            min_number_observations, critical_number_observations, use_first_motion_polarities, use_amplitudes,
            &best_fault_plane_solution,
            pscatter_sample, p_n_alloc_scatter_sample, pn_scatter_sample,
            verbose
            );

    *pfault_plane_solution = best_fault_plane_solution;

    return (best_prob);

}

/** returns variance and mean of an array of double values, checking for invalid values
 */

double calcVariance(double* darray, double* wtarray, int nvalues, double value_invalid, double *pmean, double *pmean_abs, double *pvalue_min, double *pvalue_max, int *pnum_valid) {

    double value_min = FLT_MAX;
    double value_max = -FLT_MAX;

    double sum = 0.0;
    double sum_abs = 0.0;
    double sum_sqr = 0.0;

    int nused = 0;
    double weight;
    double weight_sum = 0.0;

    int i;
    double value;
    for (i = 0; i < nvalues; i++) {
        value = darray[i];
        if (value == value_invalid)
            continue;
        nused++;
        weight = wtarray[i];
        weight_sum += weight;
        sum += value * weight;
        sum_abs += fabs(value) * weight;
        sum_sqr += value * value;
        if (value < value_min)
            value_min = value;
        if (value > value_max)
            value_max = value;
    }

    *pvalue_min = value_min;
    *pvalue_max = value_max;
    *pnum_valid = nused;

    double variance = 0.0;

    if (nused < 1 || weight_sum < FLT_MIN) {
        *pmean = 0.0;
        *pmean_abs = 0.0;
        return (variance);
    } else if (nused == 1) {
        *pmean = sum / weight_sum;
        *pmean_abs = sum_abs / weight_sum;
        return (variance);
    }

    double mean = sum / weight_sum;
    variance = (sum_sqr / weight_sum) - mean * mean;
    *pmean = mean;
    *pmean_abs = sum_abs / weight_sum;
    return (variance);

}

/** Calculates fit of y_value vs. x_value to specified line
 *
 * returns number of valid values used in the regression
 *
 *      x_value, y_value : double arrays of N=nvalues of x,y data for fit, x_value[n] == DBL_MAX flags invalid data
 */
double calcFitToLine(double *x_value, double *y_value, double *weight, int nvalues, double slope, double intercept, double *pmisfit) {

    double misfit = 0.0;

    int nvalid = 0;
    // loop over values and accumulate misfit sums
    double xval, yval, diff, wt;
    double wt_sum = 0.0;
    double xmean_abs = 0.0;
    //double xmean = 0.0;
    //double xsum_sqr = 0.0;
    for (int narr = 0; narr < nvalues; narr++) {
        // check for valid value
        if (x_value[narr] > -DBL_MAX) {
            wt = weight[narr];
            xval = x_value[narr];
            xmean_abs += fabs(xval) * wt;
            ////xmean += xval;
            //xsum_sqr += xval * xval;
            yval = y_value[narr];
            diff = xval - (yval - intercept) / slope; // fit is in x (predicted amplitudes, scale near 1.0)
            misfit += fabs(diff) * wt; // L1 norm
            //misfit += diff * diff; // L2 norm
            wt_sum += wt;
            nvalid++;
        }
    }
    if (wt_sum > FLT_MIN) {
        misfit /= wt_sum;
        xmean_abs /= wt_sum;
        misfit /= xmean_abs; // fit is in x: weight misfit by mean of abs(x) values, to downweight solutions where all obs are near a nodal plane
        //xmean /= (double) nvalid;
        //double variance = (xsum_sqr / (double) (nvalid - 1)) - xmean * xmean;
        //double xrange = sqrt(variance);
        //misfit /= xrange;  // weight misfit by range of x values
    } else {
        misfit = 9999.9;
    }

    *pmisfit = misfit;
    return (wt_sum);

}

/** Calculates fit of y_value vs. x_value to specified line trimming specified percentage of outliers
 *
 * returns number of valid values used in the regression
 *
 *      x_value, y_value : double arrays of N=nvalues of x,y data for fit, x_value[n] == DBL_MAX flags invalid data
 */
static double* outlier_diff = NULL;
static double* outlier_wt = NULL;
static double* outlier_xmean_abs = NULL;
static int num_outlier_max = 0;

double calcFitToLineTrim(double *x_value, double *y_value, double *weight, int nvalues, double slope, double intercept, double *pmisfit, double percent_trim) {

    double misfit = 0.0;

    // calculate number of outliers to find
    int noutliers = (int) ((double) nvalues * percent_trim / 100.0);
    // increase number to further penalize inversion with few values
    // 20151028 AJL  noutliers++;
    // initialize memory for outliers
    if (outlier_diff != NULL && (num_outlier_max < noutliers)) {
        free(outlier_diff);
        outlier_diff = NULL;
        free(outlier_wt);
        outlier_wt = NULL;
        free(outlier_xmean_abs);
        outlier_xmean_abs = NULL;
    }
    if (outlier_diff == NULL) {
        outlier_diff = (double*) calloc(noutliers, sizeof (double));
        outlier_wt = (double*) calloc(noutliers, sizeof (double));
        outlier_xmean_abs = (double*) calloc(noutliers, sizeof (double));
        num_outlier_max = noutliers;
    }
    for (int n = 0; n < noutliers; n++) {
        outlier_diff[n] = -1.0;
    }


    int nvalid = 0;
    // loop over values and accumulate misfit sums
    double xval, yval, diff, wt;
    double wt_sum = 0.0;
    double xmean_abs = 0.0;
    //double xmean = 0.0;
    //double xsum_sqr = 0.0;
    int ndx_outlier;
    for (int narr = 0; narr < nvalues; narr++) {
        // check for valid value
        if (x_value[narr] > -DBL_MAX) {
            wt = weight[narr];
            xval = x_value[narr];
            xmean_abs += fabs(xval) * wt;
            ////xmean += xval;
            //xsum_sqr += xval * xval;
            yval = y_value[narr];
            // TEST NO POLARITY !!  yval = fabs(y_value[narr]);
            diff = fabs(xval - (yval - intercept) / slope); // IMPORTANT: fit is in x (predicted amplitudes, scale near 1.0)  // L1 norm
            misfit += diff * wt; // L1 norm
            wt_sum += wt;
            nvalid++;
            // check if possible outlier
            ndx_outlier = -1;
            for (int n = 0; n < noutliers; n++) {
                if (diff >= outlier_diff[n]) {
                    ndx_outlier = n;
                } else {
                    break;
                }
            }
            if (ndx_outlier >= 0) {
                // shift saved outlier data and insert new outlier
                for (int n = 1; n <= ndx_outlier; n++) {
                    outlier_diff[n - 1] = outlier_diff[n];
                    outlier_wt[n - 1] = outlier_wt[n];
                    outlier_xmean_abs[n - 1] = outlier_xmean_abs[n];
                }
                outlier_diff[ndx_outlier] = diff;
                outlier_wt[ndx_outlier] = wt;
                outlier_xmean_abs[ndx_outlier] = fabs(xval) * wt;
            }
        }
    }

    // remove outliers from results
    for (int n = 0; n < noutliers; n++) {
        if (outlier_diff[n] >= 0.0) {
            misfit -= outlier_diff[n] * outlier_wt[n]; // L1 norm
            wt_sum -= outlier_wt[n];
            xmean_abs -= outlier_xmean_abs[n];
        }
    }

    // calculate misfit
    if (wt_sum > FLT_MIN) {
        misfit /= wt_sum;
        //misfit /= (double) nvalid;
        xmean_abs /= wt_sum;
        // 20151028 AJL ???        misfit /= xmean_abs; // fit is in x: weight misfit by mean of abs(x) values, to downweight solutions where all obs are near a nodal plane
        //xmean /= (double) nvalid;
        //double variance = (xsum_sqr / (double) (nvalid - 1)) - xmean * xmean;
        //double xrange = sqrt(variance);
        //misfit /= xrange;  // weight misfit by range of x values
    } else {
        misfit = 9999.9;
    }

    *pmisfit = misfit;
    return (wt_sum);

}


// Development, TODO: make program property
// 20160317
//#define PERCENT_OUTLIER_ERROR_ALLOWED 5.0
// #define PERCENT_OUTLIER_ERROR_ALLOWED 10.0  // 20160317 AJL - same as HASH
#define PERCENT_OUTLIER_ERROR_ALLOWED 0.0  // 20170412 AJL - BIG CHANGE!
// Development, TODO: remove defines when stable
#define LINEAR_REGRESSION 1
//
#define OCTTREE_UNDEF_VALUE -VERY_SMALL_DOUBLE
#define OCTTREE_TYPE 1
#define ICOUNT_INCREMENT 99999999
#ifdef LINEAR_REGRESSION   // linear regression
static double *x_value_array = NULL;
static int n_alloc_x_value_array = 0;
static double *y_value_array = NULL;
static int n_alloc_y_value_array = 0;
//static LinRegression linRegression;
#else
static double *value_array = NULL;
static int n_alloc_value_array = 0;
#endif
static double *weight_array = NULL;
static int n_alloc_weight_array = 0;
static double **obs_take_off_angles = NULL;
static int n_obs_take_off_angle = 0;
static double measureOfDistributionOnSphere;
#define VALUE_INVALID -DBL_MAX

int octtree_core(ResultTreeNode** ppResultTreeRoot, OctNode* poct_node, double oct_node_volume,
        Pick *ppick_list, int num_picks, int min_number_observations, int critical_number_observations, int use_first_motion_polarities, int use_amplitudes,
        FaultPlaneSolution *pbest_fault_plane_solution
        ) {


    // set mechanism parameters
    // strike -> x
    // dip -> y
    // rake -> z
    double strike = poct_node->center.x;
    double dip = poct_node->center.y;
    double rake = poct_node->center.z;
    double mech_phi = strike * DE2RA;
    double mech_del = dip * DE2RA;
    double mech_lam = rake * DE2RA;

    // initialize local best solution variables for this node
    int is_global_best = 0;
    int nobs_used_polarity = 0;
    int nobs_used_amplitude = 0;

    // loop over observations
    double sum_obs_pred_weight = 0.0;
    double sum_obs_weight = 0.0;
    double sum_polarity_misfit = 0.0;
    double sum_polarity_misfit_weight = 0.0;
    double sum_amplitude_misfit_weight = 0.0;
    double weight = 0.0;
    double ray_vert_ang, ray_horiz_ang, radamp;
    int pred_polarity, obs_polarity;
    double pred_amplitude, obs_amplitude;
#ifdef LINEAR_REGRESSION   // linear regression
    double obs_mean = 0.0;
    double pred_mean = 0.0;
    double pred_sum_sqr = 0.0;
    //double pred_variance = 0.0;
    //double pred_std_dev = 0.0;
#else
    double amp_ratio, obs_mean = 0.0;
#endif
    Pick *ppick;
    int npick;
    double prob_pol = 0.0;
#define USE_FMAMP_POLARITY_METHOD
#ifndef USE_FMAMP_POLARITY_METHOD
    prob_pol = 1.0;
    double inconsistent_polarity_rate = 0.2;
    double amplitude_noise_rate = 1.0 / 6.0;
#endif
    // loop over available pick data
    for (npick = 0; npick < num_picks; npick++) {
        ppick = ppick_list + npick;
        // get predicted radiation amplitude
        ray_vert_ang = ppick->take_off_angle_inc * DE2RA;
        ray_horiz_ang = ppick->take_off_angle_az * DE2RA;
        radamp = calc_rad(mech_phi, mech_del, mech_lam, ray_vert_ang, ray_horiz_ang, 'P');
        pred_polarity = radamp >= 0.0 ? 1 : -1;
        obs_polarity = ppick->fmpolarity;
        // accumulate misfit
        if (use_first_motion_polarities) { // use first motions
#ifdef USE_FMAMP_POLARITY_METHOD
            // after 20151028
            //weight *= (1.0 + fabs(radamp)) / 2.0;   // TEST!
            //weight = 1.0; // TEST! for compatibility with HASH
            if (ppick->take_off_angle_distrib_weight >= 0.0) { // flags pick to be used in inversion
                weight = ppick->fmquality * ppick->take_off_angle_distrib_weight;
                if (obs_polarity != pred_polarity) {
                    // 20160330 AJL
                    sum_polarity_misfit += weight;
                    // 20160330 AJL - TEST downweight of nodal polarities
                    //double radamp_wt = fabs(radamp);
                    //radamp_wt = (0.5 + 0.5 * radamp_wt);
                    //radamp_wt = radamp_wt < 0.1 ? radamp_wt / 0.1 : 1.0;
                    //radamp_wt = radamp_wt > 0.5 ? 1.0 : pow(radamp_wt / 0.5, 0.5);
                    //radamp_wt = radamp_wt > 0.25 ? 1.0 : pow(radamp_wt / 0.25, 0.25);
                    //radamp_wt = radamp_wt > 0.1 ? 1.0 : sin((PI / 2.0) * radamp_wt / 0.1);
                    //radamp_wt = radamp_wt > 0.5 ? 1.0 : (radamp_wt + 0.5) / 1.0;
                    //radamp_wt = pow(radamp_wt, 0.25);
                    //sum_polarity_misfit += weight * radamp_wt;  // 20160330 AJL - TEST downweight of nodal polarities
                }
                sum_polarity_misfit_weight += weight;
                nobs_used_polarity++;
            }
#else   // 20170411 TEST 2009__Walsh_et__A_Bayesian_approach_to_determining_and_parametrizing_earthquake_focal_mechanisms__GJI.pdf
            if (ppick->take_off_angle_distrib_weight >= 0.0) { // flags pick to be used in inversion
                //*
                weight = 1.0;
                double pick_prob = WalshEtAl2009_polarity_probability(inconsistent_polarity_rate, amplitude_noise_rate, radamp);
                if (obs_polarity == pred_polarity) {
                    prob_pol *= pick_prob;
                } else {
                    prob_pol *= (1.0 - pick_prob);
                }//*/
                /*
                weight = ppick->fmquality * ppick->take_off_angle_distrib_weight;
                double pick_prob = WalshEtAl2009_polarity_probability_modif(inconsistent_polarity_rate, 1.0);
                if (obs_polarity == pred_polarity) {
                    prob_pol += weight * pick_prob;
                } else {
                    prob_pol += weight * (1.0 - pick_prob);
                }//*/
                static int ncount = 0;
                if (ncount++ % 2000 == 0) {
                    printf("DEBUG: =============> weight=%g obs/pred_polarity %d/%d pick_prob=%g prob_pol=%g\n", weight, obs_polarity, pred_polarity, pick_prob, prob_pol);
                }
                sum_polarity_misfit_weight += weight;
                nobs_used_polarity++;
            }
#endif
        }
        if (use_amplitudes) { // use amplitudes
#ifdef LINEAR_REGRESSION   // linear regression
            if (ppick->take_off_angle_distrib_weight >= 0.0) { // flags pick to be used in inversion
                //weight *= (1.0 + fabs(radamp)) / 2.0; // TEST!
                //weight = fabs(radamp); // TEST!
                //weight = 1.0;
                weight = ppick->fmquality * ppick->take_off_angle_distrib_weight;
                pred_amplitude = radamp;
                obs_amplitude = ppick->amplitude * (double) obs_polarity;
                // TEST POLARITY ONLY !!
                //obs_amplitude = (double) obs_polarity;
                // END TEST POLARITY ONLY !!
                x_value_array[nobs_used_amplitude] = pred_amplitude;
                y_value_array[nobs_used_amplitude] = obs_amplitude;
#if 0           // test w/o polarity information to show information provided by amplitudes only
                x_value_array[nobs_used_amplitude] = fabs(pred_amplitude);
                y_value_array[nobs_used_amplitude] = fabs(obs_amplitude);
#endif
                weight_array[nobs_used_amplitude] = weight;
                sum_amplitude_misfit_weight += weight;
                pred_mean += fabs(pred_amplitude) * weight;
                pred_sum_sqr += pred_amplitude * pred_amplitude * weight;
                obs_mean += fabs(obs_amplitude) * weight;
                nobs_used_amplitude++;
                //}
            }
#else
            value_array[npick] = VALUE_INVALID;
            weight_array[npick] = 0.0;
            if (ppick->amplitude > FLT_MIN) { // negative amplitude flags that channel gain is not available
                //weight *= (1.0 + fabs(radamp)) / 2.0; // TEST!
                //weight = fabs(radamp); // TEST!
                weight = 1.0;
                obs_amplitude = ppick->amplitude * (double) obs_polarity;
                pred_amplitude = radamp;
                // damp predicted amplitudes
                if (pred_amplitude >= 0.0 && pred_amplitude < 0.1)
                    pred_amplitude = 0.1;
                else if (pred_amplitude > -0.1 && pred_amplitude <= 0.0)
                    pred_amplitude = -0.1;
                //if (fabs(obs_amplitude) > FLT_MIN && fabs(pred_amplitude) > 0.1) {
                amp_ratio = obs_amplitude / pred_amplitude;
                //printf("DEBUG: obs_amplitude=%f, pred_amplitude=%f, amp_ratio=%f\n", obs_amplitude, pred_amplitude, amp_ratio);
                //if (amp_ratio > 0.0) {
                value_array[npick] = amp_ratio;
                weight_array[npick] = weight;
                sum_amplitude_misfit_weight += weight;
                nobs_used_amplitude++;
                obs_mean += fabs(obs_amplitude);
                //}
            }
#endif
        }
        sum_obs_pred_weight += weight * sqrt(fabs(radamp));
        sum_obs_weight += weight;
        ppick->weight = weight;
        if (DEBUG && npick % 20 == 0) {
            printf("DEBUG: %s az=%f, inc=%f, qual=%f, wt=%f, pol=%d, amp=%f, ppol=%d, pamp=%f\n",
                    ppick->phase, ppick->take_off_angle_az, ppick->take_off_angle_inc, ppick->fmquality, ppick->weight, obs_polarity, obs_amplitude,
                    pred_polarity, pred_amplitude);
        }
    }

    // calculate solution likelihood (prob)
    // polarity inversion
    double misfit_pol = 9999.9;
    if (use_first_motion_polarities) { // use first motions
        if (nobs_used_polarity >= min_number_observations) {
#ifdef USE_FMAMP_POLARITY_METHOD
            // after 20151028
            prob_pol = 0.0;
            //
            double outlier_allowed_weighted = ((double) PERCENT_OUTLIER_ERROR_ALLOWED / 100.0) * sum_polarity_misfit_weight;
            //double outlier_allowed_weighted = ((double) PERCENT_OUTLIER_ERROR_ALLOWED / 100.0) * (double) nobs_used_polarity;
            //printf("DEBUG: outlier_allowed_weighted=%f, sum_polarity_misfit_weight=%f\n", outlier_allowed_weighted, sum_polarity_misfit_weight);
            double misfit = sum_polarity_misfit - outlier_allowed_weighted;
            //printf("DEBUG: misfit %f = sum_polarity_misfit %f - outlier_allowed_weighted %f\n", misfit, sum_polarity_misfit, outlier_allowed_weighted);
            misfit = misfit < 0.0 ? 0.0 : misfit;

            // 20160322 AJL - TEST normalizing misfit weight sums to N data
            // 20160322 14:50  misfit *= 40.0 / sum_polarity_misfit_weight;
#define MISFIT_NORMALIZATION_CUTOFF 30.0
            /* 20170411 AJL - removed, makes little difference ???
            if (sum_polarity_misfit_weight >= MISFIT_NORMALIZATION_CUTOFF) {
                // reduce misfit proportionally
                misfit *= MISFIT_NORMALIZATION_CUTOFF / sum_polarity_misfit_weight;
            } else {
                // increase misfit with bias (so as not to amplify contribution of each individual polarity misfit)
                misfit += MISFIT_NORMALIZATION_CUTOFF - sum_polarity_misfit_weight;
            }
            //*/

            //misfit = sum_polarity_misfit / (1.0 + outlier_allowed_weighted);

            //double sigma_polarity = ((double) PERCENT_OUTLIER_ERROR_ALLOWED / 100.0) * (double) nobs_used_polarity; // sets sigma = un-weighted number wrong allowed
            //double sigma_polarity = 1.0; // sets sigma
            //double sigma_polarity = 0.75; // sets sigma  // 20160321
            double sigma_polarity = 0.5; // sets sigma   // 20160321
            //double exp_pow = sum_polarity_misfit / sigma_polarity;
            //double sigma_polarity = outlier_allowed_weighted; // sets sigma   // 20160319
            //double sigma_polarity = (double) nobs_used_polarity / sum_polarity_misfit_weight; // sets sigma   // 20160319
            //double sigma_polarity = 2.0 * sum_polarity_misfit_weight / (double) nobs_used_polarity; // sets sigma   // 20160321
            double exp_pow = misfit / sigma_polarity;
            //
            misfit_pol = sum_polarity_misfit / sum_polarity_misfit_weight;
            //
            //exp_pow *= sqrt(measureOfDistributionOnSphere);
            prob_pol = exp(-exp_pow); // like L1 norm
            //prob_pol = exp(-exp_pow * 2.0);         // 20160317 AJL
            //prob_pol = exp(-exp_pow * exp_pow);         // like L2 norm  // 20160321 AJ
            //prob_pol = exp(-exp_pow * 1.5);           // 20160318 AJ
#else   // 20170411 TEST 2009__Walsh_et__A_Bayesian_approach_to_determining_and_parametrizing_earthquake_focal_mechanisms__GJI.pdf
            if (nobs_used_polarity < min_number_observations) {
                prob_pol = 0.0;
            }
#endif
        }
    }
    // amplitude inversion
    double misfit_amp = 9999.9;
    double prob_amp = -1.0;
    if (use_amplitudes) { // use amplitudes
        prob_amp = 0.0;
        if (nobs_used_amplitude >= min_number_observations) {
            // factor to weight number of available readings
            //double arg = (double) critical_number_observations / sum_amplitude_misfit_weight;
            //double amplitude_count_misfit_factor = exp(log(2) * arg * arg);
#ifdef LINEAR_REGRESSION   // linear regression
            /* 20140330
            int num_valid = calcLinearRegression(x_value_array, y_value_array, nobs_used_amplitude, &linRegression);
            // 20140325 AJL - test of regression through the origin - probably better to use standard regression so get strong penalty if fit does not pass through origin
            //int num_valid = calcLinearRegressionInterceptZero(x_value_array, y_value_array, nobs_used_amplitude, &linRegression);
            if (num_valid > 3) {
                obs_mean /= (double) nobs_used_amplitude;
                pred_mean /= (double) nobs_used_amplitude;
    #if 0
                double slope_factor = fabs(linRegression.slope / (obs_mean / pred_mean) - 1.0) / 0.2;
                double slope_dev_factor = (linRegression.slope_dev / (obs_mean / pred_mean)) / 0.1;
                double intercept_factor = fabs(linRegression.intercept) / obs_mean / 0.2;
                double corr_factor = (1.0 - linRegression.correl_coeff) / 0.1;
                misfit_amp = corr_factor + slope_dev_factor + slope_factor + intercept_factor;
                prob_amp = exp(-misfit_amp);
    #else
                double slope_factor = fabs(linRegression.slope / (obs_mean / pred_mean) - 1.0) / 0.5;
                double slope_dev_factor = (linRegression.slope_dev / (obs_mean / pred_mean) / 1.0);
                double intercept_factor = fabs(linRegression.intercept) / obs_mean / 1.0;
                double corr_factor = (1.0 - linRegression.correl_coeff);
                misfit_amp = corr_factor + slope_dev_factor + slope_factor + intercept_factor;
                prob_amp = exp(-misfit_amp * 5.0);
    #endif
                printf("DEBUG: Amplitude: obs_mean=%.2g, pred_mean=%.2g, o_p=%.2g, slope=%.2g, slope_f=%.2g, intcpt=%.2g intcpt_f=%.2g, cor_coef=%.2g, corr_f=%.2g, slope=%.2g, slope_dev=%.2g, slope_dev_f=%.2g, misfit=%.2g, prob=%.2g, sum_amp_mf_wt=%.2g\n",
                        obs_mean, pred_mean, obs_mean / pred_mean, linRegression.slope, slope_factor, linRegression.intercept, intercept_factor, linRegression.correl_coeff, corr_factor,
                        linRegression.slope, linRegression.slope_dev, slope_dev_factor, misfit_amp, prob_amp, sum_amplitude_misfit_weight);
            }*/
            // 20140325 AJL - test of regression through the origin - probably better to use standard regression so get strong penalty if fit does not pass through origin
            //int num_valid = calcLinearRegressionInterceptZero(x_value_array, y_value_array, nobs_used_amplitude, &linRegression);
            obs_mean /= sum_amplitude_misfit_weight;
            pred_mean /= sum_amplitude_misfit_weight;
            //pred_variance = (pred_sum_sqr / sum_amplitude_misfit_weight) - pred_mean * pred_mean;
            //pred_std_dev = sqrt(pred_variance);
            double intercept = 0.0;
            double misfit_line = 9999.9;
            double slope;
            double weight_sum_valid;
            // x - predicted amp, y - observed amp
#if 1
            // 20140902 AJL - test of weighted regression through the origin
            static LinRegression linRegression;
            weight_sum_valid = calcWeightedLinearRegressionInterceptZero(x_value_array, y_value_array, weight_array, nobs_used_amplitude, &linRegression);
            // the following two settings insure that misfit returned by calcFitToLine() is in predicted amplitude units (0-1)
            slope = linRegression.slope;
#else
            // fit to line with slope obs_mean / pred_mean
            slope = obs_mean / pred_mean;
#endif
#if 1   // after 20151028
            if (slope > 0.0) { // reject negative solutions (obs amp negative of pred amp)
                // 20140922 AJL - fit with outlier data removed
                weight_sum_valid = calcFitToLineTrim(x_value_array, y_value_array, weight_array, nobs_used_amplitude,
                        slope, intercept, &misfit_line, 0.0);
                if (weight_sum_valid > 3.0) {
                    double exp_pow = misfit_line / (PERCENT_OUTLIER_ERROR_ALLOWED / 100.0);
                    exp_pow *= sqrt(measureOfDistributionOnSphere);
                    prob_amp = exp(-exp_pow);
                }
                //printf("DEBUG: Amplitude: obs_mean=%.2g, pred_mean=%.2g, pred_std_dev=%.2g, misfit_line=%.2g, slope=%.2g, slope_dev=%.2g, misfit=%.2g, prob=%.2g, sum_amp_mf_wt=%.2g, amp_ct_mf_wt=%.2g\n", obs_mean, pred_mean, pred_std_dev, misfit_line, slope, linRegression.slope_dev, misfit_amp, prob_amp, sum_amplitude_misfit_weight, amplitude_count_misfit_factor);
            }
#else   // before 20151028
            if (slope > 0.0) { // reject negative solutions (obs amp negative of pred amp)
                // 20140922 AJL - fit with outlier data removed
                weight_sum_valid = calcFitToLineTrim(x_value_array, y_value_array, weight_array, nobs_used_amplitude,
                        slope, intercept, &misfit_line, PERCENT_OUTLIER_ERROR_ALLOWED);
                if (weight_sum_valid > 3.0) {
                    misfit_amp = misfit_line;
                    // 20140910 AJL - TEST  misfit_amp = linRegression.slope_dev / slope;
                    //double pred_mean_wt = pred_mean;
                    double pred_mean_wt = pred_mean < 0.25 ? 4.0 * pred_mean : 1.0; // 20140909 AJL
                    //double pred_mean_wt = pred_mean < 0.5 ? 2.0 * pred_mean : 1.0;        // 20140908 AJL
                    //misfit_amp *= 1.0 / pred_mean_wt;      // 20140908 AJL - TEST: weight misfit by mean of predicted amplitudes, to avoid solutions where all obs are near a nodal plane
                    //misfit_amp *= sqrt(1.0 / (pred_mean / 0.5));      // 20140903 AJL - TEST: weight misfit by mean of predicted amplitudes, to avoid solutions where all obs are near a nodal plane
                    //misfit_amp *= (1.0 / (pred_std_dev / 0.25));      // 20140903 AJL - TEST: weight misfit by std dev of predicted amplitudes, to avoid solutions where all obs are near a nodal plane
                    //misfit_amp *= pow(linRegression.slope / obs_mean, 0.5);      // 20140903 AJL - TEST: weight misfit by XXX, to avoid solutions where all obs are near a nodal plane
                    // 20140422 double exp_pow = (2.0 * misfit_amp / 0.1) / amplitude_count_misfit_factor;
                    // 20140910 AJL - TEST  double exp_pow = (2.0 * misfit_amp / 0.1);
                    double exp_pow = misfit_amp / 0.05;
                    //double exp_pow = (misfit_amp / 0.1);
                    //double dist_wt = measureOfDistributionOnSphere / 0.3; // 20140909 AJL
                    //double dist_wt = measureOfDistributionOnSphere < 0.5 ? 2.0 * measureOfDistributionOnSphere : 1.0;        // 20140909 AJL
                    //dist_wt = pow(dist_wt, 4.0);
                    // 20140910 AJL - TEST  exp_pow *= dist_wt;        // 20140909 AJL
                    exp_pow *= sqrt(measureOfDistributionOnSphere);
                    // 20140910 AJL - TEST
                    exp_pow /= amplitude_count_misfit_factor; // broadens likelihood function if fewer data
                    static int icount = 0;
                    if (0 && icount++ % 2000 == 0) {
                        printf("DEBUG exp_pow: pred_mean %.2g, pred_mean_wt %.2g, uniDistSphere %.2g, amplitude_count_misfit_factor %.2g\n",
                                pred_mean, pred_mean_wt, measureOfDistributionOnSphere, amplitude_count_misfit_factor);
                    }
                    prob_amp = exp(-exp_pow);
                    //printf("DEBUG: Amplitude: obs_mean=%.2g, pred_mean=%.2g, pred_std_dev=%.2g, misfit_line=%.2g, y_x_scale=%.2g, slope=%.2g, slope_dev=%.2g, misfit=%.2g, prob=%.2g, sum_amp_mf_wt=%.2g, amp_ct_mf_wt=%.2g\n",
                    //        obs_mean, pred_mean, pred_std_dev, misfit_line, y_to_x_scale, slope, linRegression.slope_dev, misfit_amp, prob_amp, sum_amplitude_misfit_weight, amplitude_count_misfit_factor);
                }
            }
#endif

#else   // variance
            if (nobs_used_amplitude >= min_number_observations) {
                double mean, mean_abs, value_min, value_max;
                int num_valid;
                double variance = calcVariance(value_array, weight_array, num_picks, VALUE_INVALID, &mean, &mean_abs, &value_min, &value_max, &num_valid);
                if (num_valid > 3 && variance > FLT_MIN) {
                    if (mean > FLT_MIN) { // negative mean implies large proportion of polarity misfits
                        misfit_amp = (variance / (mean * mean));
                        prob_amp = exp(-misfit_amp);
                        //printf("DEBUG: Amplitude: std_dev=%f, mean=%f, num_valid=%d, misfit_amp=%g, prob_amp=%g, sum_amplitude_misfit_weight=%f\n",
                        //        sqrt(variance), mean, num_valid, misfit_amp, prob_amp, sum_amplitude_misfit_weight);
                        //}
                    }
                }
            }
#endif
        }
    }
    double prob = 0.0;
    if (use_first_motion_polarities && use_amplitudes && (nobs_used_polarity + nobs_used_amplitude) > 0) {
        prob = (prob_pol * (double) nobs_used_polarity + prob_amp * (double) nobs_used_amplitude)
                / ((double) nobs_used_polarity + (double) nobs_used_amplitude);
        // 20140919 AJL TEST prob = prob_pol * prob_amp;
    } else if (use_first_motion_polarities && nobs_used_polarity > 0) {
        prob = prob_pol;
    } else if (use_amplitudes && nobs_used_amplitude > 0) {
        prob = prob_amp;
    }
    static int icount = 0;
    if (DEBUG && (icount++ % 2000 == 0)) {
        printf("DEBUG: =============> prob=%g, strike/dip/rake=%f/%f/%f", prob, poct_node->center.x, poct_node->center.y, poct_node->center.z);
        printf(" mf(%f,%f) p[%g,%g] prob %g\n", misfit_pol, misfit_amp, prob_pol, prob_amp, prob);
    }

    if (prob > FLT_MIN && prob > pbest_fault_plane_solution->probability) {
        is_global_best = 1;
        pbest_fault_plane_solution->probability = prob;
        pbest_fault_plane_solution->polarity_misfit = misfit_pol;
        pbest_fault_plane_solution->sum_polarity_misfit_weight = sum_polarity_misfit_weight;
        pbest_fault_plane_solution->polarity_probability = prob_pol;
        pbest_fault_plane_solution->amplitude_misfit = misfit_amp;
        pbest_fault_plane_solution->amplitude_probability = prob_amp;
        pbest_fault_plane_solution->sum_amplitude_misfit_weight = sum_amplitude_misfit_weight;
        pbest_fault_plane_solution->bestNP.strike = strike;
        pbest_fault_plane_solution->bestNP.dip = dip;
        pbest_fault_plane_solution->bestNP.rake = rake;
        pbest_fault_plane_solution->nreadings = num_picks;
        pbest_fault_plane_solution->nobs_used_polarity = nobs_used_polarity;
        pbest_fault_plane_solution->nobs_used_amplitude = nobs_used_amplitude;
        pbest_fault_plane_solution->stat_dist_ratio = sum_obs_pred_weight / sum_obs_weight;
    }


    // put node results in result tree
    if (prob > FLT_MIN) {
        // update node value and result tree with best found for this node location
        poct_node->value = log(prob);
        double log_value_volume = poct_node->value + log(oct_node_volume);
        if (!isnan(prob))
            *ppResultTreeRoot = addResult(*ppResultTreeRoot, log_value_volume, oct_node_volume, poct_node);
        else {
            printf("===================== addResult NaN!!! poct_node->value=%lg volume=%lg   x y z = %g %g %g\n",
                    poct_node->value, oct_node_volume, poct_node->center.x, poct_node->center.y, poct_node->center.z);
        }
    }

    return (is_global_best);

}

/** free oct-tree memory
 */
void freeOctTreeStructures(ResultTreeNode* pResultTreeRoot, Tree3D * pOctTree) {

    int freeDataPointer = 1;
    freeResultTree(pResultTreeRoot);
    pResultTreeRoot = NULL;
    freeTree3D(pOctTree, freeDataPointer);
    pOctTree = NULL;
}

/**
 * Set weights based on distribution of a set of points on the sphere
 *
 * If many points and well distributed, then mean distance is ~90 deg
 * So set weight to 1.0 if mean distance >= 90deg, otherwise linear decrease from 1.0 to 0.0 with distance 90 to 0deg
 *
 */
double* setWeightsOnSphereX(double* lat, double* lon, int size) {

    if (size < 1) {
        return (NULL);
    }

    double* weights = calloc(size, sizeof (double));

    if (size == 1) {
        weights[0] = 1.0;
        return (weights);
    }

    // set weights
    int n, m;
    for (n = 0; n < size; n++) {
        double dsum = 0.0;
        for (m = 0; m < size; m++) {
            if (m != n) {
                dsum += GCDistance_stats(lat[n], lon[n], lat[m], lon[m]);
            }
        }
        double dmean = dsum / (double) (size - 1);
        weights[n] = dmean >= 90.0 ? 1.0 : dmean / 90.0;
    }

    return (weights);

}

/**
 * Set weights based on distribution of a set of points on the sphere
 *
 * Weight of each point is based on weighted count of other points within spherical cap around point,
 *    where area of spherical cap is (surface area unit sphere) / (number of points)
 *
 */
double* setWeightsOnSphere(double* lat, double* lon, int npoints) {

    if (npoints < 1) {
        return (NULL);
    }

    double* weights = calloc(npoints, sizeof (double));

    // critical distance is angular radius of spherical cap with surface (surface area 1/2 unit sphere) / npoints = 2*PI/npoints
    //    2*PI/npoints = 2*PI(1-cos(dist_critical))
    //    dist_critical = acos(1-1/npoints)
    double dist_critical = RA2DE * acos(1.0 - (1.0 / (double) npoints));

    // set weights
    int n, m;
    for (n = 0; n < npoints; n++) {
        double wtsum = 1.0; // self counts as 1
        for (m = 0; m < npoints; m++) {
            if (m != n) { // skip self
                double dist = GCDistance_stats(lat[n], lon[n], lat[m], lon[m]);
                //if (dist < dist_critical) {
                // contribution of each distance scales with ~ surface area for dist relative to area for critical distance
                //wtsum += (1.0 - pow(dist / dist_critical, 2));
                //}
                // contribution of each distance scales with dist relative to critical distance
                wtsum += exp(-3.0 * dist / dist_critical);
                //wtsum += exp(-(dist / dist_critical) * (dist / dist_critical));
            }
        }
        weights[n] = 1.0 / wtsum;
    }

    return (weights);

}


/***************************************************************************
 * oct-tree global search for optimal fault plane solutions
 *
 * find strike (0,360), dip(0,90) and rake(0,180)
 *
 * Returns 0 on success and -1 otherwise.
 ***************************************************************************/

#define NUM_SCATTER_SAMPLE 2000

double octtreeFaultPlaneInversion(
        Pick *ppick_list, int num_picks,
        int max_num_nodes, double min_node_size_deg, double initial_grid_step,
        int min_number_observations, int critical_number_observations, int use_first_motion_polarities, int use_amplitudes,
        FaultPlaneSolution *pbest_fault_plane_solution,
        float **pscatter_sample, int *p_n_alloc_scatter_sample, int *pn_scatter_sample,
        int verbose
        ) {

    // check number observations
    if (num_picks < min_number_observations) {
        return (-1);
    }

    double strike_step = initial_grid_step;
    //double strike_min = -180.0;
    double strike_min = STRIKE_MIN; // only need half range to prevent duplicate search over fault plane and auxiliary plane
    double strike_max = STRIKE_MAX;
    double dip_step = initial_grid_step;
    double dip_min = DIP_MIN;
    double dip_max = DIP_MAX;
    double rake_step = initial_grid_step;
    double rake_min = RAKE_MIN;
    double rake_max = RAKE_MAX;

    int num_strike = (int) ((strike_max - strike_min + strike_step * 0.001) / strike_step);
    int num_dip = (int) ((dip_max - dip_min + dip_step * 0.001) / dip_step);
    int num_rake = (int) ((rake_max - rake_min + rake_step * 0.001) / rake_step);

    // set up oct-tree x, y, z grid
    Tree3D *pOctTree;
    double integral = 0.0;
    void *pdata = NULL;
    // strike -> x
    // dip -> y
    // rake -> z
    pOctTree = newTree3D(OCTTREE_TYPE,
            num_strike, num_dip, num_rake,
            strike_min, dip_min, rake_min,
            strike_step, dip_step, rake_step, OCTTREE_UNDEF_VALUE, integral, pdata);

    // initialize memory for scatter sample
    if (*pscatter_sample != NULL && (*p_n_alloc_scatter_sample < NUM_SCATTER_SAMPLE)) {
        free(*pscatter_sample);
        *pscatter_sample = NULL;
    }
    if (*pscatter_sample == NULL) {
        *pscatter_sample = (float*) calloc(4 * NUM_SCATTER_SAMPLE, sizeof (float));
        *p_n_alloc_scatter_sample = NUM_SCATTER_SAMPLE;
    }
    *pn_scatter_sample = 0;

    // initialize memory for value and weight arrays
#ifdef LINEAR_REGRESSION   // linear regression
    if (x_value_array != NULL && (n_alloc_x_value_array < num_picks)) {
        free(x_value_array);
        x_value_array = NULL;
    }
    if (x_value_array == NULL) {
        x_value_array = (double*) calloc(num_picks, sizeof (double));
        n_alloc_x_value_array = num_picks;
    }
    if (y_value_array != NULL && (n_alloc_y_value_array < num_picks)) {
        free(y_value_array);
        y_value_array = NULL;
    }
    if (y_value_array == NULL) {
        y_value_array = (double*) calloc(num_picks, sizeof (double));
        n_alloc_y_value_array = num_picks;
    }
#else
    if (value_array != NULL && (n_alloc_value_array < num_picks)) {
        free(value_array);
        value_array = NULL;
    }
    if (value_array == NULL) {
        value_array = (double*) calloc(num_picks, sizeof (double));
        n_alloc_value_array = num_picks;
    }
#endif
    if (weight_array != NULL && (n_alloc_weight_array < num_picks)) {
        free(weight_array);
        weight_array = NULL;
    }
    if (weight_array == NULL) {
        weight_array = (double*) calloc(num_picks, sizeof (double));
        n_alloc_weight_array = num_picks;
    }

    // set station distribution weights
    int n_use = 0;
    Pick *ppick;
    int npick;
    for (npick = 0; npick < num_picks; npick++) {
        ppick = ppick_list + npick;
        ppick->take_off_angle_distrib_weight = -1.0;
        if ((use_first_motion_polarities && ppick->fmpolarity)
                ||
                (use_amplitudes && ppick->fmpolarity
                && ppick->amplitude > FLT_MIN // negative amplitude flags that channel gain is not available
                && strncmp(ppick->phase, "PKP", 3) != 0 // PKP not valid for amplitude analysis
                )
                ) {
            x_value_array[n_use] = ppick->take_off_angle_az;
            y_value_array[n_use] = 90.0 - ppick->take_off_angle_inc; // convert to pseudo-latitude
            ppick->take_off_angle_distrib_weight = 1.0; // flag pick as to be used in inversion
            n_use++;
        }
    }
    // 20151104 AJL  double* weights = setDistributionWeights(y_value_array, x_value_array, n_use);
    double* weights = setWeightsOnSphere(y_value_array, x_value_array, n_use);
    // normalize weights so max is 1.0
    double wmax = 0.0;
    int n;
    for (n = 0; n < n_use; n++) {
        if (weights[n] > wmax) {
            wmax = weights[n];
        }
    }
    if (wmax > FLT_MIN) {
        for (n = 0; n < n_use; n++) {
            weights[n] /= wmax;
        }
    }
    // set weights to used picks
    int n_set = 0;
    for (npick = 0; npick < num_picks; npick++) {
        ppick = ppick_list + npick;
        if (ppick->take_off_angle_distrib_weight >= 0.0) { // flags pick to be used in inversion
            // 20160331 AJL - TEST  ppick->take_off_angle_distrib_weight = weights[n_set];
            ppick->take_off_angle_distrib_weight = weights[n_set] * weights[n_set]; // 20160331 AJL - TEST: appears to improve results
            n_set++;
        }
        //printf("%s take_off_angle_dist_weight %f, az %f, inc %f\n", ppick->station, ppick->take_off_angle_dist_weight, ppick->take_off_angle_az, ppick->take_off_angle_inc);
    }
    free(weights);

    // get measure of uniformity of distribution of observation on sphere
    if (obs_take_off_angles != NULL && (n_obs_take_off_angle < num_picks)) {
        free(obs_take_off_angles);
        obs_take_off_angles = NULL;
    }
    if (obs_take_off_angles == NULL) {
        obs_take_off_angles = (double**) calloc((unsigned int) num_picks, sizeof (double*));
        int i;
        for (i = 0; i < num_picks; i++) {
            obs_take_off_angles[i] = (double*) calloc(num_picks, 2 * sizeof (double));
        }
        n_obs_take_off_angle = num_picks;
    }
    int n_used = 0;
    for (npick = 0; npick < num_picks; npick++) {
        ppick = ppick_list + npick;
        if (ppick->take_off_angle_distrib_weight >= 0.0) { // flags pick to be used in inversion
            obs_take_off_angles[n_used][0] = ppick->take_off_angle_az; // azimuth on sphere (0-360))
            obs_take_off_angles[n_used][1] = ppick->take_off_angle_inc - 90.0; // inclination on sphere (-90 -> 90)
            //obs_take_off_angles[n_used][0] = 34; // azimuth on sphere (0-360))
            //obs_take_off_angles[n_used][1] = 34; // inclination on sphere (-90 -> 90)
            n_used++;
        }
        //printf("DEBUG: %s measureOfDistributionOnSphere %f, az %f, inc %f\n", ppick->station, ppick->take_off_angle_dist_weight, ppick->take_off_angle_az, ppick->take_off_angle_inc);
    }
    //n_used = 2;
    //measureOfDistributionOnSphere = 1.0 - getUniformityOfDistributionOnSphere(obs_take_off_angles, n_used);
    measureOfDistributionOnSphere = fractionFocalSphereCovered(obs_take_off_angles, n_used);
    //printf("DEBUG: measureOfDistributionOnSphere measureOfDistributionOnSphere %f, n_used %d\n", measureOfDistributionOnSphere, n_used);

    // initialize global best solution variables
    OctNode* pglobal_best_oct_node = NULL;
    int is_global_best = 0;
    pbest_fault_plane_solution->probability = -1.0;



    ResultTreeNode* pResultTreeRoot = NULL; // Oct-tree likelihood*volume results tree root node
    OctNode* poct_node = NULL;


    int nSamples = 0;
    double scatter_sample_value_max = -FLT_MAX;

    double smallest_node_size_x = DBL_MAX;
    double smallest_node_size_y = DBL_MAX;
    double smallest_node_size_z = DBL_MAX;

    long info_nsamples_start_time = clock();

    // set min node size (in km)
    double min_node_size_x;
    double min_node_size_y;
    double min_node_size_z;
    min_node_size_x = min_node_size_y = min_node_size_z = min_node_size_deg;

    // Stage 1: evaluate each cell in Tree3D =================================================================


    int idip, istrike, irake;
    poct_node = pOctTree->nodeArray[0][0][0];
    double oct_node_volume = poct_node->ds.x * poct_node->ds.y * poct_node->ds.z;
    // save node size
    smallest_node_size_x = poct_node->ds.x;
    smallest_node_size_y = poct_node->ds.y;
    smallest_node_size_z = poct_node->ds.z;


    for (istrike = 0; istrike < pOctTree->numx; istrike++) {
        for (idip = 0; idip < pOctTree->numy; idip++) {
            for (irake = 0; irake < pOctTree->numz; irake++) {

                poct_node = pOctTree->nodeArray[istrike][idip][irake];
                //if (poct_node == NULL) // case of Tree3D_spherical
                //    continue;

                // $$$ NOTE: this block must be identical to block $$$ below
                is_global_best = octtree_core(&pResultTreeRoot, poct_node, oct_node_volume,
                        ppick_list, num_picks, min_number_observations, critical_number_observations, use_first_motion_polarities, use_amplitudes,
                        pbest_fault_plane_solution
                        );
                // END - this block must be identical to block $$$ below

                // if best node so far
                if (is_global_best) {
                    // save node
                    pglobal_best_oct_node = poct_node;
                }


                nSamples++;

                // save indicator of solution probability
                if (0) {
                    double value = 1.0; // size
                    //value = pow(value, min_weight_sum_assoc);
                    int index = *pn_scatter_sample * 4;
                    (*pscatter_sample)[index++] = poct_node->center.x;
                    (*pscatter_sample)[index++] = poct_node->center.y;
                    (*pscatter_sample)[index++] = poct_node->center.z;
                    (*pscatter_sample)[index++] = value;
                    (*pn_scatter_sample)++;
                    if (value > scatter_sample_value_max)
                        scatter_sample_value_max = value;
                    //}
                }

            }

        }
    }

    if (verbose >= 2) {
        printf("Info: Completed initial grid search: node size: ds=%g,   nSamples=%d/%d, dt=%.3fs\n",
                poct_node->ds.x, nSamples, max_num_nodes, (double) (clock() - info_nsamples_start_time) / CLOCKS_PER_SEC);
    }
    info_nsamples_start_time = clock();

    if (0) {
        int n_scatter_samples_initial = *pn_scatter_sample;
        // normalize initial scatter sample
        if (scatter_sample_value_max > FLT_MIN) {
            int index;
            for (index = 0; index < n_scatter_samples_initial; index++) {
                (*pscatter_sample)[4 * index + 3] /= scatter_sample_value_max * 10.0;
            }

        }
    }




    // Stage 2: loop over oct-tree nodes =================================================================

    //nScatterSaved = 0;
    //ipos = 0;

    int n_neigh;
    OctNode* neighbor_node;
    //Vect3D smallest_node_size_best;
    Vect3D coords;
    //smallest_node_size_best.x = FLT_MAX;
    //smallest_node_size_best.y = FLT_MAX;
    //smallest_node_size_best.z = FLT_MAX;

    int stop_on_min_node_size = 0;

    ResultTreeNode* presult_node;
    OctNode* pparent_oct_node;

    int ix, iy, iz;
    int nneighbor_tested = 0;
    int noutside = 0;
    int nsubdivided = 0;

    while (nSamples < max_num_nodes) {

        if (stop_on_min_node_size) {
            presult_node = getHighestLeafValue(pResultTreeRoot);
            if (presult_node == NULL) {
                printf("Info: No nodes with finite probability, terminating OctTree search.\n");
                break;
            }
        } else {
            presult_node = getHighestLeafValueMinSize(pResultTreeRoot, 0.0, min_node_size_deg, 0.0);
            // check if null node
            if (presult_node == NULL) {
                printf("Info: No nodes with finite probability or no more nodes larger than min_node_size, terminating OctTree search.\n");
                break;
            }
        }
        /*printf("DEBUG: OctTree getHighestLeafValueOfSpecifiedSize ==> nsamples=%d/%d  lat %g lon %g z %g  dlat/lon/z %g/%g/%g\n",
                nSamples, max_num_nodes, presult_node->pnode->center.y, presult_node->pnode->center.x, presult_node->pnode->center.z,
                presult_node->pnode->ds.y, presult_node->pnode->ds.x, presult_node->pnode->ds.z);
        fflush(stdout); */
        //printf("===================== HighestLeafValue value=%lg volume=%lg\n", presult_node->value, presult_node->volume);


        //if (nSamples % 100 == 0)
        //fprintf(stderr, "%d getHighestLeafValue %lf\n", nSamples, presult_node->value);

        if (presult_node == NULL)
            printf("Info: \npresult_node == NULL!!\n");

        pparent_oct_node = presult_node->pnode;

        // subdivide node and evaluate solution at each child
        //subdivide(pparent_oct_node, OCTTREE_UNDEF_VALUE, NULL);

        // subdivide all HighestLeafValue neighbors

        int n_neigh_max = 7;
        for (n_neigh = 0; n_neigh < n_neigh_max; n_neigh++) {

            if (n_neigh == 0) {
                neighbor_node = pparent_oct_node;
            } else {
                coords.x = pparent_oct_node->center.x;
                coords.y = pparent_oct_node->center.y;
                coords.z = pparent_oct_node->center.z;
                if (n_neigh == 1) {
                    coords.x = pparent_oct_node->center.x
                            + (pparent_oct_node->ds.x + smallest_node_size_x) / 2.0;
                } else if (n_neigh == 2) {
                    coords.x = pparent_oct_node->center.x
                            - (pparent_oct_node->ds.x + smallest_node_size_x) / 2.0;
                } else if (n_neigh == 3) {
                    coords.y = pparent_oct_node->center.y
                            + (pparent_oct_node->ds.y + smallest_node_size_y) / 2.0;
                } else if (n_neigh == 4) {
                    coords.y = pparent_oct_node->center.y
                            - (pparent_oct_node->ds.y + smallest_node_size_y) / 2.0;
                } else if (n_neigh == 5) {
                    coords.z = pparent_oct_node->center.z
                            + (pparent_oct_node->ds.z + smallest_node_size_z) / 2.0;
                } else if (n_neigh == 6) {
                    coords.z = pparent_oct_node->center.z
                            - (pparent_oct_node->ds.z + smallest_node_size_z) / 2.0;
                }
                //printf("getLeafNodeContaining: %.1f,%.1f,%.1f\n", coords.x, coords.y, coords.z);
                // strike -> x
                // dip -> y
                // rake -> z
                // check for rake wrap-around
                if (coords.z > RAKE_MAX)
                    coords.z -= RAKE_MAX - RAKE_MIN;
                if (coords.z <= RAKE_MIN)
                    coords.z += RAKE_MAX - RAKE_MIN;
                // check for strike wrap-around
                if (coords.x >= STRIKE_MAX)
                    coords.x -= STRIKE_MAX - STRIKE_MIN;
                if (coords.x < STRIKE_MIN)
                    coords.x += STRIKE_MAX - STRIKE_MIN;

                // find neighbor node
                neighbor_node = getLeafNodeContaining(pOctTree, coords);
                //printf(" -> %.1f,%.1f,%.1f", coords.x, coords.y, coords.z);
                nneighbor_tested++;
                // outside of octTree volume
                if (neighbor_node == NULL) {
                    //printf(" -> outside\n");
                    noutside++;
                    continue;
                }
                // already subdivided
                if (neighbor_node->ds.z < 0.99 * pparent_oct_node->ds.z) {
                    //printf(" -> subdivided\n");
                    nsubdivided++;
                    continue;
                }
                //printf(" -> OK\n");
            }


            // subdivide node and evaluate solution at each child
            subdivide(neighbor_node, OCTTREE_UNDEF_VALUE, NULL);

            for (ix = 0; ix < 2; ix++) {
                for (iy = 0; iy < 2; iy++) {
                    for (iz = 0; iz < 2; iz++) {

                        poct_node = neighbor_node->child[ix][iy][iz];

                        //if (poct_node->ds.x < min_node_size || poct_node->ds.y < min_node_size || poct_node->ds.z < min_node_size)
                        //fprintf(stderr, "\nnode size too small!! %lf %lf %lf\n", poct_node->ds.x, poct_node->ds.y, poct_node->ds.z);

                        // save node size if smallest so far
                        if (poct_node->ds.x < smallest_node_size_x)
                            smallest_node_size_x = poct_node->ds.x;
                        if (poct_node->ds.y < smallest_node_size_y)
                            smallest_node_size_y = poct_node->ds.y;
                        if (poct_node->ds.z < smallest_node_size_z)
                            smallest_node_size_z = poct_node->ds.z;

                        oct_node_volume = poct_node->ds.x * poct_node->ds.y * poct_node->ds.z;

                        // $$$ NOTE: this block must be identical to block $$$ above
                        is_global_best = octtree_core(&pResultTreeRoot, poct_node, oct_node_volume,
                                ppick_list, num_picks, min_number_observations, critical_number_observations, use_first_motion_polarities, use_amplitudes,
                                pbest_fault_plane_solution
                                );
                        // END - this block must be identical to block $$$ above


                        // if best node so far
                        if (is_global_best) {
                            // save node
                            pglobal_best_oct_node = poct_node;
                        }

                        //
                        /*if (is_global_best || nSamples % 1000 == 0) {
                            printf("DEBUG: OctTree SUBDIVIDE: nsamples=%d/%d  lat %g lon %g z %g  dlat/lon/z %g/%g/%g  nassociated_P %d  nassociated %d  weight_sum %g  prob %g  ot_var %g  crit_size_km %g\n",
                                    nSamples, max_num_nodes, poct_node->center.y, poct_node->center.x, poct_node->center.z,
                                    dsy_global, dsx_global, poct_node->ds.z,
                                    best_nassociated_P, best_nassociated, best_weight_sum, best_prob, best_ot_variance, current_critical_node_size_km);
                            fflush(stdout);
                        }*/
                        nSamples++;

                    } // end triple loop over node children
                }
            }
        } // end loop over HighestLeafValue neighbors

        // check if minimum node size reached
        if (stop_on_min_node_size && (smallest_node_size_x < min_node_size_x
                || smallest_node_size_y < min_node_size_y
                || smallest_node_size_z < min_node_size_z)) {
            if (verbose >= 1)
                fprintf(stdout, "\nINFO: Min node size (%f) reached (%f), terminating Octree search.\n", min_node_size_x, smallest_node_size_x);
            break;
        }

    } // end while (nSamples < max_num_nodes)

    pbest_fault_plane_solution->nsamples = nSamples;

    if (DEBUG) printf("\nDEBUG: nSamples %d, nneighbor_tested %d -> noutside %d + nsubdivided %d = %d\n", nSamples, nneighbor_tested, noutside, nsubdivided, noutside + nsubdivided);


    // Stage 3: get inversion statistics and results =================================================================

    if (pbest_fault_plane_solution->probability > FLT_MIN) {

        // re-seed random number generator so that results will be identical to previous if no data changed.
        int RandomNumSeed = 9589;
        SRAND_FUNC(RandomNumSeed);

        *pn_scatter_sample = 0;
        int npoints = 0;
        int num_scatter = *p_n_alloc_scatter_sample;
        double oct_tree_scatter_volume = 0.0;
        double oct_node_value_ref = pglobal_best_oct_node->value;
        // determine integral of all oct-tree leaf node pdf values
        double oct_tree_integral = integrateResultTree(pResultTreeRoot, VALUE_IS_LOG_PROB_DENSITY_IN_NODE, 0.0, oct_node_value_ref);
        //printf("DEBUG: level_stats_min %d  level_stats_max %d  oct_tree_integral %g \n", level_stats_min, level_stats_max, oct_tree_integral);
        // get scatter sample for location statistics
        int tot_npoints = 0;
        int fdata_index = 0;
        npoints = getScatterSampleResultTree(pResultTreeRoot, VALUE_IS_LOG_PROB_DENSITY_IN_NODE, num_scatter, oct_tree_integral,
                *pscatter_sample, tot_npoints, &fdata_index, oct_node_value_ref, &oct_tree_scatter_volume);
        *pn_scatter_sample = npoints;
        if (verbose >= 2) {
            printf("Info: Finished generating event scatter samples.   dt=%.3fs\n", (double) (clock() - info_nsamples_start_time) / CLOCKS_PER_SEC);
        }
        info_nsamples_start_time = clock();
        pbest_fault_plane_solution->oct_tree_scatter_volume = oct_tree_scatter_volume;

        // P and T axes statistics
        double mean_dist_P, mean_dist_T;
        double mean_strike, mean_dip, mean_rake;
        /*double ref_strike = pbest_fault_plane_solution->bestNP.strike;
        double ref_dip = pbest_fault_plane_solution->bestNP.dip;
        double ref_rake = pbest_fault_plane_solution->bestNP.rake;
        CalcMeanDistancePTandMeanMechanism(*pscatter_sample, npoints, &mean_dist_P, &mean_dist_T,
                ref_strike, ref_dip, ref_rake, &mean_strike, &mean_dip, &mean_rake);
         */
        struct AXIS min_dist_P_axis, min_dist_T_axis, min_dist_N_axis;
        CalcMeanDistancePTandMinDistMechanism(*pscatter_sample, npoints, &mean_dist_P, &mean_dist_T, &mean_strike, &mean_dip, &mean_rake,
                &min_dist_P_axis, &min_dist_T_axis, &min_dist_N_axis);
        pbest_fault_plane_solution->mean_dist_P = mean_dist_P;
        pbest_fault_plane_solution->mean_dist_T = mean_dist_T;
        pbest_fault_plane_solution->meanNP.strike = mean_strike;
        pbest_fault_plane_solution->meanNP.dip = mean_dip;
        pbest_fault_plane_solution->meanNP.rake = mean_rake;
        pbest_fault_plane_solution->meanPaxis = min_dist_P_axis;
        pbest_fault_plane_solution->meanTaxis = min_dist_T_axis;
        pbest_fault_plane_solution->meanNaxis = min_dist_N_axis;

        // 20140220 AJL - location statistics not fully valid for topology of strike/dip/rake space!
#if 1
        static Vect3D expect_NULL = {0.0, 0.0, 0.0};
        static Mtrx3D cov_NULL = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        Vect3D expect = expect_NULL;
        expect = CalcExpectationSamplesFM(*pscatter_sample, npoints,
                pbest_fault_plane_solution->bestNP.strike, pbest_fault_plane_solution->bestNP.dip, pbest_fault_plane_solution->bestNP.rake);
        pbest_fault_plane_solution->expect = expect;
        Mtrx3D cov = cov_NULL;
        cov = CalcCovarianceSamplesFM(*pscatter_sample, npoints, &expect);
        pbest_fault_plane_solution->cov = cov;
        pbest_fault_plane_solution->err_strike = sqrt(DELTA_CHI_SQR_68_3 * cov.xx);
        pbest_fault_plane_solution->err_dip = sqrt(DELTA_CHI_SQR_68_3 * cov.yy);
        pbest_fault_plane_solution->err_rake = sqrt(DELTA_CHI_SQR_68_3 * cov.zz);
        if (verbose >= 2) {
            fprintf(stdout, "Info: Statistics:");
            fprintf(stdout, "  ExpectX %lg Y %lg Z %lg", expect.x, expect.y, expect.z);
            fprintf(stdout, "  CovXX %lg XY %lg XZ %lg", cov.xx, cov.xy, cov.xz);
            fprintf(stdout, " YY %lg YZ %lg", cov.yy, cov.yz);
            fprintf(stdout, " ZZ %lg", cov.zz);
            fprintf(stdout, "\n");
        }
#endif
#if 0
        static Vect3D expect_NULL = {0.0, 0.0, 0.0};
        static Mtrx3D cov_NULL = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        static Ellipsoid3D ellipsoid_NULL = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        //static Ellipse2D ellipse_NULL = {0.0, 0.0, 0.0};
        Vect3D expect = expect_NULL;
        Mtrx3D cov = cov_NULL;
        Ellipsoid3D ellipsoid = ellipsoid_NULL;
        //Ellipse2D ellipse = ellipse_NULL;
        // calculate location statistics
        expect = CalcExpectationSamplesGlobal(*pscatter_sample, npoints, best_strike);
        //cov = CalcCovarianceSamplesGlobal(*pscatter_sample, npoints, &expect);
        int isGeographic = 0;
        cov = CalcCovarianceSamplesWrappedX(*pscatter_sample, npoints, &expect, isGeographic);
        ellipsoid = CalcErrorEllipsoid(&cov, DELTA_CHI_SQR_68_3);
        //ellipse = CalcHorizontalErrorEllipse(&cov, DELTA_CHI_SQR_68_2);
        *pn_scatter_sample = npoints;
        printf("Info: Finished generating event scatter samples.   dt=%.3fs\n", (double) (clock() - info_nsamples_start_time) / CLOCKS_PER_SEC);
        info_nsamples_start_time = clock();
        fprintf(stdout,
                "Info: Statistics:");
        fprintf(stdout,
                "  ExpectX %lg Y %lg Z %lg",
                expect.x, expect.y, expect.z);
        fprintf(stdout, "  CovXX %lg XY %lg XZ %lg",
                cov.xx, cov.xy, cov.xz);
        fprintf(stdout, " YY %lg YZ %lg",
                cov.yy, cov.yz);
        fprintf(stdout, " ZZ %lg",
                cov.zz);
        fprintf(stdout, " EllAz1  %lg Dip1  %lg Len1  %lg",
                ellipsoid.az1, ellipsoid.dip1,
                ellipsoid.len1);
        fprintf(stdout, " Az2  %lg Dip2  %lg Len2  %lg",
                ellipsoid.az2, ellipsoid.dip2,
                ellipsoid.len2);
        fprintf(stdout, " Len3  %le\n", ellipsoid.len3);

        /*  20110617 AJL - Bug fix?  TODO: the constant should be DELTA_CHI_SQR_68_3 for 3-D ???
        double errx = sqrt(DELTA_CHI_SQR_68_2 * cov.xx);
        double erry = sqrt(DELTA_CHI_SQR_68_2 * cov.yy);
        double errz = sqrt(DELTA_CHI_SQR_68_2 * cov.zz);
         */
        double err_strike = sqrt(DELTA_CHI_SQR_68_3 * cov.xx);
        double err_dip = sqrt(DELTA_CHI_SQR_68_3 * cov.yy);
        double err_rake = sqrt(DELTA_CHI_SQR_68_3 * cov.zz);

        /*
        double azMaxHorUnc = ellipse.az1 + 90.0;
        if (azMaxHorUnc >= 360.0)
            azMaxHorUnc -= 360.0;
        if (azMaxHorUnc >= 180.0)
            azMaxHorUnc -= 180.0;
        fprintf(stdout,
                "  OriginUncertainty (68%%)  latUnc %lg  lonUnc %lg  minHorUnc %lg  maxHorUnc %lg  azMaxHorUnc %lg  vertUnc %g\n",
                erry, errx, ellipse.len1, ellipse.len2, azMaxHorUnc, errz
                );
         */

        fprintf(stdout,
                "  FaultPlaneUncertainty (68%%)  err_strike %lg  err_dip %lg  err_rake %lg\n",
                err_strike, err_dip, err_rake
                );

        pbest_fault_plane_solution->err_strike = err_strike;
        pbest_fault_plane_solution->err_dip = err_dip;
        pbest_fault_plane_solution->err_rake = err_rake;
        pbest_fault_plane_solution->cov = cov;
        pbest_fault_plane_solution->ellipsoid = ellipsoid;
        //pbest_fault_plane_solution->ellipse = ellipse;
#endif

    }

    // clean up

    // free oct-tree structures - IMPORTANT!
    freeOctTreeStructures(pResultTreeRoot, pOctTree);

    return (pbest_fault_plane_solution->probability);

}