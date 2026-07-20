/***************************************************************************
 * read_input.c
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
#include "early_est.h"

#define SIGNAL_TO_NOISE_RATIO_MIN 3.0

static char tmp_str[STANDARD_STRLEN];

// uncomment if using fmamp with ground motion data
#define USE_GROUND_MOTION_DATA 1

// forward function declarations
int read_input_fmamp(Settings *settings, FILE* hypoStream, char *hypofile, Hypocenter *phypocenter, int verbose);
int read_input_early_est_csv(Settings *settings, FILE* hypoStream, char *hypofile, char *pickfile, Hypocenter *phypocenter, int verbose);

/***************************************************************************
 * read hypocenter and picks input files:
 *
 * Returns 0 on success and -1 otherwise.
 ***************************************************************************/
int read_input(Settings *settings, char *input_format, Hypocenter *phypocenter, FILE* hypoStream, char *hypofile, char *pickfile, int verbose) {

    if (strcmp(input_format, "FMAMP") == 0) {
        return (read_input_fmamp(settings, hypoStream, hypofile, phypocenter, verbose));
    } else if (strcmp(input_format, "EARLY_EST_CSV") == 0) {
        return (read_input_early_est_csv(settings, hypoStream, hypofile, pickfile, phypocenter, verbose));
    }

    fprintf(stderr, "ERROR: unrecognized input format: %s\n", input_format);
    return (-1);

}

/***************************************************************************
 * read hypocenter and picks input files in fmamp format:
 *
 * Returns 0 on success and -1 otherwise.
 ***************************************************************************/

int read_input_fmamp(Settings *settings, FILE* hypoStream, char *hypofile, Hypocenter *phypocenter, int verbose) {


    // read next hypocenter
    int hypo_read = 0;
    while (1) {

        if (fgets(tmp_str, STANDARD_STRLEN - 1, hypoStream) == NULL)
            return (-1); // end of file or error

        // event_unique_id year month day hour min dec_sec rms lat lon errh depth errz
        //    nassoc_P dist_min dist_max gap_primary gap_secondary ampAttenPower magnitude mag_type
        // 19790428004444722 1979 04 28 00 44  44.7220 0.167644 37.632269 -122.472858 0.884223 13.345703 0.563081 242 0.626317 -1.000000 151.702453 160.987377 -999.000000 0.000000 NA
        int istat = sscanf(tmp_str, "%ld %d %d %d %d %d %lf %lf %lf %lf %lf %lf %lf %d %lf %lf %lf %lf %lf %s ",
                &phypocenter->unique_id,
                &phypocenter->year, &phypocenter->month, &phypocenter->day, &phypocenter->hour, &phypocenter->min, &phypocenter->dec_sec, &phypocenter->rms,
                &phypocenter->lat, &phypocenter->lon, &phypocenter->errh, &phypocenter->depth, &phypocenter->errz,
                &phypocenter->nassoc_P, &phypocenter->dist_min, &phypocenter->gap_primary, &phypocenter->gap_secondary,
                &phypocenter->ampAttenPower, &phypocenter->magnitude, phypocenter->mag_type
                );
        if (istat < 20) { // input format error
            //fprintf(stderr, "ERROR: hypo input format error hypocenter file: %s\n", hypofile);
            continue; // assume blank line separating events
        }
        phypocenter->dist_max = -1.0;

        if (DEBUG) printf("DEBUG: hypocenter: %ld  %4d.%2d.%2d-%d:%d:%.2f  %.2f %.2f %.1f  %s%.1f\n", phypocenter->unique_id,
                phypocenter->year, phypocenter->month, phypocenter->day, phypocenter->hour, phypocenter->min, phypocenter->dec_sec,
                phypocenter->lat, phypocenter->lon, phypocenter->depth,
                phypocenter->mag_type, phypocenter->magnitude
                );
        if (verbose >= 2)
            fprintf(stdout, "Info: successfully: read input hypocenter file: %s\n", hypofile);

        // read picks for this hypocenter
        char cfmpolarity[16];
        char *phase;
        Pick *pick_list = phypocenter->pick_list;
        int npicks = 0;
        while (npicks < MAX_NUM_PICKS) {

            if (fgets(tmp_str, STANDARD_STRLEN - 1, hypoStream) == NULL)
                break; // end of file or error

            Pick* ppick = pick_list + npicks;
            // event_unique_id station location channel network phase year month day hour min dec_sec pick_error pick_error_type residual
            //    fmpolarity fmquality fmtype amplitude take_off_angle_az take_off_angle_inc epicentral_distance epicentral_azimuth
            int istat = sscanf(tmp_str, "%ld %s %s %s %s %s %d %d %d %d %d %lf %lf %s %lf %s %lf %s %lf %lf %lf %lf %lf ",
                    &ppick->event_unique_id,
                    ppick->station, ppick->location, ppick->network, ppick->channel,
                    ppick->phase,
                    &ppick->year, &ppick->month, &ppick->day, &ppick->hour, &ppick->min, &ppick->dec_sec,
                    &ppick->pick_error, ppick->pick_error_type,
                    &ppick->residual,
                    cfmpolarity, &ppick->fmquality, ppick->fmtype,
                    &ppick->amplitude,
                    &ppick->take_off_angle_az, &ppick->take_off_angle_inc,
                    &ppick->epicentral_azimuth, &ppick->epicentral_distance);
            /*printf("DEBUG: reading pick: %ld  %s %s %s %s  %s   FM: %s %f %s   Amp: %f  dist: %f\n", ppick->event_unique_id,
                    ppick->network, ppick->station, ppick->location, ppick->channel, ppick->phase,
                    cfmpolarity, ppick->fmquality, ppick->fmtype, ppick->amplitude, ppick->epicentral_distance);*/

            if (istat < 23) { // input format error
                //fprintf(stderr, "ERROR: pick input format error hypocenter file: %s\n", hypofile);
                break; // error, try to continue with next event
            }
            // set integer polarity code
            ppick->fmpolarity = 0;
            if (strstr("CcUu+", cfmpolarity)) {
                ppick->fmpolarity = 1;
            } else if (strstr("DdRr-", cfmpolarity)) {
                ppick->fmpolarity = -1;
            }

            // check event
            if (ppick->event_unique_id != phypocenter->unique_id) {
                if (DEBUG) printf("DEBUG: reject: event_unique_id phypocenter->unique_id  : %s: %ld\n", ppick->station, ppick->event_unique_id);
                continue;
            }
            // check first-motion
            if (ppick->fmquality < DBL_MIN) { // first motion quality wt is zero if no first motion available
                if (DEBUG) printf("DEBUG: reject: fmquality < DBL_MIN : %s : %f\n", ppick->station, ppick->fmquality);
                continue;
            }
            if (ppick->fmpolarity == 0) { // first motion not available
                if (DEBUG) printf("DEBUG: reject: fmpolarity == 0 : %s : %d\n", ppick->station, ppick->fmpolarity);
                continue;
            }
            if (ppick->take_off_angle_inc < 0.0) { // angle incidence not available
                if (DEBUG) printf("DEBUG: reject: take_off_angle_inc < 0.0 : %s : %f\n", ppick->station, ppick->take_off_angle_inc);
                continue;
            }
            phase = ppick->phase;
            if (strcmp(phase, "P") != 0 && strncmp(phase, "PKP", 3) != 0 && strcmp(phase, "Pg") != 0 && strcmp(phase, "Pn") != 0 && strcmp(phase, "P0") != 0) {
                if (DEBUG) printf("DEBUG: reject: not P, Pg, Pn or P0 phase : %s : %s\n", ppick->station, phase);
                continue;
            }
            // 20140821 AJL - TEST! check distance
            if (0 && ppick->epicentral_distance < 5.0) { // reject local/regional data
                if (DEBUG) printf("DEBUG: reject: epicentral_distance < 5.0 : %s : %f\n", ppick->station, ppick->epicentral_distance);
                continue;
            }

            npicks++;
        }
        if (verbose >= 2)
            fprintf(stdout, "Info: successfully read %d input picks from file: %s\n", npicks, hypofile);

        phypocenter->pick_list_size = npicks;
        hypo_read = 1;
        break;
    }

    if (!hypo_read) {
        return (-2);
    }

    return (0);

}

/***************************************************************************
 * read hypocenter and picks input files in Early-est format:
 *
 * Returns 0 on success and -1 otherwise.
 ***************************************************************************/
#ifndef USE_GROUND_MOTION_DATA
// Mwp constant
#define PI M_PI
// 4.213e19 - Tsuboi 1995, 1999
static double MWP_CONST =
        4.0 * PI // 4 PI
        * 3400.0 // rho
        * 7900.0 * 7900.0 * 7900.0 // Pvel**3
        * 2.0 // FP average radiation pattern
        * (10000.0 / 90.0) // distance deg -> km
* 1000.0 // distance km -> meters
;
#endif

int read_input_early_est_csv(Settings *settings, FILE* hypoStream, char *hypofile, char *pickfile, Hypocenter *phypocenter, int verbose) {

    // properties
    // Early-est Section
    //
    int use_hf_for_amp = 1;
    if (settings_get_int_helper(settings,
            "Early-est", "use_a_ref_for_amp", &use_hf_for_amp, 1,
            verbose
            ) == INT_INVALID) {
        ; // handle error
    }
    int use_waveform_polarity = 0;
    if (settings_get_int_helper(settings,
            "Early-est", "use_waveform_polarity", &use_waveform_polarity, 0,
            verbose
            ) == INT_INVALID) {
        ; // handle error
    }


    // read in hypocenter in native format (Early-est hypos.csv file input_format)
    //    event_id assoc_ndx loc_seq_num ph_assoc ph_used dmin(deg) gap1(deg) gap2(deg) atten sigma_otime(sec) otime(UTC) lat(deg) lon(deg) errH(km) depth(km) errZ(km) Q T50Ex n Td(sec) n TdT50Ex WL_col mb n Mwp n T0(sec) n Mwpd n region n_sta_tot n_sta_active assoc_latency
    //    1299822380917 1 0 30 26 2.816 122.727 126.190 -1.418 1.398 2011.03.11-05:46:20.92 38.088 142.743 13.74 10.16 13.26 A 3.619 19 11.280 19 40.823 WHITE 6.169 17 8.082 21 148.39 15 8.736 17 Near_East_Coast_of_Honshu,_Japan 60 60 173001741
    int hypo_read = 0;
    while (1) {
        Hypocenter_early_est hypo_early_est;
        Hypocenter_early_est *phypo = &hypo_early_est;
        if (fgets(tmp_str, STANDARD_STRLEN - 1, hypoStream) == NULL)
            break;
        int istat = sscanf(tmp_str, "%ld %d %d %d %d %lf %lf %lf %lf %lf %4d.%2d.%2d-%d:%d:%lf %lf %lf %lf %lf %lf %s %lf %d %lf %d %lf %s %lf %d %lf %d %lf %d %lf %d %*s *d *d *d",
                &phypo->unique_id, &phypo->hyp_assoc_index, &phypo->loc_seq_num, &phypo->nassoc, &phypo->nassoc_P,
                &phypo->dist_min, &phypo->gap_primary, &phypo->gap_secondary, &phypo->ampAttenPower,
                &phypo->ot_std_dev,
                &phypo->year, &phypo->month, &phypo->day, &phypo->hour, &phypo->min, &phypo->dec_sec,
                &phypo->lat, &phypo->lon, &phypo->errh, &phypo->depth, &phypo->errz, phypo->quality_code,
                &phypo->t50ExLevelStatistics.centralValue, &phypo->t50ExLevelStatistics.numLevel,
                &phypo->taucLevelStatistics.centralValue, &phypo->taucLevelStatistics.numLevel,
                &phypo->tdT50ExLevelStatistics.centralValue, phypo->warningLevelString,
                &phypo->mbLevelStatistics.centralValue, &phypo->mbLevelStatistics.numLevel,
                &phypo->mwpLevelStatistics.centralValue, &phypo->mwpLevelStatistics.numLevel,
                &phypo->t0LevelStatistics.centralValue, &phypo->t0LevelStatistics.numLevel,
                &phypo->mwpdLevelStatistics.centralValue, &phypo->mwpdLevelStatistics.numLevel
                );
        if (istat < 29) { // input format error
            continue; // try next event
        }
        // load to fmamp Hypocenter
        phypocenter->unique_id = phypo->unique_id;
        phypocenter->year = phypo->year;
        phypocenter->month = phypo->month;
        phypocenter->day = phypo->day;
        phypocenter->hour = phypo->hour;
        phypocenter->min = phypo->min;
        phypocenter->dec_sec = phypo->dec_sec;
        phypocenter->rms = phypo->ot_std_dev;
        phypocenter->lat = phypo->lat;
        phypocenter->lon = phypo->lon;
        phypocenter->errh = phypo->errh;
        phypocenter->depth = phypo->depth;
        phypocenter->errz = phypo->errz;
        phypocenter->nassoc_P = phypo->nassoc_P; // number of picks that contributed with weight > 0 to location
        phypocenter->dist_min = phypo->dist_min; // minimum distance of associated phase counted as nassoc_P
        phypocenter->dist_max = phypo->dist_max; // maximum distance of associated phase counted as nassoc_P
        phypocenter->gap_primary = phypo->gap_primary; // maximum azimuth gap
        phypocenter->gap_secondary = phypo->gap_secondary; // secondary azimuth gap - largest azumth gap filled by a single station
        phypocenter->ampAttenPower = phypo->ampAttenPower;
        if (!use_hf_for_amp && phypo->mwpLevelStatistics.numLevel >= 4) {
            phypocenter->magnitude = phypo->mwpLevelStatistics.centralValue;
            strcpy(phypocenter->mag_type, "Mwp");
        } else {
            phypocenter->magnitude = phypo->mbLevelStatistics.centralValue;
            strcpy(phypocenter-> mag_type, "mb");
        }

        if (DEBUG) printf("DEBUG: hypocenter: %ld  %4d.%2d.%2d-%d:%d:%.2f  %.2f %.2f %.1f  %s%.1f mb%.1f Mwp%.1f\n", phypocenter->unique_id,
                phypocenter->year, phypocenter->month, phypocenter->day, phypocenter->hour, phypocenter->min, phypocenter->dec_sec,
                phypocenter->lat, phypocenter->lon, phypocenter->depth,
                phypocenter->mag_type, phypocenter->magnitude, phypo->mbLevelStatistics.centralValue, phypo->mwpLevelStatistics.centralValue
                );
        if (verbose >= 2)
            fprintf(stdout, "Info: successfully: read input hypocenter file: %s\n", hypofile);

        // read picks for this hypocenter
        FILE * pickStream = fopen(pickfile, "r");
        if (pickStream == NULL) {
            fprintf(stderr, "ERROR: opening pick file: %s\n", pickfile);
            return (-1);
        }
        // read in picks                ! Early-est picks.csv file input_format
        // event_id n event dist az channel stream loc time unc pol pol_type pol_wt toang phase residual tot_wt dist_wt st_q_wt T50 Aref Aerr T50Ex Tdom s/n_HF s/n_BRB s/n_BRB mb Mwp T0 Mwpd status
        // 1053542662094 1 1 3.01 8.0 GE_MAHO_--_BHZ HF L 2003.05.21-18:45:06.20 0.200 -1 W5.1 0.99 45.9 P -2.72 0.909 1.000 1.000 75.9 51.3 4.64 1.478 3.825 838.629 334.864  721.491 -9.000 7.252 83.350 7.289 OK_HF_BRB
        Pick_early_est picks_early_est[MAX_NUM_PICKS];
        Pick_early_est *ppick;
        char channel_str[64];
        char *phase;
        int npicks = 0;
        while (npicks < MAX_NUM_PICKS) {
            ppick = picks_early_est + npicks;
            /*
            read (12,*,end=40) ieventid_pick,fskip,fskip,qdist,qazi,
         &    sname(k),cskip,locflag,cskip,fskip,p_pol(k),cskip,fqual,qthe,
         &    phase
            if (ieventid_pick.ne.event_id) goto 30
            if (qdist.gt.delmax) goto 30
            if (fqual.lt.0.5) p_qual(k) = 1
            if (p_qual(k).gt.1) goto 30
            if (p_pol(k).eq.0) goto 30
            if (qthe.lt.0.0) goto 30
            if (locflag.ne.'L') goto 30
            if (phase.ne.'P'.and.phase(1:3).ne.'PKP') goto 30
             */

            int istat = -1;
#ifdef USE_GROUND_MOTION_DATA
            // event_id n event dist az channel stream loc time unc pol pol_type pol_wt toang paz paz_unc paz_calc paz_wt phase residual tot_wt dist_wt st_q_wt T50 Aref Aerr T50Ex Tdom Avel Adisp s/n_HF s/n_BRBV s/n_BRBD mb Mwp T0 Mwpd status sta_corr
            if (use_hf_for_amp) {
                // amplitude is Avel
                istat = fscanf(pickStream, "%ld %*d %d %lf %lf %s %*s %c %4d.%2d.%2d-%d:%d:%lf %lf %d %s %lf %lf %*f %*f %*f %*f %s %lf %lf %lf %lf %lf %*f %lf %*f %*f %lf %*f %lf %lf %lf %lf %lf",
                        &ppick->event_unique_id, &ppick->is_associated, &ppick->epicentral_distance, &ppick->epicentral_azimuth,
                        channel_str, &ppick->locflag,
                        &ppick->year, &ppick->month, &ppick->day, &ppick->hour, &ppick->min, &ppick->dec_sec,
                        &ppick->pick_error, &ppick->fmpolarity, ppick->fmtype, &ppick->fmquality, &ppick->take_off_angle_inc,
                        ppick->phase, &ppick->residual, &ppick->loc_weight, &ppick->dist_weight, &ppick->station_quality_weight,
                        &ppick->t50, &ppick->amplitude_error_ratio, &ppick->amplitude, &ppick->sn_hf, &ppick->sn_brb, &ppick->sn_brb_int, &ppick->mb, &ppick->mwp
                        );
            } else {
                // amplitude is Adisp
                istat = fscanf(pickStream, "%ld %*d %d %lf %lf %s %*s %c %4d.%2d.%2d-%d:%d:%lf %lf %d %s %lf %lf %*f %*f %*f %*f %s %lf %lf %lf %lf %lf %*f %lf %*f %*f %*f %lf %lf %lf %lf %lf %lf",
                        &ppick->event_unique_id, &ppick->is_associated, &ppick->epicentral_distance, &ppick->epicentral_azimuth,
                        channel_str, &ppick->locflag,
                        &ppick->year, &ppick->month, &ppick->day, &ppick->hour, &ppick->min, &ppick->dec_sec,
                        &ppick->pick_error, &ppick->fmpolarity, ppick->fmtype, &ppick->fmquality, &ppick->take_off_angle_inc,
                        ppick->phase, &ppick->residual, &ppick->loc_weight, &ppick->dist_weight, &ppick->station_quality_weight,
                        &ppick->t50, &ppick->amplitude_error_ratio, &ppick->amplitude, &ppick->sn_hf, &ppick->sn_brb, &ppick->sn_brb_int, &ppick->mb, &ppick->mwp
                        );
            }
#else
            // 20140801 AJL - before addition of Avel and Adisp to pick list!
            /*
            istat = fscanf(pickStream, "%ld %*d %d %lf %lf %s %*s %c %4d.%2d.%2d-%d:%d:%lf %lf %d %s %lf %lf %s %lf %lf %lf %lf %lf %lf %lf %*f %*f %*f %*f %*f %lf %lf",
                    &ppick->event_unique_id, &ppick->is_associated, &ppick->epicentral_distance, &ppick->epicentral_azimuth,
                    channel_str, &ppick->locflag,
                    &ppick->year, &ppick->month, &ppick->day, &ppick->hour, &ppick->min, &ppick->dec_sec,
                    &ppick->pick_error, &ppick->fmpolarity, ppick->fmtype, &ppick->fmquality, &ppick->take_off_angle_inc,
                    ppick->phase, &ppick->residual, &ppick->loc_weight, &ppick->dist_weight, &ppick->station_quality_weight,
                    &ppick->t50, &ppick->amplitude, &ppick->amplitude_error_ratio, &ppick->mb, &ppick->mwp
                    );*/
            // 20140804 AJL - after addition of Avel and Adisp to pick list!
            istat = fscanf(pickStream, "%ld %*d %d %lf %lf %s %*s %c %4d.%2d.%2d-%d:%d:%lf %lf %d %s %lf %lf %*f %*f %s %lf %lf %lf %lf %lf %lf %lf %*f %*f %*f %*f %*f %*f %*f %lf %lf",
                    &ppick->event_unique_id, &ppick->is_associated, &ppick->epicentral_distance, &ppick->epicentral_azimuth,
                    channel_str, &ppick->locflag,
                    &ppick->year, &ppick->month, &ppick->day, &ppick->hour, &ppick->min, &ppick->dec_sec,
                    &ppick->pick_error, &ppick->fmpolarity, ppick->fmtype, &ppick->fmquality, &ppick->take_off_angle_inc,
                    ppick->phase, &ppick->residual, &ppick->loc_weight, &ppick->dist_weight, &ppick->station_quality_weight,
                    &ppick->t50, &ppick->amplitude, &ppick->amplitude_error_ratio, &ppick->mb, &ppick->mwp
                    );
#endif
            if (istat == 0 && npicks == 0) { // may be header line
                // try to read to end of line
                if (fgets(tmp_str, STANDARD_STRLEN - 1, pickStream) == NULL)
                    break;
                continue;
            }
            if (istat == EOF)
                break;
            if (istat < 24) { // input_format error
                fprintf(stderr, "ERROR: istat=%d reading picks file: %s\n", istat, pickfile);
                return (-1);
            }
            // try to read to end of line
            if (fgets(tmp_str, STANDARD_STRLEN - 1, pickStream) == NULL)
                break;
            // chech reading
            /*
            if (ieventid_pick.ne.event_id) goto 30
            if (qdist.gt.delmax) goto 30
            if (fqual.lt.0.5) p_qual(k) = 1
            if (p_qual(k).gt.1) goto 30
            if (p_pol(k).eq.0) goto 30
            if (qthe.lt.0.0) goto 30
            if (locflag.ne.'L') goto 30
            if (phase.ne.'P'.and.phase(1:3).ne.'PKP') goto 30
             */
            // check event
            if (ppick->event_unique_id != phypocenter->unique_id) {
                if (DEBUG) printf("DEBUG: reject: event_unique_id phypocenter->unique_id  : %s: %ld\n", ppick->station, ppick->event_unique_id);
                continue;
            }
            // check use_waveform_polarity
            if (use_waveform_polarity && ppick->fmtype[0] != 'W') { // first motion type is W or F
                if (DEBUG) printf("DEBUG: reject: fmtype != 'W' : %s : %f\n", ppick->station, ppick->fmquality);
                continue;
            }
            // check first-motion
            if (ppick->fmquality < DBL_MIN) { // first motion quality wt is zero if no first motion available
                if (DEBUG) printf("DEBUG: reject: fmquality < DBL_MIN : %s : %f\n", ppick->station, ppick->fmquality);
                continue;
            }
            if (ppick->fmpolarity == 0) { // first motion not available
                if (DEBUG) printf("DEBUG: reject: fmpolarity == 0 : %s : %d\n", ppick->station, ppick->fmpolarity);
                continue;
            }
            if (ppick->take_off_angle_inc < 0.0) { // angle incidence not available
                if (DEBUG) printf("DEBUG: reject: take_off_angle_inc < 0.0 : %s : %f\n", ppick->station, ppick->take_off_angle_inc);
                continue;
            }
            if (ppick->locflag != 'L') { // pick not used for location
                if (DEBUG) printf("DEBUG: reject: locflag != 'L' : %s : %c\n", ppick->station, ppick->locflag);
                continue;
            }
            // check is P
            phase = ppick->phase;
            // 20170415 AJL - TEST NO PKP
            //if (strcmp(phase, "P") != 0 && strncmp(phase, "PKP", 3) != 0 && strcmp(phase, "Pg") != 0 && strcmp(phase, "Pn") != 0) {
            if (strcmp(phase, "P") != 0 && strcmp(phase, "Pg") != 0 && strcmp(phase, "Pn") != 0) {
                if (DEBUG) printf("DEBUG: reject: not P, Pg or Pn phase : %s : %s\n", ppick->station, phase);
                continue;
            }
            // 20140821 AJL - TEST! check distance
            if (0 && ppick->epicentral_distance < 5.0) { // reject local/regional data
                if (DEBUG) printf("DEBUG: reject: epicentral_distance < 5.0 : %s : %f\n", ppick->station, ppick->epicentral_distance);
                continue;
            }
            /* 20170404 AJL - TEST! check s/n -> not much difference
            if (ppick->sn_hf < 5.0) { // reject high noise data
                if (DEBUG) printf("DEBUG: reject: sn_hf < 5.0 : %s : %f\n", ppick->station, ppick->epicentral_distance);
                continue;
            }//*/
            /* 20170413 AJL - TEST! check s/n -> not much difference
            if (ppick->sn_brb < 3.0) { // reject high noise data
                if (DEBUG) printf("DEBUG: reject: sn_brb < 5.0 : %s : %f\n", ppick->station, ppick->epicentral_distance);
                continue;
            }//*/
            // decode channel
            char *c0 = channel_str;
            char *c1 = strchr(c0, '_');
            strncpy(ppick->network, c0, c1 - c0);
            c0 = c1 + 1;
            c1 = strchr(c0, '_');
            strncpy(ppick->station, c0, c1 - c0);
            c0 = c1 + 1;
            c1 = strchr(c0, '_');
            strncpy(ppick->location, c0, c1 - c0);
            c0 = c1 + 1;
            strcpy(ppick->channel, c0);
            // load to fmamp Pick
            Pick *pick_list = phypocenter->pick_list;
            pick_list[npicks].event_unique_id = ppick->event_unique_id;
            strcpy(pick_list[npicks].station, ppick->station);
            strcpy(pick_list[npicks].location, ppick->location);
            strcpy(pick_list[npicks].channel, ppick->channel);
            strcpy(pick_list[npicks].network, ppick->network);
            strcpy(pick_list[npicks].phase, ppick->phase);
            pick_list[npicks].year = ppick->year;
            pick_list[npicks].month = ppick->month;
            pick_list[npicks].day = ppick->day;
            pick_list[npicks].hour = ppick->hour;
            pick_list[npicks].min = ppick->min;
            pick_list[npicks].dec_sec = ppick->dec_sec;
            strcpy(pick_list[npicks].pick_error_type, ppick->pick_error_type);
            pick_list[npicks].pick_error = ppick->pick_error;
            pick_list[npicks].fmpolarity = ppick->fmpolarity;
            pick_list[npicks].fmquality = ppick->fmquality;
            strcpy(pick_list[npicks].fmtype, ppick->fmtype);
            pick_list[npicks].amplitude = -1.0;
#ifdef USE_GROUND_MOTION_DATA
            // use amplitude
            //if (ppick->amplitude > 0.0 && ppick->amplitude_error_ratio > 0.2 && ppick->amplitude_error_ratio < 5.0) {
            if (ppick->amplitude > 0.0) {
                double dist_atten = 1.0;
                //  IMPORTANT: use following block only if read amplitude type is same as that used in Early-est for estimating attenuation!
                /*
                if (phypocenter->ampAttenPower > -9.0) {
                    double dist = ppick->epicentral_distance;
                    double st_line_dist = sqrt(dist * dist + phypocenter->depth * phypocenter->depth * KM2DEG * KM2DEG); // straight line distance
                    dist_atten = pow(st_line_dist, phypocenter->ampAttenPower);
                    //dist_atten = pow(st_line_dist, -1.0); // TEST
                    printf("DEBUG: reading pick:ampAttenPower=%f, dist=%f, st_line_dist=%f, dist_atten=%f", phypocenter->ampAttenPower, dist, st_line_dist, dist_atten);
                }*/
                // IMPORTANT: use following block only if read amplitude type is not the same as that used in Early-est for estimating attenuation!
                {
                    double dist = ppick->epicentral_distance;
                    double st_line_dist = sqrt(dist * dist + phypocenter->depth * phypocenter->depth * KM2DEG * KM2DEG); // straight line distance
                    dist_atten = pow(st_line_dist, -1.0); // TEST
                }
                //printf(", amplitude=%g", ppick->amplitude);
                if (use_hf_for_amp) {
                    // use mb as proxy for quality of hf or ground vel amplitude
                    if (strcmp(phypocenter->mag_type, "mb") == 0 && ppick->mb > -9.0 && fabs(ppick->mb - phypocenter->magnitude) < 0.3) {
                        // use s/n ratio as quality of amplitude
                        // 20140923 AJL  if (ppick->sn_brb >= SIGNAL_TO_NOISE_RATIO_MIN) {
                        pick_list[npicks].amplitude = ppick->amplitude / dist_atten; // amplitude
                    }
                } else {
                    // use Mwp as proxy for quality of brb or ground disp amplitude
                    if (strcmp(phypocenter->mag_type, "Mwp") == 0 && ppick->mwp > -9.0 && fabs(ppick->mwp - phypocenter->magnitude) < 0.3) {
                        // use s/n ratio as quality of amplitude
                        // 20140923 AJL  if (ppick->sn_brb_int >= SIGNAL_TO_NOISE_RATIO_MIN) {
                        pick_list[npicks].amplitude = ppick->amplitude / dist_atten; // amplitude
                    }
                }
            }
#else
            if (use_hf_for_amp) {
                // use mb
                printf("DEBUG: reading mb: %s mb=%f hypoM=%f\n", phypocenter->mag_type, ppick->mb, phypocenter->magnitude);
                if (strcmp(phypocenter->mag_type, "mb") == 0 && ppick->mb > -9.0 && fabs(ppick->mb - phypocenter->magnitude) < 0.6) {
                    //if (strcmp(phypocenter->mag_type, "mb") == 0 && ppick->mb > -9.0 && fabs(ppick->mb - phypocenter->magnitude) < 1.0) {
                    // undo formulas applied in Early-est: timedomain_processing_data.c->calculate_mB_Mag()
                    double peak = pow(10.0, ppick->mb) * (2.0 * PI); // maintains Q correction
                    pick_list[npicks].amplitude = peak;
                }
                printf(" -> %g\n", pick_list[npicks].amplitude);
            } else {
                // use Mwp
                printf("DEBUG: reading Mwp: %s Mwp=%f hypoM=%f\n", phypocenter->mag_type, ppick->mwp, phypocenter->magnitude);
                if (strcmp(phypocenter->mag_type, "Mwp") == 0 && ppick->mwp > -9.0 && fabs(ppick->mwp - phypocenter->magnitude) < 0.6) {
                    //if (strcmp(phypocenter->mag_type, "Mwp") == 0 && ppick->mwp > -9.0 && fabs(ppick->mwp - phypocenter->magnitude) < 1.0) {
                    //if (strcmp(phypocenter->mag_type, "Mwp") == 0 && ppick->mwp > -9.0) {
                    // undo formulas applied in Early-est: timedomain_processing_data.c->calculate_Mwp_Mag()
                    double moment = pow(10.0, ppick->mwp / (2.0 / 3.0) + 9.1);
                    pick_list[npicks].amplitude = moment / MWP_CONST; // do not un-correct for epicentral_distance
                }
            }
#endif
            pick_list[npicks].take_off_angle_inc = ppick->take_off_angle_inc; // degrees (0/down->180/up)
            pick_list[npicks].take_off_angle_az = ppick->epicentral_azimuth; // degrees CW from N
            pick_list[npicks].epicentral_distance = ppick->epicentral_distance; // degrees
            pick_list[npicks].epicentral_azimuth = ppick->epicentral_azimuth; // degrees CW from N
            pick_list[npicks].residual = ppick->residual;
            //pick_list[npicks].weight = ppick->loc_weight;
            //printf("DEBUG: reading pick: %ld  %s %s %s %s  %s   FM: %d %f %s   Amp: %f  Mwp: %f  dist: %f\n", pick_list[npicks].event_unique_id,
            //        pick_list[npicks].network, pick_list[npicks].station, pick_list[npicks].location, pick_list[npicks].channel, pick_list[npicks].phase,
            //        pick_list[npicks].fmpolarity, pick_list[npicks].fmquality, pick_list[npicks].fmtype, pick_list[npicks].amplitude, ppick->mwp, ppick->epicentral_distance);
            npicks++;
        }
        fclose(pickStream);
        if (verbose >= 2)
            fprintf(stdout, "Info: successfully read %d input picks from file: %s\n", npicks, pickfile);

        phypocenter->pick_list_size = npicks;
        hypo_read = 1;
        break;
    }

    if (!hypo_read) {
        return (-1);
    }

    return (0);

}

