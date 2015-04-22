/*****************************************************************************
FILE: convert_lpgs_to_espa.h
  
PURPOSE: Contains defines and prototypes to read the LPGS MTL file, create
the XML metadata file, and convert from GeoTIFF to raw binary file format.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/30/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef CONVERT_LPGS_TO_ESPA_H
#define CONVERT_LPGS_TO_ESPA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "geotiffio.h"
#include "xtiffio.h"
#include "error_handler.h"
#include "espa_metadata.h"
#include "espa_geoloc.h"
#include "raw_binary_io.h"
#include "write_metadata.h"
#include "envi_header.h"

/* Defines */
/* Maximum number of LPGS bands in a file; OLI/TIRS products have the most
   bands (11 image bands plus the quality band); TM has 7 bands; ETM+ has
   9 bands */
#define MAX_LPGS_BANDS 12

/* Prototypes */
int read_lpgs_mtl
(
    char *mtl_file,                  /* I: name of the MTL metadata file to
                                           be read */
    Espa_internal_meta_t *metadata,  /* I/O: input metadata structure to be
                                           populated from the MTL file */
    int *nlpgs_bands,                /* O: number of bands in LPGS product */
    char lpgs_bands[][STR_SIZE]      /* O: array containing the filenames of
                                           the LPGS bands */
);

int convert_gtif_to_img
(
    char *gtif_file,           /* I: name of the input GeoTIFF file */
    Espa_band_meta_t *bmeta,   /* I: pointer to band metadata for this band */
    Espa_global_meta_t *gmeta  /* I: pointer to global metadata */
);

int convert_lpgs_to_espa
(
    char *lpgs_mtl_file,   /* I: input LPGS MTL metadata filename */
    char *espa_xml_file,   /* I: output ESPA XML metadata filename */
    bool del_src           /* I: should the source .tif files be removed after
                                 conversion? */
);

#endif
