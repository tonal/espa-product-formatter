/*****************************************************************************
FILE: subset_metadata.h
  
PURPOSE: Contains prototypes and defines for subsetting the XML metadata.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/10/2014    Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef SUBSET_METADATA_H
#define SUBSET_METADATA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "error_handler.h"
#include "espa_metadata.h"
#include "parse_metadata.h"
#include "write_metadata.h"

/* Defines */

/* Prototypes */
int subset_metadata_by_product
(
    Espa_internal_meta_t *inmeta,  /* I: input metadata structure to be
                                           subset */
    Espa_internal_meta_t *outmeta, /* O: output metadata structure containing
                                         only the specified bands */
    int nproducts,                 /* I: number of product types to be included
                                         in the subset product */
    char products[][STR_SIZE]      /* I: array of nproducts product types to be
                                         used for subsetting */
);

int subset_metadata_by_band
(
    Espa_internal_meta_t *inmeta,  /* I: input metadata structure to be
                                           subset */
    Espa_internal_meta_t *outmeta, /* O: output metadata structure containing
                                         only the specified bands */
    int nbands,                    /* I: number of bands to be included in
                                         the subset product */
    char bands[][STR_SIZE]         /* I: array of nbands band names to be used
                                         for subsetting */
);

int subset_xml_by_product
(
    char *in_xml_file,   /* I: input XML file to be subset */
    char *out_xml_file,  /* I: output XML file to be subset */
    int nproducts,       /* I: number of product types to be included in the
                               subset product */
    char products[][STR_SIZE] /* I: array of nproducts product types to be used
                               for subsetting */
);

int subset_xml_by_band
(
    char *in_xml_file,   /* I: input XML file to be subset */
    char *out_xml_file,  /* I: output XML file to be subset */
    int nbands,          /* I: number of bands to be included in the subset
                               XML file */
    char bands[][STR_SIZE] /* I: array of nbands band names to be appear in
                               the subset XML file */
);

#endif
