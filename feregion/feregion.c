
/*

20110120 AJL - Converted to C from perl source at ftp://hazards.cr.usgs.gov/feregion/fe_1995/

#!/usr/local/bin/perl -w

# feregion.pl - returns Flinn-Engdahl Region name from decimal lon,lat values given on command line.

# Version 0.2 - Bob Simpson January, 2003  <simpson@usgs.gov>
#               With fix supplied by George Randall <ger@geophysics.lanl.gov>  2003-02-03
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <math.h>

#include "feregion.h"

#include "fe_names.h"
#include "fe_quadsidx.h"
#include "fe_nesect.h"
#include "fe_nwsect.h"
#include "fe_sesect.h"
#include "fe_swsect.h"

#define QUAD_NE 0
#define QUAD_NW 1
#define QUAD_SE 2
#define QUAD_SW 3

#define ERROR_TEXT_SIZE "ERROR - feregion_str size < 128 char."
#define ERROR_TEXT "ERROR - FE-region for lat=%.1f lon=%.1f not found."

char *feregion(double lat_in, double lon_in, char* feregion_str, int feregion_str_len) {

    if (feregion_str == NULL)
        return (NULL);

    if (feregion_str_len < 128) {
        if (feregion_str_len > (int) strlen(ERROR_TEXT_SIZE)) {
            strcpy(feregion_str, ERROR_TEXT_SIZE);
            return (feregion_str);
        } else {
            return (NULL);
        }
    }

    double lat = lat_in;
    double lon = lon_in;

    // Adjust lat-lon values...
    if (lon <= -180.0) {
        lon += 360.0;
    } else if (lon > 180.0) {
        lon -= 360.0;
    }

    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
        sprintf(feregion_str, ERROR_TEXT, lat_in, lon_in);
        return (feregion_str);
    }


    // Get quadrant
    int iquadrant = -1;
    if (lat >= 0.0 && lon >= 0.0) {
        iquadrant = QUAD_NE;
    } else if (lat >= 0.0 && lon < 0.0) {
        iquadrant = QUAD_NW;
    } else if (lat < 0.0 && lon >= 0.0) {
        iquadrant = QUAD_SE;
    } else if (lat < 0.0 && lon < 0.0) {
        iquadrant = QUAD_SW;
    }

    // Take absolute values...
    double alat = fabs(lat);
    double alon = fabs(lon);

    // Truncate absolute values to integers...
    int ilat = (int) alat;
    int ilon = (int) alon;

    // Find location of the latitude tier in the appropriate quadrant file.
    int ibegin = 0;
    int ilat_count = 0;
    while (ilat_count < ilat && ilat_count < NUM_LAT_FE_QUADSIDX) {
        ibegin += fe_quadsidx[iquadrant][ilat_count];
        ilat_count++;
    }

    int inum = fe_quadsidx[iquadrant][ilat]; // Number of items for latitude lt.

    int *sect = NULL;
    if (iquadrant == QUAD_NE)
        sect = fe_nesect;
    else if (iquadrant == QUAD_NW)
        sect = fe_nwsect;
    else if (iquadrant == QUAD_SE)
        sect = fe_sesect;
    else if (iquadrant == QUAD_SW)
        sect = fe_swsect;

    int n = ibegin;
    while (n < (ibegin + inum) && sect[2 * n] <= ilon)
        n++;
    n--;

    int ifenum = sect[2 * n + 1];
    ifenum--;
    if (ifenum < NUM_FE_NAMES)
        strcpy(feregion_str, fe_names[ifenum]);

    return (feregion_str);

}