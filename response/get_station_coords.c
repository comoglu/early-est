/*
 * File:   get_station_coords.c
 * Author: anthony
 *
 * Created on February 17, 2010, 7:03 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "response_lib.h"

void usage() {
    fprintf(stderr, "USAGE: get_station_coords query_type(IRIS_WS_STATION|FDSN_WS_STATION) host query_root net sta loc chan year month day\n");
}

/*
 *
 */
int main(int argc, char** argv) {

    if (argc < 11) {
        usage();
        exit(2);
    }

    int n = 0;
    char* internet_station_query_type = argv[++n];
    char* internet_station_query_host = argv[++n];
    char* internet_station_query = argv[++n];
    char* net = argv[++n];
    char* sta = argv[++n];
    char* loc = argv[++n];
    char* chan = argv[++n];
    int year = atoi(argv[++n]);
    int month = atoi(argv[++n]);
    int mday = atoi(argv[++n]);

    double lat = 0.0;
    double lon = 0.0;
    double elev = 0.0;
    double azimuth = -1.0;
    double dip = -999.0;

    int return_value = -99;
    if (strcmp(internet_station_query_type, "IRIS_WS_STATION") == 0) {
        return_value = get_station_coords_internet_station_query_IRIS_WS_STATION(internet_station_query_host, internet_station_query,
                net, sta, year, month, mday, &lat, &lon, &elev);
    } else if (strcmp(internet_station_query_type, "FDSN_WS_STATION") == 0) {
        return_value = get_station_coords_internet_station_query_FDSN_WS_STATION(internet_station_query_host, internet_station_query,
                net, sta, loc, chan, year, month, mday, &lat, &lon, &elev, &azimuth, &dip);
    }

    fprintf(stdout, "\nReturn value=%d\n", return_value);
    fprintf(stdout, "Lat=%f Lon=%f, Elev=%f\n", lat, lon, elev);

    return (EXIT_SUCCESS);
}

