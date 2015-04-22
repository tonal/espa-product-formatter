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
4/17/2014    Gail Schmidt     Modified to support additional projections
4/23/2014    Gail Schmidt     Modified to support additional datums
11/13/2014   Gail Schmidt     fill_value is now optional

NOTES:
  1. Only supports GEO, UTM, ALBERS, PS, and SIN projections.
  2. Only supports WGS84, NAD27, NAD83 datums (GEO, UTM, ALBERS, PS).
  3. Sinusoidal needs to pass the radius of the sphere in the first
     projection parameter for the ENVI header.
  4. The following are the strings to use for the various datums, obtained from
     ExcelisVis via http://www.exelisvis.com/services/Files/envi_pe/envi_pe_v10/EnviPEReferenceDocs/EnviPeGeogcs_v10.txt
     WGS84: GEOGCS["GCS_WGS_1984",DATUM["D_WGS_1984",SPHEROID["WGS_1984",6378137.0,298.257223563]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]]
     NAD27: GEOGCS["GCS_North_American_1927",DATUM["D_North_American_1927",SPHEROID["Clarke_1866",6378206.4,294.9786982]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]]
     NAD83: GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]]
******************************************************************************/
int write_envi_hdr
(
    char *hdr_file,     /* I: name of ENVI header file to be generated */
    Envi_header_t *hdr  /* I: input ENVI header information */
)
{
    char FUNC_NAME[] = "write_envi_hdr";   /* function name */
    char errmsg[STR_SIZE];       /* error message */
    char geogcs_str[STR_SIZE];   /* string for the GCS code */
    char datum_str[STR_SIZE];    /* string for the datum code */
    char proj_datum_str[STR_SIZE];  /* string for the datum code in projection
                                       info section */
    char spheroid_str[STR_SIZE]; /* string for the spheroid code */
    int i;                       /* looping variable */
    double semi_major_axis;      /* semi-major axis for the spheroid */
    double semi_minor_axis;      /* semi-minor axis for the spheroid */
    double inv_flattening;       /* inverse flattening for the spheroid */
    FILE *hdr_fptr = NULL;       /* file pointer to the ENVI header file */

    /* Open the header file */
    hdr_fptr = fopen (hdr_file, "w");
    if (hdr_fptr == NULL)
    {
        sprintf (errmsg, "Opening %s for write access.", hdr_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Verify the projection is GEO, UTM, ALBERS, PS, or SIN and datum is
       WGS-84 */
    if (hdr->proj_type != GCTP_GEO_PROJ &&
        hdr->proj_type != GCTP_UTM_PROJ &&
        hdr->proj_type != GCTP_ALBERS_PROJ &&
        hdr->proj_type != GCTP_PS_PROJ &&
        hdr->proj_type != GCTP_SIN_PROJ)
    {
        sprintf (errmsg, "Unsupported projection code (%d).  GEO projection "
            "code (%d) or UTM projection code (%d) or ALBERS projection code "
            "(%d) or PS projection code (%d) or SIN projection code (%d) "
            "expected.", hdr->proj_type, GCTP_GEO_PROJ, GCTP_UTM_PROJ,
            GCTP_ALBERS_PROJ, GCTP_PS_PROJ, GCTP_SIN_PROJ);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (hdr->datum_type != ESPA_WGS84 &&
        hdr->datum_type != ESPA_NAD27 &&
        hdr->datum_type != ESPA_NAD83 &&
        hdr->datum_type != ESPA_NODATUM)
    {
        sprintf (errmsg, "Unsupported datum code (%d). WGS84 datum code (%d) "
            "or NAD27 datum code (%d) or NAD83 datum code (%d) or NODATUM "
            "datum code (%d) expected.", hdr->datum_type, ESPA_WGS84,
            ESPA_NAD27, ESPA_NAD83, ESPA_NODATUM);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Determine the GCS, DATUM, and SPHEROID strings that will get written
       along with the semi-major and inverse flattening numbers */
    switch (hdr->datum_type)
    {
        case ESPA_WGS84:  /* WGS84 sphere */
            strcpy (geogcs_str, "GCS_WGS_1984");
            strcpy (datum_str, "D_WGS_1984");
            strcpy (spheroid_str, "WGS_1984");
            strcpy (proj_datum_str, "WGS-84");
            semi_major_axis = GCTP_WGS84_SEMI_MAJOR;
            semi_minor_axis = GCTP_WGS84_SEMI_MINOR;
            inv_flattening = GCTP_WGS84_INV_FLATTENING;
            break;

        case ESPA_NAD27:  /* Clarke 1866 sphere */
            strcpy (geogcs_str, "GCS_North_American_1927");
            strcpy (datum_str, "D_North_American_1927");
            strcpy (spheroid_str, "Clarke_1866");
            strcpy (proj_datum_str, "North America 1927");
            semi_major_axis = GCTP_CLARKE_1866_SEMI_MAJOR;
            semi_minor_axis = GCTP_CLARKE_1866_SEMI_MINOR;
            inv_flattening = GCTP_CLARKE_1866_INV_FLATTENING;
            break;

        case ESPA_NAD83:  /* GRS 1980 sphere */
            strcpy (geogcs_str, "GCS_North_American_1983");
            strcpy (datum_str, "D_North_American_1983");
            strcpy (spheroid_str, "GRS_1980");
            strcpy (proj_datum_str, "North America 1983");
            semi_major_axis = GCTP_GRS80_SEMI_MAJOR;
            semi_minor_axis = GCTP_GRS80_SEMI_MINOR;
            inv_flattening = GCTP_GRS80_INV_FLATTENING;
            break;
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
        "data type = %d\n",
        hdr->description, hdr->nsamps, hdr->nlines, hdr->nbands,
        hdr->header_offset, hdr->byte_order, hdr->file_type, hdr->data_type);

    if (hdr->data_ignore_value != ESPA_INT_META_FILL)
        fprintf (hdr_fptr, "data ignore value = %ld\n", hdr->data_ignore_value);
   
    fprintf (hdr_fptr,
        "interleave = %s\n"
        "sensor_type = %s\n",
        hdr->interleave, hdr->sensor_type);
   
    if (hdr->proj_type == GCTP_GEO_PROJ)
    {
        fprintf (hdr_fptr,
            "map info = {Geographic Lat/Lon, %d, %d, %f, %f, %g, %g, %s, "
            "units=Degrees}\n", hdr->xy_start[0], hdr->xy_start[1],
            hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
            hdr->pixel_size[1], proj_datum_str);
        fprintf (hdr_fptr,
            "coordinate system string = GEOGCS[\"%s\", DATUM[\"%s\", "
            "SPHEROID[\"%s\",%g,%g]], PRIMEM[\"Greenwich\",0.0], "
            "UNIT[\"Degree\",0.0174532925199433]]\n", geogcs_str, datum_str,
            spheroid_str, semi_major_axis, inv_flattening);
    }
    else if (hdr->proj_type == GCTP_UTM_PROJ)
    {
        if (hdr->utm_zone > 0)
            fprintf (hdr_fptr,
                "map info = {UTM, %d, %d, %f, %f, %g, %g, %d, North, %s, "
                "units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
                hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
                hdr->pixel_size[1], hdr->utm_zone, proj_datum_str);
        else
            fprintf (hdr_fptr,
                "map info = {UTM, %d, %d, %f, %f, %f, %f, %d, South, %s, "
                "units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
                hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
                hdr->pixel_size[1], -(hdr->utm_zone), proj_datum_str);
    }
    else if (hdr->proj_type == GCTP_ALBERS_PROJ)
    {
        fprintf (hdr_fptr,
            "map info = {Albers Conical Equal Area, %d, %d, %f, %f, %g, %g, "
            "%s, units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
            hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
            hdr->pixel_size[1], proj_datum_str);
        fprintf (hdr_fptr,
            "projection info = {%d, %g, %g, %g, %g, %g, %g, %g, %g, "
            "%s, Albers Conical Equal Area, units=Meters}\n",
            ENVI_ALBERS_PROJ, semi_major_axis, semi_minor_axis,
            hdr->proj_parms[5], hdr->proj_parms[4], hdr->proj_parms[6],
            hdr->proj_parms[7], hdr->proj_parms[2], hdr->proj_parms[3],
            proj_datum_str);
        fprintf (hdr_fptr,
            "coordinate system string = "
            "{PROJCS[\"Albers\",GEOGCS[\"%s\", DATUM[\"%s\", "
            "SPHEROID[\"%s\",%g,%g]], PRIMEM[\"Greenwich\",0.0], "
            "UNIT[\"Degree\",0.0174532925199433]], "
            "PROJECTION[\"Albers\"], PARAMETER[\"False_Easting\",%f], "
            "PARAMETER[\"False_Northing\",%f], "
            "PARAMETER[\"Central_Meridian\",%f], "
            "PARAMETER[\"Standard_Parallel_1\",%f], "
            "PARAMETER[\"Standard_Parallel_2\",%f], "
            "PARAMETER[\"Latitude_Of_Origin\",%f], UNIT[\"Meter\",1.0]]}\n",
            geogcs_str, datum_str, spheroid_str, semi_major_axis,
            inv_flattening, hdr->proj_parms[6], hdr->proj_parms[7],
            hdr->proj_parms[4], hdr->proj_parms[2], hdr->proj_parms[3],
            hdr->proj_parms[5]);
    }
    else if (hdr->proj_type == GCTP_PS_PROJ)
    {
        fprintf (hdr_fptr,
            "map info = {Polar Stereographic, %d, %d, %f, %f, %g, %g, %s, "
            "units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
            hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
            hdr->pixel_size[1], proj_datum_str);
        fprintf (hdr_fptr,
            "projection info = {%d, %g, %g, %g, %g, %g, %g, %s, "
            "Polar Stereographic, units=Meters}\n", ENVI_PS_PROJ,
            semi_major_axis, semi_minor_axis, hdr->proj_parms[5],
            hdr->proj_parms[4], hdr->proj_parms[6], hdr->proj_parms[7],
            proj_datum_str);
        fprintf (hdr_fptr,
            "coordinate system string = "
            "{PROJCS[\"Stereographic_South_Pole\", "
            "GEOGCS[\"%s\", DATUM[\"%s\", SPHEROID[\"%s\",%g,%g]], "
            "PRIMEM[\"Greenwich\",0.0], UNIT[\"Degree\",0.0174532925199433]], "
            "PROJECTION[\"Stereographic_South_Pole\"], "
            "PARAMETER[\"False_Easting\",%f], "
            "PARAMETER[\"False_Northing\",%f], "
            "PARAMETER[\"Central_Meridian\",%f], "
            "PARAMETER[\"Standard_Parallel_1\",%f], "
            "UNIT[\"Meter\",1.0]]}\n",
            geogcs_str, datum_str, spheroid_str, semi_major_axis,
            inv_flattening, hdr->proj_parms[6], hdr->proj_parms[7],
            hdr->proj_parms[4], hdr->proj_parms[5]);
    }
    else if (hdr->proj_type == GCTP_SIN_PROJ)
    {
        /* Note: No datum is used for this projection, just the radius of the
           sphere */
        fprintf (hdr_fptr,
            "map info = {Sinusoidal, %d, %d, %f, %f, %g, %g, "
            "units=Meters}\n", hdr->xy_start[0], hdr->xy_start[1],
            hdr->ul_corner[0], hdr->ul_corner[1], hdr->pixel_size[0],
            hdr->pixel_size[1]);
        fprintf (hdr_fptr,
            "projection info = {%d, %f, %f, %f, %f, Sinusoidal, "
            "units=Meters}\n", ENVI_SIN_PROJ, hdr->proj_parms[0],
            hdr->proj_parms[4], hdr->proj_parms[6], hdr->proj_parms[7]);
        fprintf (hdr_fptr,
            "coordinate system string = {PROJCS[\"Sphere_Sinusoidal\", "
            "GEOGCS[\"GCS_Sphere\", DATUM[\"D_Sphere\", "
            "SPHEROID[\"Sphere\",%f,0.0]], "
            "PRIMEM[\"Greenwich\",0.0], "
            "UNIT[\"Degree\",0.0174532925199433]], "
            "PROJECTION[\"Sinusoidal\"], PARAMETER[\"Central_Meridian\",%f], "
            "PARAMETER[\"False_Easting\",%f], "
            "PARAMETER[\"False_Northing\",%f], UNIT[\"Meter\",1.0]]}\n",
            hdr->proj_parms[0], hdr->proj_parms[4], hdr->proj_parms[6],
            hdr->proj_parms[7]);

/*        fprintf (hdr_fptr,
            "coordinate system string = {PROJCS[\"Sinusoidal\", "
            "GEOGCS[\"GCS_ELLIPSE_BASED_1\", DATUM[\"D_ELLIPSE_BASED_1\", "
            "SPHEROID[\"S_ELLIPSE_BASED_1\",%f,0.0]], "
            "PRIMEM[\"Greenwich\",0.0], "
            "UNIT[\"Degree\",0.0174532925199433]], "
            "PROJECTION[\"Sinusoidal\"], PARAMETER[\"Central_Meridian\",%f], "
            "PARAMETER[\"False_Easting\",%f], "
            "PARAMETER[\"False_Northing\",%f], UNIT[\"Meter\",1.0]]}\n",
            hdr->proj_parms[0], hdr->proj_parms[4], hdr->proj_parms[6],
            hdr->proj_parms[7]);
*/
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
4/17/2014    Gail Schmidt     Modified to support additional projections
4/23/2014    Gail Schmidt     Modified to support additional datums

NOTES:
  1. Only supports GEO, UTM, ALBERS, PS, SIN projections.
  2. Only supports WGS84 datum.
  3. Sinusoidal needs to pass the radius of the sphere in the first
     projection parameter for the ENVI header.
  4. Refer to Working with ENVI Header Files in
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
        case GCTP_GEO_PROJ:
            /* just use the already initialized zeros for the proj parms */
            break;

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

        case GCTP_SIN_PROJ:
            hdr->proj_parms[0] = gmeta->proj_info.sphere_radius;
            hdr->proj_parms[4] = gmeta->proj_info.central_meridian;
            hdr->proj_parms[6] = gmeta->proj_info.false_easting;
            hdr->proj_parms[7] = gmeta->proj_info.false_northing;
            break;

        default:
            sprintf (errmsg, "Unsupported projection type (%d).  GEO "
                "projection code (%d) or UTM projection code (%d) or ALBERS "
                "projection code (%d) or PS projection code (%d) or SIN "
                "projection code (%d) expected.", gmeta->proj_info.proj_type,
                GCTP_GEO_PROJ, GCTP_UTM_PROJ, GCTP_ALBERS_PROJ, GCTP_PS_PROJ,
                GCTP_SIN_PROJ);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
    }

    hdr->datum_type = gmeta->proj_info.datum_type;
    if (gmeta->proj_info.datum_type != ESPA_WGS84 &&
        gmeta->proj_info.datum_type != ESPA_NAD27 &&
        gmeta->proj_info.datum_type != ESPA_NAD83 &&
        gmeta->proj_info.datum_type != ESPA_NODATUM)
    {
        sprintf (errmsg, "Unsupported datum code (%d). WGS84 datum code (%d) "
            "or NAD27 datum code (%d) or NAD83 datum code (%d) or NODATUM "
            "datum code (%d) expected.", gmeta->proj_info.datum_type,
            ESPA_WGS84, ESPA_NAD27, ESPA_NAD83, ESPA_NODATUM);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

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

