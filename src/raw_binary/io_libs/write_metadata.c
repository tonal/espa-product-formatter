/*****************************************************************************
FILE: write_metadata.c
  
PURPOSE: Contains functions for writing/appending the ESPA internal metadata
files along with printing to stdout.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development
2/25/2014    Gail Schmidt     Added support for source and category attributes
                              for the band metadata

NOTES:
  1. The XML metadata format written via this library follows the ESPA internal
     metadata format found in ESPA Raw Binary Format v1.0.doc.  The schema for
     the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/static/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/

#include <math.h>
#include "write_metadata.h"

/******************************************************************************
MODULE:  write_metadata

PURPOSE: Write the metadata structure to the specified XML metadata file

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error writing the metadata file
SUCCESS         Successfully wrote the metadata file

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development
4/17/2014    Gail Schmidt     Modified to support additional projections
4/22/2014    Gail Schmidt     Modified to support additional datums

NOTES:
  1. If the XML file specified already exists, it will be overwritten.
  2. Use this routine to create a new metadata file.  To append bands to an
     existing metadata file, use append_metadata.
  3. It is recommended that validate_meta be used after writing the XML file
     to make sure the new file is valid against the ESPA schema.
******************************************************************************/
int write_metadata
(
    Espa_internal_meta_t *metadata,  /* I: input metadata structure to be
                                           written to XML */
    char *xml_file                   /* I: name of the XML metadata file to
                                           be written to or overwritten */
)
{
    char FUNC_NAME[] = "write_metadata";       /* function name */
    char errmsg[STR_SIZE];   /* error message */
    char myproj[STR_SIZE];   /* projection type string */
    char mydatum[STR_SIZE];  /* datum string */
    char my_dtype[STR_SIZE]; /* data type string */
    int i, j;                /* looping variables */
    FILE *fptr = NULL;       /* file pointer to the XML metadata file */
    Espa_global_meta_t *gmeta = &metadata->global;  /* pointer to the global
                                                       metadata structure */
    Espa_band_meta_t *bmeta = metadata->band;  /* pointer to the array of
                                                  bands metadata */

    /* Open the metadata XML file for write or rewrite privelages */
    fptr = fopen (xml_file, "w");
    if (fptr == NULL)
    {
        sprintf (errmsg, "Opening %s for write access.", xml_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write the overall header */
    fprintf (fptr,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
        "<espa_metadata version=\"%s\"\n"
        "xmlns=\"%s\"\n"
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
        "xsi:schemaLocation=\"%s %s\">\n\n", ESPA_SCHEMA_VERSION, ESPA_NS,
        ESPA_SCHEMA_LOCATION, ESPA_SCHEMA);

    /* Write the global metadata */
    fprintf (fptr,
        "    <global_metadata>\n"
        "        <data_provider>%s</data_provider>\n"
        "        <satellite>%s</satellite>\n"
        "        <instrument>%s</instrument>\n"
        "        <acquisition_date>%s</acquisition_date>\n"
        "        <scene_center_time>%s</scene_center_time>\n"
        "        <level1_production_date>%s</level1_production_date>\n"
        "        <solar_angles zenith=\"%f\" azimuth=\"%f\" units=\"%s\"/>\n"
        "        <wrs system=\"%d\" path=\"%d\" row=\"%d\"/>\n"
        "        <lpgs_metadata_file>%s</lpgs_metadata_file>\n"
        "        <corner location=\"UL\" latitude=\"%lf\" longitude=\"%lf\"/>\n"
        "        <corner location=\"LR\" latitude=\"%lf\" longitude=\"%lf\"/>\n"
        "        <bounding_coordinates>\n"
        "            <west>%lf</west>\n"
        "            <east>%lf</east>\n"
        "            <north>%lf</north>\n"
        "            <south>%lf</south>\n"
        "        </bounding_coordinates>\n",
        gmeta->data_provider, gmeta->satellite, gmeta->instrument,
        gmeta->acquisition_date, gmeta->scene_center_time,
        gmeta->level1_production_date, gmeta->solar_zenith,
        gmeta->solar_azimuth, gmeta->solar_units, gmeta->wrs_system,
        gmeta->wrs_path, gmeta->wrs_row, gmeta->lpgs_metadata_file,
        gmeta->ul_corner[0], gmeta->ul_corner[1],
        gmeta->lr_corner[0], gmeta->lr_corner[1],
        gmeta->bounding_coords[ESPA_WEST], gmeta->bounding_coords[ESPA_EAST],
        gmeta->bounding_coords[ESPA_NORTH], gmeta->bounding_coords[ESPA_SOUTH]);

    /* Write the global metadata - projection information */
    switch (gmeta->proj_info.proj_type)
    {
        case GCTP_GEO_PROJ: strcpy (myproj, "GEO"); break;
        case GCTP_UTM_PROJ: strcpy (myproj, "UTM"); break;
        case GCTP_ALBERS_PROJ: strcpy (myproj, "ALBERS"); break;
        case GCTP_PS_PROJ: strcpy (myproj, "PS"); break;
        case GCTP_SIN_PROJ: strcpy (myproj, "SIN"); break;
        default: strcpy (myproj, "undefined"); break;
    }
    if (gmeta->proj_info.datum_type != ESPA_NODATUM)
    {
        switch (gmeta->proj_info.datum_type)
        {
            case ESPA_WGS84: strcpy (mydatum, "WGS84"); break;
            case ESPA_NAD27: strcpy (mydatum, "NAD27"); break;
            case ESPA_NAD83: strcpy (mydatum, "NAD83"); break;
        }
        fprintf (fptr,
            "        <projection_information projection=\"%s\" datum=\"%s\" "
            "units=\"%s\">\n", myproj, mydatum,
            gmeta->proj_info.units);
    }
    else
    {
        fprintf (fptr,
            "        <projection_information projection=\"%s\" units=\"%s\">\n",
            myproj, gmeta->proj_info.units);
    }
    fprintf (fptr,
        "            <corner_point location=\"UL\" x=\"%lf\" y=\"%lf\"/>\n"
        "            <corner_point location=\"LR\" x=\"%lf\" y=\"%lf\"/>\n"
        "            <grid_origin>%s</grid_origin>\n",
        gmeta->proj_info.ul_corner[0], gmeta->proj_info.ul_corner[1],
        gmeta->proj_info.lr_corner[0], gmeta->proj_info.lr_corner[1],
        gmeta->proj_info.grid_origin);

    /* UTM-specific parameters */
    if (gmeta->proj_info.proj_type == GCTP_UTM_PROJ)
    {
        fprintf (fptr,
            "            <utm_proj_params>\n"
            "                <zone_code>%d</zone_code>\n"
            "            </utm_proj_params>\n",
            gmeta->proj_info.utm_zone);
    }

    /* ALBERS-specific parameters */
    if (gmeta->proj_info.proj_type == GCTP_ALBERS_PROJ)
    {
        fprintf (fptr,
            "            <albers_proj_params>\n"
            "                <standard_parallel1>%lf</standard_parallel1>\n"
            "                <standard_parallel2>%lf</standard_parallel2>\n"
            "                <central_meridian>%lf</central_meridian>\n"
            "                <origin_latitude>%lf</origin_latitude>\n"
            "                <false_easting>%lf</false_easting>\n"
            "                <false_northing>%lf</false_northing>\n"
            "            </albers_proj_params>\n",
            gmeta->proj_info.standard_parallel1,
            gmeta->proj_info.standard_parallel2,
            gmeta->proj_info.central_meridian, gmeta->proj_info.origin_latitude,
            gmeta->proj_info.false_easting, gmeta->proj_info.false_northing);
    }

    /* PS-specific parameters */
    if (gmeta->proj_info.proj_type == GCTP_PS_PROJ)
    {
        fprintf (fptr,
            "            <ps_proj_params>\n"
            "                <longitude_pole>%lf</longitude_pole>\n"
            "                <latitude_true_scale>%lf</latitude_true_scale>\n"
            "                <false_easting>%lf</false_easting>\n"
            "                <false_northing>%lf</false_northing>\n"
            "            </ps_proj_params>\n",
            gmeta->proj_info.longitude_pole,
            gmeta->proj_info.latitude_true_scale,
            gmeta->proj_info.false_easting, gmeta->proj_info.false_northing);
    }

    /* SIN-specific parameters */
    if (gmeta->proj_info.proj_type == GCTP_SIN_PROJ)
    {
        fprintf (fptr,
            "            <sin_proj_params>\n"
            "                <sphere_radius>%lf</sphere_radius>\n"
            "                <central_meridian>%lf</central_meridian>\n"
            "                <false_easting>%lf</false_easting>\n"
            "                <false_northing>%lf</false_northing>\n"
            "            </sin_proj_params>\n",
            gmeta->proj_info.sphere_radius, gmeta->proj_info.central_meridian,
            gmeta->proj_info.false_easting, gmeta->proj_info.false_northing);
    }

    fprintf (fptr,
        "        </projection_information>\n");

    /* Continue with the global metadata */
    fprintf (fptr,
        "        <orientation_angle>%f</orientation_angle>\n",
            gmeta->orientation_angle);

    fprintf (fptr,
        "    </global_metadata>\n\n");

    /* Write the bands metadata */
    fprintf (fptr,
        "    <bands>\n");

    /* Write the bands themselves.  Make sure the optional parameters have
       been specified and are not fill, otherwise don't write them out. */
    for (i = 0; i < metadata->nbands; i++)
    {
        switch (bmeta[i].data_type)
        {
            case ESPA_INT8: strcpy (my_dtype, "INT8"); break;
            case ESPA_UINT8: strcpy (my_dtype, "UINT8"); break;
            case ESPA_INT16: strcpy (my_dtype, "INT16"); break;
            case ESPA_UINT16: strcpy (my_dtype, "UINT16"); break;
            case ESPA_INT32: strcpy (my_dtype, "INT32"); break;
            case ESPA_UINT32: strcpy (my_dtype, "UINT32"); break;
            case ESPA_FLOAT32: strcpy (my_dtype, "FLOAT32"); break;
            case ESPA_FLOAT64: strcpy (my_dtype, "FLOAT64"); break;
            default: strcpy (my_dtype, "undefined"); break;
        }

        if (!strcmp (bmeta[i].source, ESPA_STRING_META_FILL)) /*no source type*/
            fprintf (fptr,
                "        <band product=\"%s\" name=\"%s\" category=\"%s\" "
                "data_type=\"%s\" nlines=\"%d\" nsamps=\"%d\" "
                "fill_value=\"%ld\"", bmeta[i].product, bmeta[i].name,
                bmeta[i].category, my_dtype, bmeta[i].nlines,
                bmeta[i].nsamps, bmeta[i].fill_value);
        else  /* contains a source type */
            fprintf (fptr,
                "        <band product=\"%s\" source=\"%s\" name=\"%s\" "
                "category=\"%s\" data_type=\"%s\" nlines=\"%d\" nsamps=\"%d\" "
                "fill_value=\"%ld\"", bmeta[i].product, bmeta[i].source,
                bmeta[i].name, bmeta[i].category, my_dtype, bmeta[i].nlines,
                bmeta[i].nsamps, bmeta[i].fill_value);

        if (bmeta[i].saturate_value != ESPA_INT_META_FILL)
            fprintf (fptr, " saturate_value=\"%d\"",
            bmeta[i].saturate_value);
        if (fabs (bmeta[i].scale_factor-ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
            fprintf (fptr, " scale_factor=\"%f\"", bmeta[i].scale_factor);
        if (fabs (bmeta[i].add_offset-ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
            fprintf (fptr, " add_offset=\"%f\"", bmeta[i].add_offset);
        fprintf (fptr, ">\n");

        fprintf (fptr,
            "            <short_name>%s</short_name>\n"
            "            <long_name>%s</long_name>\n"
            "            <file_name>%s</file_name>\n"
            "            <pixel_size x=\"%g\" y=\"%g\" units=\"%s\"/>\n"
            "            <data_units>%s</data_units>\n",
            bmeta[i].short_name, bmeta[i].long_name, bmeta[i].file_name,
            bmeta[i].pixel_size[0], bmeta[i].pixel_size[1],
            bmeta[i].pixel_units, bmeta[i].data_units);

        if (bmeta[i].valid_range[0] != ESPA_INT_META_FILL &&
            bmeta[i].valid_range[1] != ESPA_INT_META_FILL)
        {
            fprintf (fptr,
                "            <valid_range min=\"%ld\" max=\"%ld\"/>\n",
                bmeta[i].valid_range[0], bmeta[i].valid_range[1]);
        }

        if (fabs (bmeta[i].toa_gain - ESPA_FLOAT_META_FILL) > ESPA_EPSILON &&
            fabs (bmeta[i].toa_bias - ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
        {
            fprintf (fptr,
                "            <toa_reflectance gain=\"%g\" bias=\"%g\"/>\n",
                bmeta[i].toa_gain, bmeta[i].toa_bias);
        }

        if (bmeta[i].nbits != ESPA_INT_META_FILL && bmeta[i].nbits > 0)
        {
            fprintf (fptr,
                "            <bitmap_description>\n");
            for (j = 0; j < bmeta[i].nbits; j++)
            {
                fprintf (fptr,
                    "                <bit num=\"%d\">%s</bit>\n",
                    j, bmeta[i].bitmap_description[j]);
            }
            fprintf (fptr,
                "            </bitmap_description>\n");
        }

        if (bmeta[i].nclass != ESPA_INT_META_FILL && bmeta[i].nclass > 0)
        {
            fprintf (fptr,
                "            <class_values>\n");
            for (j = 0; j < bmeta[i].nclass; j++)
            {
                fprintf (fptr,
                    "                <class num=\"%d\">%s</class>\n",
                     bmeta[i].class_values[j].class,
                     bmeta[i].class_values[j].description);
            }
            fprintf (fptr,
                "            </class_values>\n");
        }

        if (fabs (bmeta[i].calibrated_nt-ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
        {
            fprintf (fptr,
                "            <calibrated_nt>%f</calibrated_nt>\n",
                bmeta[i].calibrated_nt);
        }

        fprintf (fptr,
            "            <app_version>%s</app_version>\n"
            "            <production_date>%s</production_date>\n"
            "        </band>\n",
            bmeta[i].app_version, bmeta[i].production_date);
    }

    /* Finish it off */
    fprintf (fptr,
        "    </bands>\n");
    fprintf (fptr,
        "</espa_metadata>\n");

    /* Close the XML file */
    fclose (fptr);

    /* Successful generation */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  append_metadata

PURPOSE: Append additional bands to an existing metadata file

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error appending the metadata file
SUCCESS         Successfully appended to the metadata file

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/30/2013   Gail Schmidt     Original development

NOTES:
  1. If the XML file specified already exists, it will be overwritten.
  2. Use this routine to append bands to and existing metadata file, use
     write_metadata to create a new metadata file.
  3. It is recommended that validate_meta be used after appending to the XML
     file to make sure the new file is valid against the ESPA schema.
******************************************************************************/
int append_metadata
(
    int nbands,               /* I: number of bands to be appended */
    Espa_band_meta_t *bmeta,  /* I: pointer to the array of bands metadata
                                    containing nbands */
    char *xml_file            /* I: name of the XML metadata file for appending
                                    the bands in bmeta */
)
{
    char FUNC_NAME[] = "append_metadata";       /* function name */
    char errmsg[STR_SIZE];   /* error message */
    char my_dtype[STR_SIZE]; /* data type string */
    char linebuf[MAX_LINE_SIZE];  /* buffer to hold each line */
    char *cur_ptr;           /* pointer index in the line buffer */
    int i, j;                /* looping variables */
    FILE *fptr = NULL;       /* file pointer to the XML metadata file */
    fpos_t cur_pos;          /* current position in the file */

    /* Open the metadata XML file for write or rewrite privelages */
    fptr = fopen (xml_file, "r+");
    if (fptr == NULL)
    {
        sprintf (errmsg, "Opening %s for write access.", xml_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Skip through the XML file looking for the closing </bands> element.
       That's where we want to append the new bands and then close everything
       off (i.e. bands and espa_metadata). Note, if the closing </bands>
       element is not found in the XML file, then the bands will simply be
       appended at the end of the XML file. This will likely leave an XML
       file which does not validate against the ESPA schema, but the input
       XML likely didn't validate either in this case. */
    if (fgetpos (fptr, &cur_pos) == -1)
    {
        sprintf (errmsg, "Getting the current position in the XML fstream.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    while (fgets (linebuf, MAX_LINE_SIZE, fptr))
    {
        /* Skip past the front end white space from proper indentation in
           the metadata file */
        cur_ptr = linebuf;
        while (cur_ptr[0] == ' ' || cur_ptr[0] == '\t')
            cur_ptr++;
        if (!strncmp (cur_ptr, "</bands>", 8))
        {
            /* </bands> line was found.  Now seek back to the beginning of
               that line in the file. */
            if (fsetpos (fptr, &cur_pos) == -1)
            {
                sprintf (errmsg, "Setting current position in the XML fstream "
                    "to the start of the </bands> line.");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Clear the while loop */
            break;
        }

        /* Store the location of the current stream so we can get back here */
        if (fgetpos (fptr, &cur_pos) == -1)
        {
            sprintf (errmsg, "Getting current position in the XML fstream.");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Append the new bands.  Make sure the optional parameters have been
       specified and are not fill, otherwise don't write them out. */
    for (i = 0; i < nbands; i++)
    {
        switch (bmeta[i].data_type)
        {
            case ESPA_INT8: strcpy (my_dtype, "INT8"); break;
            case ESPA_UINT8: strcpy (my_dtype, "UINT8"); break;
            case ESPA_INT16: strcpy (my_dtype, "INT16"); break;
            case ESPA_UINT16: strcpy (my_dtype, "UINT16"); break;
            case ESPA_INT32: strcpy (my_dtype, "INT32"); break;
            case ESPA_UINT32: strcpy (my_dtype, "UINT32"); break;
            case ESPA_FLOAT32: strcpy (my_dtype, "FLOAT32"); break;
            case ESPA_FLOAT64: strcpy (my_dtype, "FLOAT64"); break;
            default: strcpy (my_dtype, "undefined"); break;
        }

        if (!strcmp (bmeta[i].source, ESPA_STRING_META_FILL)) /*no source type*/
            fprintf (fptr,
                "        <band product=\"%s\" name=\"%s\" category=\"%s\" "
                "data_type=\"%s\" nlines=\"%d\" nsamps=\"%d\" "
                "fill_value=\"%ld\"", bmeta[i].product, bmeta[i].name,
                bmeta[i].category, my_dtype, bmeta[i].nlines,
                bmeta[i].nsamps, bmeta[i].fill_value);
        else  /* contains a source type */
            fprintf (fptr,
                "        <band product=\"%s\" source=\"%s\" name=\"%s\" "
                "category=\"%s\" data_type=\"%s\" nlines=\"%d\" nsamps=\"%d\" "
                "fill_value=\"%ld\"", bmeta[i].product, bmeta[i].source,
                bmeta[i].name, bmeta[i].category, my_dtype, bmeta[i].nlines,
                bmeta[i].nsamps, bmeta[i].fill_value);

        if (bmeta[i].saturate_value != ESPA_INT_META_FILL)
            fprintf (fptr, " saturate_value=\"%d\"",
            bmeta[i].saturate_value);
        if (fabs (bmeta[i].scale_factor-ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
            fprintf (fptr, " scale_factor=\"%f\"", bmeta[i].scale_factor);
        if (fabs (bmeta[i].add_offset-ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
            fprintf (fptr, " add_offset=\"%f\"", bmeta[i].add_offset);
        fprintf (fptr, ">\n");

        fprintf (fptr,
            "            <short_name>%s</short_name>\n"
            "            <long_name>%s</long_name>\n"
            "            <file_name>%s</file_name>\n"
            "            <pixel_size x=\"%g\" y=\"%g\" units=\"%s\"/>\n"
            "            <data_units>%s</data_units>\n",
            bmeta[i].short_name, bmeta[i].long_name, bmeta[i].file_name,
            bmeta[i].pixel_size[0], bmeta[i].pixel_size[1],
            bmeta[i].pixel_units, bmeta[i].data_units);

        if (bmeta[i].valid_range[0] != ESPA_INT_META_FILL &&
            bmeta[i].valid_range[1] != ESPA_INT_META_FILL)
        {
            fprintf (fptr,
                "            <valid_range min=\"%ld\" max=\"%ld\"/>\n",
                bmeta[i].valid_range[0], bmeta[i].valid_range[1]);
        }

        if (fabs (bmeta[i].toa_gain - ESPA_FLOAT_META_FILL) > ESPA_EPSILON &&
            fabs (bmeta[i].toa_bias - ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
        {
            fprintf (fptr,
                "            <toa_reflectance gain=\"%g\" bias=\"%g\"/>\n",
                bmeta[i].toa_gain, bmeta[i].toa_bias);
        }

        if (bmeta[i].nbits != ESPA_INT_META_FILL && bmeta[i].nbits > 0)
        {
            fprintf (fptr,
                "            <bitmap_description>\n");
            for (j = 0; j < bmeta[i].nbits; j++)
            {
                fprintf (fptr,
                    "                <bit num=\"%d\">%s</bit>\n",
                    j, bmeta[i].bitmap_description[j]);
            }
            fprintf (fptr,
                "            </bitmap_description>\n");
        }

        if (bmeta[i].nclass != ESPA_INT_META_FILL && bmeta[i].nclass > 0)
        {
            fprintf (fptr,
                "            <class_values>\n");
            for (j = 0; j < bmeta[i].nclass; j++)
            {
                fprintf (fptr,
                    "                <class num=\"%d\">%s</class>\n",
                     bmeta[i].class_values[j].class,
                     bmeta[i].class_values[j].description);
            }
            fprintf (fptr,
                "            </class_values>\n");
        }

        if (fabs (bmeta[i].calibrated_nt-ESPA_FLOAT_META_FILL) > ESPA_EPSILON)
        {
            fprintf (fptr,
                "            <calibrated_nt>%f</calibrated_nt>\n",
                bmeta[i].calibrated_nt);
        }

        fprintf (fptr,
            "            <app_version>%s</app_version>\n"
            "            <production_date>%s</production_date>\n"
            "        </band>\n",
            bmeta[i].app_version, bmeta[i].production_date);
    }

    /* Finish it off */
    fprintf (fptr,
        "    </bands>\n");
    fprintf (fptr,
        "</espa_metadata>\n");

    /* Close the XML file */
    fclose (fptr);

    /* Successful append */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  print_metadata_struct

PURPOSE: Print the metadata structure to stdout for debugging purposes.

RETURN VALUE: N/A

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/26/2013   Gail Schmidt     Original development
4/17/2014    Gail Schmidt     Modified to support additional projections

NOTES:
******************************************************************************/
void print_metadata_struct
(
    Espa_internal_meta_t *metadata  /* I: input metadata structure to be
                                          printed */
)
{
    int i, j;                                   /* looping variables */

    /* Print the metadata structure to stdout */
    printf ("DEBUG Metadata structure:\n");
    printf ("  namespace: %s\n", metadata->meta_namespace);
    printf ("  nbands: %d\n", metadata->nbands);

    printf ("DEBUG Global Metadata structure:\n");
    printf ("  data_provider: %s\n", metadata->global.data_provider);
    printf ("  satellite: %s\n", metadata->global.satellite);
    printf ("  instrument: %s\n", metadata->global.instrument);
    printf ("  acquisition_date: %s\n", metadata->global.acquisition_date);
    printf ("  scene_center_time: %s\n", metadata->global.scene_center_time);
    printf ("  level1_production_date: %s\n",
        metadata->global.level1_production_date);
    printf ("  solar_zenith: %f\n", metadata->global.solar_zenith);
    printf ("  solar_azimuth: %f\n", metadata->global.solar_azimuth);
    printf ("  solar_units: %s\n", metadata->global.solar_units);
    printf ("  wrs_system: %d\n", metadata->global.wrs_system);
    printf ("  wrs_path: %d\n", metadata->global.wrs_path);
    printf ("  wrs_row: %d\n", metadata->global.wrs_row);
    printf ("  lpgs_metadata_file: %s\n",
        metadata->global.lpgs_metadata_file);
    printf ("  ul_corner (lat, long): %f %f\n",
        metadata->global.ul_corner[0], metadata->global.ul_corner[1]);
    printf ("  lr_corner (lat, long): %f %f\n",
        metadata->global.lr_corner[0], metadata->global.lr_corner[1]);
    printf ("  bounding_coords (west, east, north, south): %f %f %f %f\n",
        metadata->global.bounding_coords[ESPA_WEST],
        metadata->global.bounding_coords[ESPA_EAST],
        metadata->global.bounding_coords[ESPA_NORTH],
        metadata->global.bounding_coords[ESPA_SOUTH]);

    if (metadata->global.proj_info.datum_type == ESPA_WGS84)
        printf ("  datum: WGS84\n");
    else if (metadata->global.proj_info.datum_type == ESPA_NAD27)
        printf ("  datum: NAD27\n");
    else if (metadata->global.proj_info.datum_type == ESPA_NAD83)
        printf ("  datum: NAD83\n");
    else if (metadata->global.proj_info.datum_type == ESPA_NODATUM)
        printf ("  datum: No Datum\n");

    if (metadata->global.proj_info.proj_type == GCTP_GEO_PROJ)
        printf ("  projection type: GEO\n");
    else if (metadata->global.proj_info.proj_type == GCTP_UTM_PROJ)
        printf ("  projection type: UTM\n");
    else if (metadata->global.proj_info.proj_type == GCTP_ALBERS_PROJ)
        printf ("  projection type: ALBERS\n");
    else if (metadata->global.proj_info.proj_type == GCTP_PS_PROJ)
        printf ("  projection type: POLAR STEREOGRAPHIC\n");
    else if (metadata->global.proj_info.proj_type == GCTP_SIN_PROJ)
        printf ("  projection type: SINUSOIDAL\n");
    printf ("  projection units: %s\n", metadata->global.proj_info.units);
    printf ("  UL projection x,y: %f, %f\n",
        metadata->global.proj_info.ul_corner[0],
        metadata->global.proj_info.ul_corner[1]);
    printf ("  LR projection x,y: %f, %f\n",
        metadata->global.proj_info.lr_corner[0],
        metadata->global.proj_info.lr_corner[1]);
    printf ("  grid origin: %s\n", metadata->global.proj_info.grid_origin);

    if (metadata->global.proj_info.proj_type == GCTP_UTM_PROJ)
    {
        printf ("  UTM zone: %d\n", metadata->global.proj_info.utm_zone);
    }
    else if (metadata->global.proj_info.proj_type == GCTP_PS_PROJ)
    {
        printf ("  longitude_pole: %f\n",
            metadata->global.proj_info.longitude_pole);
        printf ("  latitude_true_scale: %f\n",
            metadata->global.proj_info.latitude_true_scale);
        printf ("  false_easting: %f\n",
            metadata->global.proj_info.false_easting);
        printf ("  false_northing: %f\n",
            metadata->global.proj_info.false_northing);
    }
    else if (metadata->global.proj_info.proj_type == GCTP_ALBERS_PROJ)
    {
        printf ("  standard_parallel1: %f\n",
            metadata->global.proj_info.standard_parallel1);
        printf ("  standard_parallel2: %f\n",
            metadata->global.proj_info.standard_parallel2);
        printf ("  central_meridian: %f\n",
            metadata->global.proj_info.central_meridian);
        printf ("  origin_latitude: %f\n",
            metadata->global.proj_info.origin_latitude);
        printf ("  false_easting: %f\n",
            metadata->global.proj_info.false_easting);
        printf ("  false_northing: %f\n",
            metadata->global.proj_info.false_northing);
    }
    else if (metadata->global.proj_info.proj_type == GCTP_SIN_PROJ)
    {
        printf ("  sphere_radius: %f\n",
            metadata->global.proj_info.sphere_radius);
        printf ("  central_meridian: %f\n",
            metadata->global.proj_info.central_meridian);
        printf ("  false_easting: %f\n",
            metadata->global.proj_info.false_easting);
        printf ("  false_northing: %f\n",
            metadata->global.proj_info.false_northing);
    }

    printf ("  orientation_angle: %f\n",
        metadata->global.orientation_angle);
    printf ("\n");

    printf ("DEBUG Bands Metadata structure:\n");
    printf ("  %d bands are represented in this structure\n", metadata->nbands);
    for (i = 0; i < metadata->nbands; i++)
    {
        printf ("  Band %d -->\n", i+1);
        printf ("    product: %s\n", metadata->band[i].product);
        printf ("    source: %s\n", metadata->band[i].source);
        printf ("    name: %s\n", metadata->band[i].name);
        printf ("    category: %s\n", metadata->band[i].category);
        printf ("    data_type: ");
        switch (metadata->band[i].data_type)
        {
            case ESPA_INT8: printf ("INT8\n"); break;
            case ESPA_UINT8: printf ("UINT8\n"); break;
            case ESPA_INT16: printf ("INT16\n"); break;
            case ESPA_UINT16: printf ("UINT16\n"); break;
            case ESPA_INT32: printf ("INT32\n"); break;
            case ESPA_UINT32: printf ("UINT32\n"); break;
            case ESPA_FLOAT32: printf ("FLOAT32\n"); break;
            case ESPA_FLOAT64: printf ("FLOAT64\n"); break;
        }
        printf ("    nlines: %d\n", metadata->band[i].nlines);
        printf ("    nsamps: %d\n", metadata->band[i].nsamps);
        printf ("    fill_value: %ld\n", metadata->band[i].fill_value);
        printf ("    saturate_value: %d\n", metadata->band[i].saturate_value);
        printf ("    scale_factor: %f\n", metadata->band[i].scale_factor);
        printf ("    add_offset: %f\n", metadata->band[i].add_offset);
        printf ("    short_name: %s\n", metadata->band[i].short_name);
        printf ("    long_name: %s\n", metadata->band[i].long_name);
        printf ("    file_name: %s\n", metadata->band[i].file_name);
        printf ("    pixel_size (x, y) : %g %g\n",
            metadata->band[i].pixel_size[0], metadata->band[i].pixel_size[1]);
        printf ("    data_units: %s\n", metadata->band[i].data_units);
        if (metadata->band[i].valid_range[0] != 0 ||
            metadata->band[i].valid_range[1] != 0)
        {
            printf ("    valid_range (x, y) : %ld %ld\n",
                metadata->band[i].valid_range[0],
                metadata->band[i].valid_range[1]);
        }
        if (metadata->band[i].toa_gain != 0 ||
            metadata->band[i].toa_bias != 0)
        {
            printf ("    toa_reflectance gain, bias : %g %g\n",
                metadata->band[i].toa_gain, metadata->band[i].toa_bias);
        }
        if (metadata->band[i].nbits != 0)
        {
            printf ("    Bit descriptions:\n");
            for (j = 0; j < metadata->band[i].nbits; j++)
            {
                printf ("      bit %d: %s\n", j,
                     metadata->band[i].bitmap_description[j]);
            }
        }
        if (metadata->band[i].nclass != 0)
        {
            printf ("    Class descriptions:\n");
            for (j = 0; j < metadata->band[i].nclass; j++)
            {
                printf ("      class value %d: %s\n",
                     metadata->band[i].class_values[j].class,
                     metadata->band[i].class_values[j].description);
            }
        }
        printf ("    calibrated_nt: %f\n", metadata->band[i].calibrated_nt);
        printf ("    app_version: %s\n", metadata->band[i].app_version);
        printf ("    production_date: %s\n", metadata->band[i].production_date);
        printf ("\n");
    }
}

