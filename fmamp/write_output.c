/***************************************************************************
 * write_output.c
 *
 * Input function for different input formats.
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
#include "fmamp_utils.h"

#define NUM_ACCEPTABLE_TO_WRITE 150
// forward function declarations

/**
 * write summary file entry
 *
 * 20160920 AJL - added to support export to NLL / SeismicityViewer
 */
int write_sum_entry(FILE* sumStream, Hypocenter *phypocenter, FaultPlaneSolution *pfault_plane_solution, NodalPlane *pprefferedNP,
        char *variant_name, int nobs_used, double misfit, double sum_misfit_weight, char quality_code, int iwrite_sum_header) {

    if (iwrite_sum_header) {
        fprintf(sumStream,
                "event_unique_id year month day hour min dec_sec rms lat lon errh depth errz "
                "nassoc_P dist_min dist_max gap_primary gap_secondary ampAttenPower magnitude mag_type "
                "variant_name strike dip rake nreadings nobs_used "
                "stat_dist_ratio misfit sum_misfit_weight mean_dist_P mean_dist_T quality"
                "\n");
    }

    // skip solutions with no obs
    if (nobs_used < 1) {
        return(-1);
    }
    // event information
    fprintf(sumStream, "%ld ", phypocenter->unique_id);
    fprintf(sumStream, "%4.2d %2.2d %2.2d %2.2d %2.2d %8.4f %f ",
            phypocenter->year, phypocenter->month, phypocenter->day, phypocenter->hour, phypocenter->min, phypocenter->dec_sec, phypocenter->rms);
    fprintf(sumStream, "%f %f %f %f %f ", phypocenter->lat, phypocenter->lon, phypocenter->errh, phypocenter->depth, phypocenter->errz);
    fprintf(sumStream, "%d %f %f %f %f ", phypocenter->nassoc_P, phypocenter->dist_min, phypocenter->dist_max, phypocenter->gap_primary, phypocenter->gap_secondary);
    fprintf(sumStream, "%f %f %s ", phypocenter->ampAttenPower, phypocenter->magnitude, phypocenter->mag_type);
    // mechanism information
    fprintf(sumStream, "%s ", variant_name);
    fprintf(sumStream, "%f %f %f ", pprefferedNP->strike, pprefferedNP->dip, pprefferedNP->rake);
    fprintf(sumStream, "%d %d ", pfault_plane_solution->nreadings, nobs_used);
    fprintf(sumStream, "%f ", pfault_plane_solution->stat_dist_ratio);
    fprintf(sumStream, "%f %f ", misfit, sum_misfit_weight);
    fprintf(sumStream, "%f %f ", pfault_plane_solution->mean_dist_P, pfault_plane_solution->mean_dist_T);
    fprintf(sumStream, "%c ", quality_code);
    fprintf(sumStream, "\n");

    return(0);

}

/** write output files:
 * @param outpath
 * @param variant_name
 * @param phypocenter
 * @param min_number_observations
 * @param critical_number_observations
 * @param use_first_motion_polarities
 * @param use_amplitudes
 * @param best_prob
 * @param pfault_plane_solution
 * @param pscatter_sample
 * @param n_scatter_sample
 * @param iwrite_sum_only
 * @param sumStream
 * @param verbose
 * @return 0 on success and -1 otherwise.
 */
int write_output(char *outpath, char *variant_name, Hypocenter *phypocenter,
        int min_number_observations, int critical_number_observations, int use_first_motion_polarities, int use_amplitudes,
        double best_prob, FaultPlaneSolution *pfault_plane_solution, float *pscatter_sample, int n_scatter_sample,
        int iwrite_sum_only, FILE* sumStream, int iwrite_sum_header, int verbose) {

    char outfile[STANDARD_STRLEN];
    FILE *fp_out;
    Pick *ppick;
    int npick, nscatter;
    double strike, dip, rake;
    float *sample;

    //NodalPlane *pprefferedNP = &pfault_plane_solution->bestNP;
    NodalPlane *pprefferedNP = &pfault_plane_solution->meanNP;

    int nobs_used = 0;
    double misfit = -1.0;
    double sum_misfit_weight = -1.0;
    if (use_first_motion_polarities && pfault_plane_solution->nobs_used_polarity > 0) {
        nobs_used = pfault_plane_solution->nobs_used_polarity;
        misfit = pfault_plane_solution->polarity_misfit;
        sum_misfit_weight = pfault_plane_solution->sum_polarity_misfit_weight;
    }
    if (use_amplitudes && pfault_plane_solution->nobs_used_polarity < 1 && pfault_plane_solution->nobs_used_amplitude > 0) {
        nobs_used = pfault_plane_solution->nobs_used_amplitude;
        misfit = pfault_plane_solution->amplitude_misfit;
        sum_misfit_weight = pfault_plane_solution->sum_amplitude_misfit_weight;
    }

    if (DEBUG) printf("Event: %ld\n", phypocenter->unique_id);
    if (DEBUG) printf("DEBUG: best_prob=%g, nobs=%d, nused=%d, pmf/w/p=%f/%f/%g, amf/w/p=%f/%f/%g, strike/dip/rake=%f/%f/%f, unc=%f/%f/%f\n", best_prob,
            pfault_plane_solution->nreadings, nobs_used,
            pfault_plane_solution->polarity_misfit, pfault_plane_solution->sum_polarity_misfit_weight, pfault_plane_solution->polarity_probability,
            pfault_plane_solution->amplitude_misfit, pfault_plane_solution->sum_amplitude_misfit_weight, pfault_plane_solution->amplitude_probability,
            pprefferedNP->strike, pprefferedNP->dip, pprefferedNP->rake,
            pfault_plane_solution->err_strike, pfault_plane_solution->err_dip, pfault_plane_solution->err_rake);

    // set second nodal plane
    strike = pprefferedNP->strike;
    dip = pprefferedNP->dip;
    rake = pprefferedNP->rake;
    double fp1_norm[3], fp1_slip[3];
    strikeRakeDip2faultNormSlip(&strike, &dip, &rake, fp1_norm, fp1_slip, 1);
    double strike_np2, dip_np2, rake_np2;
    strikeRakeDip2faultNormSlip(&strike_np2, &dip_np2, &rake_np2, fp1_slip, fp1_norm, -1);
    if (DEBUG) printf("DEBUG:                np2: strike/dip/rake=%f/%f/%f\n", strike_np2, dip_np2, rake_np2);

    // set solution quality
    double fp_error = pow(pfault_plane_solution->oct_tree_scatter_volume, 1.0 / 3.0);
    //double pt_axes_error = (pfault_plane_solution->mean_dist_P + pfault_plane_solution->mean_dist_T) / 2.0;
    // use vector sum for axes error
    double pt_axes_error = sqrt(
            pfault_plane_solution->mean_dist_P * pfault_plane_solution->mean_dist_P +
            pfault_plane_solution->mean_dist_T * pfault_plane_solution->mean_dist_T);
    if (DEBUG) printf("DEBUG: best_prob=%g, strike/dip/rake=%f/%f/%f, pdf_vol=%f/%f, mean_dist_P/T=%f/%f/%f\n", best_prob,
            pprefferedNP->strike, pprefferedNP->dip, pprefferedNP->rake,
            pfault_plane_solution->oct_tree_scatter_volume, fp_error,
            pfault_plane_solution->mean_dist_P, pfault_plane_solution->mean_dist_T, pt_axes_error);
    double polarity_count_error_weight = 1.0;
    double amplitude_error_weight = 1.0;
    if (0 && use_first_motion_polarities) {
        double polarity_correct = pfault_plane_solution->sum_polarity_misfit_weight
                - 2.0 * pfault_plane_solution->polarity_misfit; // factor of two gives larger penalty if < 1/2 polarities matched
        if (polarity_correct > 0.0) {
            polarity_count_error_weight = pfault_plane_solution->sum_polarity_misfit_weight / polarity_correct;
        } else {
            polarity_count_error_weight = DBL_MAX;
        }
    }
    /* moved to oct-tree core
    if (use_first_motion_polarities && nobs_used >= min_number_observations) {
        //20140326 AJL polarity_count_error_weight = exp(((double) min_number_observations) / (double) nobs_used);
        polarity_count_error_weight = exp((double) min_number_observations / (double) (nobs_used - min_number_observations + 1));
    } else if (0 && use_amplitudes && nobs_used >= min_number_observations) {
        amplitude_error_weight = pfault_plane_solution->amplitude_misfit;
    }*/
    double quality = pt_axes_error * polarity_count_error_weight * amplitude_error_weight;
    if (DEBUG) printf("DEBUG: quality=%f, pt_axes_error=%f, polarity_count_error_weight=%f, amplitude_error_weight=%f\n",
            quality, pt_axes_error, polarity_count_error_weight, amplitude_error_weight);
    char quality_code = 'D';
    if (nobs_used < min_number_observations) {
        quality_code = 'X';
        //} else if (quality < 20.0) {
    } else if (quality < 25.0) { // 20160916 AJL - compensate for change in fmamp_utils->getMinMeanDistanceAxis())
        quality_code = 'A';
        //} else if (quality < 35.0) {
    } else if (quality < 45.0) { // 20160916 AJL - compensate for change in fmamp_utils->getMinMeanDistanceAxis())
        quality_code = 'B';
        //} else if (quality < 50.0) {
    } else if (quality < 65.0) { // 20160916 AJL - compensate for change in fmamp_utils->getMinMeanDistanceAxis())
        quality_code = 'C';
    }


    // write summary file entry
    if (sumStream != NULL) {
        write_sum_entry(sumStream, phypocenter, pfault_plane_solution, pprefferedNP, variant_name, nobs_used, misfit, sum_misfit_weight, quality_code, iwrite_sum_header);
    }

    if (iwrite_sum_only) {
        return (0);
    }

    // write preferred mechanism in pseudo QuakeML format
    sprintf(outfile, "%s%s%ld.mech.fmamp%s.qml_data", outpath, "/hypo.", phypocenter->unique_id, variant_name);
    fp_out = fopen(outfile, "w");
    if (fp_out == NULL) {
        fprintf(stderr, "ERROR: opening outfile file: %s\n", outfile);
        return (-1);
    }
    fprintf(fp_out, "publicID %ld\n", phypocenter->unique_id);
    fprintf(fp_out, "triggeringOriginID %ld\n", phypocenter->unique_id);
    fprintf(fp_out, "nodalPlanes.nodalPlane1.strike %f\n", pprefferedNP->strike);
    fprintf(fp_out, "nodalPlanes.nodalPlane1.dip %f\n", pprefferedNP->dip);
    fprintf(fp_out, "nodalPlanes.nodalPlane1.rake %f\n", pprefferedNP->rake);
    fprintf(fp_out, "nodalPlanes.nodalPlane2.strike %f\n", strike_np2);
    fprintf(fp_out, "nodalPlanes.nodalPlane2.dip %f\n", dip_np2);
    fprintf(fp_out, "nodalPlanes.nodalPlane2.rake %f\n", rake_np2);
    fprintf(fp_out, "nodalPlanes.preferredPlane %d\n", -1);
    //       fprintf(fp_out, "nodalPlanes.nodalPlane2* %XXX\n",  XXX);
    //       fprintf(fp_out, "principalAxes* %XXX\n",  XXX);
    fprintf(fp_out, "azimuthalGap %f\n", -1.0);
    fprintf(fp_out, "stationPolarityCount %d\n", nobs_used);
    fprintf(fp_out, "misfit %f\n", misfit);
    fprintf(fp_out, "stationDistributionRatio %f\n", pfault_plane_solution->stat_dist_ratio);
    fprintf(fp_out, "polarityMisfit %f\n", nobs_used > 0 ? pfault_plane_solution->polarity_misfit : -1.0);
    fprintf(fp_out, "sumPolarityMisfitWeight %f\n", nobs_used > 0 ? pfault_plane_solution->sum_polarity_misfit_weight : -1.0);
    fprintf(fp_out, "amplitudeMisfit %f\n", nobs_used > 0 ? pfault_plane_solution->amplitude_misfit : -1.0);
    fprintf(fp_out, "sumAmplitudeMisfitWeight %f\n", nobs_used > 0 ? pfault_plane_solution->sum_amplitude_misfit_weight : -1.0);
    fprintf(fp_out, "meanDistP %f\n", nobs_used > 0 ? pfault_plane_solution->mean_dist_P : -1.0);
    fprintf(fp_out, "meanDistT %f\n", nobs_used > 0 ? pfault_plane_solution->mean_dist_T : -1.0);
    fprintf(fp_out, "rmsAngDiffAccPref %f\n", -1.0);
    fprintf(fp_out, "fracAcc30degPref %f\n", -1.0);
    fprintf(fp_out, "qualityCode %c\n", quality_code);
    char method_detail[STANDARD_STRLEN] = "";
    strcat(method_detail, variant_name);
    fprintf(fp_out, "methodID %s%s_%s_%s/%s\n", "smi:net.alomax/focalmec/", PACKAGE, VERSION, VERSION_DATE, method_detail);
    //       fprintf(fp_out, "comment %XXX\n",  XXX);
    //       fprintf(fp_out, "creationInfo %XXX\n",  XXX);
    fclose(fp_out);

    // write preferred mechanism in GMT meca format
    sprintf(outfile, "%s%s%ld.mech.fmamp%s.preferred.meca", outpath, "/hypo.", phypocenter->unique_id, variant_name);
    fp_out = fopen(outfile, "w");
    if (fp_out == NULL) {
        fprintf(stderr, "ERROR: opening outfile file: %s\n", outfile);
        return (-1);
    }
    /* ouput psmeca -Sa format
     * 1,2:      longitude, latitude of event (−: option interchanges order)     ! AJL use lat lon and -:
     * 3:        depth of event in kilometers
     * 4,5,6:    strike, dip and rake in degrees
     * 7:        magnitude
     * 8,9:      longitude, latitude at which to place beach ball.
     *           Entries in these columns are necessary with the −C option.
     *           Using 0,0 in columns 8 and 9 will plot the beach ball at the
     *           longitude, latitude given in columns 1 and 2.
     *           The −: option will interchange the order of columns (1,2) and (8,9).
     * 10:       Text string to appear above the beach ball (optional).
     */
    fprintf(fp_out, "lon lat depth str dip slip st plon plat text\n");
    fprintf(fp_out, "%f %f %f  %f %f %f  %f  0 0  %4.4d.%2.2d.%2.2d-%2.2d:%2.2d:%05.2f n= %d q= %c\n",
            phypocenter->lat, phypocenter->lon, phypocenter->depth,
            pprefferedNP->strike, pprefferedNP->dip, pprefferedNP->rake,
            phypocenter->magnitude,
            phypocenter->year, phypocenter->month, phypocenter->day, phypocenter->hour, phypocenter->min, phypocenter->dec_sec,
            nobs_used, quality_code);
    fclose(fp_out);

    // write preferred mechanism in GMT pspolar format
    sprintf(outfile, "%s%s%ld.mech.fmamp%s.preferred.polar", outpath, "/hypo.", phypocenter->unique_id, variant_name);
    fp_out = fopen(outfile, "w");
    if (fp_out == NULL) {
        fprintf(stderr, "ERROR: opening outfile file: %s\n", outfile);
        return (-1);
    }
    /* output pspolar
     * 1,2,3,4   station_code, azimuth, take-off angle, polarity
     *           polarity:
     *           - compression can be c,C,u,U,+
     *           - rarefaction can be d,D,r,R,-
     *           - not defined is anything else
     */
    fprintf(fp_out, "station_code azimuth take-off_angle polarity pred_amp obs_amp fmquality to_angle_wt weight\n");
    char cpolarity;
    for (npick = 0; npick < phypocenter->pick_list_size; npick++) {
        ppick = phypocenter->pick_list + npick;
        // output picks that have first-motion polarities and, for fm-amp,  gain corrected amplitudes
        if (ppick->fmpolarity && (use_first_motion_polarities || (use_amplitudes && ppick->amplitude > FLT_MIN))) { // negative amplitude flags that channel gain is not available
            cpolarity = '-';
            if (ppick->fmpolarity == 1) {
                // 20160901 AJL GMT5  cpolarity = 'C';
                cpolarity = '+';
            } else if (ppick->fmpolarity == -1) {
                // 20160901 AJL GMT5  cpolarity = 'X'; // use X symbol for down, in GMT 'X' means no first-motion!
                cpolarity = '-';
            }
            fprintf(fp_out, "%s_%s_%s_%s %.2f %.2f %c", ppick->network, ppick->station, ppick->location, ppick->channel,
                    ppick->take_off_angle_az, ppick->take_off_angle_inc, cpolarity);
            fprintf(fp_out, " %g",
                    calc_rad(pprefferedNP->strike * DE2RA, pprefferedNP->dip * DE2RA, pprefferedNP->rake * DE2RA,
                    // TEST NO POLARITY !!    fabs(calc_rad(pprefferedNP->strike * DE2RA, pprefferedNP->dip * DE2RA, pprefferedNP->rake * DE2RA,
                    ppick->take_off_angle_inc * DE2RA, ppick->take_off_angle_az * DE2RA, 'P')
                    );
            // TEST NO POLARITY !!  ));
            if (ppick->amplitude > FLT_MIN) { // negative amplitude flags that channel gain is not available
                fprintf(fp_out, " %g",
                        ppick->amplitude * (double) ppick->fmpolarity
                        // TEST NO POLARITY !!  ppick->amplitude
                        );
            } else {
                fprintf(fp_out, " 0");
            }
            fprintf(fp_out, " %g %g %g", ppick->fmquality, ppick->take_off_angle_distrib_weight, ppick->weight);
            fprintf(fp_out, "\n");
        }
    }
    fclose(fp_out);

    // write preferred mechanism P, T and N axes
    sprintf(outfile, "%s%s%ld.mech.fmamp%s.preferred.axes", outpath, "/hypo.", phypocenter->unique_id, variant_name);
    fp_out = fopen(outfile, "w");
    if (fp_out == NULL) {
        fprintf(stderr, "ERROR: opening outfile file: %s\n", outfile);
        return (-1);
    }
    fprintf(fp_out, "Pstr Pdip Tstr Tdip Nstr Ndip\n");
    fprintf(fp_out, "%f %f  %f %f  %f %f\n",
            pfault_plane_solution->meanPaxis.str, pfault_plane_solution->meanPaxis.dip,
            pfault_plane_solution->meanTaxis.str, pfault_plane_solution->meanTaxis.dip,
            pfault_plane_solution->meanNaxis.str, pfault_plane_solution->meanNaxis.dip
            );
    fclose(fp_out);

    // write acceptable mechanisms in GMT meca format
    sprintf(outfile, "%s%s%ld.mech.fmamp%s.acceptable.meca", outpath, "/hypo.", phypocenter->unique_id, variant_name);
    fp_out = fopen(outfile, "w");
    if (fp_out == NULL) {
        fprintf(stderr, "ERROR: opening outfile file: %s\n", outfile);
        return (-1);
    }
    /* ouput psmeca -Sa format
     * 1,2:      longitude, latitude of event (−: option interchanges order)     ! AJL use lat lon and -:
     * 3:        depth of event in kilometers
     * 4,5,6:    strike, dip and rake in degrees
     * 7:        magnitude
     * 8,9:      longitude, latitude at which to place beach ball.
     *           Entries in these columns are necessary with the −C option.
     *           Using 0,0 in columns 8 and 9 will plot the beach ball at the
     *           longitude, latitude given in columns 1 and 2.
     *           The −: option will interchange the order of columns (1,2) and (8,9).
     * 10:       Text string to appear above the beach ball (optional).
     */
    fprintf(fp_out, "lon lat depth str dip slip st plon plat text\n");
    sample = pscatter_sample;
    int istep = 1 + n_scatter_sample / NUM_ACCEPTABLE_TO_WRITE;
    for (nscatter = 0; nscatter < n_scatter_sample; nscatter += istep) {
        strike = *sample;
        sample++;
        dip = *sample;
        sample++;
        rake = *sample;
        sample++;
        sample++; // value
        fprintf(fp_out, "%f %f %f  %f %f %f  %f  0 0  %d.%d.%d-%d:%d:%5.2f n= %d q= %c\n",
                phypocenter->lat, phypocenter->lon, phypocenter->depth,
                strike, dip, rake,
                phypocenter->magnitude,
                phypocenter->year, phypocenter->month, phypocenter->day, phypocenter->hour, phypocenter->min, phypocenter->dec_sec,
                nobs_used, quality_code);
        sample += (istep - 1) * 4;
    }
    fclose(fp_out);


#if 0
    {
        // DEBUG: write predicted amplitudes for preferred mechanism in GMT pspolar format
        FILE * fp_amp_out[9];
        int n;
        for (n = 0; n < 9; n++) {
            sprintf(outfile, "%s%s%ld.mech.fmamp%s.preferred.amplitude_0%d", outpath, "/hypo.", phypocenter->unique_id, variant_name, n + 1);
            fp_amp_out[n] = fopen(outfile, "w");
            if (fp_amp_out[n] == NULL) {
                fprintf(stderr, "ERROR: opening outfile file: %s\n", outfile);
                return (-1);
            }
            fprintf(fp_amp_out[n], "station_code azimuth take-off_angle polarity pred_amp obs_amp\n");
        }
        /* output pspolar
         * 1,2,3,4   station_code, azimuth, take-off angle, polarity
         *           polarity:
         *           - compression can be c,C,u,U,+
         *           - rarefaction can be d,D,r,R,-
         *           - not defined is anything else
         */
        char cpolarity;
        double take_off_angle_inc = 0.0;
        while (take_off_angle_inc < 180.0) {
            double take_off_angle_az = 0.0;
            while (take_off_angle_az < 360.0) {
                // output picks that have first-motion polarities and, for fm-amp,  gain corrected amplitudes
                double amplitude = calc_rad(pprefferedNP->strike * DE2RA, pprefferedNP->dip * DE2RA, pprefferedNP->rake * DE2RA,
                        take_off_angle_inc * DE2RA, take_off_angle_az * DE2RA, 'P');
                cpolarity = amplitude >= 0.0 ? 'C' : 'X'; // use X symbol for down, in GMT 'X' means no first-motion!
                int namp = (int) (fabs(amplitude) * 9.999) - 1;
                printf("%d PRED_AMP %.2f %.2f %c %g 0\n", namp, take_off_angle_az, take_off_angle_inc, cpolarity, amplitude);
                if (namp >= 0 && namp < 10) {
                    fprintf(fp_amp_out[namp], "PRED_AMP %.2f %.2f %c %g 0\n", take_off_angle_az, take_off_angle_inc, cpolarity, amplitude);
                }
                take_off_angle_az += 5.0;
            }
            take_off_angle_inc += 5.0;
        }
        for (n = 0; n < 9; n++) {
            fclose(fp_amp_out[n]);
        }
    }
#endif

    return (0);

}


