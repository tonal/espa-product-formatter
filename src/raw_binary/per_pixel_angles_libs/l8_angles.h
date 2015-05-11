#ifndef _L8_ANGLES_H_
#define _L8_ANGLES_H_

/* Standard Library Includes */
#include <limits.h>

/* IAS Library Includes */
#include "ias_angle_gen_distro.h"
#include "ias_angle_gen_includes.h"
#include "ias_miscellaneous.h"

/* ESPA Library Includes */
#include "raw_binary_io.h"
#include "envi_header.h"

#define L8_NBANDS 11

typedef enum angle_type
{
    AT_UNKNOWN = 0, /* Unknown angle type */
    AT_BOTH,        /* Both Solar and Satellite angle types */
    AT_SATELLITE,   /* Satellite angle type */
    AT_SOLAR        /* Solar angle type */
} ANGLE_TYPE;

/* This struct is used as a transport median from the API and the band meta data
   to the L8 Angles application and the write image routine. */
typedef struct angles_frame
{
    int band_number;        /* Band number */
    int num_lines;          /* Number of lines in frame */
    int num_samps;          /* Number of samples in frame */
    IAS_DBL_XY ul_corner;   /* Upper left corner coordinates */
    double pixel_size;      /* Pixel size in meters */
    IAS_PROJECTION projection; /* Projection information */ 
} ANGLES_FRAME;

/* Holds all the information needed to run L8 Angles */
typedef struct l8_angles_parameters
{
    int process_band[IAS_MAX_NBANDS]; /* Process bands array */
    char metadata_filename[PATH_MAX]; /* Metadata filename */
    int use_dem_flag;            /* Flag used to check if DEM should be used */
    ANGLE_TYPE angle_type;       /* Type of angles to be generated */
    int sub_sample_factor;       /* Sub-sampling factor to be used */
    short background;            /* Background value used for fill pixels */
} L8_ANGLES_PARAMETERS;

/***************************** PROTOTYPES START *******************************/
int l8_per_pixel_angles
(
    char *angle_coeff_name, /* I: Angle coefficient filename */
    int subsamp_fact,       /* I: Subsample factor used when calculating the
                                  angles (1=full resolution). OW take every Nth
                                  sample from the line, where N=subsamp_fact */
    short fill_pix_value,   /* I: Fill pixel value to use (-32768:32767) */
    char *band_list,        /* I: Band list used to calculate angles for.
                                  "ALL" - defaults to all bands 1 - 11.
                                  Must be comma separated with no spaces in
                                  between.  Example: 1,2,3,4,5,6,7,8,9
                                  The solar/sat_zenith/azimuth arrays should
                                  will have angles processed for these bands */
    ANGLES_FRAME frame[L8_NBANDS],   /* O: Image frame info for each band */
    short *solar_zenith[L8_NBANDS],  /* O: Array of pointers for the solar
                                           zenith angle array, one per band
                                           (if NULL, don't process) */
    short *solar_azimuth[L8_NBANDS], /* O: Array of pointers for the solar
                                           azimuth angle array, one per band
                                           (if NULL, don't process) */
    short *sat_zenith[L8_NBANDS],    /* O: Array of pointers for the satellite
                                           zenith angle array, one per band
                                           (if NULL, don't process)*/
    short *sat_azimuth[L8_NBANDS],   /* O: Array of pointers for the satellite
                                           azimuth angle array, one per band
                                           (if NULL, don't process)*/
    int nlines[L8_NBANDS],  /* O: Number of lines for each band */
    int nsamps[L8_NBANDS]   /* O: Number of samples for each band */
);

int l8_per_pixel_avg_refl_angles
(
    char *angle_coeff_name, /* I: Angle coefficient filename */
    int subsamp_fact,       /* I: Subsample factor used when calculating the
                                  angles (1=full resolution). OW take every Nth
                                  sample from the line, where N=subsamp_fact */
    short fill_pix_value,   /* I: Fill pixel value to use (-32768:32767) */
    ANGLES_FRAME *avg_frame,  /* O: Image frame info for the scene */
    short **avg_solar_zenith, /* O: Addr of pointer for the average solar zenith
                                    angle array (if NULL, don't process),
                                    degrees scaled by 100 */
    short **avg_solar_azimuth,/* O: Addr of pointer for the average solar
                                    azimuth angle array (if NULL, don't
                                    process), degrees scaled by 100 */
    short **avg_sat_zenith,   /* O: Addr of pointer for the average satellite
                                    zenith angle array (if NULL, don't process),
                                    degrees scaled by 100 */
    short **avg_sat_azimuth,  /* O: Addr of pointer for the average satellite
                                    azimuth angle array (if NULL, don't
                                    process), degrees scaled by 100 */
    int *avg_nlines,          /* O: Number of lines for the bands, based on
                                    on the subsample factor */
    int *avg_nsamps           /* O: Number of samples for the bands, based on
                                    the subsample factor */
);

int calculate_angles
(
    const IAS_ANGLE_GEN_METADATA *metadata, /* I: Angle metadata structure */ 
    int line,                               /* I: L1T line coordinate */
    int samp,                               /* I: L1T sample coordinate */
    int band_index,                         /* I: Spectral band number */
    ANGLE_TYPE angle_type,                  /* I: Type of angles to generate */
    double *sat_angles,                     /* O: Satellite angles */
    double *sun_angles                      /* O: Solar angles */
);

const double *get_active_lines
(
    const IAS_ANGLE_GEN_METADATA *metadata, /* I: Angle metadata structure */ 
    int band_index                          /* I: Band index */
);

const double *get_active_samples
(
    const IAS_ANGLE_GEN_METADATA *metadata, /* I: Angle metadata structure */ 
    int band_index                          /* I: Band index */
);

int get_frame
(
    const IAS_ANGLE_GEN_METADATA *metadata, /* I: Angle metadata structure */ 
    int band_index,                         /* I: Band index */
    ANGLES_FRAME *frame                     /* O: Image frame info */
);

int read_parameters
(
    const char *parameter_filename,       /* I: Parameter file name */
    L8_ANGLES_PARAMETERS *parameters      /* O: Generate Chips parameters */
);

#endif
