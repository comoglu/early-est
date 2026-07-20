/*
 * File:   response_lib.h
 * Author: anthony
 *
 * Created on February 17, 2010, 7:04 AM
 */

#ifndef _REPONSE_LIB_H
#define _REPONSE_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../math/complex.h"

#define BOOLEAN int
#define TRUE 1
#define FALSE 0

    //#define PI 3.14159265359
#define PI M_PI

#define MAX_NUM_PZ 100

#define ERROR_TYPE -1
#define UNKNOWN_TYPE 0
#define SCALING_TYPE 1
#define INTEGRAL_TYPE 2
#define DERIVATIVE_TYPE 3
#define DOUBLE_DERIVATIVE_TYPE 4
#define OTHER_CONVERSION_TYPE 5
#define NO_CONVERSION_TYPE 6

    /** channel data structure/class */

    typedef struct {
        int type;
        double frequency;
        double gain;
    }
    ResponseChannelData;

    /** pole-zero response */

    typedef struct {
        int type;
        double gain;
        int num_poles;
        int num_zeros;
        fcomplex poles[MAX_NUM_PZ];
        fcomplex zeros[MAX_NUM_PZ];
    }
    PoleZeroResponse;




    /* functions */

    double get_gain_internet_gain_query_IRIS_WS_RESP(char* internet_pole_zero_query_host, char* internet_pole_zero_query,
            char* prtnet, char* prtsta, char* prtloc, char* prtchan, int year, int month, int mday, double frequency,
            int* presponseType);
    double get_gain_internet_station_query_FDSN_WS_STATION(char* internet_pole_zero_query_host, char* internet_pole_zero_query,
            char* prtnet, char* prtsta, char* prtloc, char* prtchan, int year, int month, int mday, double* frequency,
            int* presponseType);
    double get_disp_gain_seed_resp(char* seed_resp, int n_char_total, double frequency, int* presponseType);
    PoleZeroResponse readSEED_RESP(char* seed_resp_str, int n_char_total);
    double get_gain_xml(char* xml_str, double* pfrequency, int* presponseType);
    fcomplex evaluateResponse(PoleZeroResponse poleZeroResp, double frequency);
    int get_station_coords_internet_station_query_FDSN_WS_STATION(char* internet_station_query_host, char* internet_station_query,
            char* prtnet, char* prtsta, char* prtloc, char* prtchan, int year, int month, int mday,
            double* plat, double* plon, double* pelev, double* pazimuth, double* pdip);
    int get_station_coords_internet_station_query_IRIS_WS_STATION(char* internet_station_query_host, char* internet_station_query,
            char* prtnet, char* prtsta, int year, int month, int mday, double* plat, double* plon, double* pelev);
    int get_station_coords_xml(int use_channel, char* xml_str, double* plat, double* plon, double* pelev, double* pazimuth, double* pdip);


#ifdef __cplusplus
}
#endif

#endif /* _REPONSE_LIB_H */

