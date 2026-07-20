/***************************************************************************
 * miniseed_utils.c:
 *
 * Utility library for managing time-ordered lists of miniseed records.
 *
 * Copyright (C) 2013 Anthony Lomax <anthony@alomax.net, http://www.alomax.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.

 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 ***************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <limits.h>

#include <libmseed.h>

#include "miniseed_utils.h"

/**  Method to extract msr data as newly allocated float array */

float *msu_getDataAsFloat(MSRecord *msr, int *pflag_clipped) {

    float * data_float = NULL;

    int i;

    // make sure data is in a float array
    //printf("DEBUG: msr->sampletype %c\n", msr->sampletype);
    if (msr->sampletype == 'f') {
        float *data_float_msr = (float *) msr->datasamples;
        data_float = calloc(msr->numsamples, sizeof (float));
        for (i = 0; i < msr->numsamples; i++) {
            data_float[i] = (float) data_float_msr[i];
        }
    } else if (msr->sampletype == 'd') { // 20140407 AJL - added 'd' type
        double *data_double_msr = (double *) msr->datasamples;
        data_float = calloc(msr->numsamples, sizeof (float));
        for (i = 0; i < msr->numsamples; i++) {
            data_float[i] = (float) data_double_msr[i];
        }
    } else if (msr->sampletype == 'i') {
        int32_t *data_int_msr = (int32_t *) msr->datasamples;
        data_float = calloc(msr->numsamples, sizeof (float));
        long idata_min = LONG_MAX;
        long idata_max = LONG_MIN;
        long idata_last = 0L;
        int iclip_counter = 0;
        for (i = 0; i < msr->numsamples; i++) {
            long idata = data_int_msr[i];
            data_float[i] = (float) data_int_msr[i];
            // clip test
            if (idata < idata_min)
                idata_min = idata;
            if (idata > idata_max)
                idata_max = idata;
            if (idata_last != 0L && idata == idata_last && (idata == idata_min || idata == idata_max)) {
                iclip_counter++;
                if (iclip_counter > 5) {
                    *pflag_clipped = 1;
                    iclip_counter = 0;
                }
            } else {
                iclip_counter = 0;
            }
            idata_last = idata;
        }
    }

    return (data_float);
}

/**  Method to extract msr data as newly allocated double array */

double *msu_getDataAsDouble(MSRecord *msr, int *pflag_clipped) {

    double * data_double = NULL;

    int i;

    // make sure data is in a double array
    //printf("DEBUG: msr->sampletype %c\n", msr->sampletype);
    if (msr->sampletype == 'f') {
        float *data_float_msr = (float *) msr->datasamples;
        data_double = calloc(msr->numsamples, sizeof (double));
        for (i = 0; i < msr->numsamples; i++) {
            data_double[i] = (double) data_float_msr[i];
        }
    } else if (msr->sampletype == 'd') { // 20140407 AJL - added 'd' type
        double *data_double_msr = (double *) msr->datasamples;
        data_double = calloc(msr->numsamples, sizeof (double));
        for (i = 0; i < msr->numsamples; i++) {
            data_double[i] = data_double_msr[i];
        }
    } else if (msr->sampletype == 'i') {
        int32_t *data_int_msr = (int32_t *) msr->datasamples;
        data_double = calloc(msr->numsamples, sizeof (double));
        long idata_min = LONG_MAX;
        long idata_max = LONG_MIN;
        long idata_last = 0L;
        int iclip_counter = 0;
        for (i = 0; i < msr->numsamples; i++) {
            long idata = data_int_msr[i];
            data_double[i] = (double) data_int_msr[i];
            // clip test
            if (idata < idata_min)
                idata_min = idata;
            if (idata > idata_max)
                idata_max = idata;
            if (idata_last != 0L && idata == idata_last && (idata == idata_min || idata == idata_max)) {
                iclip_counter++;
                if (iclip_counter > 5) {
                    *pflag_clipped = 1;
                    iclip_counter = 0;
                }
            } else {
                iclip_counter = 0;
            }
            idata_last = idata;
        }
    }

    return (data_double);
}

/** add a miniseed record to a miniseed record list
 * list will be sorted by increasing msrecord->starttime
 */

#define SIZE_INCREMENT_LIST 128

int mslist_addMsRecord(MSRecord* pnew_msrecord, MSRecord*** pms_list, int* sizeofmslist, int* pnum_records) {

    // 20220202 AJL - bug fix, added sizeofXXX parameter to hold current size of allocated array

    // get copy of new MSRecord
    MSRecord* pdup_msr = msr_duplicate(pnew_msrecord, 1);
    if (pdup_msr == NULL) {
        ms_log(1, "Warning: mslist_addMsRecord: cannot duplicate miniseed record: %s_%s_%s_%s nrec=%d.\n",
                pnew_msrecord->network, pnew_msrecord->station, pnew_msrecord->location, pnew_msrecord->channel, *pnum_records);
        return (-1);
    }

    if (*pms_list == NULL) { // list not yet created
        *pms_list = calloc(SIZE_INCREMENT_LIST, sizeof (MSRecord*));
        *sizeofmslist = SIZE_INCREMENT_LIST;
        int n;
        for (n = 0; n < SIZE_INCREMENT_LIST; n++) {
            (*pms_list)[n] = NULL;
        }
        // 20130930 AJL - bug fix
        //} else if (*pnum_records != 0 && (*pnum_records % SIZE_INCREMENT_LIST) == 0) { // list will be too small
    } else if (*pnum_records + 1 > *sizeofmslist) { // list will be too small
        MSRecord** new_ms_list = NULL;
        new_ms_list = calloc(*pnum_records + SIZE_INCREMENT_LIST, sizeof (MSRecord*));
        *sizeofmslist = *pnum_records + SIZE_INCREMENT_LIST;
        int n;
        for (n = 0; n < *pnum_records; n++) {
            new_ms_list[n] = (*pms_list)[n];
        }
        free(*pms_list);
        for (n = *pnum_records; n < *pnum_records + SIZE_INCREMENT_LIST; n++) {
            new_ms_list[n] = NULL;
        }
        *pms_list = new_ms_list;
    }

    // find first msrecord later than new msrecord
    int ninsert = 0;
    for (ninsert = 0; ninsert < *pnum_records; ninsert++) {
        if ((*pms_list)[ninsert] == NULL) {
            ms_log(1, "Warning: NULL pointer mseed record in ms_list: %s_%s_%s_%s nrec=%d: this should not happen!\n",
                    pnew_msrecord->network, pnew_msrecord->station, pnew_msrecord->location, pnew_msrecord->channel, *pnum_records);
            {
                FILE* fp_debug = fopen("DEBUG.log", "a");
                time_t tp = time(NULL);
                fprintf(fp_debug, "%s Warning: NULL pointer mseed record in ms_list: %s_%s_%s_%s nrec=%d: this should not happen!\n",
                        asctime(gmtime(&tp)), pnew_msrecord->network, pnew_msrecord->station, pnew_msrecord->location, pnew_msrecord->channel, *pnum_records);
                fclose(fp_debug);
            }
            continue;
        }
        if ((*pms_list)[ninsert]->starttime > pnew_msrecord->starttime) {
            break;
        }
    }
    // shift later records
    if (ninsert < *pnum_records) {
        int m;
        for (m = *pnum_records - 1; m >= ninsert; m--) {
            (*pms_list)[m + 1] = (*pms_list)[m];
        }
    }
    // insert copy of new MSRecord
    (*pms_list)[ninsert] = pdup_msr;
    (*pnum_records)++;

    return (1);

}

/** remove all miniseed records in a specified time window from a miniseed record list */

int mslist_removeMsRecords(hptime_t remove_before_time, MSRecord*** pms_list, int* pnum_records) {

    if (*pnum_records < 1) {
        return (0);
    }

    // find first record overlapping or after remove_before_time
    int istart = 0;
    while (istart < *pnum_records) {
        if (msr_endtime((*pms_list)[istart]) >= remove_before_time) {
            break;
        }
        istart++;
    }
    if (istart == 0) { // all records end at or after remove_before_time
        return (0);
    }

    // shift records at or after remove_before_time to beginning of list
    int inew = 0;
    int i = istart;
    //20131016 AJL - bug fix?
    //while (i < *pnum_records && (*pms_list)[i] != NULL) {
    while (i < *pnum_records) {
        if ((*pms_list)[inew] != NULL) {
            //printf("DEBUG: (*pms_list)[inew] %ld, inew %d, istart %d\n", (*pms_list)[inew], inew, istart);
            msr_free(*pms_list + inew);
        }
        (*pms_list)[inew] = (*pms_list)[i];
        (*pms_list)[i] = NULL;
        i++;
        inew++;
    }
    // free records between new position of last preserved record (inew-1) and original position of first preserved record (istart)
    for (i = inew; i < istart && i < *pnum_records; i++) {
        msr_free(*pms_list + i);
        (*pms_list)[i] = NULL;
    }
    *pnum_records = inew;

    return (istart);

}

/** clean up list memory */

void mslist_free(MSRecord*** pms_list, int* pnum_records) {

    if (*pms_list == NULL)
        return;

    int n;
    for (n = 0; n < *pnum_records; n++) {
        msr_free(*pms_list + n);
    }
    *pnum_records = 0;

    free(*pms_list);
    *pms_list = NULL;

}

static char *write_filename;

static void record_handler(char *record, int write_reclen, void *fpoutstream) {

    if (fwrite(record, write_reclen, 1, (FILE*) fpoutstream) != 1) {
        ms_log(2, "Error writing %s to output file.\n", write_filename);
    }

}

int mslist_writeToFile(char* filename, hptime_t start_time, hptime_t end_time, MSRecord** pms_list, int num_records, int verbose,
        hptime_t *pstart_time_written, hptime_t *pend_time_written) {

    int64_t nsamples;
    char srcname[50];
    int precords;

    FILE *fpoutstream = fopen(filename, "w");
    if (fpoutstream == NULL) {
        printf("Info: miniseed_utils.mslist_writeToFile(): cannot open file: %s\n", filename);
        return (-1);
    }

    msr_srcname(pms_list[0], srcname, 0);

    *pstart_time_written = -1;
    hptime_t end_time_record;
    int num_samples_written = 0;
    int n;
    MSRecord* msr;
    for (n = 0; n < num_records; n++) {
        msr = pms_list[n];
        //printf("DEBUG: starttime-start_time=%ld, msr_endtime-start_time=%ld, end_time-start_time=%ld\n",
        //(msr->starttime - start_time) / HPTMODULUS, (msr_endtime(msr) - start_time) / HPTMODULUS, (end_time - start_time) / HPTMODULUS);
        if ((end_time_record = msr_endtime(msr)) >= start_time && msr->starttime <= end_time) {
            if (*pstart_time_written < 0) {
                *pstart_time_written = msr->starttime;
            }
            *pend_time_written = end_time_record;
            // Pack copy of the record(s) and write to outstream (see record_handler)
            MSRecord* dup_msr = msr_duplicate(msr, 1);
            precords = msr_pack(dup_msr, record_handler, fpoutstream, &nsamples, 1, verbose - 2);
            num_samples_written += nsamples;
            // extern int msr_pack (MSRecord *msr, void (*record_handler) (char *, int, void *), void *handlerdata, int64_t *packedsamples, flag flush, flag verbose );
            if (verbose > 3)
                ms_log(0, "Info: mslist_writeToFile.msr_pack(): Packed %d samples into %d records.\n", nsamples, precords);
            msr_free(&dup_msr);
        }
    }

    fclose(fpoutstream);

    return (num_samples_written);

}

/** get start time of first miniseed records in a miniseed record list */

hptime_t mslist_getStartTime(MSRecord** pms_list, int num_records) {

    if (num_records < 1) {
        return (-1);
    }

    return (pms_list[0]->starttime);

}

/** get end time of last miniseed records in a miniseed record list */

hptime_t mslist_getEndTime(MSRecord** pms_list, int num_records) {

    if (num_records < 1) {
        return (-1);
    }

    return (msr_endtime(pms_list[num_records - 1]));

}

/** replace all occurrences in string of c1 with c2 */

char* replace(char* string, char c1, char c2) {

    char* pc = string;
    while (*pc) {
        if (*pc == c1) {
            *pc = c2;
        }
        pc++;
    }

    return (string);

}

/** Write channel header information to an SeisGram2K ASCII output stream.
 *
 */
int writeSG2Kheader(char* filename, char* evtName,
        int iyear, int ijday, int ihour, int imin, double sec, double timeMin,
        char* network, char* staName, char* instName, char* chanName, char* locName, char* compName,
        double comp_azimuth, double comp_inclination,
        double sta_latitude, double sta_longitude, double sta_depth, double sta_elevation, double gain,
        int sampleLength, double samplingInterval, char* ampUnits, char* timeUnits,
        double hypocenter_latitude, double hypocenter_longitude, double hypocenter_depth,
        char* hypocenter_otime_comma_delimited_str,
        double hypocenter_ms, double hypocenter_mb, double hypocenter_mw, double hypocenter_ml, double hypocenter_mo, double hypocenter_mag,
        double gcarc, double dist, double az, double baz,
        int write_hypo_hdr
        ) {

    FILE *fpoutstream = fopen(filename, "w");
    if (fpoutstream == NULL) {
        printf("Info: miniseed_utils.writeSG2Kheader(): cannot open file: %s\n", filename);
        return (-1);
    }

    fprintf(fpoutstream, "SG2K_ASCII ");
    fprintf(fpoutstream, " ");

    // SG2K BasicSeismogram
    if (evtName != NULL)
        fprintf(fpoutstream, "event=%s ", replace(evtName, ' ', '_'));
    fprintf(fpoutstream, " ");
    fprintf(fpoutstream, "year=%d ", iyear);
    fprintf(fpoutstream, "jday=%d ", ijday);
    fprintf(fpoutstream, "hour=%d ", ihour);
    fprintf(fpoutstream, "min=%d ", imin);
    fprintf(fpoutstream, "sec=%f ", sec);
    fprintf(fpoutstream, " ");
    fprintf(fpoutstream, "begTime=%f ", timeMin); // offset in seconds of first sample from data/time
    fprintf(fpoutstream, " ");

    // SG2K BasicChannel
    if (network != NULL)
        fprintf(fpoutstream, "network=%s ", replace(network, ' ', '_'));
    if (staName != NULL)
        fprintf(fpoutstream, "sta=%s ", replace(staName, ' ', '_'));
    if (instName != NULL)
        fprintf(fpoutstream, "inst=%s ", replace(instName, ' ', '_'));
    if (chanName != NULL)
        fprintf(fpoutstream, "chan=%s ", replace(chanName, ' ', '_'));
    if (locName != NULL)
        fprintf(fpoutstream, "loc=%s ", replace(locName, ' ', '_'));
    if (compName != NULL)
        fprintf(fpoutstream, "comp=%s ", replace(compName, ' ', '_'));
    fprintf(fpoutstream, "comp.az=%f ", comp_azimuth);
    fprintf(fpoutstream, "comp.inc=%f ", comp_inclination);
    fprintf(fpoutstream, "sta.lat=%f ", sta_latitude);
    fprintf(fpoutstream, "sta.lon=%f ", sta_longitude);
    fprintf(fpoutstream, "sta.depth=%f ", sta_depth);
    fprintf(fpoutstream, "sta.elev=%f ", sta_elevation);
    fprintf(fpoutstream, "gain=%e ", gain);
    fprintf(fpoutstream, " ");

    // TimeSeries
    fprintf(fpoutstream, "nPoints=%d ", sampleLength);
    fprintf(fpoutstream, "sampleInt=%f ", samplingInterval);
    fprintf(fpoutstream, " ");
    if (ampUnits != NULL)
        fprintf(fpoutstream, "ampUnits=%s ", replace(ampUnits, ' ', '_'));
    if (timeUnits != NULL)
        fprintf(fpoutstream, "timeUnits=%s ", replace(timeUnits, ' ', '_'));
    fprintf(fpoutstream, " ");

    // BasicHypocenter
    if (write_hypo_hdr) {
        fprintf(fpoutstream, "hypo.lat=%f ", hypocenter_latitude);
        fprintf(fpoutstream, "hypo.lon=%f ", hypocenter_longitude);
        fprintf(fpoutstream, "hypo.depth=%f ", hypocenter_depth);
        if (hypocenter_otime_comma_delimited_str != NULL)
            fprintf(fpoutstream, "hypo.otime=%s ", hypocenter_otime_comma_delimited_str);
        fprintf(fpoutstream, "hypo.ms=%f ", hypocenter_ms);
        fprintf(fpoutstream, "hypo.mb=%f ", hypocenter_mb);
        fprintf(fpoutstream, "hypo.mw=%f ", hypocenter_mw);
        fprintf(fpoutstream, "hypo.ml=%f ", hypocenter_ml);
        fprintf(fpoutstream, "hypo.mo=%f ", hypocenter_mo);
        fprintf(fpoutstream, "hypo.mag=%f ", hypocenter_mag);
        fprintf(fpoutstream, " ");

        // BasicSeismogram - hypocenter
        fprintf(fpoutstream, "gcarc=%f ", gcarc);
        fprintf(fpoutstream, "dist=%f ", dist);
        fprintf(fpoutstream, "az=%f ", az);
        fprintf(fpoutstream, "baz=%f ", baz);
    }

    fprintf(fpoutstream, "\n");

    fclose(fpoutstream);

    return (0);

}

/**  Method to extract data array with specified start and stop times to newly allocated array */

double *mslist_getDataWindowAsDouble(hptime_t start_time, hptime_t end_time, MSRecord** pms_list, int num_records, int verbose,
        hptime_t *pstart_time_return, hptime_t *pend_time_return, int *pnum_samples_return) {

    *pend_time_return = -1;
    *pstart_time_return = -1;
    *pnum_samples_return = 0;

    int numsamples_est = 1 + (int) ((pms_list[0]->samprate * (double) (end_time - start_time)) / (double) HPTMODULUS);
    numsamples_est += 32; // fudge factor if time/sample rounding error
    double *data_double = calloc(numsamples_est, sizeof (double));

    *pstart_time_return = -1;
    hptime_t end_time_record;
    int num_samples_return = 0;
    int flag_clipped = 0;
    MSRecord* msr;
    int n;
    for (n = 0; n < num_records; n++) {
        msr = pms_list[n];
        //printf("DEBUG: starttime-start_time=%ld, msr_endtime-start_time=%ld, end_time-start_time=%ld\n",
        //(msr->starttime - start_time) / HPTMODULUS, (msr_endtime(msr) - start_time) / HPTMODULUS, (end_time - start_time) / HPTMODULUS);
        if ((end_time_record = msr_endtime(msr)) >= start_time && msr->starttime <= end_time) {
            double *data_double_msr = msu_getDataAsDouble(msr, &flag_clipped);
            // set start index in data array
            int istart = 0;
            if (*pstart_time_return < 0) { // have not yet passed start time
                if (start_time >= msr->starttime) {
                    /* Define the high precision time tick interval as 1/modulus seconds */
                    /* Default modulus of 1000000 defines tick interval as a microsecond */
                    //#define HPTMODULUS 1000000
                    istart = (int) ((msr->samprate * (double) (start_time - msr->starttime)) / (double) HPTMODULUS);
                    *pstart_time_return = start_time;
                } else {
                    *pstart_time_return = msr->starttime;
                }
            }
            // set end index in data array
            *pend_time_return = end_time_record;
            int iend = msr->numsamples;
            if (end_time_record >= end_time) { // last record needed
                iend = 1 + (int) ((msr->samprate * (double) (end_time - msr->starttime)) / (double) HPTMODULUS);
                iend = iend > msr->numsamples ? msr->numsamples : iend; // in case of rounding up past end of data array
                *pend_time_return = end_time;
            }
            for (int i = istart; i < iend && num_samples_return < numsamples_est; i++) {
                data_double[num_samples_return] = data_double_msr[i];
                num_samples_return++;
            }
            free(data_double_msr);
            if (num_samples_return >= numsamples_est || end_time_record >= end_time) {
                break;
            }
        }
    }

    //*pend_time_return = *pstart_time_return + (hptime_t) (((double) (num_samples_return - 1) * (double) HPTMODULUS) / msr->samprate);
    *pnum_samples_return = num_samples_return;

    if (verbose > 3)
        ms_log(0, "Info: mslist_getDataWindow(): Returned %d samples (%fsec) from %ld to %ld.\n",
            num_samples_return, (double) num_samples_return / pms_list[0]->samprate, *pstart_time_return, *pend_time_return);

    return (data_double);

}

