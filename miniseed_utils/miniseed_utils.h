/***************************************************************************
 * miniseed_utils.h:
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

#include <libmseed.h>



float *msu_getDataAsFloat(MSRecord *msr, int *pflag_clipped);
double *msu_getDataAsDouble(MSRecord *msr, int *pflag_clipped);

int mslist_addMsRecord(MSRecord* pnew_msrecord, MSRecord*** pms_list, int* sizeofmslist, int* pnum_records);
int mslist_removeMsRecords(hptime_t remove_before_time, MSRecord*** pms_list, int* pnum_records);
void mslist_free(MSRecord*** pms_list, int* pnum_records);
int mslist_writeToFile(char* filename, hptime_t start_time, hptime_t end_time, MSRecord** pms_list, int num_records, int verbose, hptime_t *pstart_time_written, hptime_t *pend_time_written);
hptime_t mslist_getStartTime(MSRecord** pms_list, int num_records);
hptime_t mslist_getEndTime(MSRecord** pms_list, int num_records);
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
        );

double *mslist_getDataWindowAsDouble(hptime_t start_time, hptime_t end_time, MSRecord** pms_list, int num_records, int verbose,
        hptime_t *pstart_time_return, hptime_t *pend_time_return, int *pnum_samples_return);