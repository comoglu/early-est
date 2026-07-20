/***************************************************************************
 * fmamp.h
 *
 * Focal mechanism determination from P first motions and amplitudes using
 * probabilistic, OctTree search.
 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 * created: 2014.02.18
 ***************************************************************************/

#define DEBUG 0

#define VERSION "0.0.5X"
#define VERSION_DATE "2016.06.14"
#define PACKAGE "fmamp"

#ifdef EXTERN_MODE
#define	EXTERN_TXT extern
#else
#define EXTERN_TXT
#endif

#include "settings/settings.h"
#include "geometry/geometry.h"
#include "octtree/octtree.h"
#include "ran1/ran1.h"
#include "matrix_statistics/matrix_statistics.h"
//#include "alomax_matrix/alomax_matrix.h"
//#include "alomax_matrix/alomax_matrix_svd.h"

#include "meca.h"
#include "fmamp_utils.h"


// hypocenter =====================================================================================

#define WARNING_LEVEL_STRING_LEN 64

#define MAX_NUM_PICKS 10000

#define STANDARD_STRLEN 4096

#define KM2DEG (90.0/10000.0)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define PI M_PI
#define RA2DE (180.0/PI)
#define DE2RA (PI/180.0)


#define POLARITY_POS 		1
#define POLARITY_UNKNOWN 	0
#define POLARITY_NEG 		-1

//#define  STRIKE_MIN -180.0    // test
#define  STRIKE_MIN 0.0    // only need half range to prevent duplicate search over fault plane and auxiliary plane
#define  STRIKE_MAX 180.0
#define  DIP_MIN 0.0
#define  DIP_MAX 90.0
#define  RAKE_MIN -180.0
#define  RAKE_MAX 180.0

typedef struct {
    long event_unique_id;
    char station[16];
    char location[16];
    char channel[16];
    char network[16];
    char phase[16];
    int year, month, day, hour, min; // pick time
    double dec_sec;
    char pick_error_type[32];
    double pick_error;
    int fmpolarity; // broad-band polarity measure
    double fmquality; // broad-band polarity measure
    char fmtype[32];
    double amplitude; // amplitude
    double take_off_angle_inc; // degrees (0/down->180/up)
    double take_off_angle_az; // degrees CW from N
    double epicentral_distance; // degrees
    double epicentral_azimuth; // degrees CW from N
    double residual;
    double take_off_angle_distrib_weight; // calculated take-off angle distribution weight on sphere
    double weight; // calculated first motion obs quality weight
}
Pick;

typedef struct {
    long unique_id;
    int year, month, day, hour, min; // origin time
    double dec_sec;
    double rms;
    double lat;
    double lon;
    double errh;
    double depth;
    double errz;
    int nassoc_P; // number of picks that contributed with weight > 0 to location
    double dist_min; // minimum distance of associated phase counted as nassoc_P
    double dist_max; // maximum distance of associated phase counted as nassoc_P
    double gap_primary; // maximum azimuth gap
    double gap_secondary; // secondary azimuth gap - largest azumth gap filled by a single station
    double ampAttenPower; // attenuation decay power from linear regression fit of P amplitudes to distance
    double magnitude; // 1/n_stations * distance of sum of vectors from epicenter to stations
    char mag_type[16];
    // picks
    Pick pick_list[MAX_NUM_PICKS];
    int pick_list_size;
}
Hypocenter;

// nodal plane using Aki and Richards conventions
typedef struct {
    double strike;  // Strike direction (along strike azimuth in degrees,clockwise from north) (0->360deg))
    double dip;     // Dip angle in degrees down from horizontal (0->90deg)
    double rake;    // Rake in degrees (-180->180deg) : 0=left lateral, 90=reverse, +-180=right lateral, -90=normal
}
NodalPlane;

typedef struct {
    NodalPlane bestNP;
    NodalPlane meanNP;
    struct AXIS meanPaxis;
    struct AXIS meanTaxis;
    struct AXIS meanNaxis;
    double err_strike;
    double err_dip;
    double err_rake;
    int nreadings;
    int nobs_used_polarity;
    double polarity_misfit; // first-motion misfit
    double stat_dist_ratio;
    /* The station distribution ratio (STDR), introduced by Reasenberg and Oppenheimer (1985).
         The STDR quantifies how the observations are spaced on the focal sphere,
         relative to the nodal planes, with a larger STDR indicating a better distribution.
         STDR is the station distribution ratio (0.0 <= STDR <= 1.0). This quantity is sensitive
         to the distribution of the data on the focal sphere, relative to the radiation pattern.
         When this ratio has a low value (say, STDR < 0.5), then a relatively large number of the
         data lie near nodal planes in the solution. Such a solution is less robust than one
         for which STDR > 0.5, and, consequently, should be scrutinized closely and possibly rejected.
     */
    double sum_polarity_misfit_weight;
    double polarity_probability;
    int nobs_used_amplitude;
    double amplitude_misfit; // amplitude misfit
    double sum_amplitude_misfit_weight;
    double amplitude_probability;
    double probability;
    double nsamples; // number of oct-tree nodes evaluated
    double oct_tree_scatter_volume;
    double mean_dist_P; // mead distance on sphere of scatter sample P axes
    double mean_dist_T; // mead distance on sphere of scatter sample T axes
    // statistics
    Vect3D expect; // "traditional" expectation
    Mtrx3D cov; // "traditional" covariance matrix - units in deg
    Ellipsoid3D ellipsoid; // error ellipsoid description
    Ellipse2D ellipse; // horizontal (X/Y) error ellipse description
}
FaultPlaneSolution;

int read_input(Settings *settings, char *input_format, Hypocenter *phypocenter, FILE* hypoStream, char *hypofile, char *pickfile, int verbose);
double fmamp_invert(Hypocenter *phypocenter, int max_num_nodes, double min_node_size_deg, double initial_grid_step,
        int min_number_observations, int critical_number_observations, int use_first_motion_polarities, int use_amplitudes,
        FaultPlaneSolution *pfault_plane_solution, float **pscatter_sample, int *p_n_alloc_scatter_sample, int *pn_scatter_sample,
        int verbose);
int write_output(char *outpath, char *variant_name, Hypocenter *phypocenter,
        int min_number_observations, int critical_number_observations, int use_first_motion_polarities, int use_amplitudes,
        double best_prob, FaultPlaneSolution *pfault_plane_solution, float *pscatter_sample, int n_scatter_sample,
        int iwrite_sum_only, FILE* sumStream, int iwrite_sum_header, int verbose);