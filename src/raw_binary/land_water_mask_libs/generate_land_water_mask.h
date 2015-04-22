#ifndef LAND_WATER_MASK_H
#define LAND_WATER_MASK_H

/* ESPA Includes */
#include "error_handler.h"
#include "espa_metadata.h"
#include "espa_hdf_eos.h"

/* IAS Includes */
#include "ias_lw_geo.h"
#include "gctp.h"

#define NAME_STRLEN  256

double deg_to_dms
(
    double flt_deg   /* I: input decimal degree value */
);

int generate_land_water_mask
(
    Espa_internal_meta_t *xml_meta,   /* I: input XML metadata */
    const char land_mass_polygon[],   /* I: name of land mass polygon file */
    unsigned char **land_water_mask,  /* O: pointer to land water mask buffer,
                                            memory is allocated and the
                                            mask is populated */
    int *nlines,                      /* O: number of lines in the mask */
    int *nsamps                       /* O: number of samples in the mask */
);

#endif
