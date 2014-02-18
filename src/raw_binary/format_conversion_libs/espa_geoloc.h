/*****************************************************************************
FILE: espa_geoloc.h
  
PURPOSE: Contains defines and prototypes for handling geolocation-related
conversions and computations.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/23/2014    Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef ESPA_GEOLOC_H
#define ESPA_GEOLOC_H

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "common.h"
#include "error_handler.h"
#include "espa_metadata.h"

/* Constants */
#ifndef NPROJ_PARAM
#define NPROJ_PARAM 15
#endif

#ifndef PI
#define PI (3.141592653589793238)
#endif

#define DEG (180.0 / PI)
#define RAD (PI / 180.0)

/* Utilities for max/min computation */
#define max(A,B) (A>B ? A:B)
#define min(A,B) (A>B ? B:A)


/* Bounding coordinates data structure */
typedef struct
{
  double min_lon;  /* Geodetic longitude coordinate (degrees) */ 
  double min_lat;  /* Geodetic latitude coordinate (degrees) */ 
  double max_lon;  /* Geodetic longitude coordinate (degrees) */ 
  double max_lat;  /* Geodetic latitude coordinate (degrees) */ 
  bool is_fill;    /* Flag to indicate whether the point is a fill value; */
} Geo_bounds_t;

/* Integer image coordinates data structure */
typedef struct
{
    int l;           /* line number */        
    int s;           /* sample number */
} Img_coord_int_t;

/* Floating point image coordinates data structure */
typedef struct
{
    float l;          /* line number */
    float s;          /* sample number */
    bool is_fill;     /* fill value flag; 
                         'true' = is fill; 'false' = is not fill */
} Img_coord_float_t;

/* Structure to store map projection coordinates */
typedef struct
{
    double x;        /* Map projection X coordinate (meters) */
    double y;        /* Map projection Y coordinate (meters) */
    bool is_fill;    /* Flag to indicate whether the point is a fill value;
                        'true' = fill; 'false' = not fill */
} Map_coord_t;

/* Structure to store geodetic coordinates */
typedef struct
{
    double lon;      /* Geodetic longitude coordinate (radians) */ 
    double lat;      /* Geodetic latitude coordinate (radians) */ 
    bool is_fill;    /* Flag to indicate whether the point is a fill value;
                        'true' = fill; 'false' = not fill */
} Geo_coord_t;

/* Structure to store the space definition */
typedef struct
{
    int proj_num;           /* GCTP map projection number */
    double proj_param[NPROJ_PARAM]; /* GCTP map projection parameters */
    float pixel_size[2];    /* Pixel size (meters) (x, y) */
    Map_coord_t ul_corner;  /* Map projection coordinates of the upper left 
                               corner of the pixel in the upper left corner 
                               of the image */
    bool ul_corner_set;     /* Flag to indicate whether the upper left corner
                               has been set; 'true' = set; 'false' = not set */
    Img_coord_int_t img_size;  /* Image size (lines, samples) */
    int zone;               /* GCTP zone number */
    bool zone_set;          /* Flag to indicate whether the zone has been set;
                               'true' = set; 'false' = not set */
    int sphere;             /* GCTP sphere number */
    double orientation_angle;  /* Orientation of the image with respect to 
                               map north (radians).  A positive angle causes 
                               the image to be rotated counter-clockwise. */
} Space_def_t;

/* Structure to store the geolocation information */
typedef struct
{
    Space_def_t def;       /* Space definition structure */
    int (*for_trans)(double lat, double lon, double *x, double *y);
                           /* Forward transformation function call */
    int (*inv_trans)(double x, double y, double *lat, double *lon);
                           /* Inverse transformation function call */
    double cos_orien;      /* Cosine of the orientation angle */
    double sin_orien;      /* Sine of the orientation angle */
} Geoloc_t;

/* Prototypes */
bool get_geoloc_info
(
    Espa_internal_meta_t *xml_metadata, /* I: XML metadata information */
    Space_def_t *geoloc_info            /* O: geolocation space information */
);

Geoloc_t *setup_mapping
(
    Space_def_t *space_def     /* I: space definition structure */
);

bool to_space
(
    Geoloc_t *this,          /* I: geolocation structure; for_trans function
                                   is used for the forward mapping */
    Geo_coord_t *geo,        /* I: geodetic coordinates (radians) */
    Img_coord_float_t *img   /* O: image coordinates */
);

bool from_space
(
    Geoloc_t *this,          /* I: geolocation structure; inv_trans function
                                   is used for the inverse mapping */
    Img_coord_float_t *img,  /* I: image coordinates */
    Geo_coord_t *geo         /* O: geodetic coordinates (radians) */
);

bool compute_bounds
(
    Geoloc_t *space,          /* I: geolocation structure which contains the
                                    information used for forward/inverse
                                    mapping */
    int nlines,               /* I: number of lines in the scene */
    int nsamps,               /* I: number of samples in the scene */
    Geo_bounds_t *bounds      /* O: output boundary for the scene */
);

bool degdms
(
    double *deg,     /* I: input angular value in degrees, minutes, or seconds
                           to be converted to DMS; or input angle in DMS to be
                           validated (no converstion occurs) */
    double *dms,     /* O: output angular value in DMS converted from degrees,
                           minutes, or seconds; or copy of 'deg' value
                           (code=DMS) */
    char *code,      /* I: code to identify if the input angle is in degrees,
                           minutes, or seconds (DEG, MIN, SEC) for conversion
                           to DMS; or code to identify the input 'deg' value
                           is in DMS in which case the DMS value will simply
                           be validated and not converted */
    char *check      /* I: angle type to check/validate (LAT, LON, OTHER) */
);

int find_deg
(
    double angle    /* I: angle in total degrees */
);

int find_min
(
    double angle    /* I: angle in total degrees */
);

double find_sec
(
    double angle    /* I: angle in total degrees */
);

#endif
