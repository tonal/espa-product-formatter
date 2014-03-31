/*****************************************************************************
FILE: envi_header.c
  
PURPOSE: Contains functions for reading/writing ENVI header files.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development
3/31/2014    Ron Dilley       Modified to support a data fill value in the
                              ENVI header

NOTES:
*****************************************************************************/

#include "envi_header.h"

/******************************************************************************
MODULE:  write_envi_hdr

PURPOSE:  Writes the ENVI header to the specified file using the input info
provided.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           An error occurred generating the header file
SUCCESS         Header file was successful

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development

NOTES:
  1. Only supports UTM, ALBERS, and PS projections.
  2. Only supports WGS84 datum.
******************************************************************************/
int write_envi_hdr
(
    char *hdr_file,     /* I: name of ENVI header file to be generated */
    Envi_header_t *hdr  /* I: input ENVI header information */
)
{
    char FUNC_NAME[] = "write_envi_hdr";   /* function name */
    char errmsg[STR_SIZE];   /* error message */
    int i;                   /* looping variable */
    FILE *hdr_fptr = NULL;   /* file pointer to the ENVI header file */

    /* Open the header file */
    hdr_fptr = fopen (hdr_file, "w");
    if (hdr_fptr == NULL)
    {
        sprintf (errmsg, "Opening %s for write access.", hdr_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Verify the projection is UTM, ALBERS, or PS and datum is WGS-84 */
    if (hdr->proj_type != GCTP_UTM_PROJ &&
        hdr->proj_type != GCTP_ALBERS_PROJ &&
        hdr->proj_type != GCTP_PS_PROJ)
    {
        sprintf (errmsg, "UTM projection code (%d) or ALBERS projection "
            "code (%d) or PS projection code (%d) expected.", GCTP_UTM_PROJ,
            GCTP_ALBERS_PROJ, GCTP_PS_PROJ);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (hdr->datum_type != ENVI_WGS84)
    {
        sprintf (errmsg, "Error WGS-84 datum code (%d) expected.", ENVI_WGS84);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write the header to the file */
    fprintf (hdr_fptr,
        "ENVI\n"
        "description = {%s}\n"
        "samples = %d\n"
        "lines   = %d\n"
        "bands   = %d\n"
        "header offset = %d\n"
        "byte order = %d\n"
        "file type = %s\n"
        "data type = %d\n"
        "data ignore value = %ld\n"
        "interleave = %s\n"
        "sensor_type = %s\n",
        hdr->description, hdr->nsamps, hdr->nlines, hdr->nbands,
        hdr->header_offset, hdr->byte_order, hdr->file_type, hdr->data_type,
        hdr->data_ignore_value, hdr->interleave, hdr->sensor_type);
   
    if (hdr->proj_type == GCTP_UTM_PROJ)
    {
        if (hdr->utm_zone > 0)
            fprintf (hdr_fptr,
                "map info = {UTM, %d, %d, %f, %f, %f, %f, %d, North, "
                "WGS-84, units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
                hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
                hdr->pixel_size[1], hdr->utm_zone);
        else
            fprintf (hdr_fptr,
                "map info = {UTM, %d, %d, %f, %f, %f, %f, %d, North, "
                "WGS-84, units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
                hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
                hdr->pixel_size[1], -(hdr->utm_zone));
    }
    else if (hdr->proj_type == GCTP_ALBERS_PROJ)
    {
        fprintf (hdr_fptr,
            "map info = {Albers Conical Equal Area, %d, %d, %f, %f, %f, %f, "
            "WGS-84, units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
            hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
            hdr->pixel_size[1]);
        fprintf (hdr_fptr,
            "projection info = {%d, 6378137.0, 6356752.314245179, %lf, %lf, "
            "%lf, %lf, %lf, %lf, WGS-84, Albers Conical Equal Area, "
            "units=Meters}\n", ENVI_ALBERS_PROJ, hdr->proj_parms[5],
            hdr->proj_parms[4], hdr->proj_parms[6], hdr->proj_parms[7],
            hdr->proj_parms[2], hdr->proj_parms[3]);
    }
    else if (hdr->proj_type == GCTP_PS_PROJ)
    {
        fprintf (hdr_fptr,
            "map info = {Polar Stereographic, %d, %d, %f, %f, %f, %f, "
            "WGS-84, units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
            hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
            hdr->pixel_size[1]);
        fprintf (hdr_fptr,
            "projection info = {%d, 6378137.0, 6356752.314245179, %lf, "
            "%lf, %lf, %lf, WGS-84, Polar Stereographic, units=Meters}\n",
            ENVI_PS_PROJ, hdr->proj_parms[5], hdr->proj_parms[4],
            hdr->proj_parms[6], hdr->proj_parms[7]);
    }

    /* Write the array of band names */
    fprintf (hdr_fptr, "band names = {%s", hdr->band_names[0]);
    for (i = 1; i < hdr->nbands; i++)
        fprintf (hdr_fptr, ", %s", hdr->band_names[i]);
    fprintf (hdr_fptr, "}\n");

    /* Close the header file */
    fclose (hdr_fptr);

    /* Successful completion */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  create_envi_struct

PURPOSE:  Creates the ENVI header structure from the ESPA global and band
metadata.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           An error occurred generating the ENVI header structure
SUCCESS         Header structure creation was successful

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/3/2014     Gail Schmidt     Original development

NOTES:
  1. Only supports UTM, ALBERS, and PS projections.
  2. Only supports WGS84 datum.
  3. Refer to ENVI Header Format in
     http://www.exelisvis.com/portals/0/pdfs/envi/Getting_Started_with_ENVI.pdf
******************************************************************************/
int create_envi_struct
(
    Espa_band_meta_t *bmeta,   /* I: pointer to band metadata for this band */
    Espa_global_meta_t *gmeta, /* I: pointer to global metadata */
    Envi_header_t *hdr         /* I/O: output ENVI header information */
)
{
    char FUNC_NAME[] = "create_envi_struct";   /* function name */
    char errmsg[STR_SIZE];   /* error message */
    int i;                   /* looping variable */
    int count;               /* number of chars copied in snprintf */

    strcpy (hdr->description, "ESPA-generated file");
    hdr->nlines = bmeta->nlines;
    hdr->nsamps = bmeta->nsamps;
    hdr->nbands = 1;
    hdr->header_offset = 0;
    hdr->byte_order = 0;  /* assume Linux systems -- if Windows switch to 1 */
    strcpy (hdr->file_type, "ENVI Standard");
    strcpy (hdr->interleave, "BSQ");
    count = snprintf (hdr->sensor_type, sizeof (hdr->sensor_type), "%s %s",
        gmeta->satellite, gmeta->instrument);
    if (count < 0 || count >= sizeof (hdr->sensor_type))
    {
        sprintf (errmsg, "Overflow of hdr->sensor_type string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Data type */
    switch (bmeta->data_type)
    {
        case ESPA_INT8: hdr->data_type=1; break;
        case ESPA_UINT8: hdr->data_type=1; break;
        case ESPA_INT16: hdr->data_type=2; break;
        case ESPA_UINT16: hdr->data_type=12; break;
        case ESPA_INT32: hdr->data_type=3; break;
        case ESPA_UINT32: hdr->data_type=13; break;
        case ESPA_FLOAT32: hdr->data_type=4; break;
        case ESPA_FLOAT64: hdr->data_type=5; break;
    }

    /* Data ignore value */
    hdr->data_ignore_value = bmeta->fill_value;

    /* Projection information */
    for (i = 0; i < 15; i++)
        hdr->proj_parms[i] = 0.0;
    hdr->proj_type = gmeta->proj_info.proj_type;
    switch (gmeta->proj_info.proj_type)
    {
        case GCTP_UTM_PROJ:
            hdr->utm_zone = gmeta->proj_info.utm_zone;
            break;

        case GCTP_ALBERS_PROJ:
            hdr->proj_parms[2] = gmeta->proj_info.standard_parallel1;
            hdr->proj_parms[3] = gmeta->proj_info.standard_parallel2;
            hdr->proj_parms[4] = gmeta->proj_info.central_meridian;
            hdr->proj_parms[5] = gmeta->proj_info.origin_latitude;
            hdr->proj_parms[6] = gmeta->proj_info.false_easting;
            hdr->proj_parms[7] = gmeta->proj_info.false_northing;
            break;

        case GCTP_PS_PROJ:
            hdr->proj_parms[4] = gmeta->proj_info.longitude_pole;
            hdr->proj_parms[5] = gmeta->proj_info.latitude_true_scale;
            hdr->proj_parms[6] = gmeta->proj_info.false_easting;
            hdr->proj_parms[7] = gmeta->proj_info.false_northing;
            break;

        default:
            sprintf (errmsg, "Unsupported projection type (%d).  Currently "
                "only support UTM, ALBERS, or PS.", gmeta->proj_info.proj_type);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
    }

    if (gmeta->proj_info.sphere_code != GCTP_WGS84)
    {
        sprintf (errmsg, "Unsupported datum type.  Currently only support "
            "WGS84.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    hdr->datum_type = ENVI_WGS84;
    hdr->pixel_size[0] = bmeta->pixel_size[0];
    hdr->pixel_size[1] = bmeta->pixel_size[1];
    count = snprintf (hdr->band_names[0], sizeof (hdr->band_names[0]), "%s",
        bmeta->long_name);
    if (count < 0 || count >= sizeof (hdr->band_names[0]))
    {
        sprintf (errmsg, "Overflow of hdr->band_names[0] string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* The UL corner coordinate refer to 1,1 in the line, sample grid.  If the
       grid origin is center, then adjust for the resolution. */
    hdr->xy_start[0] = hdr->xy_start[1] = 1;
    if (!strcmp (gmeta->proj_info.grid_origin, "CENTER"))
    {
        hdr->ul_corner[0] = gmeta->proj_info.ul_corner[0] -
            0.5 * bmeta->pixel_size[0];
        hdr->ul_corner[1] = gmeta->proj_info.ul_corner[1] +
            0.5 * bmeta->pixel_size[1];
    }
    else
    {
        hdr->ul_corner[0] = gmeta->proj_info.ul_corner[0];
        hdr->ul_corner[1] = gmeta->proj_info.ul_corner[1];
    }

    /* Successful completion */
    return (SUCCESS);
}

