/***************************************************************************
 * fmamp_utils.h
 *
 * Utility functions for fault plane analysis.
 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 * created: 2014.02.19
 ***************************************************************************/

//#include "meca.h"

void strikeRakeDip2faultNormSlip(double *pstrike, double *pdip, double *prake, double *fnorm, double *slip, int idir);
void CalcMeanDistancePT(float* fdata, int nSamples, double *pmean_dist_P, double *pmean_dist_T);
double getMeanDistance(struct AXIS *axis, int naxes, int check_upper_and_lower);
double getUniformityOfDistributionOnSphere(double *points_on_sphere[2], int npoints);
double fractionFocalSphereCovered(double *points_on_sphere[2], int npoints);

void CalcMeanDistancePTandMinDistMechanism(
        float* fdata, int nSamples,
        double *pdistance_mean_min_P, double *pdistance_mean_min_T,
        double *pmin_dist_strike, double *pmin_dist_dip, double *pmin_dist_rake,
        struct AXIS *pmin_dist_P_axis, struct AXIS *pmin_dist_T_axis, struct AXIS *pmin_dist_N_axis);
void CalcMeanDistancePTandMeanMechanism(float* fdata, int nSamples, double *pmean_dist_P, double *pmean_dist_T,
        double ref_strike, double ref_dip, double ref_rake, double *pmean_strike, double *pmean_dip, double *pmean_rake);

Vect3D CalcExpectationSamplesFM(float* fdata, int nSamples, double xReference, double yReference, double zReference);
Mtrx3D CalcCovarianceSamplesFM(float* fdata, int nSamples, Vect3D * pexpect);
double calc_rad(double mech_phi, double mech_del, double mech_lam, double ray_vert_ang, double ray_horiz_ang, char orig_wave);
double WalshEtAl2009_polarity_probability(double inconsistent_polarity_rate, double amplitude_noise_rate, double rad_amp);
double WalshEtAl2009_polarity_probability_modif(double inconsistent_polarity_rate, double weight);

