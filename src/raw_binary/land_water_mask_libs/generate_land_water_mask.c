/*****************************************************************************
FILE:  generate_land_water_mask
  
PURPOSE: This application generates a land/water mask for L1G and L1T products,
based on an input land-mask polygon.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/17/2015    Gail Schmidt     Original development, based on code provided
                              by the Landsat 8 Development team

NOTES:
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include "generate_land_water_mask.h"

/******************************************************************************
MODULE:  generate_land_water_mask

PURPOSE:  This function creates the land/water mask (land = 1) for the area
covered by the scene.  If the output mask image name is provided, the mask
image is written to a file with that name.

RETURN VALUE:
Type = int
Value        Description
-------      -----------
ERROR        Error occurred opening or reading the file
SUCCESS      Successful completion

HISTORY:
Date         Programmer       Reason
----------   ---------------  -------------------------------------
4/10/2015    Landsat 8 Team   Original code received from the Landsat 8 IAS
4/14/2015    Gail Schmidt     Modified for use in ESPA   

NOTES:
1. Memory for the land water mask will be allocated for the entire image
   (nlines x nsamps x sizeof (unsigned char)).  It is up to the calling routine
   to free this memory.
******************************************************************************/
int generate_land_water_mask
(
    Espa_internal_meta_t *xml_meta,   /* I: input XML metadata */
    const char land_mass_polygon[],   /* I: name of land mass polygon file */
    unsigned char **land_water_mask,  /* O: pointer to land water mask buffer,
                                            memory is allocated and the
                                            mask is populated */
    int *nlines,                      /* O: number of lines in the mask */
    int *nsamps                       /* O: number of samples in the mask */
)
{
    char FUNC_NAME[] = "generate_land_water_mask";   /* function name */
    char errmsg[STR_SIZE];            /* error message */
    int i;                            /* looping variable */
    int refl_indx = -9;               /* band index in XML file for the
                                         representative reflectance band */
    double upper_left_x;              /* upper left X coordinate */
    double upper_left_y;              /* upper left Y coordinate */
    double lower_right_x;             /* lower right X coordinate */
    double lower_right_y;             /* lower right Y coordinate */
    Espa_global_meta_t *gmeta = &xml_meta->global;
                                      /* pointer to global metadata structure */
    IAS_IMAGE mask_image;             /* image data used to build mask */
    IAS_PROJECTION mask_projection;   /* projection data */

    /* Use band 1 as the representative band in the XML */
    for (i = 0; i < xml_meta->nbands; i++)
    {
        if (!strcmp (xml_meta->band[i].name, "band1"))
        {
            /* this is the index we'll use for reflectance band info */
            refl_indx = i;
            break;
        }
    }

    /* Make sure the representative band was found in the XML file */
    if (refl_indx == -9)
    {
        sprintf (errmsg, "Band 1 (band1) was not found in the XML file");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* If the grid origin is center, then adjust for the resolution.  The
       corners will be written for the outer extents of the corner. */
    if (!strcmp (gmeta->proj_info.grid_origin, "CENTER"))
    {
        /* UL corner - go from center to UL of UL */
        upper_left_x = gmeta->proj_info.ul_corner[0] -
            0.5 * xml_meta->band[refl_indx].pixel_size[0];
        upper_left_y = gmeta->proj_info.ul_corner[1] +
            0.5 * xml_meta->band[refl_indx].pixel_size[1];

        /* LR corner - go from center to LR of LR */
        lower_right_x = gmeta->proj_info.lr_corner[0] +
            0.5 * xml_meta->band[refl_indx].pixel_size[0];
        lower_right_y = gmeta->proj_info.lr_corner[1] -
            0.5 * xml_meta->band[refl_indx].pixel_size[1];
    }
    else
    {
        /* Metadata corners are UL of the pixel, thus make the LR corner the
           outer extent of the corner */
        upper_left_x = gmeta->proj_info.ul_corner[0];
        upper_left_y = gmeta->proj_info.ul_corner[1];

        /* LR corner - go from UL of LR to LR of LR */
        lower_right_x = gmeta->proj_info.lr_corner[0] +
            xml_meta->band[refl_indx].pixel_size[0];
        lower_right_y = gmeta->proj_info.lr_corner[1] -
            xml_meta->band[refl_indx].pixel_size[1];
    }

    /* Set the l1g image band metadata to image structure */
    mask_image.corners = (struct IAS_CORNERS)
        {
            {upper_left_x, upper_left_y},
            {lower_right_x, upper_left_y},
            {upper_left_x, lower_right_y},
            {lower_right_x, lower_right_y}
        };
    mask_image.pixel_size_x = xml_meta->band[refl_indx].pixel_size[0];
    mask_image.pixel_size_y = xml_meta->band[refl_indx].pixel_size[1];
    mask_image.nl = xml_meta->band[refl_indx].nlines;
    mask_image.ns = xml_meta->band[refl_indx].nsamps;
    mask_image.band_number = xml_meta->nbands+1;  /* not used */
    *nlines = xml_meta->band[refl_indx].nlines;
    *nsamps = xml_meta->band[refl_indx].nsamps;

    /* Set the projection contents based on info from the XML */
    for (i = 0; i < NPROJ_PARAM; i++)
        mask_projection.parameters[i] = 0.0;

    if (gmeta->proj_info.proj_type == GCTP_UTM_PROJ)
    {
        mask_projection.proj_code = GCTP_UTM_PROJ;
        mask_projection.units = METER;
        mask_projection.zone = gmeta->proj_info.utm_zone;
    }
    else if (gmeta->proj_info.proj_type == GCTP_PS_PROJ)
    {
        mask_projection.proj_code = GCTP_PS_PROJ;
        mask_projection.units = METER;
        mask_projection.parameters[4] =
            deg_to_dms (gmeta->proj_info.longitude_pole);
        mask_projection.parameters[5] =
            deg_to_dms (gmeta->proj_info.latitude_true_scale);
        mask_projection.parameters[6] = gmeta->proj_info.false_easting;
        mask_projection.parameters[7] = gmeta->proj_info.false_northing;
    }

    switch (gmeta->proj_info.datum_type)
    {
        case (ESPA_WGS84):
            mask_projection.spheroid = SPHERE_WGS84;
            break;
        case (ESPA_NAD83):
            mask_projection.spheroid = SPHERE_GRS80;
            break;
        case (ESPA_NAD27):
            mask_projection.spheroid = SPHERE_CLARKE_1866;
            break;
        case (ESPA_NODATUM):
            sprintf (errmsg, "ESPA_NODATUM is not supported for converting "
                "the land/water polygon to the frame of the current scene.");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
    }

    /* Write some summary information */
    printf("==================================================\n");
    printf("= Summary of land/water image and projection     =\n");
    printf("==================================================\n");
    printf("   upper_left_x = %lf\n", mask_image.corners.upleft.x);
    printf("   upper_left_y = %lf\n", mask_image.corners.upleft.y);
    printf("  upper_right_x = %lf\n", mask_image.corners.upright.x);
    printf("  upper_right_y = %lf\n", mask_image.corners.upright.y);
    printf("   lower_left_x = %lf\n", mask_image.corners.loleft.x);
    printf("   lower_left_y = %lf\n", mask_image.corners.loleft.y);
    printf("  lower_right_x = %lf\n", mask_image.corners.loright.x);
    printf("  lower_right_y = %lf\n", mask_image.corners.loright.y);
    printf("   pixel_size_y = %lf\n", mask_image.pixel_size_y);
    printf("   pixel_size_x = %lf\n", mask_image.pixel_size_x);
    printf("  nLinesInImage = %d\n", mask_image.nl);
    printf("       nSamples = %d\n", mask_image.ns);
    printf("    band_number = %d\n", mask_image.band_number);
    printf("projection code = %d\n", mask_projection.proj_code);
    printf("      zone code = %d\n", mask_projection.zone);
    printf("  spheroid code = %d\n", mask_projection.spheroid);
    printf("          units = %d\n", mask_projection.units);

    /* Allocate memory for the land/water mask and initialize to all zeros */
    *land_water_mask = calloc (mask_image.nl * mask_image.ns,
        sizeof (unsigned char));
    if (*land_water_mask == NULL)
    {
        sprintf (errmsg, "Error allocating memory for the land/water mask.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Use the land-mass polygon to generate a land/water mask for this
       scene */
    if (ias_geo_shape_mask_projection(land_mass_polygon, &mask_image,
        &mask_projection, *land_water_mask) != SUCCESS)
    {
        sprintf (errmsg, "Creating land and water mask");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    return (SUCCESS);
}
