/*****************************************************************************
FILE: espa_metadata.h
  
PURPOSE: Contains ESPA internal metadata related defines and structures

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/13/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef ESPA_METADATA_H
#define ESPA_METADATA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlschemastypes.h>
#include "error_handler.h"
#include "meta_stack.h"
#include "gctp_defines.h"

/* Defines */
#define LIBXML_SCHEMAS_ENABLED
#define ESPA_SCHEMA_VERSION "1.0.0"
#define ESPA_NS "http://espa.cr.usgs.gov/v1.0"
#define ESPA_SCHEMA_LOCATION "http://espa.cr.usgs.gov/v1.0"
//#define ESPA_SCHEMA "http://espa.cr.usgs.gov/static/schema/espa_internal_metadata_v1_0.xsd"
#define ESPA_SCHEMA "/home/gschmidt/espa_internal_metadata_v1_0.xsd"

/* Data types */
enum Espa_data_type
{
    ESPA_INT8, ESPA_UINT8, ESPA_INT16, ESPA_UINT16, ESPA_INT32, ESPA_UINT32,
    ESPA_FLOAT32, ESPA_FLOAT64
};

/* Geographic types */
enum Espa_geographic_type
{
    ESPA_WEST, ESPA_EAST, ESPA_NORTH, ESPA_SOUTH
};

/* Local defines to use for the datum */
#define GCTP_WGS84 12

/* Local define to specify the maximum total bands and product types.  This
   should be sufficient for ESPA products. */
#define MAX_TOTAL_BANDS 100
#define MAX_TOTAL_PRODUCT_TYPES 50

/* Local defines for fill or not used data values in the metadata */
#define ESPA_INT_META_FILL -3333
#define ESPA_FLOAT_META_FILL -3333.00
#define ESPA_STRING_META_FILL "undefined"
#define ESPA_EPSILON 0.00001

/* Structures to support the global and band metadata information stored in
   the ESPA internal metadata file */
typedef struct
{
    int class;                    /* class value */
    char description[STR_SIZE];   /* class description */
} Espa_class_t;

typedef struct
{
    int proj_type;        /* projection number (see #defines above) */
    int sphere_code;      /* spheroid number (see #defines above) */
    char units[STR_SIZE]; /* projection units (degrees, meters) */
    double ul_corner[2];  /* projection UL x, y (store center of the pixel
                             for multi-res products) */
    double lr_corner[2];  /* projection LR x, y (store center of the pixel
                             for multi-res products) */
    char grid_origin[STR_SIZE];  /* origin of the gridded data (UL, CENTER) */

    /* UTM projection parameters */
    int utm_zone;         /* UTM zone; use a negative number if this is a
                             southern zone */
    /* PS projection parameters */
    double longitude_pole;
    double latitude_true_scale;
    double false_easting;
    double false_northing;

    /* ALBERS projection parameters */
    double standard_parallel1;
    double standard_parallel2;
    double central_meridian;
    double origin_latitude;
    /* double false_easting;   -- used from the PS proj parms */
    /* double false_northing;  -- used from the PS proj parms */
} Espa_proj_meta_t;

typedef struct
{
    char data_provider[STR_SIZE]; /* name of the original data provider */
    char satellite[STR_SIZE];     /* name of the satellite (LANDSAT_4,
                                     LANDSAT_5, LANDSAT_7) */
    char instrument[STR_SIZE];    /* name of instrument (MSS, TM, ETM+, OLI,
                                     ...) */
    char acquisition_date[STR_SIZE];  /* date of scene acquisition */
    char scene_center_time[STR_SIZE]; /* GMT time at scene center */
    char level1_production_date[STR_SIZE];  /* date the scene was processed
                                               to a level 1 product */
    float solar_zenith;           /* solar zenith angle in degrees */
    float solar_azimuth;          /* solar azimuth angle in degrees */
    char solar_units[STR_SIZE];   /* degrees */
    int wrs_system;               /* 1 or 2 */
    int wrs_path;                 /* WRS path of this scene */
    int wrs_row;                  /* WRS row of this scene */
    char lpgs_metadata_file[STR_SIZE];  /* name of LPGS metadata file */
    double ul_corner[2];  /* geographic UL lat, long */
    double lr_corner[2];  /* geographic LR lat, long */
    double bounding_coords[4];   /* geographic west, east, north, south */
    Espa_proj_meta_t proj_info;  /* projection information structure */
    float orientation_angle;     /* orientation angle of this scene (degrees) */
} Espa_global_meta_t;

typedef struct
{
    char product[STR_SIZE];      /* product type */
    char source[STR_SIZE];       /* source type (level1, toa_refl, sr_refl) */
    char name[STR_SIZE];         /* band name */
    char category[STR_SIZE];     /* category type (image, qa, browse, index) */
    enum Espa_data_type data_type;  /* data type of this band */
    int nlines;                  /* number of lines in the dataset */
    int nsamps;                  /* number of samples in the dataset */
    long fill_value;             /* use long to support long data types */
    int saturate_value;          /* saturation value */
    float scale_factor;          /* scaling factor */
    float add_offset;            /* offset to be added */
    char short_name[STR_SIZE];   /* short band name */
    char long_name[STR_SIZE];    /* long band name */
    char file_name[STR_SIZE];    /* raw binary file name for this band w/o the
                                    pathname */
    float pixel_size[2];         /* pixel size x, y */
    char pixel_units[STR_SIZE];  /* units for pixel size (meters, degrees) */
    char data_units[STR_SIZE];   /* units of data stored in this band */
    long valid_range[2];         /* use long to support the long data types
                                    min, max */
    float toa_gain;              /* gain values for top-of-atmosphere refl */
    float toa_bias;              /* bias values for top-of-atmosphere refl */
    int nbits;                   /* number of bits in bitmap_description */
    char **bitmap_description;   /* support bit mapping description;
                                    0-based going from right to left in the
                                    binary representation; assume the XML file
                                    has a description for every bit number
                                    inclusive from 0 to nbits-1 */
    int nclass;                  /* number of classes in class_values */
    Espa_class_t *class_values;  /* support class value descriptions */
    float calibrated_nt;
    char app_version[STR_SIZE];  /* version of the application which produced
                                    the current band */
    char production_date[STR_SIZE];  /* date the band was produced */
} Espa_band_meta_t;

typedef struct
{
    char meta_namespace[STR_SIZE];  /* namespace for this metadata file */
    Espa_global_meta_t global;  /* global metadata */
    int nbands;                 /* number of bands in the metadata file */
    Espa_band_meta_t *band;     /* array of band metadata */
} Espa_internal_meta_t;

/* Prototypes */
int validate_xml_file
(
    char *meta_file,          /* I: name of metadata file to be validated */
    char *schema_file         /* I: name of schema file or URL to be validated
                                    against */
);

void init_metadata_struct
(
    Espa_internal_meta_t *internal_meta   /* I: pointer to internal metadata
                                                structure to be initialized */
);

int allocate_band_metadata
(
    Espa_internal_meta_t *internal_meta,  /* I: pointer to internal metadata
                                                structure */
    int nbands                            /* I: number of bands to allocate
                                                for the band field in the
                                                internal_meta */
);

int allocate_class_metadata
(
    Espa_band_meta_t *band_meta,  /* I: pointer to band metadata structure */
    int nclass                    /* I: number of classes to allocate for the
                                        band metadata */
);

int allocate_bitmap_metadata
(
    Espa_band_meta_t *band_meta,  /* I: pointer to band metadata structure */
    int nbits                     /* I: number of bits to allocate for the
                                        bitmap metadata */
);

void free_metadata
(
    Espa_internal_meta_t *internal_meta   /* I: pointer to internal metadata
                                                structure */
);

void print_element_names
(
    xmlNode *a_node   /* I: pointer to the current node in the tree to start
                            printing */
);

void print_metadata_struct
(
    Espa_internal_meta_t *metadata  /* I: input metadata structure to be
                                          printed */
);

#endif
