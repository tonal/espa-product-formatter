/*****************************************************************************
FILE: parse_metadata.h
  
PURPOSE: Contains prototypes for parsing the ESPA internal metadata

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development
4/17/2014    Gail Schmidt     Added support for additional projection params

NOTES:
*****************************************************************************/

#ifndef PARSE_METADATA_H
#define PARSE_METADATA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlschemastypes.h>
#include "error_handler.h"
#include "espa_metadata.h"

int add_global_metadata_proj_info_albers
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
);

int add_global_metadata_proj_info_ps
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
);

int add_global_metadata_proj_info_utm
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
);

int add_global_metadata_proj_info_sin
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
);

int add_global_metadata_proj_info
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
);

int add_global_metadata_bounding_coords
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
);

int add_global_metadata
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_global_meta_t *gmeta   /* I: global metadata structure */
);

int add_band_metadata_bitmap_description
(
    xmlNode *a_node,            /* I/O: pointer to the element node to
                                        process */
    Espa_band_meta_t *bmeta     /* I: band metadata structure for current
                                      band in the bands structure */
);

int add_band_metadata_class_values
(
    xmlNode *a_node,            /* I/O: pointer to the element node to
                                        process */
    Espa_band_meta_t *bmeta     /* I: band metadata structure for current
                                      band in the bands structure */
);

int add_band_metadata
(
    xmlNode *a_node,            /* I: pointer to the element node to process */
    Espa_band_meta_t *bmeta     /* I: band metadata structure for current
                                      band in the bands structure */
);

int parse_xml_into_struct
(
    xmlNode *a_node,                  /* I: pointer to the current node */
    Espa_internal_meta_t *metadata,   /* I: ESPA internal metadata structure
                                            to be filled */
    int *top_of_stack,                /* I: pointer to top of the stack */
    char **stack                      /* I: stack to use for parsing */
);

int parse_metadata
(
    char *metafile,                 /* I: input metadata file or URL */
    Espa_internal_meta_t *metadata  /* I: input metadata structure which has
                                          been initialized via
                                          init_metadata_struct */
);

#endif
