/***************************************************************************
 * fmamp.c
 *
 * Focal mechanism determination from P first motions and amplitudes using
 * probabilistic, OctTree search.
 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 * created: 2014.02.18
 ***************************************************************************/



#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <string.h>
//#include <math.h>
//#include <time.h>
//#include <sys/time.h>
//#include <sys/resource.h>
#include <errno.h>

#undef EXTERN_MODE

#include "fmamp.h"
#include "fmamp_utils.h"



int verbose = 1;
char propfile[STANDARD_STRLEN];

#define PROP_FILE_NAME_DEFAULT "fmamp.prop"
static Settings *settings = NULL;

// properties
// Program
char outpath[STANDARD_STRLEN];
#define OUTPATH_DEFAULT "./out"
char variant_name[STANDARD_STRLEN];
#define VARIANT_NAME_DEFAULT ""
char input_format[STANDARD_STRLEN];
#define INPUT_FORMAT_DEFAULT "FMAMP"
char hypofile[STANDARD_STRLEN];
#define HYPOFILE_DEFAULT "./hypos.csv"
char pickfile[STANDARD_STRLEN];
#define PICKFILE_DEFAULT "./picks.csv"
// Inversion
int min_number_observations;
#define MIN_NUMBER_OBSERVATIONS_DEFAULT 10
int critical_number_observations;
#define CRITICAL_NUMBER_OBSERVATIONS_DEFAULT 10
int max_num_nodes;
#define MAX_NUM_NODES_DEFAULT 10000
double min_node_size_deg;
#define MIN_NODE_SIZE_DEG_DEFAULT 1.0
double initial_grid_step;
#define INITIAL_GRID_STEP_DEFAULT 10.0
int use_first_motion_polarities;
#define USE_FIRST_MOTION_POLARITIES_DEFAULT 1
int use_amplitudes;
#define USE_AMPLITUDES_DEFAULT 0

/***************************************************************************
 * usage():
 * Print the usage message and exit.
 ***************************************************************************/
static void usage(void) {

    fprintf(stdout, "%s version: %s (%s)\n", PACKAGE, VERSION, VERSION_DATE);
    fprintf(stdout, "Usage: %s [options]\n", PACKAGE);
    fprintf(stdout,
            " Options:\n"
            " -p propfile    properties file name (default: fmamp.prop)\n"
            " -V             Report program version\n"
            " -h             Show this usage message\n"
            " -v             Be more verbose, multiple flags can be used\n"
            );

} /* End of usage() */

/***************************************************************************
 * parameter_proc():
 * Process the command line parameters.
 *
 * Returns 0 on success, and -1 on failure
 ***************************************************************************/
static int parameter_proc(int argcount, char **argvec) {

    int optind = 1;
    int error = 0;

    if (argcount <= 1)
        error++;

    // Process all command line arguments
    for (optind = 1; optind < argcount; optind++) {

        if (strcmp(argvec[optind], "-V") == 0) {
            fprintf(stdout, "%s version: %s (%s)\n", PACKAGE, VERSION, VERSION_DATE);
            exit(0);
        } else if (strcmp(argvec[optind], "-h") == 0) {
            (*usage)();
            exit(0);
        } else if (strncmp(argvec[optind], "-v", 2) == 0) {
            verbose += strspn(&argvec[optind][1], "v");
        } else if (strcmp(argvec[optind], "-p") == 0) {
            strcpy(propfile, argvec[++(optind)]);
        } else if (strncmp(argvec[optind], "-", 1) == 0) {
            fprintf(stdout, "%s: Unknown option: %s\n", PACKAGE, argvec[optind]);
            exit(1);
        }
    }

    // Report the program version
    if (verbose) {
        fprintf(stdout, "%s version: %s (%s)\n", PACKAGE, VERSION, VERSION_DATE);
    }

    return 0;

} /* End of parameter_proc() */

/***************************************************************************
 * init_properties():
 * Initialize properties from properties file
 ***************************************************************************/
int init_properties(char *propfile) {

    // read properties file
    FILE *fp_prop = fopen(propfile, "r");
    if (fp_prop == NULL) {
        fprintf(stdout, "Info: Cannot open application properties file: %s\n", propfile);
        settings = NULL;
        return (0);
    }
    settings = settings_open(fp_prop);
    fclose(fp_prop);
    if (settings == NULL) {
        fprintf(stderr, "ERROR: fmamp: Reading application properties file: %s\n", propfile);
        return (-1);
    }

    // Program section
    //
    if (settings_get_helper(settings,
            "Program", "outpath", outpath, sizeof (outpath), OUTPATH_DEFAULT,
            verbose) == 0) {
        ; // handle error
    }
    //
    if (settings_get_helper(settings,
            "Program", "variant_name", variant_name, sizeof (variant_name), VARIANT_NAME_DEFAULT,
            verbose) == 0) {
        ; // handle error
    }
    //
    if (settings_get_helper(settings,
            "Program", "input_format", input_format, sizeof (input_format), INPUT_FORMAT_DEFAULT,
            verbose) == 0) {
        ; // handle error
    }
    //
    if (settings_get_helper(settings,
            "Program", "hypofile", hypofile, sizeof (hypofile), HYPOFILE_DEFAULT,
            verbose) == 0) {
        ; // handle error
    }
    //
    if (settings_get_helper(settings,
            "Program", "pickfile", pickfile, sizeof (pickfile), PICKFILE_DEFAULT,
            verbose) == 0) {
        ; // handle error
    }


    // Inversion Section
    //
    if (settings_get_int_helper(settings,
            "Inversion", "min_number_observations", &min_number_observations, MIN_NUMBER_OBSERVATIONS_DEFAULT,
            verbose
            ) == INT_INVALID) {
        ; // handle error
    }
    //
    if (settings_get_int_helper(settings,
            "Inversion", "critical_number_observations", &critical_number_observations, CRITICAL_NUMBER_OBSERVATIONS_DEFAULT,
            verbose
            ) == INT_INVALID) {
        ; // handle error
    }
    //
    if (settings_get_int_helper(settings,
            "Inversion", "max_num_nodes", &max_num_nodes, MAX_NUM_NODES_DEFAULT,
            verbose
            ) == INT_INVALID) {
        ; // handle error
    }
    //
    if (settings_get_double_helper(settings,
            "Inversion", "min_node_size_deg", &min_node_size_deg, MIN_NODE_SIZE_DEG_DEFAULT,
            verbose
            ) == DBL_INVALID) {
        ; // handle error
    }
    //
    if (settings_get_double_helper(settings,
            "Inversion", "initial_grid_step", &initial_grid_step, INITIAL_GRID_STEP_DEFAULT,
            verbose
            ) == DBL_INVALID) {
        ; // handle error
    }
    //
    if (settings_get_int_helper(settings,
            "Inversion", "use_first_motion_polarities", &use_first_motion_polarities, USE_FIRST_MOTION_POLARITIES_DEFAULT,
            verbose
            ) == INT_INVALID) {
        ; // handle error
    }
    //
    if (settings_get_int_helper(settings,
            "Inversion", "use_amplitudes", &use_amplitudes, USE_AMPLITUDES_DEFAULT,
            verbose
            ) == INT_INVALID) {
        ; // handle error
    }

    // check inversion values
    // check for valid OctTree values
    int init_n_cells = (360.01 / initial_grid_step) * (90.01 / initial_grid_step) * (180.01 / initial_grid_step);
    if (init_n_cells >= max_num_nodes) {
        fprintf(stderr, "ERROR: fmamp: OctTree init_num_cells (%d) >= max_num_nodes (%d): no oct-tree subdivision can be performed.\n",
                init_n_cells, max_num_nodes);
        return (-1);
    } else if (max_num_nodes - init_n_cells < 10000) {
        fprintf(stdout, "Warning: OctTree max_num_nodes - init_num_cells (%d) < 10000: very few oct-tree subdivisions can be performed.\n",
                max_num_nodes - init_n_cells);
    }
    // check which data to use
    if (!use_first_motion_polarities && !use_amplitudes) {
        fprintf(stderr, "ERROR: fmamp: At least one of use_first_motion_polarities (%d) or use_amplitudes (%d) must be true.\n",
                use_first_motion_polarities, use_amplitudes);
        return (-1);
    }


    return (0);

}

int main(int argc, char **argv) {

    int istat;

    // Process input parameters
    strcpy(propfile, PROP_FILE_NAME_DEFAULT);
    if ((istat = parameter_proc(argc, argv)) < 0) {
        fprintf(stderr, "ERROR: fmamp: invoking parameter_proc(): %d\n", istat);
        exit(1);
    }
    if ((istat = init_properties(propfile)) < 0) {
        fprintf(stderr, "ERROR: fmamp: invoking init_properties(): %d\n", istat);
        exit(2);
    }

    // open hypocenter file
    FILE* hypoStream = fopen(hypofile, "r");
    if (hypoStream == NULL) {
        fprintf(stderr, "ERROR: fmamp: opening hypocenter file: %s\n", hypofile);
        return (-1);
    }

    // open output fmamp sum file
    int iwrite_sum_only = 0;
    char sumfile[2*STANDARD_STRLEN];
    sprintf(sumfile, "%s/fmamp.sum", outpath);
    FILE* sumStream = NULL; // flags do not write sum file
    if (strcmp(input_format, "FMAMP") == 0) {
        iwrite_sum_only = 0; // TODO: make program setting parameter
        sumStream = fopen(sumfile, "w");
        if (sumStream == NULL) {
            fprintf(stderr, "ERROR: fmamp: opening output sum file: %s\n", sumfile);
            return (-1);
        }
    }


    // read and process each Hypocenter
    Hypocenter hypocenter;
    int iwrite_sum_header = 1;
    int n_hypo_read = 0;
    while (1) {
        if ((istat = read_input(settings, input_format, &hypocenter, hypoStream, hypofile, pickfile, verbose)) < 0) {
            if (istat == -1) {
                if (n_hypo_read > 0) { // probably end of file
                    break;
                } else {
                    fprintf(stderr, "INFO: fmamp: No events found in hypocenter file: %s\n", hypofile);
                    break;
                }
            }
            fprintf(stderr, "ERROR: fmamp: invoking read_input(): %d\n", istat);
            break;
        }
        n_hypo_read++;
        // perform oct-tree inversion
        FaultPlaneSolution fault_plane_solution;
        static float *scatter_sample = NULL;
        static int n_alloc_scatter_sample;
        int n_scatter_sample = 0;
        double best_prob = fmamp_invert(&hypocenter, max_num_nodes, min_node_size_deg, initial_grid_step,
                min_number_observations, critical_number_observations, use_first_motion_polarities, use_amplitudes,
                &fault_plane_solution, &scatter_sample, &n_alloc_scatter_sample, &n_scatter_sample,
                verbose);
        // write output files
        if ((istat = write_output(outpath, variant_name, &hypocenter,
                min_number_observations, critical_number_observations, use_first_motion_polarities, use_amplitudes,
                best_prob, &fault_plane_solution, scatter_sample, n_scatter_sample, iwrite_sum_only, sumStream, iwrite_sum_header, verbose)) < 0) {
            fprintf(stderr, "ERROR: fmamp: invoking write_output(): %d\n", istat);
            iwrite_sum_header = 0;
            continue;
        }
        iwrite_sum_header = 0;
    }

    if (sumStream != NULL) {
        fclose(sumStream);
        printf("INFO: fmamp: output written to sum file: %s\n", sumfile);
    }
    fclose(hypoStream);

    exit(0);

} /* End of main() */

