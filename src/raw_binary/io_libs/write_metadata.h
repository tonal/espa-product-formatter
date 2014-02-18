/*****************************************************************************
FILE: write_metadata.h
  
PURPOSE: Contains ESPA metadata write and print related prototypes and defines

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/27/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef WRITE_METADATA_H
#define WRITE_METADATA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "error_handler.h"
#include "espa_metadata.h"

/* Defines */
/* maximum number of characters per line in the XML file */
#define MAX_LINE_SIZE 1024

/* Prototypes */
int write_metadata
(
    Espa_internal_meta_t *metadata,  /* I: input metadata structure to be
                                           written to XML */
    char *xml_file                   /* I: name of the XML metadata file to
                                           be written to or overwritten */
);

int append_metadata
(
    int nbands,               /* I: number of bands to be appended */
    Espa_band_meta_t *bmeta,  /* I: pointer to the array of bands metadata
                                    containing nbands */
    char *xml_file            /* I: name of the XML metadata file for appending
                                    the bands in bmeta */
);

void print_metadata_struct
(
    Espa_internal_meta_t *metadata  /* I: input metadata structure to be
                                          printed */
);

#endif
