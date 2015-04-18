#ifndef LOCAL_H
#define LOCAL_H

#include "gctp.h"

/* FIXME - temporary defines to rename routines until they are all updated */
#define gctp_get_spheroid sphdz

/* Define the maximum projection code number */
#define GCTP_MAX_PROJ_CODE 31

/* Forward reference to the TRANSFORMATION so it can be used to define the
   print info routine type. */
typedef struct transformation TRANSFORMATION;

/* Define the type for a function pointer to a routine that prints information
   about a projection. */
typedef void (*PRINT_PROJ_INFO)(const TRANSFORMATION *);

/* Define the type for a function pointer to a routine that cleans up the
   transformation. */
typedef void (*DESTROY_TRANSFORM)(TRANSFORMATION *);

/* Function typedef for a transform function.  The same definition works for
   both forward and inverse transforms.  For a forward transform, the inputs
   are lon/lat and the outputs are x/y.  For an inverse transform, the inputs
   are x/y and the outputs are lat/lon. */
typedef int (*TRANSFORM_FUNC)(const TRANSFORMATION *trans,
    double in_x, double in_y, double *out_x, double *out_y);

/* Define a structure for tracking the information for a transformation.  The
   same structure works for both forward and inverse transformations. */
struct transformation
{
    GCTP_PROJECTION proj;     /* projection information */
    TRANSFORM_FUNC transform; /* function pointer for the transform function */
    DESTROY_TRANSFORM destroy; /* Function pointer to clean up the
                                  transformation.  Note that most projections
                                  can leave this at the default NULL. */
    PRINT_PROJ_INFO print_info; /* function to print projection information */
    double unit_conversion_factor; /* unit conversion factor to convert the
                                      coordinates to/from the internally used
                                      degrees or meters */

    /* Pointer to a cache of data that is initialized by the projection's
       init function.  It is a void pointer to allow each projection to define
       its own set of data that it needs to track. */
    void *cache;

};

/* Define a structure to hold information about a projection transformation.
   There is a forward reference to this structure in gctp.h that defines the
   GCTP_TRANSFORMATION type. */
struct gctp_transformation
{
    TRANSFORMATION forward;     /* Forward transformation information */
    TRANSFORMATION inverse;     /* Inverse transformation information */
    int use_gctp;               /* Flag to use the original gctp routine
                                   since one or both of the projections has not
                                   been converted to the new interface yet */
};

#define PRINT_FORMAT_ATTRIBUTE  __attribute__ ((format(printf,4,5)))
void gctp_print_message 
(
    GCTP_MESSAGE_TYPE_ENUM message_type, /* I: message type */
    const char *filename,     /* I: source code file name for input */
    int line_number,          /* I: source code line number for input */
    const char *format, ...   /* I: format string for message */
) PRINT_FORMAT_ATTRIBUTE; 

/* Define a macro for printing info messages to allow it to be changed
   easily. */
#define GCTP_PRINT_INFO(format,...) \
    gctp_print_message(GCTP_INFO_MESSAGE,__FILE__, __LINE__, format, \
        ##__VA_ARGS__)

/* Define a macro for printing error messages to allow it to be changed
   easily. */
#define GCTP_PRINT_ERROR(format,...) \
    gctp_print_message(GCTP_ERROR_MESSAGE,__FILE__, __LINE__, format, \
        ##__VA_ARGS__)

/*********** Internal routines for initializing projections ***************/
int gctp_geo_init(TRANSFORMATION *trans);

int gctp_lamcc_inverse_init(TRANSFORMATION *trans);

int gctp_lamcc_forward_init(TRANSFORMATION *trans);

int gctp_om_inverse_init(TRANSFORMATION *trans);

int gctp_om_forward_init(TRANSFORMATION *trans);

int gctp_poly_inverse_init(TRANSFORMATION *trans);

int gctp_poly_forward_init(TRANSFORMATION *trans);

int gctp_ps_inverse_init(TRANSFORMATION *trans);

int gctp_ps_forward_init(TRANSFORMATION *trans);

int gctp_state_plane_inverse_init(TRANSFORMATION *trans);

int gctp_state_plane_forward_init(TRANSFORMATION *trans);

int gctp_som_inverse_init(TRANSFORMATION *trans);

int gctp_som_forward_init(TRANSFORMATION *trans);

int gctp_tm_inverse_init(TRANSFORMATION *trans);

int gctp_tm_forward_init(TRANSFORMATION *trans);

int gctp_utm_inverse_init(TRANSFORMATION *trans);

int gctp_utm_forward_init(TRANSFORMATION *trans);


/************ Internal routines for printing out projection info *********/
void gctp_print_title
(
    const char *proj_name
);
void gctp_print_radius
(
    double radius
);
void gctp_print_radius2
(
    double radius1,
    double radius2
);
void gctp_print_cenlon
(
    double A
);
void gctp_print_cenlonmer
(
    double A
);
void gctp_print_cenlat
(
    double A
);
void gctp_print_origin
(
    double A
);
void gctp_print_stanparl
(
    double A,
    double B
);
void gctp_print_stparl1
(
    double A
);
void gctp_print_offsetp
(
    double A,
    double B
);
void gctp_print_lat_zone
(
    double A
);
void gctp_print_justify_cols
(
    double A
);
void gctp_print_genrpt
(
    double A,
    const char *S
);
void gctp_print_genrpt_long
(
    long A,
    const char *S
);

int gctp_get_sign
(
    double x
);

double gctp_calc_e0
(
    double x
);

double gctp_calc_e1
(
    double x
);

double gctp_calc_e2
(
    double x
);

double gctp_calc_e3
(
    double x
);

double gctp_calc_e4
(
    double x
);

double gctp_calc_dist_from_equator
(
    double e0,
    double e1,
    double e2,
    double e3,
    double phi
);

int gctp_calc_phi2
(
    double eccent,      /* I: Spheroid eccentricity */
    double ts,          /* I: Constant value t */
    double *phi2        /* O: calculated value of phi2 */
);

int gctp_dms2degrees
(
    double ang,         /* I: angle in DMS */
    double *degrees     /* O: angle in degrees */
);

double gctp_calc_small_radius
(
    double eccent,
    double sinphi,
    double cosphi
);

double gctp_calc_small_t
(
    double eccent,	/* Eccentricity of the spheroid		*/
    double phi,		/* Latitude phi				*/
    double sinphi	/* Sine of the latitude			*/
);

#endif
