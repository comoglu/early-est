#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "response_lib.h"
#include "../net/net_lib.h"

#define DEBUG 0

/**
 * get_gain_internet_station_query_FDSN_WS_STATION:
 *
 * returns gain from FDSNStationXML InstrumentSensitivity Value
 *
 * TODO: add doc.
 */
double get_gain_internet_station_query_FDSN_WS_STATION(char* internet_gain_query_host, char* internet_gain_query,
        char* prtnet, char* prtsta, char* prtloc, char* prtchan, int year, int month, int mday, double *pfrequency,
        int* presponseType) {

    // TODO: Temporary? fix to bug that IRIS service.iris.edu/fdsnws/station/1 does not work with loc=--
    // service.iris.edu/fdsnws/station/1/query?net=II&sta=AAK&loc=--&cha=BHZ&starttime=1992-12-12&endtime=1992-12-12&level=station
    static char locstr[64];
    if (strlen(prtloc) > 0 && strcmp(prtloc, "--") != 0) {
        sprintf(locstr, "&loc=%s", prtloc);
    } else {
        strcpy(locstr, "");
    }

    static char page[4096];
    // "ws/station/query?net=IU&sta=SAML&loc=00&chan=BHZ&timewindow=2010-02-27,2010-02-27&level=station"
    // http://service.iris.edu/fdsnws/station/1/query?net=IU&sta=ANMO&cha=BHZ&loc=00&starttime=2013-09-05&endtime=2013-09-05&level=station
    sprintf(page, "%s?net=%s&sta=%s%s&cha=%s&starttime=%4.4d-%2.2d-%2.2d&endtime=%4.4d-%2.2d-%2.2d&level=channel",  // 20171212 AJL - bug fix, changed chan to channel
            internet_gain_query, prtnet, prtsta, locstr, prtchan, year, month, mday, year, month, mday);
    fprintf(stdout, "Query: http://%s/%s\n", internet_gain_query_host, page);

    //struct sockaddr_in *internet_station_socket_remote;
    int internet_station_socket = -1;
    //internet_station_socket = get_socket_connection(internet_gain_query_host, &internet_station_socket_remote);
    internet_station_socket = get_socket_connection(internet_gain_query_host, "http");

    int page_length = 0;
    char *page_contents = get_page(internet_gain_query_host, internet_station_socket, page, &page_length);
    if (page_contents == NULL) {
        shutdown(internet_station_socket, SHUT_RDWR);
        close(internet_station_socket);
        //free(internet_station_socket_remote);
        return (-1);
    }

    fprintf(stdout, "%100.100s\n", page_contents);

    double gain = -1.0;
    gain = get_gain_xml(page_contents, pfrequency, presponseType);
    if (DEBUG) printf("get_gain_internet_station_query_FDSN_WS_STATION: gain=%f freq=%f presponseType=%d\n", gain, *pfrequency, *presponseType);

    free(page_contents);

    shutdown(internet_station_socket, SHUT_RDWR);
    close(internet_station_socket);
    //free(internet_station_socket_remote);

    return (gain);

}

/**
 * get_gain_internet_gain_query_IRIS_WS_RESP:
 *
 * returns gain from recording units (e.g. M/S) to raw data units (e.g. COUNTS)
 * *presponseType contains the recording units type relative to displacement (e.g. DERIVATIVE_TYPE)
 *
 * TODO: add doc.
 */
double get_gain_internet_gain_query_IRIS_WS_RESP(char* internet_gain_query_host, char* internet_gain_query,
        char* prtnet, char* prtsta, char* prtloc, char* prtchan, int year, int month, int mday, double frequency,
        int* presponseType) {

    static char page[4096];
    // "resp/resp.do?net=G&sta=TAM&loc=--&cha=BHZ&time=1994.160"
    // ws/resp/query?net=IU&sta=ANMO&loc=00&cha=BHZ&time=2003-12-01T12:00:00
    sprintf(page, "%s?net=%s&sta=%s&loc=%s&cha=%s&time=%4.4d-%2.2d-%2.2dT00:00:00",
            internet_gain_query, prtnet, prtsta, prtloc, prtchan, year, month, mday);
    fprintf(stdout, "Query: http://%s/%s\n", internet_gain_query_host, page);

    //struct sockaddr_in *internet_gain_socket_remote = NULL;
    int internet_gain_socket = -1;
    //internet_gain_socket = get_socket_connection(internet_gain_query_host, &internet_gain_socket_remote);
    internet_gain_socket = get_socket_connection(internet_gain_query_host, "http");

    int page_length = 0;
    char *page_contents = get_page(internet_gain_query_host, internet_gain_socket, page, &page_length);
    if (page_contents == NULL) {
        shutdown(internet_gain_socket, SHUT_RDWR);
        close(internet_gain_socket);
        //free(internet_gain_socket_remote);
        return (-1.0);
    }

    fprintf(stdout, "%100.100s\n", page_contents);

    double gain = get_disp_gain_seed_resp(page_contents, page_length, frequency, presponseType);

    free(page_contents);

    shutdown(internet_gain_socket, SHUT_RDWR);
    close(internet_gain_socket);
    //free(internet_gain_socket_remote);

    return (gain);

}

/**
 * get_gain_seeed_resp:
 *
 * TODO: add doc.
 */
double get_disp_gain_seed_resp(char* seed_resp_str, int page_length, double frequency, int* presponseType) {

    PoleZeroResponse poleZeroResp = readSEED_RESP(seed_resp_str, page_length);
    *presponseType = poleZeroResp.type;

    fcomplex response = evaluateResponse(poleZeroResp, frequency);

    double gain = Cmag(response);
    if (DEBUG) printf("get_disp_gain_seed_resp: gain: %g\n", gain);

    // check response type - we want to return gain for raw data type,
    // while response already includes conversion to displacement (e.g. as extra zeros for each integration)
    // so we need to correct gain to compensate (remove) the effective conversion to displacement in the response function
    if (poleZeroResp.type == INTEGRAL_TYPE) {
        gain *= 2.0 * PI * frequency; // integral is multiplication by omega
        if (DEBUG) printf("get_disp_gain_seed_resp: Response.INTEGRAL\n");
    } else if (poleZeroResp.type == DERIVATIVE_TYPE) { // raw data is velocity
        gain /= 2.0 * PI * frequency; // Derivative is division by omega
        if (DEBUG) printf("get_disp_gain_seed_resp: Response.DERIVATIVE\n");
    } else if (poleZeroResp.type == DOUBLE_DERIVATIVE_TYPE) { // raw data is acceleration
        gain /= 2.0 * PI * frequency; // Derivative is division by omega
        gain /= 2.0 * PI * frequency; // Derivative is division by omega
        if (DEBUG) printf("get_disp_gain_seed_resp: Response.DOUBLE_DERIVATIVE\n");
    } else { // raw data is displacement
        ;
        if (DEBUG) printf("get_disp_gain_seed_resp: Response.NONE\n");
    }

    if (DEBUG) printf("get_disp_gain_seed_resp: gain: %g\n", gain);

    return (gain);

}

/**
 * get_line: read a '\n' terminated line from string into line_str, return length of line_str.
 *
 * read a maximum of limit-1 characters
 * line_str is '\0' terminated and includes the terminating '\n' character.
 *
 */
int get_line(char *line_str, int limit, char *string) {

    char* string_ptr = string;
    int i;
    int c = 0;

    for (i = 0; i < limit - 1 && (c = *(string_ptr++)) != '\0' && c != '\n'; i++) {
        line_str[i] = c;
    }
    if (c == '\n') {
        line_str[i] = c;
        i++;
    }
    line_str[i] = '\0';

    return (i);
}

#define MAX_LINE_LEN 16383
char line_tmp[MAX_LINE_LEN + 1];

/**
 * Utiltiy funciton from ah_resp.c
 */
double calc_A0(fcomplex* poles, int num_poles, fcomplex* zeros, int num_zeros, double ref_freq) {
    int i;

    fcomplex numer;
    fcomplex denom;
    fcomplex f0;
    fcomplex hold;

    double a0;


    f0.r = 0;

    f0.i = 2 * PI * ref_freq;

    hold.i = zeros[0].i;
    hold.r = zeros[0].r;
    denom = Csub(f0, hold);

    for (i = 1; i < num_zeros; i++) {
        hold.i = zeros[i].i;
        hold.r = zeros[i].r;

        denom = Cmul(denom, Csub(f0, hold));

    }

    hold.i = poles[0].i;
    hold.r = poles[0].r;

    numer = Csub(f0, hold);

    for (i = 1; i < num_poles; i++) {
        hold.i = poles[i].i;
        hold.r = poles[i].r;

        numer = Cmul(numer, Csub(f0, hold));

    }

    a0 = Cmag(Cdiv(numer, denom));

    return a0;

}

/**
 * reads a SEED_RESP-format pole-zero file.
 *
 * converted from net.alomax.freq.PoleZeroResponse.java
 *
 */
PoleZeroResponse readSEED_RESP(char* seed_resp_str, int page_length) {

    /*
    B053F03     Transfer function type:                A
    B053F04     Stage sequence number:                 1
    B053F05     Response in units lookup:              M/S - Velocity in Meters Per Second
    B053F06     Response out units lookup:             V - Volts
    B053F07     A0 normalization factor:               +8.60830E+04
    B053F08     Normalization frequency:               +2.00000E-02
    B053F09     Number of zeroes:                      2
    B053F14     Number of poles:                       5
    #              Complex zeroes:
    #              i  real          imag          real_error    imag_error
    B053F10-13     0  +0.00000E+00  +0.00000E+00  +0.00000E+00  +0.00000E+00
    B053F10-13     1  +0.00000E+00  +0.00000E+00  +0.00000E+00  +0.00000E+00
    #              Complex poles:
    #              i  real          imag          real_error    imag_error
    B053F15-18     0  -5.94313E+01  +0.00000E+00  +0.00000E+00  +0.00000E+00
    B053F15-18     1  -2.27121E+01  +2.71065E+01  +0.00000E+00  +0.00000E+00
    B053F15-18     2  -2.27121E+01  -2.71065E+01  +0.00000E+00  +0.00000E+00
    B053F15-18     3  -4.80040E-03  +0.00000E+00  +0.00000E+00  +0.00000E+00
    B053F15-18     4  -7.31990E-02  +0.00000E+00  +0.00000E+00  +0.00000E+00


    B058F03     Stage sequence number:                 0
    B058F04     Sensitivity:                           +8.64730E+08
    B058F05     Frequency of sensitivity:              +2.00000E-02
    B058F06     Number of calibrations:                0
     */

    int i;

    PoleZeroResponse poleZeroResp;
    poleZeroResp.type = UNKNOWN_TYPE;
    poleZeroResp.gain = 1.0; // default gain is 1.0
    poleZeroResp.num_poles = 0;
    poleZeroResp.num_zeros = 0;


    BOOLEAN notSupported = FALSE;

    BOOLEAN inTransferFunctionType_A = FALSE;
    BOOLEAN inTransferFunctionType_B = FALSE;
    BOOLEAN isTransferFunctionType_B = FALSE;

    BOOLEAN inStageSequence_0 = FALSE;
    BOOLEAN inStageSequence_1 = FALSE;

    int gamma = -1;
    double A0_norm = DBL_MAX;
    double f_norm = DBL_MAX;
    double Sd_chan = DBL_MAX;
    double f_chan = DBL_MAX;

    int iNumPoles = 0;
    int iNumZeros = 0;

    char* word_token;
    int n_char_total = 0;
    int n_char = 0;
    while (n_char_total < page_length && (n_char = get_line(line_tmp, MAX_LINE_LEN, seed_resp_str + n_char_total)) > 0) {

        n_char_total += n_char;

        // empty lines
        word_token = strtok(line_tmp, " \n");
        if (word_token == NULL)
            continue;

        // a comment line

        if (word_token[0] == '*' || word_token[0] == '#') {
            continue;
        }

        // check for keywords

        if (0 == strcmp(word_token, "B053F03")) {
            //B053F03     Transfer function type:                A
            inTransferFunctionType_A = FALSE;
            inTransferFunctionType_B = FALSE;
            while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
            word_token = strtok(NULL, " \n");
            if (0 == strcmp(word_token, "A")) {
                inTransferFunctionType_A = TRUE;
            } else if (0 == strcmp(word_token, "B")) {
                inTransferFunctionType_B = TRUE;
                isTransferFunctionType_B = TRUE;
            }
            continue;
        }
        if (0 == strcmp(word_token, "B053F04") || 0 == strcmp(word_token, "B058F03")) {
            //B053F04     Stage sequence number:                 1
            //B058F03     Stage sequence number:                 0
            inStageSequence_0 = FALSE;
            inStageSequence_1 = FALSE;
            while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
            word_token = strtok(NULL, " \n");
            if (0 == strcmp(word_token, "0")) {
                inStageSequence_0 = TRUE;
            } else if (0 == strcmp(word_token, "1")) {
                inStageSequence_1 = TRUE;
            }
            continue;
        }

        if ((inTransferFunctionType_A || inTransferFunctionType_B) && inStageSequence_1) {
            if (0 == strcmp(word_token, "B053F05")) {
                //B053F05     Response in units lookup:              M/S - Velocity in Meters Per Second
                /*
                 * ah_resp.c
                 * Convert to displacement :
                 * if acceleration, gamma=2    \
                 * elseif velocity,     gamma=1 \
                 * elseif displacement, gamma=0  \___Done above
                 */
                while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
                word_token = strtok(NULL, " \n");
                if (0 == strcmp(word_token, "M")) {
                    poleZeroResp.type = NO_CONVERSION_TYPE;
                    gamma = 0;
                    continue;
                } else if (0 == strcmp(word_token, "M/S")) {
                    poleZeroResp.type = DERIVATIVE_TYPE;
                    gamma = 1;
                    continue;
                } else if (0 == strcmp(word_token, "NM/S")) {
                    poleZeroResp.type = DERIVATIVE_TYPE;
                    gamma = 1;
                    continue;
                } else if (0 == strcmp(word_token, "M/S**2")) {
                    poleZeroResp.type = DOUBLE_DERIVATIVE_TYPE;
                    gamma = 2;
                    continue;
                }
                notSupported = TRUE;
                break;
            } else if (0 == strcmp(word_token, "B053F06")) {
                //B053F06     Response out units lookup:             V - Volts
                while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
                word_token = strtok(NULL, " \n");
                if (0 == strcmp(word_token, "V")) {
                    continue;
                }
                if (0 == strcmp(word_token, "COUNTS")) { // NOTE: AJL 20100212 Is it OK to simply do nothing special if COUNTS instead of V, try: http://www.iris.edu/resp/resp.do?net=CN&sta=GAC&loc=--&cha=BHZ&time=2010.001
                    continue; // uncomment here if OK
                }
                notSupported = TRUE;
                break;
            } else if (0 == strcmp(word_token, "B053F07")) {
                //B053F07     A0 normalization factor:               +8.60830E+04
                while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
                A0_norm = atof(strtok(NULL, " \n"));
                continue;
            } else if (0 == strcmp(word_token, "B053F08")) {
                //B053F08     Normalization frequency:               +2.00000E-02
                while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
                f_norm = atof(strtok(NULL, " \n"));
                continue;
            } else if (0 == strcmp(word_token, "B053F09")) {
                //B053F09     Number of zeroes:                      2
                while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
                iNumZeros = atoi(strtok(NULL, " \n"));
                if (iNumZeros < 0) {
                    fprintf(stderr, "ERROR: response.readSEED_RESP: invalid_number_of_zeros: %d\n", iNumZeros);
                }
                for (i = 0; i < iNumZeros; i++) {
                    poleZeroResp.zeros[i] = Cmplx(0.0, 0.0); // unspecified zeros will be at origin
                }
                continue;
            } else if (0 == strcmp(word_token, "B053F14")) {
                //B053F14     Number of poles:                       5
                while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
                iNumPoles = atoi(strtok(NULL, " \n"));
                if (iNumPoles < 0) {
                    fprintf(stderr, "ERROR: response.readSEED_RESP: invalid_number_of_poles: %d\n", iNumPoles);
                }
                for (i = 0; i < iNumPoles; i++) {
                    poleZeroResp.poles[i] = Cmplx(0.0, 0.0); // unspecified poles will be at origin
                }
            } else if (0 == strcmp(word_token, "B053F10-13")) {
                //#              Complex zeroes:
                //#              i  real          imag          real_error    imag_error
                //B053F10-13     0  +0.00000E+00  +0.00000E+00  +0.00000E+00  +0.00000E+00
                if (poleZeroResp.num_zeros >= MAX_NUM_PZ) {
                    fprintf(stderr, "ERROR: response.readSEED_RESP: too_many_zeros_read\n");
                }
                strtok(NULL, " \n");
                double re = atof(strtok(NULL, " \n"));
                double im = atof(strtok(NULL, " \n"));
                poleZeroResp.zeros[poleZeroResp.num_zeros] = Cmplx(re, im);
                poleZeroResp.num_zeros++;
            } else if (0 == strcmp(word_token, "B053F15-18")) {
                //#              Complex poles:
                //#              i  real          imag          real_error    imag_error
                //B053F15-18     0  -5.94313E+01  +0.00000E+00  +0.00000E+00  +0.00000E+00
                if (poleZeroResp.num_poles >= MAX_NUM_PZ) {
                    fprintf(stderr, "ERROR: response.readSEED_RESP: too_many_poles_read\n");
                }
                strtok(NULL, " \n");
                double re = atof(strtok(NULL, " \n"));
                double im = atof(strtok(NULL, " \n"));
                poleZeroResp.poles[poleZeroResp.num_poles] = Cmplx(re, im);
                poleZeroResp.num_poles++;
            }
        }

        if (inStageSequence_0) {
            if (0 == strcmp(word_token, "B058F04")) {
                //B058F04     Sensitivity:                           +8.64730E+08
                while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
                Sd_chan = atof(strtok(NULL, " \n"));
                continue;
            } else if (0 == strcmp(word_token, "B058F05")) {
                //B058F05     Frequency of sensitivity:              +2.00000E-02
                while (strlen(word_token = strtok(NULL, " \n")) > 0 && !(word_token[strlen(word_token) - 1] == ':'));
                f_chan = atof(strtok(NULL, " \n"));
                continue;
            }
        }

    }
    poleZeroResp.num_poles = iNumPoles;
    poleZeroResp.num_zeros = iNumZeros;

    if (DEBUG)
        printf("DEBUG: gamma %d A0_norm %g f_norm %g Sd_chan %g f_chan %g poleZeroResp.num_poles %d poleZeroResp.num_zeros %d\n",
            gamma, A0_norm, f_norm, Sd_chan, f_chan, poleZeroResp.num_poles, poleZeroResp.num_zeros);

    if (notSupported) {
        fprintf(stderr, "ERROR: response.readSEED_RESP: unsupperted_transfer_function_type: %s\n", line_tmp);
    }
    if ((gamma < 0 || (DBL_MAX == A0_norm) || (DBL_MAX == f_norm) || (DBL_MAX == Sd_chan) || (DBL_MAX == f_chan))
            || (poleZeroResp.num_poles == 0 && poleZeroResp.num_zeros == 0)) {
        fprintf(stderr, "ERROR: response.readSEED_RESP: empty_transfer_function\n");
        poleZeroResp.type = ERROR_TYPE;
        return (poleZeroResp);
    }


    /*
     * First, AH assumes the units of the poles and zeros are rad/sec,
     * so we convert Type B (Hz) to Type A (rad/sec) if necessary.
     *
     * If Type==B then convert to type A format by:
     *
     * P(n) = 2*pi*P(n)      { n=1...Np }
     * Z(m) = 2*pi*Z(m)      { m=1...Nz }
     * A0   = A0 * (2*pi)**(Np-Nz)
     */

    if (isTransferFunctionType_B) {
        for (i = 0; i < poleZeroResp.num_poles; i++) {
            poleZeroResp.poles[i].r *= 2.0 * PI;
            poleZeroResp.poles[i].i *= 2.0 * PI;
        }
        for (i = 0; i < poleZeroResp.num_zeros; i++) {
            poleZeroResp.zeros[i].r *= 2.0 * PI;
            poleZeroResp.zeros[i].i *= 2.0 * PI;
        }
        /* A0   = A0 * (2*pi)**(Np-Nz) */
        A0_norm = A0_norm * pow(2.0 * PI, (double) (poleZeroResp.num_poles - poleZeroResp.num_zeros));
    } /* if transfer function was analog - 'B' */


    /*
     * ah_resp.c
     *
     * Convert to displacement :
     * if acceleration, gamma=2    \
     * elseif velocity,     gamma=1 \
     * elseif displacement, gamma=0  \___Done above
     * else  print error message     /
     * endif                        /
     *
     * Sd = Sd * (2*pi*fs)**gamma
     * Nz = Nz + gamma
     * set values of new zeros equal zero
     * A0 = A0 / (2*pi*fn)**gamma
     * Units = M - Displacement Meters
     */

    /*
     * ah_resp.c
     *
     * add gamma zeros
     * Nz = Nz + gamma
     * set values of new zeros equal zero
     */
    if (gamma > 0) {
        for (i = poleZeroResp.num_zeros; i < poleZeroResp.num_zeros + gamma; i++) {
            poleZeroResp.zeros[i] = Cmplx(0.0, 0.0); // unspecified zeros will be at origin
        }
        poleZeroResp.num_zeros += gamma;
    }


    /*
     * ah_resp.c
     *
     * Sd = Sd * (2*pi*fs)**gamma
     * A0 = A0 / (2*pi*fn)**gamma
     * Units = M - Displacement Meters
     */
    if (f_norm != f_chan) {
        A0_norm = A0_norm / pow(2.0 * PI * f_norm, gamma);
        Sd_chan = Sd_chan * pow(2.0 * PI * f_chan, gamma);
    }

    //2007.180.18.04.30.1650.MN.TIR..BHZ.R.SAC
    /*
     * Third, there is no place in the AH header to specify either
     * the frequency of normalization or the frequency of the
     * digital sensitivity.  This is not a problem as long as these
     * two are the same.  If they are different then evaluate the
     * normalization at the frequency of the digital sensitivity.
     *
     *
     * if fn is not equal to fs then
     *  A0 = abs(prod{n=1...Np} [2*pi*i*fs - P(n)] /
    prod{m=1..Nz} [2*pi*i*fs - Z(m)])
     * endif
     */
    if (f_norm != f_chan) {
        A0_norm = calc_A0(poleZeroResp.poles, poleZeroResp.num_poles, poleZeroResp.zeros, poleZeroResp.num_zeros, f_chan);
    }

    poleZeroResp.gain = A0_norm * Sd_chan;

    // DEBUG
    if (DEBUG) {
        printf("ZEROS %d\n", poleZeroResp.num_zeros);
        for (i = 0; i < poleZeroResp.num_zeros; i++) {
            printf("%g %g\n", poleZeroResp.zeros[i].r, poleZeroResp.zeros[i].i);
        }
        printf("POLES %d\n", poleZeroResp.num_poles);
        for (i = 0; i < poleZeroResp.num_poles; i++) {
            printf("%g %g\n", poleZeroResp.poles[i].r, poleZeroResp.poles[i].i);
        }
        printf("CONSTANT %g\n", poleZeroResp.gain);
    }

    return (poleZeroResp);

}

/** Returns the response at a given frequency.
 *  following eq. 10.11 in Scherbaum, F. , Of Poles and Zeros Fundamentals of Digital Seismology
 *   Series: Modern Approaches in Geophysics, Vol. 15
 *
 * @param     frequency  frequency in Hz at which to evaluate response.
 * @return    the complex response.
 */
fcomplex evaluateResponse(PoleZeroResponse poleZeroResp, double frequency) {

    int i;

    fcomplex jw = Cmplx(0.0, frequency * 2.0 * PI);

    fcomplex numerator = Cmplx(1.0, 0.0);
    for (i = 0; i < poleZeroResp.num_zeros; i++) {
        numerator = Cmul(numerator, Csub(jw, poleZeroResp.zeros[i]));
    }

    fcomplex denominator = Cmplx(1.0, 0.0);
    for (i = 0; i < poleZeroResp.num_poles; i++) {
        denominator = Cmul(denominator, Csub(jw, poleZeroResp.poles[i]));
    }

    fcomplex response = Cmul(Cdiv(numerator, denominator), Cmplx(poleZeroResp.gain, 0.0));

    return (response);

}

/**
 * get_station_coords_internet_station_query from FDSN Station Webservice (e.g. http://service.iris.edu/fdsnws/station/1/)
 *
 * returns station coordinates lat, lon, elev
 *
 * TODO: add doc.
 */
int get_station_coords_internet_station_query_FDSN_WS_STATION(char* internet_station_query_host, char* internet_station_query,
        char* prtnet, char* prtsta, char* prtloc, char* prtchan, int year, int month, int mday,
        double* plat, double* plon, double* pelev, double* pazimuth, double* pdip) {

    static char page[4096];
    // "ws/station/query?net=IU&sta=SAML&loc=00&chan=BHZ&timewindow=2010-02-27,2010-02-27&level=station"
    // http://service.iris.edu/fdsnws/station/1/query?net=IU&sta=ANMO&cha=BHZ&loc=00&starttime=2013-09-05&endtime=2013-09-05&level=station
    sprintf(page, "%s?net=%s&sta=%s&loc=%s&cha=%s&starttime=%4.4d-%2.2d-%2.2d&endtime=%4.4d-%2.2d-%2.2d&level=channel",
            internet_station_query, prtnet, prtsta, prtloc, prtchan, year, month, mday, year, month, mday);
    fprintf(stdout, "Query: http://%s/%s\n", internet_station_query_host, page);

    //struct sockaddr_in *internet_station_socket_remote;
    int internet_station_socket = -1;
    //internet_station_socket = get_socket_connection(internet_station_query_host, &internet_station_socket_remote);
    internet_station_socket = get_socket_connection(internet_station_query_host, "http");

    int page_length = 0;
    char *page_contents = get_page(internet_station_query_host, internet_station_socket, page, &page_length);
    if (page_contents == NULL) {
        shutdown(internet_station_socket, SHUT_RDWR);
        close(internet_station_socket);
        //free(internet_station_socket_remote);
        return (-1);
    }

    fprintf(stdout, "%100.100s\n", page_contents);

    int return_value = 0;
    return_value = get_station_coords_xml(1, page_contents, plat, plon, pelev, pazimuth, pdip);

    free(page_contents);

    shutdown(internet_station_socket, SHUT_RDWR);
    close(internet_station_socket);
    //free(internet_station_socket_remote);

    return (return_value);

}

/**
 * get_station_coords_internet_station_query from IRIS Station Webservice (Deprecated)
 *
 * returns station coordinates lat, lon, elev
 *
 * TODO: add doc.
 */
int get_station_coords_internet_station_query_IRIS_WS_STATION(char* internet_station_query_host, char* internet_station_query,
        char* prtnet, char* prtsta, int year, int month, int mday, double* plat, double* plon, double* pelev) {

    static char page[4096];
    // "ws/station/query?net=IU&sta=SAML&loc=00&chan=BHZ&timewindow=2010-02-27,2010-02-27&level=station"
    /*if (strcmp(prtloc, "--") == 0)
        sprintf(page, "%s?net=%s&sta=%s&timewindow=%4.4d-%2.2d-%2.2d,%4.4d-%2.2d-%2.2d&level=station",
            internet_station_query, prtnet, prtsta, year, month, mday, year, month, mday);
    else
        sprintf(page, "%s?net=%s&sta=%s&loc=%s&timewindow=%4.4d-%2.2d-%2.2d,%4.4d-%2.2d-%2.2d&level=station",
            internet_station_query, prtnet, prtsta, prtloc, year, month, mday, year, month, mday);
     */
    // use only net sta, loc may cause errors at iris ???
    sprintf(page, "%s?net=%s&sta=%s&timewindow=%4.4d-%2.2d-%2.2d,%4.4d-%2.2d-%2.2d&level=station",
            internet_station_query, prtnet, prtsta, year, month, mday, year, month, mday);
    fprintf(stdout, "Query: http://%s/%s\n", internet_station_query_host, page);

    //struct sockaddr_in *internet_station_socket_remote;
    int internet_station_socket = -1;
    //internet_station_socket = get_socket_connection(internet_station_query_host, &internet_station_socket_remote);
    internet_station_socket = get_socket_connection(internet_station_query_host, "http");

    int page_length = 0;
    char *page_contents = get_page(internet_station_query_host, internet_station_socket, page, &page_length);
    if (page_contents == NULL) {
        shutdown(internet_station_socket, SHUT_RDWR);
        close(internet_station_socket);
        //free(internet_station_socket_remote);
        return (-1);
    }

    fprintf(stdout, "%100.100s\n", page_contents);

    int return_value = 0;
    double azimuth, dip;
    return_value = get_station_coords_xml(0, page_contents, plat, plon, pelev, &azimuth, &dip);

    free(page_contents);

    shutdown(internet_station_socket, SHUT_RDWR);
    close(internet_station_socket);
    //free(internet_station_socket_remote);

    return (return_value);

}

/**
 * get_station_coords_xml:
 *
 * 20140310 AJL - created this version of function which correctly parses xml that does not have CR-LF or other white-space between elements
 *
 * 20160805 AJL - modified to use FDSN station service at channel level, gives correct sensor lat/lon/depth (which may be different from station values)
 * 20160805 AJL - modified to get channel Azimuth and Dip
 *
 * TODO: add doc.
 */
int get_station_coords_xml(int use_channel, char* xml_str, double* plat, double* plon, double* pelev, double* pazimuth, double* pdip) {

    /*
    <?xml version="1.0" encoding="ISO-8859-1"?>

    <FDSNStationXML xmlns="http://www.fdsn.org/xml/station/1" schemaVersion="1.0" xsi:schemaLocation="http://www.fdsn.org/xml/station/1 http://www.fdsn.org/xml/station/fdsn-station-1.0.xsd" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:iris="http://www.fdsn.org/xml/station/1/iris">
     <Source>IRIS-DMC</Source>
     <Sender>IRIS-DMC</Sender>
     <Module>IRIS WEB SERVICE: fdsnws-station | version: 1.1.20</Module>
     <ModuleURI>http://service.iris.edu/fdsnws/station/1/query?net=IU&amp;sta=ANMO&amp;loc=00&amp;cha=BHZ&amp;starttime=2009-03-19&amp;endtime=2009-03-19&amp;level=chan</ModuleURI>
     <Created>2016-08-05T09:21:22</Created>
     <Network code="IU" startDate="1988-01-01T00:00:00" endDate="2500-12-31T23:59:59" restrictedStatus="open">
      <Description>Global Seismograph Network (GSN - IRIS/USGS)</Description>
      <TotalNumberStations>269</TotalNumberStations>
      <SelectedNumberStations>1</SelectedNumberStations>
      <Station code="ANMO" startDate="2008-06-30T20:00:00" endDate="2599-12-31T23:59:59" restrictedStatus="open" iris:alternateNetworkCodes="_US-REF,_CEUSN,_FDSN,_GSN-BROADBAND,.EARTHSCOPE,_CARIBE-EWS,_US-TA,_FDSN-ALL,.UNRESTRICTED,_GSN,.TEST,_US-ALL,_ANSS-BB,.CEUSN-CONTRIB,_REALTIME">
       <Latitude>34.94591</Latitude>
       <Longitude>-106.4572</Longitude>
       <Elevation>1820.0</Elevation>
       <Site>
        <Name>Albuquerque, New Mexico, USA</Name>
       </Site>
       <CreationDate>1989-08-29T00:00:00</CreationDate>
       <TotalNumberChannels>153</TotalNumberChannels>
       <SelectedNumberChannels>1</SelectedNumberChannels>
       <Channel code="BHZ" endDate="2011-02-18T19:11:00" locationCode="00" restrictedStatus="open" startDate="2008-06-30T20:00:00">
        <Latitude>34.945981</Latitude>
        <Longitude>-106.457133</Longitude>
        <Elevation>1671.0</Elevation>
        <Depth>145.0</Depth>
        <Azimuth>0.0</Azimuth>
        <Dip>-90.0</Dip>
        <Type>CONTINUOUS</Type>
        <Type>GEOPHYSICAL</Type>
        <SampleRate>20.0</SampleRate>
        <ClockDrift>0.0</ClockDrift>
        <Sensor>
         <Description>Geotech KS-54000 Borehole Seismometer</Description>
        </Sensor>
        <Response>
         <InstrumentSensitivity>
          <Value>3.27508E9</Value>
          <Frequency>0.02</Frequency>
          <InputUnits>
           <Name>M/S</Name>
           <Description>Velocity in Meters Per Second</Description>
          </InputUnits>
          <OutputUnits>
           <Name>COUNTS</Name>
           <Description>Digital Counts</Description>
          </OutputUnits>
         </InstrumentSensitivity>
        </Response>
       </Channel>
      </Station>
     </Network>
    </FDSNStationXML>
     */

    int lat_found = 0;
    int lon_found = 0;
    int elev_found = 0;
    int azimuth_found = 1; // backwards compatibility with discontinued IRIS_WS_STATION service
    int dip_found = 1; // backwards compatibility with discontinued IRIS_WS_STATION service

    *plat = 0.0;
    *plon = 0.0;
    *pelev = 0.0;

    //printf("DEBUG: get_station_coords_xml: word_token: |%s|\n", xml_str);

    // check for keywords

    char* word_token;

    // if use channel specified, skip to channel block
    //20160805 AJL - added
    if (use_channel) {
        if ((word_token = strstr(xml_str, "<Channel")) == NULL) {
            fprintf(stderr, "ERROR: get_station_coords_xml: failed find <Channel> element: %s\n", xml_str);
            return (-1);
        }
        xml_str = word_token;
        // channel level should have azimuth and dip
        azimuth_found = 0;
        dip_found = 0;
    }

    if ((word_token = strstr(xml_str, "<Lat>")) != NULL) {
        if (sscanf(word_token, "<Lat>%lf</Lat>", plat) != 1) {
            fprintf(stderr, "ERROR: get_station_coords_xml: failed to read Lat from: %s\n", word_token);
            return (-1);
        }
        lat_found = 1;
    } else if ((word_token = strstr(xml_str, "<Latitude>")) != NULL) {
        if (sscanf(word_token, "<Latitude>%lf</Latitude>", plat) != 1) {
            fprintf(stderr, "ERROR: get_station_coords_xml: failed to read Latitude from: %s\n", word_token);
            return (-1);
        }
        lat_found = 1;
    }
    if ((word_token = strstr(xml_str, "<Lon>")) != NULL) {
        if (sscanf(word_token, "<Lon>%lf</Lon>", plon) != 1) {
            fprintf(stderr, "ERROR: get_station_coords_xml: failed to read Lon from: %s\n", word_token);
            return (-1);
        }
        lon_found = 1;
    } else if ((word_token = strstr(xml_str, "<Longitude>")) != NULL) {
        if (sscanf(word_token, "<Longitude>%lf</Longitude>", plon) != 1) {
            fprintf(stderr, "ERROR: get_station_coords_xml: failed to read Longitude from: %s\n", word_token);
            return (-1);
        }
        lon_found = 1;
    }
    if ((word_token = strstr(xml_str, "<Elevation>")) != NULL) {
        if (sscanf(word_token, "<Elevation>%lf</Elevation>", pelev) != 1) {
            fprintf(stderr, "ERROR: get_station_coords_xml: failed to read Elevation from: %s\n", word_token);
            return (-1);
        }
        elev_found = 1;
    }
    if ((word_token = strstr(xml_str, "<Azimuth>")) != NULL) {
        if (sscanf(word_token, "<Azimuth>%lf</Azimuth>", pazimuth) != 1) {
            fprintf(stderr, "ERROR: get_station_coords_xml: failed to read Azimuth from: %s\n", word_token);
            return (-1);
        }
        azimuth_found = 1;
    }
    if ((word_token = strstr(xml_str, "<Dip>")) != NULL) {
        if (sscanf(word_token, "<Dip>%lf</Dip>", pdip) != 1) {
            fprintf(stderr, "ERROR: get_station_coords_xml: failed to read Dip from: %s\n", word_token);
            return (-1);
        }
        dip_found = 1;
    }

    if (DEBUG) printf("get_station_coords_xml: lat=%g lon=%g elev=%g az=%g dip=%g\n", *plat, *plon, *pelev, *pazimuth, *pdip);

    if (!(lat_found && lon_found && elev_found && azimuth_found && dip_found))
        return (-1);

    return (0);

}

/**
 * get_gain_xml:
 *
 * 20150414 AJL - created this function
 *
 * TODO: add doc.
 */
double get_gain_xml(char* xml_str, double* pfrequency, int* presponseType) {

    /*
    <FDSNStationXML schemaVersion="1.0">
        <Source>SeisComP3</Source>
        <Sender>INGV</Sender>
        <Created>2015-04-14T13:02:46</Created>
        <Network code="IV" startDate="1980-01-01T00:00:00" restrictedStatus="open">
            <Description>Italian Seismic Network</Description>
            <Station code="SALO" startDate="2005-06-24T12:00:00" restrictedStatus="open">
                <Latitude>45.6183</Latitude>
                <Longitude>10.5243</Longitude>
                <Elevation>600</Elevation>
                <Site>
                    <Name>Salo</Name>
                </Site>
                <CreationDate>2005-06-24T12:00:00</CreationDate>
                <Channel code="BHZ" startDate="2009-06-03T13:30:00" restrictedStatus="open" locationCode="">
                    <Latitude>45.6183</Latitude>
                    <Longitude>10.5243</Longitude>
                    <Elevation>600</Elevation>
                    <Depth>1</Depth>
                    <Azimuth>0</Azimuth>
                    <Dip>-90</Dip>
                    <SampleRate>20</SampleRate>
                    <SampleRateRatio>
                        <NumberSamples>20</NumberSamples>
                        <NumberSeconds>1</NumberSeconds>
                    </SampleRateRatio>
                    <StorageFormat>Steim2</StorageFormat>
                    <ClockDrift>0</ClockDrift>
                    <Sensor resourceId="Sensor#20131121100339.067575.41">
                        <Type>TRILLIUM-120S</Type>
                        <Description>TRILLIUM-120S</Description>
                        <Model>TRILLIUM-120S</Model>
                    </Sensor>
                    <DataLogger resourceId="Datalogger#20141127094910.249483.40">
                        <Description>SALO.2009.154.BHZ</Description>
                    </DataLogger>
                    <Response>
                        <InstrumentSensitivity>
                            <Value>480400000</Value>
                            <Frequency>1</Frequency>
                            <InputUnits>
                                <Name>M/S</Name>
                            </InputUnits>
                            <OutputUnits>
                                <Name/>
                            </OutputUnits>
                        </InstrumentSensitivity>
                    </Response>
                </Channel>
            </Station>
        </Network>
    </FDSNStationXML>
     */

    int gain_found = 0;
    int freq_found = 0;
    int units_found = 0;

    double gain = -1.0;
    *pfrequency = -1.0;
    char input_units[64] = "UNKNOWN";

    //printf("DEBUG: get_gain_xml: word_token: |%s|\n", xml_str);

    // check for keywords

    char* word_token;
    char* word_token2;
    if ((word_token = strstr(xml_str, "<InstrumentSensitivity>")) != NULL) {
        if ((word_token2 = strstr(word_token, "<Value>")) != NULL) {
            if (sscanf(word_token2, "<Value>%lf", &gain) != 1) {
                fprintf(stderr, "ERROR: get_gain_xml: failed to read InstrumentSensitivity from: %s\n", word_token);
                return (-1);
            }
            gain_found = 1;
        }
        if ((word_token2 = strstr(word_token, "<Frequency>")) != NULL) {
            if (sscanf(word_token2, "<Frequency>%lf", pfrequency) != 1) {
                fprintf(stderr, "ERROR: get_gain_xml: failed to read Frequency from: %s\n", word_token);
                return (-1);
            }
            freq_found = 1;
        }
        if ((word_token2 = strstr(word_token, "<InputUnits>")) != NULL) {
            if ((word_token2 = strstr(word_token2, "<Name>")) != NULL) {
                if (sscanf(word_token2, "<Name>%63s", input_units) != 1) {
                    fprintf(stderr, "ERROR: get_gain_xml: failed to read InputUnits from: %s\n", word_token);
                    return (-1);
                }
                char *c;
                if ((c = strchr(input_units, '<')) != NULL) { // trim trailing "</Name>"
                    *c = '\0';
                }
                units_found = 1;
            }
        }
    }

    if (DEBUG) printf("get_gain_xml: gain=%f freq=%f input_units=<%s>\n", gain, *pfrequency, input_units);

    if (!(gain_found && freq_found && units_found))
        return (-1.0);

    *presponseType = UNKNOWN_TYPE;
    if (strcmp(input_units, "M/S") == 0 || strcmp(input_units, "m/s") == 0) {
        *presponseType = DERIVATIVE_TYPE;
    } else if (strcmp(input_units, "NM/S") == 0 || strcmp(input_units, "nm/s") == 0) {
        *presponseType = DERIVATIVE_TYPE;
    }
    if (DEBUG) printf("get_gain_xml: *presponseType=%d\n", *presponseType);

    return (gain);

}
