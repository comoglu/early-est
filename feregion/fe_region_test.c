
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "feregion.h"

int main(int argc, char **argv) {

    if (argc > 1) {
        double lat = atof(argv[1]);
        double lon = atof(argv[2]);
        char feregion_str[1024];
        printf("lat=%g, lon=%g, ", lat, lon);
        fflush(stdout);
        printf("FE region = \'%s\'\n", feregion(lat, lon, feregion_str, 1024));
    } else {
        double lat, lon;
        char feregion_str_neic[1024];
        char feregion_str[1024];
        int istat = -999;
        while ((istat = scanf(" %lf %lf %s ", &lat, &lon, feregion_str_neic)) == 3) {
            feregion(lat, lon, feregion_str, 1024);
            if (feregion_str[0] != feregion_str_neic[0])
                printf(" XXX ERROR! >>>>> ");
            printf("lat=%g, lon=%g, ", lat, lon);
            printf("FE region: NEIC = \'%s\', ", feregion_str_neic);
            fflush(stdout);
            printf("this = \'%s\'\n", feregion_str);
        }
        printf("istat = %d\n", istat);
    }

    return (0);

}

