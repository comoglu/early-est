/*
 * File:   get_gain.c
 * Author: anthony
 *
 * Created on February 17, 2010, 7:03 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "response_lib.h"

void usage() {
    fprintf(stderr, "USAGE: get_gain query_type(IRIS_WS_RESP|FDSN_WS_STATION) host query_root net sta loc chan year month day frequency\n");
}

/*
 *
 */
int main(int argc, char** argv) {

    if (argc < 12) {
        usage();
        exit(2);
    }

    int n = 0;
    char* internet_gain_query_type = argv[++n];
    char* internet_gain_query_host = argv[++n];
    char* internet_gain_query = argv[++n];
    char* net = argv[++n];
    char* sta = argv[++n];
    char* loc = argv[++n];
    char* chan = argv[++n];
    int year = atoi(argv[++n]);
    int month = atoi(argv[++n]);
    int mday = atoi(argv[++n]);
    double frequency = atof(argv[++n]);

    int responseType = ERROR_TYPE;
    double gain = -1.0;
    if (strcmp(internet_gain_query_type, "IRIS_WS_RESP") == 0) {
        gain = get_gain_internet_gain_query_IRIS_WS_RESP(internet_gain_query_host, internet_gain_query,
            net, sta, loc, chan, year, month, mday, frequency, &responseType);
    } else if (strcmp(internet_gain_query_type, "FDSN_WS_STATION") == 0) {
        gain = get_gain_internet_station_query_FDSN_WS_STATION(internet_gain_query_host, internet_gain_query,
            net, sta, loc, chan, year, month, mday, &frequency, &responseType);
    }

    fprintf(stdout, "\nGain=%e, Freq=%f\n", gain, frequency);
    fprintf(stdout, "Response type=%d (DERIVATIVE_TYPE=%d, DOUBLE_DERIVATIVE_TYPE=%d)\n", responseType, DERIVATIVE_TYPE, DOUBLE_DERIVATIVE_TYPE);

    return (EXIT_SUCCESS);
}

