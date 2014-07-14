/*****************************************************************************
FILE: envi_header.h
  
PURPOSE: Contains ENVI header related defines and structures

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development
3/31/2014    Ron Dilley       Added fill value to the ENVI header structure
4/17/2014    Gail Schmidt     Added support for additional projections

NOTES:
*****************************************************************************/

#ifndef ENVI_HEADER_H
#define ENVI_HEADER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "error_handler.h"
#include "espa_metadata.h"
#include "gctp_defines.h"

/* Defines */
/* ENVI projection numbers */
#define ENVI_GEO_PROJ 1
#define ENVI_UTM_PROJ 2
#define ENVI_ALBERS_PROJ 9
#define ENVI_SIN_PROJ 16
#define ENVI_PS_PROJ 31

/* most of the time there will be only one band per ENVI file, but define
   the maximum to be 50 for the HDF ENVI headers */
#define MAX_ENVI_BANDS 50

/* Structure to contain the ENVI header information */
typedef struct {
    char description[STR_SIZE];  /* description of file */
    int nlines;          /* number of lines in the file */
    int nsamps;          /* number of samples in the file */
    int nbands;          /* number of bands in the file */
    int header_offset;   /* number of bytes to skip when reading the image
                            data, usually 0 for our applications */
    int byte_order;      /* ENVI byte order; 0 - Intel, 1 - network (IEEE) */
    char file_type[STR_SIZE]; /* "ENVI Standard", "HDF Scientific Data",
                                 "GeoTIFF", etc. */
    int data_type;       /* ENVI data type
                            1 = 8-bit byte 
                            2 = 16-bit signed integer 
                            3 = 32-bit signed long integer 
                            4 = 32-bit floating point 
                            5 = 64-bit double-precision floating point 
                            12 = 16-bit unsigned integer 
                            13 = 32-bit unsigned long integer 
                            14 = 64-bit signed long integer 
                            15 = 64-bit unsigned long integer */
    long data_ignore_value;  /* value designated as fill for the data */
    char interleave[4];  /* "BSQ" for band sequential,
                            "BIL" for band interleave by line,
                            "BIP" for band interleave by pixel */
    char sensor_type[STR_SIZE];   /* "Landsat TM", "Landat ETM", etc. */
    int proj_type;       /* ENVI projection number (see #defines above) */
    int datum_type;      /* ENVI datum type (see #defines above) */
    int utm_zone;        /* UTM zone; use a negative number if this is a
                            southern zone */
    double proj_parms[15];  /* projection parameters */
    double pixel_size[2];   /* pixel size - x, y */
    double ul_corner[2];    /* upper left corner projection coords - x, y */
    int xy_start[2];        /* x/y starting locations for the UL projection
                               coords; usually 1, 1 */
    char band_names[MAX_ENVI_BANDS][STR_SIZE];  /* array of band names;
                               size is nbands */
} Envi_header_t;


/* Prototypes */
int write_envi_hdr
(
    char *hdr_file,     /* I: name of ENVI header file to be generated */
    Envi_header_t *hdr  /* I: input ENVI header information */
);

int create_envi_struct
(
    Espa_band_meta_t *bmeta,   /* I: pointer to band metadata for this band */
    Espa_global_meta_t *gmeta, /* I: pointer to global metadata */
    Envi_header_t *hdr         /* I/O: output ENVI header information */
);

#endif
