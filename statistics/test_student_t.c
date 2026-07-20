/*
 * File:   test_student_t.c
 * Author: anthony
 *
 * Created on February 17, 2010, 7:03 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "student_t.h"

void usage() {
    fprintf(stderr, "USAGE: test_student_t\n");
}

/*
 *
 */
int main(int argc, char** argv) {

    if (argc > 1) {
        usage();
        exit(2);
    }

    int deg_freedom[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 100, INT_MAX};
    int nd, ndeg_freedom = 16;
    double percentages[] = {75, 80, 85, 90, 95, 97.5, 99, 99.5, 99.75, 99.9, 99.95};
    int np, npercentages = 11;

    fprintf(stdout, "   \t");
    for (np = 0; np < npercentages; np++) {
        fprintf(stdout, "%.3f\t", percentages[np]);
    }
    fprintf(stdout, "\n");

    for (nd = 0; nd < ndeg_freedom; nd++) {
        fprintf(stdout, "%d\t", deg_freedom[nd]);
        for (np = 0; np < npercentages; np++) {
            fprintf(stdout, "%.3f\t", student_t_inv(deg_freedom[nd], percentages[np] / 100.0));
        }
        fprintf(stdout, "\n");
    }

    return (0);
}

