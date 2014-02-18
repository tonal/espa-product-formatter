/*****************************************************************************
FILE: espa_hdf.h
  
PURPOSE: Contains defines and prototypes for handling HDF files and attributes.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/7/2014     Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef ESPA_HDF_H
#define ESPA_HDF_H

#include <stdlib.h>
#include <stdbool.h>
#include "hdf.h"
#include "mfhdf.h"
#include "error_handler.h"

/* Defines */
/* maximum number of attribute values expected */
#define MYHDF_MAX_NATTR_VAL (3000)
#define HDF_ERROR -1

/* structure to store information about the HDF attribute */
typedef struct
{
  int32 id, type, nval;	 /* id, data type and number of values */
  char *name;            /* attribute name */
} Espa_hdf_attr_t;

/* Prototypes */
int put_attr_double
(
    int32 sds_id,          /* I: SDS ID to write attribute to */
    Espa_hdf_attr_t *attr, /* I: attribute data structure */
    double *val            /* I: array of values to be written as native type */
);

int put_attr_string
(
    int32 sds_id,          /* I: SDS ID to write attribute to */
    Espa_hdf_attr_t *attr, /* I: attribute data structure */
    char *string           /* I: string value to be written */
);

#endif
