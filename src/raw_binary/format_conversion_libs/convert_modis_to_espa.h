/*****************************************************************************
FILE: convert_modis_to_espa.h
  
PURPOSE: Contains defines and prototypes to read supported MODIS files, create
the XML metadata file, and convert from HDF-EOS to raw binary file format.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/22/2014    Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef CONVERT_MODIS_TO_ESPA_H
#define CONVERT_MODIS_TO_ESPA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hdf.h>
#include <mfhdf.h>
#include <HdfEosDef.h>
#include "error_handler.h"
#include "espa_metadata.h"
#include "espa_geoloc.h"
#include "raw_binary_io.h"
#include "write_metadata.h"
#include "envi_header.h"

/* Defines */
/* maximum number of MODIS bands/SDSs in a file */
#define MAX_MODIS_BANDS 50

/* maximum number of dimensions for each SDS, even though we will only support
   2D - still need to be able to read the dimensions from the SDS */
#define MAX_MODIS_DIMS 10

/* maximum number of grids for each file */
#define MAX_MODIS_GRIDS 10

/* Prototypes */
int read_modis_hdf
(
    char *modis_hdf_name,            /* I: name of MODIS file to be read */
    Espa_internal_meta_t *metadata   /* I/O: input metadata structure to be
                                           populated from the MODIS file */
);

int convert_hdf_to_img
(
    char *modis_hdf_name,      /* I: name of MODIS file to be processed */
    Espa_internal_meta_t *xml_metadata /* I: metadata structure for HDF file */
);

int convert_modis_to_espa
(
    char *modis_hdf_file,  /* I: input MODIS HDF filename */
    char *espa_xml_file,   /* I: output ESPA XML metadata filename */
    bool del_src           /* I: should the source .tif files be removed after
                                 conversion? */
);

#endif
