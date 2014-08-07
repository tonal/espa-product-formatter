/*****************************************************************************
FILE: subset_metadata.c
  
PURPOSE: Contains functions for subsetting the metadata, currently just the
bands in the metadata, including subsetting the bands by band name or by
product type.  At this time, spatial subsetting is not supported.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/10/2014    Gail Schmidt     Original development
2/25/2014    Gail Schmidt     Added support for source and category attributes
                              for the band metadata

NOTES:
  1. The XML metadata format written via this library follows the ESPA internal
     metadata format found in ESPA Raw Binary Format v1.0.doc.  The schema for
     the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/
#include <math.h>
#include "subset_metadata.h"

/******************************************************************************
MODULE:  subset_metadata_by_product

PURPOSE: Subset the current metadata structure to contain only the specified
bands which match the product type.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error subsetting the metadata structure
SUCCESS         Successfully subset the metadata structure

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/10/2014    Gail Schmidt     Original development
4/22/2014    Gail Schmidt     Updated for additional projection parameters and
                              datums
5/7/2014     Gail Schmidt     Updated for modis tiles

NOTES:
  1. If no bands match the product type, then the global and projection
     information will still be copied.
******************************************************************************/
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
)
{
    char FUNC_NAME[] = "subset_metadata_by_product";   /* function name */
    char errmsg[STR_SIZE];   /* error message */
    int i, j, k;             /* looping variables */
    int iband;               /* current output band */
    int count;               /* number of chars copied in snprintf */
    bool found;              /* was the product found */

    /* Initialize the output metadata structure */
    init_metadata_struct (outmeta);

    /* Allocate output metadata for the bands to be included.  Allocate for
       the total number of input bands, then we'll modify later for the
       actual number of bands when it's known. */
    if (allocate_band_metadata (outmeta, inmeta->nbands) != SUCCESS)
    {  /* Error messages already printed */
        return (ERROR);
    }

    /* Copy the high level metadata structure attributes */
    count = snprintf (outmeta->meta_namespace, sizeof (outmeta->meta_namespace),
        "%s", inmeta->meta_namespace);
    if (count < 0 || count >= sizeof (outmeta->meta_namespace))
    {
        sprintf (errmsg, "Overflow of outmeta->meta_namespace string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Copy the global metadata */
    count = snprintf (outmeta->global.data_provider,
        sizeof (outmeta->global.data_provider), "%s",
        inmeta->global.data_provider);
    if (count < 0 || count >= sizeof (outmeta->global.data_provider))
    {
        sprintf (errmsg, "Overflow of outmeta->global.data_provider string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.satellite, 
        sizeof (outmeta->global.satellite), "%s", inmeta->global.satellite);
    if (count < 0 || count >= sizeof (outmeta->global.satellite))
    {
        sprintf (errmsg, "Overflow of outmeta->global.satellite string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.instrument,
        sizeof (outmeta->global.instrument), "%s", inmeta->global.instrument);
    if (count < 0 || count >= sizeof (outmeta->global.instrument))
    {
        sprintf (errmsg, "Overflow of outmeta->global.instrument string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.acquisition_date,
        sizeof (outmeta->global.acquisition_date), "%s",
        inmeta->global.acquisition_date);
    if (count < 0 || count >= sizeof (outmeta->global.acquisition_date))
    {
        sprintf (errmsg, "Overflow of outmeta->global.acquisition_date string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.scene_center_time,
        sizeof (outmeta->global.scene_center_time), "%s",
        inmeta->global.scene_center_time);
    if (count < 0 || count >= sizeof (outmeta->global.scene_center_time))
    {
        sprintf (errmsg, "Overflow of outmeta->global.scene_center_time");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.level1_production_date,
        sizeof (outmeta->global.level1_production_date), "%s",
        inmeta->global.level1_production_date);
    if (count < 0 || count >= sizeof (outmeta->global.level1_production_date))
    {
        sprintf (errmsg, "Overflow of outmeta->global.level1_production_date");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.solar_zenith = inmeta->global.solar_zenith;
    outmeta->global.solar_azimuth = inmeta->global.solar_azimuth;

    count = snprintf (outmeta->global.solar_units,
        sizeof (outmeta->global.solar_units), "%s", inmeta->global.solar_units);
    if (count < 0 || count >= sizeof (outmeta->global.solar_units))
    {
        sprintf (errmsg, "Overflow of outmeta->global.solar_units string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.wrs_system = inmeta->global.wrs_system;
    outmeta->global.wrs_path = inmeta->global.wrs_path;
    outmeta->global.wrs_row = inmeta->global.wrs_row;

    outmeta->global.htile = inmeta->global.htile;
    outmeta->global.vtile = inmeta->global.vtile;

    count = snprintf (outmeta->global.lpgs_metadata_file,
        sizeof (outmeta->global.lpgs_metadata_file), "%s",
        inmeta->global.lpgs_metadata_file);
    if (count < 0 || count >= sizeof (outmeta->global.lpgs_metadata_file))
    {
        sprintf (errmsg, "Overflow of outmeta->global.lpgs_metadata_file");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.ul_corner[0] = inmeta->global.ul_corner[0];
    outmeta->global.ul_corner[1] = inmeta->global.ul_corner[1];
    outmeta->global.lr_corner[0] = inmeta->global.lr_corner[0];
    outmeta->global.lr_corner[1] = inmeta->global.lr_corner[1];
    outmeta->global.bounding_coords[ESPA_WEST] =
        inmeta->global.bounding_coords[ESPA_WEST];
    outmeta->global.bounding_coords[ESPA_EAST] =
        inmeta->global.bounding_coords[ESPA_EAST];
    outmeta->global.bounding_coords[ESPA_NORTH] =
        inmeta->global.bounding_coords[ESPA_NORTH];
    outmeta->global.bounding_coords[ESPA_SOUTH] =
        inmeta->global.bounding_coords[ESPA_SOUTH];

    /* Copy the projection information */
    outmeta->global.proj_info.proj_type = inmeta->global.proj_info.proj_type;
    outmeta->global.proj_info.datum_type = inmeta->global.proj_info.datum_type;

    count = snprintf (outmeta->global.proj_info.units,
        sizeof (outmeta->global.proj_info.units), "%s",
        inmeta->global.proj_info.units);
    if (count < 0 || count >= sizeof (outmeta->global.proj_info.units))
    {
        sprintf (errmsg, "Overflow of outmeta->global.proj_info.units string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.proj_info.ul_corner[0] =
        inmeta->global.proj_info.ul_corner[0];
    outmeta->global.proj_info.ul_corner[1] =
        inmeta->global.proj_info.ul_corner[1];
    outmeta->global.proj_info.lr_corner[0] =
        inmeta->global.proj_info.lr_corner[0];
    outmeta->global.proj_info.lr_corner[1] =
        inmeta->global.proj_info.lr_corner[1];

    count = snprintf (outmeta->global.proj_info.grid_origin,
        sizeof (outmeta->global.proj_info.grid_origin), "%s",
        inmeta->global.proj_info.grid_origin);
    if (count < 0 || count >= sizeof (outmeta->global.proj_info.grid_origin))
    {
        sprintf (errmsg, "Overflow of outmeta->global.proj_info.grid_origin");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.proj_info.utm_zone = inmeta->global.proj_info.utm_zone;
    outmeta->global.proj_info.longitude_pole =
        inmeta->global.proj_info.longitude_pole;
    outmeta->global.proj_info.latitude_true_scale =
        inmeta->global.proj_info.latitude_true_scale;
    outmeta->global.proj_info.false_easting =
        inmeta->global.proj_info.false_easting;
    outmeta->global.proj_info.false_northing =
        inmeta->global.proj_info.false_northing;
    outmeta->global.proj_info.standard_parallel1 =
        inmeta->global.proj_info.standard_parallel1;
    outmeta->global.proj_info.standard_parallel2 =
        inmeta->global.proj_info.standard_parallel2;
    outmeta->global.proj_info.central_meridian =
        inmeta->global.proj_info.central_meridian;
    outmeta->global.proj_info.origin_latitude =
        inmeta->global.proj_info.origin_latitude;
    outmeta->global.proj_info.sphere_radius =
        inmeta->global.proj_info.sphere_radius;
    outmeta->global.orientation_angle = inmeta->global.orientation_angle;

    /* Copy the bands metadata, for those bands specified to be subset into
       the new metadata structure */
    iband = 0;
    for (i = 0; i < inmeta->nbands; i++)
    {
        /* Is this band one of those specified for the product subset? */
        found = false;
        for (j = 0; j < nproducts; j++)
        {
            if (!strcmp (inmeta->band[i].product, products[j]))
            {
                found = true;
                break;
            }
        }
        if (!found)
            continue;

        /* Add this band to the new metadata structure */
        count = snprintf (outmeta->band[iband].product,
            sizeof (outmeta->band[iband].product), "%s",
            inmeta->band[i].product);
        if (count < 0 || count >= sizeof (outmeta->band[iband].product))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].product string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].source,
            sizeof (outmeta->band[iband].source), "%s", inmeta->band[i].source);
        if (count < 0 || count >= sizeof (outmeta->band[iband].source))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].source string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].name,
            sizeof (outmeta->band[iband].name), "%s", inmeta->band[i].name);
        if (count < 0 || count >= sizeof (outmeta->band[iband].name))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].name string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].category,
            sizeof (outmeta->band[iband].category), "%s",
            inmeta->band[i].category);
        if (count < 0 || count >= sizeof (outmeta->band[iband].category))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].category "
                "string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        outmeta->band[iband].data_type = inmeta->band[i].data_type;
        outmeta->band[iband].nlines = inmeta->band[i].nlines;
        outmeta->band[iband].nsamps = inmeta->band[i].nsamps;
        outmeta->band[iband].fill_value = inmeta->band[i].fill_value;
        outmeta->band[iband].saturate_value = inmeta->band[i].saturate_value;
        outmeta->band[iband].scale_factor = inmeta->band[i].scale_factor;
        outmeta->band[iband].add_offset = inmeta->band[i].add_offset;
        count = snprintf (outmeta->band[iband].short_name,
            sizeof (outmeta->band[iband].short_name), "%s",
            inmeta->band[i].short_name);
        if (count < 0 || count >= sizeof (outmeta->band[iband].short_name))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].short_name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].long_name,
            sizeof (outmeta->band[iband].long_name), "%s",
            inmeta->band[i].long_name);
        if (count < 0 || count >= sizeof (outmeta->band[iband].long_name))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].long_name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].file_name,
            sizeof (outmeta->band[iband].file_name), "%s",
            inmeta->band[i].file_name);
        if (count < 0 || count >= sizeof (outmeta->band[iband].file_name))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].file_name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        outmeta->band[iband].pixel_size[0] = inmeta->band[i].pixel_size[0];
        outmeta->band[iband].pixel_size[1] = inmeta->band[i].pixel_size[1];
        count = snprintf (outmeta->band[iband].pixel_units,
            sizeof (outmeta->band[iband].pixel_units), "%s",
            inmeta->band[i].pixel_units);
        if (count < 0 || count >= sizeof (outmeta->band[iband].pixel_units))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].pixel_units");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].data_units,
            sizeof (outmeta->band[iband].data_units), "%s",
            inmeta->band[i].data_units);
        if (count < 0 || count >= sizeof (outmeta->band[iband].data_units))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].data_units");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        outmeta->band[iband].valid_range[0] = inmeta->band[i].valid_range[0];
        outmeta->band[iband].valid_range[1] = inmeta->band[i].valid_range[1];
        outmeta->band[iband].valid_range[0] = inmeta->band[i].valid_range[0];
        outmeta->band[iband].valid_range[1] = inmeta->band[i].valid_range[1];
        outmeta->band[iband].toa_gain = inmeta->band[i].toa_gain;
        outmeta->band[iband].toa_bias = inmeta->band[i].toa_bias;
        outmeta->band[iband].toa_gain = inmeta->band[i].toa_gain;
        outmeta->band[iband].toa_bias = inmeta->band[i].toa_bias;

        /* If there is a bitmap description, then allocate memory and copy
           the information */
        outmeta->band[iband].nbits = inmeta->band[i].nbits;
        if (inmeta->band[i].nbits != 0)
        {
            if (allocate_bitmap_metadata (&outmeta->band[iband],
                outmeta->band[iband].nbits) != SUCCESS)
            {  /* Error messages already printed */
                return (ERROR);
            }

            for (k = 0; k < inmeta->band[i].nbits; k++)
            {
                count = snprintf (outmeta->band[iband].bitmap_description[k],
                    STR_SIZE, "%s", inmeta->band[i].bitmap_description[k]);
                if (count < 0 || count >= STR_SIZE)
                {
                    sprintf (errmsg, "Overflow of "
                        "outmeta->band[iband].bitmap_description[k] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
        }

        /* If there is are class descriptions, then allocate memory and copy
           the information */
        outmeta->band[iband].nclass = inmeta->band[i].nclass;
        if (inmeta->band[i].nclass != 0)
        {
            if (allocate_class_metadata (&outmeta->band[iband],
                outmeta->band[iband].nclass) != SUCCESS)
            {  /* Error messages already printed */
                return (ERROR);
            }

            for (k = 0; k < inmeta->band[i].nclass; k++)
            {
                 outmeta->band[iband].class_values[k].class =
                     inmeta->band[i].class_values[k].class;
                 count = snprintf (
                     outmeta->band[iband].class_values[k].description,
                     sizeof (outmeta->band[iband].class_values[k].description),
                     "%s", inmeta->band[i].class_values[k].description);
                 if (count < 0 || count >= sizeof
                      (outmeta->band[iband].class_values[k].description))
                 {
                     sprintf (errmsg, "Overflow of "
                         "outmeta->band[iband].class_values[k].description");
                     error_handler (true, FUNC_NAME, errmsg);
                     return (ERROR);
                 }

            }
        }

        count = snprintf (outmeta->band[iband].qa_desc,
            sizeof (outmeta->band[iband].qa_desc), "%s",
            inmeta->band[i].qa_desc);
        if (count < 0 || count >= sizeof (outmeta->band[iband].qa_desc))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].qa_desc");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        outmeta->band[iband].calibrated_nt = inmeta->band[i].calibrated_nt;
        count = snprintf (outmeta->band[iband].app_version,
            sizeof (outmeta->band[iband].app_version), "%s",
            inmeta->band[i].app_version);
        if (count < 0 || count >= sizeof (outmeta->band[iband].app_version))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].app_version");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].production_date,
            sizeof (outmeta->band[iband].production_date), "%s",
            inmeta->band[i].production_date);
        if (count < 0 || count >= sizeof (outmeta->band[iband].production_date))
        {
            sprintf(errmsg, "Overflow of outmeta->band[iband].production_date");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Increment the band count */
        iband++;
    }

    /* Subtract the number of skipped bands from the subset metadata band
       count */
    outmeta->nbands = iband;

    /* If no bands matched the product type, then print a warning */
    if (iband == 0)
    {
        sprintf (errmsg, "No bands in the XML file matched the product types.");
        error_handler (false, FUNC_NAME, errmsg);
    }

    /* Successful subset */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  subset_metadata_by_band

PURPOSE: Subset the current metadata structure to contain only the specified
bands.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error subsetting the metadata structure
SUCCESS         Successfully subset the metadata structure

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/10/2014    Gail Schmidt     Original development
4/22/2014    Gail Schmidt     Updated for additional projection parameters and
                              datums
5/7/2014     Gail Schmidt     Updated for modis tiles

NOTES:
  1. If nbands is 0, then the global and projection information will still
     be copied.
******************************************************************************/
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
)
{
    char FUNC_NAME[] = "subset_metadata_by_band";   /* function name */
    char errmsg[STR_SIZE];   /* error message */
    int i, j, k;             /* looping variables */
    int count;               /* number of chars copied in snprintf */
    int iband;               /* current output band */
    int nskip;               /* number of bands skipped as they weren't found
                                in the input metadata structure */
    bool found;              /* was the band found */

    /* Initialize the output metadata structure */
    init_metadata_struct (outmeta);

    /* Allocate output metadata for the bands to be included */
    if (allocate_band_metadata (outmeta, nbands) != SUCCESS)
    {  /* Error messages already printed */
        return (ERROR);
    }

    /* Copy the high level metadata structure attributes */
    count = snprintf (outmeta->meta_namespace, sizeof (outmeta->meta_namespace),
        "%s", inmeta->meta_namespace);
    if (count < 0 || count >= sizeof (outmeta->meta_namespace))
    {
        sprintf (errmsg, "Overflow of outmeta->meta_namespace string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }


    /* Copy the global metadata */
    count = snprintf (outmeta->global.data_provider,
        sizeof (outmeta->global.data_provider), "%s",
        inmeta->global.data_provider);
    if (count < 0 || count >= sizeof (outmeta->global.data_provider))
    {
        sprintf (errmsg, "Overflow of outmeta->global.data_provider string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.satellite,
        sizeof (outmeta->global.satellite), "%s", inmeta->global.satellite);
    if (count < 0 || count >= sizeof (outmeta->global.satellite))
    {
        sprintf (errmsg, "Overflow of outmeta->global.satellite string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.instrument,
        sizeof (outmeta->global.instrument), "%s", inmeta->global.instrument);
    if (count < 0 || count >= sizeof (outmeta->global.instrument))
    {
        sprintf (errmsg, "Overflow of outmeta->global.instrument string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.acquisition_date,
        sizeof (outmeta->global.acquisition_date), "%s",
        inmeta->global.acquisition_date);
    if (count < 0 || count >= sizeof (outmeta->global.acquisition_date))
    {
        sprintf (errmsg, "Overflow of outmeta->global.acquisition_date string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.scene_center_time,
        sizeof (outmeta->global.scene_center_time), "%s",
        inmeta->global.scene_center_time);
    if (count < 0 || count >= sizeof (outmeta->global.scene_center_time))
    {
        sprintf (errmsg, "Overflow of outmeta->global.scene_center_time");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (outmeta->global.level1_production_date,
        sizeof (outmeta->global.level1_production_date), "%s",
        inmeta->global.level1_production_date);
    if (count < 0 || count >= sizeof (outmeta->global.level1_production_date))
    {
        sprintf (errmsg, "Overflow of outmeta->global.level1_production_date");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.solar_zenith = inmeta->global.solar_zenith;
    outmeta->global.solar_azimuth = inmeta->global.solar_azimuth;
    count = snprintf (outmeta->global.solar_units,
        sizeof (outmeta->global.solar_units), "%s", inmeta->global.solar_units);
    if (count < 0 || count >= sizeof (outmeta->global.solar_units))
    {
        sprintf (errmsg, "Overflow of outmeta->global.solar_units string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.wrs_system = inmeta->global.wrs_system;
    outmeta->global.wrs_path = inmeta->global.wrs_path;
    outmeta->global.wrs_row = inmeta->global.wrs_row;

    outmeta->global.htile = inmeta->global.htile;
    outmeta->global.vtile = inmeta->global.vtile;

    count = snprintf (outmeta->global.lpgs_metadata_file,
        sizeof (outmeta->global.lpgs_metadata_file), "%s",
        inmeta->global.lpgs_metadata_file);
    if (count < 0 || count >= sizeof (outmeta->global.lpgs_metadata_file))
    {
        sprintf (errmsg, "Overflow of outmeta->global.lpgs_metadata_file");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.ul_corner[0] = inmeta->global.ul_corner[0];
    outmeta->global.ul_corner[1] = inmeta->global.ul_corner[1];
    outmeta->global.lr_corner[0] = inmeta->global.lr_corner[0];
    outmeta->global.lr_corner[1] = inmeta->global.lr_corner[1];
    outmeta->global.bounding_coords[ESPA_WEST] =
        inmeta->global.bounding_coords[ESPA_WEST];
    outmeta->global.bounding_coords[ESPA_EAST] =
        inmeta->global.bounding_coords[ESPA_EAST];
    outmeta->global.bounding_coords[ESPA_NORTH] =
        inmeta->global.bounding_coords[ESPA_NORTH];
    outmeta->global.bounding_coords[ESPA_SOUTH] =
        inmeta->global.bounding_coords[ESPA_SOUTH];

    /* Copy the projection information */
    outmeta->global.proj_info.proj_type = inmeta->global.proj_info.proj_type;
    outmeta->global.proj_info.datum_type = inmeta->global.proj_info.datum_type;
    count = snprintf (outmeta->global.proj_info.units,
        sizeof (outmeta->global.proj_info.units), "%s",
        inmeta->global.proj_info.units);
    if (count < 0 || count >= sizeof (outmeta->global.proj_info.units))
    {
        sprintf (errmsg, "Overflow of outmeta->global.proj_info.units string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.proj_info.ul_corner[0] =
        inmeta->global.proj_info.ul_corner[0];
    outmeta->global.proj_info.ul_corner[1] =
        inmeta->global.proj_info.ul_corner[1];
    outmeta->global.proj_info.lr_corner[0] =
        inmeta->global.proj_info.lr_corner[0];
    outmeta->global.proj_info.lr_corner[1] =
        inmeta->global.proj_info.lr_corner[1];
    count = snprintf (outmeta->global.proj_info.grid_origin,
        sizeof (outmeta->global.proj_info.grid_origin), "%s",
        inmeta->global.proj_info.grid_origin);
    if (count < 0 || count >= sizeof (outmeta->global.proj_info.grid_origin))
    {
        sprintf (errmsg, "Overflow of outmeta->global.proj_info.grid_origin");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    outmeta->global.proj_info.utm_zone = inmeta->global.proj_info.utm_zone;
    outmeta->global.proj_info.longitude_pole =
        inmeta->global.proj_info.longitude_pole;
    outmeta->global.proj_info.latitude_true_scale =
        inmeta->global.proj_info.latitude_true_scale;
    outmeta->global.proj_info.false_easting =
        inmeta->global.proj_info.false_easting;
    outmeta->global.proj_info.false_northing =
        inmeta->global.proj_info.false_northing;
    outmeta->global.proj_info.standard_parallel1 =
        inmeta->global.proj_info.standard_parallel1;
    outmeta->global.proj_info.standard_parallel2 =
        inmeta->global.proj_info.standard_parallel2;
    outmeta->global.proj_info.central_meridian =
        inmeta->global.proj_info.central_meridian;
    outmeta->global.proj_info.origin_latitude =
        inmeta->global.proj_info.origin_latitude;
    outmeta->global.proj_info.sphere_radius =
        inmeta->global.proj_info.sphere_radius;
    outmeta->global.orientation_angle = inmeta->global.orientation_angle;

    /* Copy the bands metadata, for those bands specified to be subset into
       the new metadata structure */
    nskip = 0;
    for (i = 0; i < nbands; i++)
    {
        /* Is this band one of those specified for the band subset? */
        found = false;
        for (j = 0; j < inmeta->nbands; j++)
        {
            if (!strcmp (inmeta->band[j].name, bands[i]))
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            sprintf (errmsg, "Band '%s' not found in the XML structure. "
                "Skipping.", bands[i]);
            error_handler (false, FUNC_NAME, errmsg);
            nskip++;
            continue;
        }

        /* Adjust i by number of skipped bands for the correct location into
           the output band data */
        iband = i - nskip;

        /* Add this band to the new metadata structure */
        count = snprintf (outmeta->band[iband].product,
            sizeof (outmeta->band[iband].product), "%s",
            inmeta->band[j].product);
        if (count < 0 || count >= sizeof (outmeta->band[iband].product))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].product string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].source,
            sizeof (outmeta->band[iband].source), "%s", inmeta->band[i].source);
        if (count < 0 || count >= sizeof (outmeta->band[iband].source))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].source string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].name,
            sizeof (outmeta->band[iband].name), "%s",
            inmeta->band[j].name);
        if (count < 0 || count >= sizeof (outmeta->band[iband].name))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].name string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].category,
            sizeof (outmeta->band[iband].category), "%s",
            inmeta->band[i].category);
        if (count < 0 || count >= sizeof (outmeta->band[iband].category))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].category "
                "string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        outmeta->band[iband].data_type = inmeta->band[j].data_type;
        outmeta->band[iband].nlines = inmeta->band[j].nlines;
        outmeta->band[iband].nsamps = inmeta->band[j].nsamps;
        outmeta->band[iband].fill_value = inmeta->band[j].fill_value;
        outmeta->band[iband].saturate_value = inmeta->band[j].saturate_value;
        outmeta->band[iband].scale_factor = inmeta->band[j].scale_factor;
        outmeta->band[iband].add_offset = inmeta->band[j].add_offset;
        count = snprintf (outmeta->band[iband].short_name,
            sizeof (outmeta->band[iband].short_name), "%s",
            inmeta->band[j].short_name);
        if (count < 0 || count >= sizeof (outmeta->band[iband].short_name))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].short_name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].long_name,
            sizeof (outmeta->band[iband].long_name), "%s",
            inmeta->band[j].long_name);
        if (count < 0 || count >= sizeof (outmeta->band[iband].long_name))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].long_name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].file_name,
            sizeof (outmeta->band[iband].file_name), "%s",
            inmeta->band[j].file_name);
        if (count < 0 || count >= sizeof (outmeta->band[iband].file_name))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].file_name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        outmeta->band[iband].pixel_size[0] = inmeta->band[j].pixel_size[0];
        outmeta->band[iband].pixel_size[1] = inmeta->band[j].pixel_size[1];
        count = snprintf (outmeta->band[iband].pixel_units,
            sizeof (outmeta->band[iband].pixel_units), "%s",
            inmeta->band[j].pixel_units);
        if (count < 0 || count >= sizeof (outmeta->band[iband].pixel_units))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].pixel_units");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].data_units,
            sizeof (outmeta->band[iband].data_units), "%s",
            inmeta->band[j].data_units);
        if (count < 0 || count >= sizeof (outmeta->band[iband].data_units))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].data_units");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        outmeta->band[iband].valid_range[0] = inmeta->band[j].valid_range[0];
        outmeta->band[iband].valid_range[1] = inmeta->band[j].valid_range[1];
        outmeta->band[iband].valid_range[0] = inmeta->band[j].valid_range[0];
        outmeta->band[iband].valid_range[1] = inmeta->band[j].valid_range[1];
        outmeta->band[iband].toa_gain = inmeta->band[j].toa_gain;
        outmeta->band[iband].toa_bias = inmeta->band[j].toa_bias;
        outmeta->band[iband].toa_gain = inmeta->band[j].toa_gain;
        outmeta->band[iband].toa_bias = inmeta->band[j].toa_bias;

        /* If there is a bitmap description, then allocate memory and copy
           the information */
        outmeta->band[iband].nbits = inmeta->band[j].nbits;
        if (inmeta->band[j].nbits != 0)
        {
            if (allocate_bitmap_metadata (&outmeta->band[iband],
                outmeta->band[iband].nbits) != SUCCESS)
            {  /* Error messages already printed */
                return (ERROR);
            }

            for (k = 0; k < inmeta->band[j].nbits; k++)
            {
                count = snprintf (outmeta->band[iband].bitmap_description[k],
                    STR_SIZE, "%s", inmeta->band[j].bitmap_description[k]);
                if (count < 0 || count >= STR_SIZE)
                {
                    sprintf (errmsg, "Overflow of "
                        "outmeta->band[iband].bitmap_description[k] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
        }

        /* If there is are class descriptions, then allocate memory and copy
           the information */
        outmeta->band[iband].nclass = inmeta->band[j].nclass;
        if (inmeta->band[j].nclass != 0)
        {
            if (allocate_class_metadata (&outmeta->band[iband],
                outmeta->band[iband].nclass) != SUCCESS)
            {  /* Error messages already printed */
                return (ERROR);
            }

            for (k = 0; k < inmeta->band[j].nclass; k++)
            {
                outmeta->band[iband].class_values[k].class =
                    inmeta->band[j].class_values[k].class;
                count = snprintf (
                    outmeta->band[iband].class_values[k].description,
                    sizeof (outmeta->band[iband].class_values[k].description),
                    "%s", inmeta->band[j].class_values[k].description);
                if (count < 0 || count >=
                    sizeof (outmeta->band[iband].class_values[k].description))
                {
                    sprintf (errmsg, "Overflow of "
                        "outmeta->band[iband].class_values[k].description");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
        }

        count = snprintf (outmeta->band[iband].qa_desc,
            sizeof (outmeta->band[iband].qa_desc), "%s",
            inmeta->band[i].qa_desc);
        if (count < 0 || count >= sizeof (outmeta->band[iband].qa_desc))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].qa_desc");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        outmeta->band[iband].calibrated_nt = inmeta->band[j].calibrated_nt;
        count = snprintf (outmeta->band[iband].app_version,
            sizeof (outmeta->band[iband].app_version), "%s",
            inmeta->band[j].app_version);
        if (count < 0 || count >= sizeof (outmeta->band[iband].app_version))
        {
            sprintf (errmsg, "Overflow of outmeta->band[iband].app_version");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (outmeta->band[iband].production_date,
            sizeof (outmeta->band[iband].production_date), "%s",
            inmeta->band[j].production_date);
        if (count < 0 || count >= sizeof (outmeta->band[iband].production_date))
        {
            sprintf (errmsg, "Overflow of "
                "outmeta->band[iband].production_date string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Subtract the number of skipped bands from the subset metadata band
       count */
    outmeta->nbands -= nskip;

    /* Successful subset */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  subset_xml_by_product

PURPOSE: Subset an XML file to contain only the specified product types.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error subsetting the XML file
SUCCESS         Successfully subset the XML file

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/10/2014    Gail Schmidt     Original development

NOTES:
  1. If no bands match the product type, then the global and projection
     information will still be copied.
******************************************************************************/
int subset_xml_by_product
(
    char *in_xml_file,   /* I: input XML file to be subset */
    char *out_xml_file,  /* I: output XML file to be subset */
    int nproducts,       /* I: number of product types to be included in the
                               subset product */
    char products[][STR_SIZE]  /* I: array of nproducts product types to be used
                               for subsetting */
)
{
    char FUNC_NAME[] = "subset_xml_by_product";   /* function name */
    char errmsg[STR_SIZE];   /* error message */
    Espa_internal_meta_t in_xml_metadata;  /* XML metadata structure to be
                                populated by reading the input XML file */
    Espa_internal_meta_t out_xml_metadata; /* XML metadata structure to be
                                populated by subsetting the input XML */

    /* Validate the input metadata file */
    if (validate_xml_file (in_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Initialize the input metadata structure */
    init_metadata_struct (&in_xml_metadata);

    /* Parse the metadata file into our internal metadata structure; also
       allocates space as needed for various pointers in the global and band
       metadata */
    if (parse_metadata (in_xml_file, &in_xml_metadata) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Subset the input XML file using the specified bands */
    if (subset_metadata_by_product (&in_xml_metadata, &out_xml_metadata,
        nproducts, products) != SUCCESS)
    {
        sprintf (errmsg, "Subsetting the XML file for the specified products.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write the subset metadata structure to the output XML filename */
    if (write_metadata (&out_xml_metadata, out_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Validate the output metadata file */
    if (validate_xml_file (out_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Free the metadata structures */
    free_metadata (&in_xml_metadata);
    free_metadata (&out_xml_metadata);

    /* Successful subset */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  subset_xml_by_band

PURPOSE: Subset an XML file to contain only the specified bands.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error subsetting the XML file
SUCCESS         Successfully subset the XML file

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/10/2014    Gail Schmidt     Original development

NOTES:
  1. If nbands is 0, then the global and projection information will still
     be copied.
******************************************************************************/
int subset_xml_by_band
(
    char *in_xml_file,   /* I: input XML file to be subset */
    char *out_xml_file,  /* I: output XML file to be subset */
    int nbands,          /* I: number of bands to be included in the subset
                               XML file */
    char bands[][STR_SIZE] /* I: array of nbands band names to be appear in
                               the subset XML file */
)
{
    char FUNC_NAME[] = "subset_xml_by_band";   /* function name */
    char errmsg[STR_SIZE];   /* error message */
    Espa_internal_meta_t in_xml_metadata;  /* XML metadata structure to be
                                populated by reading the input XML file */
    Espa_internal_meta_t out_xml_metadata; /* XML metadata structure to be
                                populated by subsetting the input XML */

    /* Validate the input metadata file */
    if (validate_xml_file (in_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Initialize the input metadata structure */
    init_metadata_struct (&in_xml_metadata);

    /* Parse the metadata file into our internal metadata structure; also
       allocates space as needed for various pointers in the global and band
       metadata */
    if (parse_metadata (in_xml_file, &in_xml_metadata) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Subset the input XML file using the specified bands */
    if (subset_metadata_by_band (&in_xml_metadata, &out_xml_metadata,
        nbands, bands) != SUCCESS)
    {
        sprintf (errmsg, "Subsetting the XML file for the specified bands.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write the subset metadata structure to the output XML filename */
    if (write_metadata (&out_xml_metadata, out_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Validate the output metadata file */
    if (validate_xml_file (out_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Free the metadata structures */
    free_metadata (&in_xml_metadata);
    free_metadata (&out_xml_metadata);

    /* Successful subset */
    return (SUCCESS);
}

