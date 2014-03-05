/*****************************************************************************
FILE: convert_espa_hdf.h
  
PURPOSE: Contains defines and prototypes to read the ESPA XML metadata file
and imagery, and convert from raw binary to HDF file format.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/6/2014     Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef CONVERT_ESPA_TO_HDF_H
#define CONVERT_ESPA_TO_HDF_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hdf.h>
#include <mfhdf.h>
#include "error_handler.h"
#include "espa_metadata.h"
#include "parse_metadata.h"
#include "espa_hdf.h"
#include "espa_hdf_eos.h"
#include "envi_header.h"
#include "raw_binary_io.h"

/* Defines */
#define HDF_ERROR -1

/* Prototypes */
int write_global_attributes
(
    int32 hdf_id,               /* I: HDF file ID to write attributes */
    Espa_internal_meta_t *xml_metadata  /* I: pointer to metadata structure */
);

int write_sds_attributes
(
    int32 sds_id,             /* I: SDS ID to write attributes */
    Espa_band_meta_t *bmeta   /* I: pointer to band metadata structure */
);

int create_hdf_metadata
(
    char *hdf_file,                     /* I: output HDF filename */
    Espa_internal_meta_t *xml_metadata  /* I: XML metadata structure */
);

int convert_espa_to_hdf
(
    char *espa_xml_file,   /* I: input ESPA XML metadata filename */
    char *hdf_file         /* I: output HDF filename */
);

#endif
