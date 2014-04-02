/*****************************************************************************
FILE: convert_espa_gtif.h
  
PURPOSE: Contains defines and prototypes to read the ESPA XML metadata file
and imagery, and convert from raw binary to HDF file format.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/9/2014     Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef CONVERT_ESPA_TO_GTIF_H
#define CONVERT_ESPA_TO_GTIF_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "error_handler.h"
#include "espa_metadata.h"
#include "parse_metadata.h"

/* Defines */

/* Prototypes */
int convert_espa_to_gtif
(
    char *espa_xml_file,   /* I: input ESPA XML metadata filename */
    char *gtif_file,       /* I: base output GeoTIFF filename */
    bool del_src           /* I: should the source files be removed after
                                 conversion? */
);

#endif
