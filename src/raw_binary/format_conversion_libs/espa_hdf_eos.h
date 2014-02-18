/*****************************************************************************
FILE: espa_hdf_eos.h
  
PURPOSE: Contains defines and prototypes for handling HDF-EOS files and
attributes.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/7/2014     Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef ESPA_HDF_EOS_H
#define ESPA_HDF_EOS_H

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"
#include "espa_metadata.h"
#include "espa_hdf.h"
#include "error_handler.h"


/* Constants */
#define NPROJ_PARAM 15

/* size of the metadata string for defining the HDF-EOS metadata */
#define ESPA_MAX_METADATA_SIZE 10000

/* Prototypes */
bool append_meta
(
    char *cbuf,  /* I/O: input metadata buffer */
    int *ic,     /* I/O: index of current location in metadata buffer */
    char *str    /* I: string to append to the metadata buffer */
);

int write_hdf_eos_attr
(
    char *hdf_file,            /* I: HDF file to write attributes to */
    Espa_internal_meta_t *xml_metadata  /* I: XML metadata structure */
);

#endif
