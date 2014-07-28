/*****************************************************************************
FILE: espa_hdf.c
  
PURPOSE: Contains functions and defines for creating HDF-EOS metadata and
assembling the SDSs into HDF-EOS Grids.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/8/2014     Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#include "espa_hdf_eos.h"

#define OUTPUT_HDFEOS_VERSION ("HDFEOSVersion");
#define OUTPUT_STRUCT_METADATA ("StructMetadata.0");
#define OUTPUT_ORIENTATION_ANGLE_HDF ("OrientationAngle")

/******************************************************************************
MODULE:  deg_to_dms

PURPOSE:  Converts the decimal degree value to DMS in the form of DDDMMMSSS.ss.

RETURN VALUE:
Type = double
Value      Description
-----      -----------
{all}      Packed DMS value

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
7/24/2014    Gail Schmidt     Original Development

NOTES:
******************************************************************************/
double deg_to_dms
(
    double flt_deg   /* I: input decimal degree value */
)
{
    int deg;         /* integer degree portion of the input value */
    int min;         /* integer minute portion of the input value */
    float sec;       /* seconds portion of the input value */
    double dms;      /* packed degrees, minutes, seconds */

    /* Grab the integer degrees from the input value */
    deg = (int) flt_deg;

    /* Obtain the integer minutes from the input value */
    min = (int) ((flt_deg - deg) * 60.0);

    /* Obtain the seconds from the input value */
    sec = (flt_deg - deg - min / 60.0) * 3600.0;

    /* If the seconds or minutes are 60, handle that case */
    if (sec >= 60.0)
    {
        sec -= 60.0;
        min++;
    }
    if (min >= 60)
    {
        min -= 60;
        deg++;
    }

    /* Create the packed DMS value */
    dms = deg * 1000000.0 + min * 1000.0 + sec;

    return (dms);
}


/******************************************************************************
MODULE:  append_meta

PURPOSE:  Appends the string to the metadata buffer.

RETURN VALUE:
Type = bool
Value      Description
-----      -----------
false      Error in the number of attributes
true       Successful processing

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
1/7/2014     Gail Schmidt     Original Development (based on input routines
                              from the LEDAPS lndsr application)

NOTES:
******************************************************************************/
bool append_meta
(
    char *cbuf,  /* I/O: input metadata buffer */
    int *ic,     /* I/O: index of current location in metadata buffer */
    char *str    /* I: string to append to the metadata buffer */
)
{
    int nc, i;
  
    /* Validate the string and number of attributes */
    if (*ic < 0)
        return false;
    nc = strlen (str);
    if (nc <= 0)
        return false;
    if (*ic + nc > ESPA_MAX_METADATA_SIZE)
        return false;
  
    /* Add the string to the metadata */
    for (i = 0; i < nc; i++)
    {
        cbuf[*ic] = str[i];
        (*ic)++;
    }
    cbuf[*ic] = '\0';
  
    return true;
}

/******************************************************************************
MODULE:  write_hdf_eos_attr

PURPOSE:  Write the spatial definition HDF-EOS attributes to the HDF file and
move the SDSs to the Grid.

RETURN VALUE:
Type = int
Value      Description
-----      -----------
ERROR      Error occurred writing the metadata to the HDF file
SUCCESS    Successful completion

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
1/7/2014     Gail Schmidt     Original Development (based on input routines
                              from the LEDAPS lndsr application)
4/22/2014    Gail Schmidt     Updated to support additional projections and
                              datums
4/24/2014    Gail Schmidt     Modified grid and dimension naming (in the case
                              of multiple resolutions) for Geographic
                              projections to not use the pixel size
7/24/2014    Gail Schmidt     Modified the output Grid fields to utilize
                              packed DMS for lat/long values
7/24/2014    Gail Schmidt     Need to write the sphere code for correct
                              handling of the HDF-EOS products.  We will leave
                              the datum string as an attribute.

NOTES:
******************************************************************************/
int write_hdf_eos_attr
(
    char *hdf_file,            /* I: HDF file to write attributes to */
    Espa_internal_meta_t *xml_metadata   /* I: XML metadata structure */
)
{
    char FUNC_NAME[] = "write_hdf_eos_attr";  /* function name */
    char errmsg[STR_SIZE];                    /* error message */
    char grid_name[] = "Grid";                /* name of the HDF-EOS grid */
    char struct_meta[ESPA_MAX_METADATA_SIZE]; /* structural metadata */
    char cbuf[ESPA_MAX_METADATA_SIZE];        /* temp buffer for metadata */
    char *dim_names[2] = {"YDim", "XDim"};    /* base names for dimensions */
    char proj_str[STR_SIZE];                  /* projection string */
    char datum_str[STR_SIZE];                 /* datum string */
    char dtype[STR_SIZE];                     /* data type */
    char temp_name[STR_SIZE];                 /* temporary grid name */
    double ul_corner[2];     /* UL corner x,y -- Geographic is DMS */
    double lr_corner[2];     /* LR corner x,y -- Geographic is DMS */
    double proj_parms[NPROJ_PARAM];  /* projection parameters */
    double dval;             /* temporary double value */
    int meta_indx;           /* index of current location in metadata buffer */
    int sphere_code;         /* GCTP value for the associated spheroid */
    int i;                   /* looping variable */
    int count;               /* number of chars copied in snprintf */
    int mycount;             /* integer value to use in the name of the 2nd,
                                3rd, etc. grid dimensions */
    int isds;                /* looping variable for SDSs */
    int ngrids;              /* number of grids written to HDF file */
    int nfields;             /* number of fields written for this grid */
    int igrid;               /* looping variable for the grids */
    int grid[MAX_TOTAL_BANDS]; /* which band in XML was the grid based on */
    int32 hdf_id;            /* HDF-EOS file ID */
    int32 hdf_file_id;       /* HDF file ID */
    bool processed[MAX_TOTAL_BANDS];  /* was this band processed already */
    bool done;               /* are we done processing all bands */
    int32 vgroup_id[3];      /* array to hold Vgroup IDs */
    int32 sds_index;         /* index of SDS in the HDF file */
    int32 sds_id;            /* SDS ID */
    Espa_hdf_attr_t attr;    /* attributes for writing the metadata */
    Espa_global_meta_t *gmeta = &xml_metadata->global;
                             /* pointer to global metadata structure */
  
    /* Initialize the bands to not processed */
    for (isds = 0; isds < xml_metadata->nbands; isds++)
        processed[isds] = false;

    /* Build the HDF-EOS header */
    meta_indx = 0;
    count = snprintf (cbuf, sizeof (cbuf),
        "\nGROUP=SwathStructure\n" 
        "END_GROUP=SwathStructure\n" 
        "GROUP=GridStructure\n" 
        "\tGROUP=GRID_1\n");
    if (count < 0 || count >= sizeof (cbuf))
    {
        sprintf (errmsg, "Overflow of cbuf string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (!append_meta (struct_meta, &meta_indx, cbuf))
    {
        sprintf (errmsg, "Error appending to the start of the metadata string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /** Build the metadata for the first grid (possibly the only grid if this
        isn't a multi-resolution product) using the information from the first
        band **/
    /* Get the projection name string */
    switch (gmeta->proj_info.proj_type)
    {
        case (GCTP_GEO_PROJ): strcpy (proj_str, "GEO"); break;
        case (GCTP_UTM_PROJ): strcpy (proj_str, "UTM"); break;
        case (GCTP_PS_PROJ): strcpy (proj_str, "PS"); break;
        case (GCTP_ALBERS_PROJ): strcpy (proj_str, "ALBERS"); break;
        case (GCTP_SIN_PROJ): strcpy (proj_str, "SIN"); break;
    }
  
    /* If the grid origin is center, then adjust for the resolution.  The
       corners will be written for the UL of the corner. */
    if (!strcmp (gmeta->proj_info.grid_origin, "CENTER"))
    {
        ul_corner[0] = gmeta->proj_info.ul_corner[0] -
            0.5 * xml_metadata->band[0].pixel_size[0];
        ul_corner[1] = gmeta->proj_info.ul_corner[1] +
            0.5 * xml_metadata->band[0].pixel_size[1];
        lr_corner[0] = gmeta->proj_info.lr_corner[0] -
            0.5 * xml_metadata->band[0].pixel_size[0];
        lr_corner[1] = gmeta->proj_info.lr_corner[1] +
            0.5 * xml_metadata->band[0].pixel_size[1];
    }
    else
    {
        ul_corner[0] = gmeta->proj_info.ul_corner[0];
        ul_corner[1] = gmeta->proj_info.ul_corner[1];
        lr_corner[0] = gmeta->proj_info.lr_corner[0];
        lr_corner[1] = gmeta->proj_info.lr_corner[1];
    }

    /* Write the Grid information for the first resolution in the product */
    count = snprintf (cbuf, sizeof (cbuf),
        "\t\tGridName=\"%s\"\n" 
        "\t\tXDim=%d\n" 
        "\t\tYDim=%d\n" 
        "\t\tPixelSize=%g,%g\n",
        grid_name, xml_metadata->band[0].nsamps, xml_metadata->band[0].nlines, 
        xml_metadata->band[0].pixel_size[0],
        xml_metadata->band[0].pixel_size[1]);
    if (count < 0 || count >= sizeof (cbuf))
    {
        sprintf (errmsg, "Overflow of cbuf string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (!append_meta (struct_meta, &meta_indx, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (grid information "
            "start)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Convert the UL and LR corners to DDDMMMSSS.SS if the projection
       is geographic, otherwise write the corners as-is. */
    if (gmeta->proj_info.proj_type == GCTP_GEO)
    {
        ul_corner[0] = deg_to_dms (ul_corner[0]);
        ul_corner[1] = deg_to_dms (ul_corner[1]);
        lr_corner[0] = deg_to_dms (lr_corner[0]);
        lr_corner[1] = deg_to_dms (lr_corner[1]);

        count = snprintf (cbuf, sizeof (cbuf),
            "\t\tUpperLeftPointMtrs=(%.2f,%.2f)\n" 
            "\t\tLowerRightMtrs=(%.2f,%.2f)\n" 
            "\t\tProjection=GCTP_%s\n", 
            ul_corner[0], ul_corner[1], lr_corner[0], lr_corner[1], proj_str);
        if (count < 0 || count >= sizeof (cbuf))
        {
            sprintf (errmsg, "Overflow of cbuf string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else
    {
        count = snprintf (cbuf, sizeof (cbuf),
            "\t\tUpperLeftPointMtrs=(%.6f,%.6f)\n" 
            "\t\tLowerRightMtrs=(%.6f,%.6f)\n" 
            "\t\tProjection=GCTP_%s\n", 
            ul_corner[0], ul_corner[1], lr_corner[0], lr_corner[1], proj_str);
        if (count < 0 || count >= sizeof (cbuf))
        {
            sprintf (errmsg, "Overflow of cbuf string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    if (!append_meta (struct_meta, &meta_indx, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (grid information "
            "start)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Write projection information */
    if (gmeta->proj_info.proj_type == GCTP_UTM_PROJ)
    {
        /* Write the UTM zone */
        count = snprintf (cbuf, sizeof (cbuf),
            "\t\tZoneCode=%d\n", gmeta->proj_info.utm_zone);
        if (count < 0 || count >= sizeof (cbuf))
        {
            sprintf (errmsg, "Overflow of cbuf string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (!append_meta (struct_meta, &meta_indx, cbuf))
        {
            sprintf (errmsg, "Error appending to metadata string (zone "
                "number)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else if (gmeta->proj_info.proj_type != GCTP_GEO_PROJ)
    {  /* don't write projection parameters for Geographic */
        /* Write the projection parameters */
        count = snprintf (cbuf, sizeof (cbuf), "\t\tProjParams=(");
        if (count < 0 || count >= sizeof (cbuf))
        {
            sprintf (errmsg, "Overflow of cbuf string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (!append_meta (struct_meta, &meta_indx, cbuf))
        {
            sprintf (errmsg, "Error appending to metadata string (grid "
                "projection parameters start)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        for (i = 0; i < NPROJ_PARAM; i++)
            proj_parms[i] = 0.0;

        switch (gmeta->proj_info.proj_type)
        {
            case GCTP_GEO_PROJ:
                /* just use the already initialized zeros for the proj parms */
                break;
    
            case GCTP_UTM_PROJ:
                /* UTM handled in the if statement above */
                break;

            case GCTP_ALBERS_PROJ:
                proj_parms[2] =
                    deg_to_dms (gmeta->proj_info.standard_parallel1);
                proj_parms[3] =
                    deg_to_dms (gmeta->proj_info.standard_parallel2);
                proj_parms[4] = deg_to_dms (gmeta->proj_info.central_meridian);
                proj_parms[5] = deg_to_dms (gmeta->proj_info.origin_latitude);
                proj_parms[6] = gmeta->proj_info.false_easting;
                proj_parms[7] = gmeta->proj_info.false_northing;
                break;
    
            case GCTP_PS_PROJ:
                proj_parms[4] = deg_to_dms (gmeta->proj_info.longitude_pole);
                proj_parms[5] =
                    deg_to_dms (gmeta->proj_info.latitude_true_scale);
                proj_parms[6] = gmeta->proj_info.false_easting;
                proj_parms[7] = gmeta->proj_info.false_northing;
                break;
    
            case GCTP_SIN_PROJ:
                proj_parms[0] = gmeta->proj_info.sphere_radius;
                proj_parms[4] = deg_to_dms (gmeta->proj_info.central_meridian);
                proj_parms[6] = gmeta->proj_info.false_easting;
                proj_parms[7] = gmeta->proj_info.false_northing;
                break;
    
            default:
                sprintf (errmsg, "Unsupported projection type (%d).  GEO "
                    "projection code (%d) or UTM projection code (%d) or "
                    "ALBERS projection code (%d) or PS projection code (%d) or "
                    "SIN projection code (%d) expected.",
                    gmeta->proj_info.proj_type, GCTP_GEO_PROJ, GCTP_UTM_PROJ,
                    GCTP_ALBERS_PROJ, GCTP_PS_PROJ, GCTP_SIN_PROJ);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
        }

        for (i = 0; i < NPROJ_PARAM; i++)
        {
            if (i == NPROJ_PARAM-1)
                count = snprintf (cbuf, sizeof (cbuf), "%.6f)", proj_parms[i]);
            else
                count = snprintf (cbuf, sizeof (cbuf), "%.6f,", proj_parms[i]);
            if (count < 0 || count >= sizeof (cbuf))
            {
                sprintf (errmsg, "Overflow of cbuf string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string ("
                    "individual grid projection parameters)");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }

        count = snprintf (cbuf, sizeof (cbuf), "\n");
        if (count < 0 || count >= sizeof (cbuf))
        {
            sprintf (errmsg, "Overflow of cbuf string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (!append_meta (struct_meta, &meta_indx, cbuf))
        {
            sprintf (errmsg, "Error appending to metadata string (grid "
                "projection parameters end)");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
  
    switch (gmeta->proj_info.datum_type)
    {
        case (ESPA_WGS84):
            sphere_code = SPHERE_WGS84;
            strcpy (datum_str, "WGS84");
            break;
        case (ESPA_NAD83):
            sphere_code = SPHERE_GRS80;
            strcpy (datum_str, "NAD83");
            break;
        case (ESPA_NAD27):
            sphere_code = SPHERE_CLARKE_1866;
            strcpy (datum_str, "NAD27");
            break;
        case (ESPA_NODATUM):
            sphere_code = -999;
            strcpy (datum_str, "NoDatum");
            break;
    }
  
    /* Don't write the sphere code if this is the Geographic projection */
    if (gmeta->proj_info.proj_type != GCTP_GEO_PROJ)
    {
        if (gmeta->proj_info.datum_type != ESPA_NODATUM)
        {
            count = snprintf (cbuf, sizeof (cbuf),
                "\t\tSphereCode=%d\n", sphere_code);
            if (count < 0 || count >= sizeof (cbuf))
            {
                sprintf (errmsg, "Overflow of cbuf string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string (grid "
                    "information end)");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }
  
    count = snprintf (cbuf, sizeof (cbuf),
        "\t\tDatum=%s\n"
        "\t\tGridOrigin=HDFE_GD_UL\n", datum_str);
    if (count < 0 || count >= sizeof (cbuf))
    {
        sprintf (errmsg, "Overflow of cbuf string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (!append_meta (struct_meta, &meta_indx, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (grid information "
            "end)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Put SDS group */
    count = snprintf (cbuf, sizeof (cbuf),
        "\t\tGROUP=Dimension\n" 
        "\t\tEND_GROUP=Dimension\n"
        "\t\tGROUP=DataField\n");
    if (count < 0 || count >= sizeof (cbuf))
    {
        sprintf (errmsg, "Overflow of cbuf string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (!append_meta (struct_meta, &meta_indx, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (SDS group "
            "start)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Loop through the bands and write those that match the resolution of
       the first band */
    nfields = 0;
    for (isds = 0; isds < xml_metadata->nbands; isds++)
    {
        if ((xml_metadata->band[isds].pixel_size[0] ==
             xml_metadata->band[0].pixel_size[0]) &&
            (xml_metadata->band[isds].pixel_size[1] ==
             xml_metadata->band[0].pixel_size[1]))
        {
            processed[isds] = true;
            switch (xml_metadata->band[isds].data_type)
            {
                case ESPA_INT8: strcpy (dtype, "DFNT_INT8"); break;
                case ESPA_UINT8: strcpy (dtype, "DFNT_UINT8"); break;
                case ESPA_INT16: strcpy (dtype, "DFNT_INT16"); break;
                case ESPA_UINT16: strcpy (dtype, "DFNT_UINT16"); break;
                case ESPA_INT32: strcpy (dtype, "DFNT_INT32"); break;
                case ESPA_UINT32: strcpy (dtype, "DFNT_UINT32"); break;
                case ESPA_FLOAT32: strcpy (dtype, "DFNT_FLOAT32"); break;
                case ESPA_FLOAT64: strcpy (dtype, "DFNT_FLOAT64"); break;
            }

            count = snprintf (cbuf, sizeof (cbuf),
                "\t\t\tOBJECT=DataField_%d\n"
                "\t\t\t\tDataFieldName=\"%s\"\n"
                "\t\t\t\tDataType=%s\n"
                "\t\t\t\tDimList=(\"%s\",\"%s\")\n"
                "\t\t\tEND_OBJECT=DataField_%d\n",
                nfields+1, xml_metadata->band[isds].name, dtype,
                dim_names[0], dim_names[1], nfields+1);
            if (count < 0 || count >= sizeof (cbuf))
            {
                sprintf (errmsg, "Overflow of cbuf string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string "
                    "(SDS group)");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            nfields++;
        }
    }
  
    /* Close off the grid */
    count = snprintf (cbuf, sizeof (cbuf),
      "\t\tEND_GROUP=DataField\n" 
      "\t\tGROUP=MergedFields\n" 
      "\t\tEND_GROUP=MergedFields\n"
      "\tEND_GROUP=GRID_1\n");
    if (count < 0 || count >= sizeof (cbuf))
    {
        sprintf (errmsg, "Overflow of cbuf string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (!append_meta (struct_meta, &meta_indx, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (SDS group end)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Keep track of the number of grids processed and which band they were
       built from */
    ngrids = 1;
    grid[0] = 0;   /* first grid based on band 0 in the XML structure */

    /* Check to see if all the bands were processed in this grid (i.e. all
       were the same resolution).  If not, then loop through the bands and
       handle the remaining bands, putting separate resolutions in different
       grids. */
    done = true;
    for (isds = 0; isds < xml_metadata->nbands; isds++)
    {
        if (!processed[isds])
        {
            done = false;
            break;
        }
    }
    while (!done)
    {   /* Process the remaining resolutions in separate grids */
        for (isds = 0; isds < xml_metadata->nbands; isds++)
        {
            /* If this band was already processed, then go to the next one */
            if (processed[isds])
                continue;

            /* Build the GRID header */
            count = snprintf (cbuf, sizeof (cbuf),
                "\tGROUP=GRID_%d\n", ngrids+1);
            if (count < 0 || count >= sizeof (cbuf))
            {
                sprintf (errmsg, "Overflow of cbuf string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending grid header to the metadata "
                    "string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
  
            /* Get the projection name string */
            switch (gmeta->proj_info.proj_type)
            {
                case (GCTP_GEO_PROJ): strcpy (proj_str, "GEO"); break;
                case (GCTP_UTM_PROJ): strcpy (proj_str, "UTM"); break;
                case (GCTP_PS_PROJ): strcpy (proj_str, "PS"); break;
                case (GCTP_ALBERS_PROJ): strcpy (proj_str, "ALBERS"); break;
                case (GCTP_SIN_PROJ): strcpy (proj_str, "SIN"); break;
            }
  
            /* If the grid origin is center, then adjust for the resolution.
               The corners will be written for the UL of the corner. */
            if (!strcmp (gmeta->proj_info.grid_origin, "CENTER"))
            {
                ul_corner[0] = gmeta->proj_info.ul_corner[0] -
                    0.5 * xml_metadata->band[isds].pixel_size[0];
                ul_corner[1] = gmeta->proj_info.ul_corner[1] +
                    0.5 * xml_metadata->band[isds].pixel_size[1];
                lr_corner[0] = gmeta->proj_info.lr_corner[0] -
                    0.5 * xml_metadata->band[isds].pixel_size[0];
                lr_corner[1] = gmeta->proj_info.lr_corner[1] +
                    0.5 * xml_metadata->band[isds].pixel_size[1];
            }
            else
            {
                ul_corner[0] = gmeta->proj_info.ul_corner[0];
                ul_corner[1] = gmeta->proj_info.ul_corner[1];
                lr_corner[0] = gmeta->proj_info.lr_corner[0];
                lr_corner[1] = gmeta->proj_info.lr_corner[1];
            }

            /* Write the Grid information for the resolution of this band.
               Append the resolution to the grid name or the count of the grids
               if dealing with Geographic. */
            if (xml_metadata->global.proj_info.proj_type == GCTP_GEO)
                mycount = ngrids+1;
            else
                mycount = (int) xml_metadata->band[isds].pixel_size[0];

            count = snprintf (cbuf, sizeof (cbuf),
                "\t\tGridName=\"%s_%d\"\n" 
                "\t\tXDim=%d\n" 
                "\t\tYDim=%d\n" 
                "\t\tPixelSize=%g,%g\n",
                grid_name, mycount,
                xml_metadata->band[isds].nsamps,
                xml_metadata->band[isds].nlines,
                xml_metadata->band[isds].pixel_size[0],
                xml_metadata->band[isds].pixel_size[1]);
            if (count < 0 || count >= sizeof (cbuf))
            {
                sprintf (errmsg, "Overflow of cbuf string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string (grid "
                    "information start)");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
  
            /* Convert the UL and LR corners to DDDMMMSSS.SS if the projection
               is geographic, otherwise write the corners as-is. */
            if (gmeta->proj_info.proj_type == GCTP_GEO)
            {
                ul_corner[0] = deg_to_dms (ul_corner[0]);
                ul_corner[1] = deg_to_dms (ul_corner[1]);
                lr_corner[0] = deg_to_dms (lr_corner[0]);
                lr_corner[1] = deg_to_dms (lr_corner[1]);
        
                count = snprintf (cbuf, sizeof (cbuf),
                    "\t\tUpperLeftPointMtrs=(%.2f,%.2f)\n" 
                    "\t\tLowerRightMtrs=(%.2f,%.2f)\n" 
                    "\t\tProjection=GCTP_%s\n", 
                    ul_corner[0], ul_corner[1], lr_corner[0], lr_corner[1],
                    proj_str);
                if (count < 0 || count >= sizeof (cbuf))
                {
                    sprintf (errmsg, "Overflow of cbuf string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else
            {
                count = snprintf (cbuf, sizeof (cbuf),
                    "\t\tUpperLeftPointMtrs=(%.6f,%.6f)\n" 
                    "\t\tLowerRightMtrs=(%.6f,%.6f)\n" 
                    "\t\tProjection=GCTP_%s\n", 
                    ul_corner[0], ul_corner[1], lr_corner[0], lr_corner[1],
                    proj_str);
                if (count < 0 || count >= sizeof (cbuf))
                {
                    sprintf (errmsg, "Overflow of cbuf string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }

            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string (grid "
                    "information start)");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Write projection information */
            if (gmeta->proj_info.proj_type == GCTP_UTM_PROJ)
            {
                /* Write the UTM zone */
                count = snprintf (cbuf, sizeof (cbuf),
                    "\t\tZoneCode=%d\n", gmeta->proj_info.utm_zone);
                if (count < 0 || count >= sizeof (cbuf))
                {
                    sprintf (errmsg, "Overflow of cbuf string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                if (!append_meta (struct_meta, &meta_indx, cbuf))
                {
                    sprintf (errmsg, "Error appending to metadata string (zone "
                        "number)");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (gmeta->proj_info.proj_type != GCTP_GEO_PROJ)
            {  /* don't write projection parameters for Geographic */
                /* Write the projection parameters */
                count = snprintf (cbuf, sizeof (cbuf), "\t\tProjParams=(");
                if (count < 0 || count >= sizeof (cbuf))
                {
                    sprintf (errmsg, "Overflow of cbuf string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                if (!append_meta (struct_meta, &meta_indx, cbuf))
                {
                    sprintf (errmsg, "Error appending to metadata string (grid "
                        "projection parameters start)");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
  
                for (i = 0; i < NPROJ_PARAM; i++)
                    proj_parms[i] = 0.0;
        
                switch (gmeta->proj_info.proj_type)
                {
                    case GCTP_GEO_PROJ:
                        /* just use the already initialized zeros for the proj
                           parms */
                        break;
            
                    case GCTP_UTM_PROJ:
                        /* UTM handled in the if statement above */
                        break;
        
                    case GCTP_ALBERS_PROJ:
                        proj_parms[2] =
                            deg_to_dms (gmeta->proj_info.standard_parallel1);
                        proj_parms[3] =
                            deg_to_dms (gmeta->proj_info.standard_parallel2);
                        proj_parms[4] =
                            deg_to_dms (gmeta->proj_info.central_meridian);
                        proj_parms[5] =
                            deg_to_dms (gmeta->proj_info.origin_latitude);
                        proj_parms[6] = gmeta->proj_info.false_easting;
                        proj_parms[7] = gmeta->proj_info.false_northing;
                        break;
            
                    case GCTP_PS_PROJ:
                        proj_parms[4] =
                            deg_to_dms (gmeta->proj_info.longitude_pole);
                        proj_parms[5] =
                            deg_to_dms (gmeta->proj_info.latitude_true_scale);
                        proj_parms[6] = gmeta->proj_info.false_easting;
                        proj_parms[7] = gmeta->proj_info.false_northing;
                        break;
            
                    case GCTP_SIN_PROJ:
                        proj_parms[0] = gmeta->proj_info.sphere_radius;
                        proj_parms[4] =
                            deg_to_dms (gmeta->proj_info.central_meridian);
                        proj_parms[6] = gmeta->proj_info.false_easting;
                        proj_parms[7] = gmeta->proj_info.false_northing;
                        break;

                    default:
                        sprintf (errmsg, "Unsupported projection type (%d).  "
                            "GEO projection code (%d) or UTM projection code "
                            "(%d) or ALBERS projection code (%d) or PS "
                            "projection code (%d) or SIN projection code (%d) "
                            "expected.", gmeta->proj_info.proj_type,
                            GCTP_GEO_PROJ, GCTP_UTM_PROJ, GCTP_ALBERS_PROJ,
                            GCTP_PS_PROJ, GCTP_SIN_PROJ);
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                }

                for (i = 0; i < NPROJ_PARAM; i++)
                {
                    if (i == NPROJ_PARAM-1)
                        count = snprintf (cbuf, sizeof (cbuf), "%.6f)",
                            proj_parms[i]);
                    else
                        count = snprintf (cbuf, sizeof (cbuf), "%.6f,",
                            proj_parms[i]);
                    if (count < 0 || count >= sizeof (cbuf))
                    {
                        sprintf (errmsg, "Overflow of cbuf string");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }

                    if (!append_meta (struct_meta, &meta_indx, cbuf))
                    {
                        sprintf (errmsg, "Error appending to metadata string ("
                            "individual grid projection parameters)");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                }

                count = snprintf (cbuf, sizeof (cbuf), "\n");
                if (count < 0 || count >= sizeof (cbuf))
                {
                    sprintf (errmsg, "Overflow of cbuf string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }

                if (!append_meta (struct_meta, &meta_indx, cbuf))
                {
                    sprintf (errmsg, "Error appending to metadata string (grid "
                        "projection parameters end)");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
  
            switch (gmeta->proj_info.datum_type)
            {
                case (ESPA_WGS84):
                    sphere_code = SPHERE_WGS84;
                    strcpy (datum_str, "WGS84");
                    break;
                case (ESPA_NAD83):
                    sphere_code = SPHERE_GRS80;
                    strcpy (datum_str, "NAD83");
                    break;
                case (ESPA_NAD27):
                    sphere_code = SPHERE_CLARKE_1866;
                    strcpy (datum_str, "NAD27");
                    break;
                case (ESPA_NODATUM):
                    sphere_code = -999;
                    strcpy (datum_str, "NoDatum");
                    break;
            }
          
            /* Don't write the sphere code if this is the Geographic
               projection */
            if (gmeta->proj_info.proj_type != GCTP_GEO_PROJ)
            {
                if (gmeta->proj_info.datum_type != ESPA_NODATUM)
                {
                    count = snprintf (cbuf, sizeof (cbuf),
                        "\t\tSphereCode=%d\n", sphere_code);
                    if (count < 0 || count >= sizeof (cbuf))
                    {
                        sprintf (errmsg, "Overflow of cbuf string");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
        
                    if (!append_meta (struct_meta, &meta_indx, cbuf))
                    {
                        sprintf (errmsg, "Error appending to metadata string "
                            "(grid information end)");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                }
            }
        
            count = snprintf (cbuf, sizeof (cbuf),
                "\t\tDatum=%s\n"
                "\t\tGridOrigin=HDFE_GD_UL\n", datum_str);
            if (count < 0 || count >= sizeof (cbuf))
            {
                sprintf (errmsg, "Overflow of cbuf string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        
            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string (grid "
                    "information end)");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
  
            /* Put SDS group */
            count = snprintf (cbuf, sizeof (cbuf),
                "\t\tGROUP=Dimension\n" 
                "\t\tEND_GROUP=Dimension\n"
                "\t\tGROUP=DataField\n");
            if (count < 0 || count >= sizeof (cbuf))
            {
                sprintf (errmsg, "Overflow of cbuf string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string (SDS "
                    "group start)");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Loop through the bands and write those that match the current
               resolution */
            nfields = 0;
            for (i = 0; i < xml_metadata->nbands; i++)
            {
                if ((xml_metadata->band[i].pixel_size[0] ==
                     xml_metadata->band[isds].pixel_size[0]) &&
                    (xml_metadata->band[i].pixel_size[1] ==
                     xml_metadata->band[isds].pixel_size[1]))
                {
                    processed[i] = true;
                    switch (xml_metadata->band[i].data_type)
                    {
                        case ESPA_INT8: strcpy(dtype, "DFNT_INT8"); break;
                        case ESPA_UINT8: strcpy(dtype, "DFNT_UINT8"); break;
                        case ESPA_INT16: strcpy(dtype, "DFNT_INT16"); break;
                        case ESPA_UINT16: strcpy(dtype, "DFNT_UINT16"); break;
                        case ESPA_INT32: strcpy(dtype, "DFNT_INT32"); break;
                        case ESPA_UINT32: strcpy(dtype, "DFNT_UINT32"); break;
                        case ESPA_FLOAT32: strcpy(dtype, "DFNT_FLOAT32"); break;
                        case ESPA_FLOAT64: strcpy(dtype, "DFNT_FLOAT64"); break;
                    }

                    count = snprintf (cbuf, sizeof (cbuf),
                        "\t\t\tOBJECT=DataField_%d\n"
                        "\t\t\t\tDataFieldName=\"%s\"\n"
                        "\t\t\t\tDataType=%s\n"
                        "\t\t\t\tDimList=(\"%s\",\"%s\")\n"
                        "\t\t\tEND_OBJECT=DataField_%d\n",
                        nfields+1, xml_metadata->band[i].name, dtype,
                        dim_names[0], dim_names[1], nfields+1);
                    if (count < 0 || count >= sizeof (cbuf))
                    {
                        sprintf (errmsg, "Overflow of cbuf string");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                    if (!append_meta (struct_meta, &meta_indx, cbuf))
                    {
                        sprintf (errmsg, "Error appending to metadata string "
                            "(SDS group)");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                    nfields++;
                }
            }
  
            /* Close off the grid */
            count = snprintf (cbuf, sizeof (cbuf),
              "\t\tEND_GROUP=DataField\n" 
              "\t\tGROUP=MergedFields\n" 
              "\t\tEND_GROUP=MergedFields\n"
              "\tEND_GROUP=GRID_%d\n", ngrids+1);
            if (count < 0 || count >= sizeof (cbuf))
            {
                sprintf (errmsg, "Overflow of cbuf string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (!append_meta (struct_meta, &meta_indx, cbuf))
            {
                sprintf (errmsg, "Error appending to metadata string (SDS "
                    "group end)");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
  
            /* Keep track of the number of grids processed and which band they
               were built from */
            grid[ngrids++] = isds;
        }  /* for isds */

        /* Check to see if all the bands were processed now with this grid.
           If not, then loop through the bands again and handle the remaining
           bands, putting separate resolutions in different grids. */
        done = true;
        for (i = 0; i < xml_metadata->nbands; i++)
        {
            if (!processed[i])
            {
                done = false;
                break;
            }
        }
    }  /* end while !done */

    /* Put trailer */
    count = snprintf (cbuf, sizeof (cbuf),
        "END_GROUP=GridStructure\n"
        "GROUP=PointStructure\n"
        "END_GROUP=PointStructure\n"
        "END\n");
    if (count < 0 || count >= sizeof (cbuf))
    {
        sprintf (errmsg, "Overflow of cbuf string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (!append_meta (struct_meta, &meta_indx, cbuf))
    {
        sprintf (errmsg, "Error appending to metadata string (tail)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Write file attributes */
    hdf_file_id = SDstart ((char *) hdf_file, DFACC_RDWR);
    if (hdf_file_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error opening file for SD access: %s", hdf_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    
    attr.type = DFNT_FLOAT64;
    attr.nval = 1;
    attr.name = OUTPUT_ORIENTATION_ANGLE_HDF;
    dval = (double) gmeta->orientation_angle;
    if (put_attr_double (hdf_file_id, &attr, &dval) != SUCCESS)
    {
        sprintf (errmsg, "Error writing attribute (orientation angle)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    attr.type = DFNT_CHAR8;
    attr.nval = strlen (struct_meta);
    attr.name = OUTPUT_STRUCT_METADATA;
    if (put_attr_string (hdf_file_id, &attr, struct_meta) != SUCCESS)
    {
        sprintf (errmsg, "Error writing attribute (struct_meta)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    if (SDend (hdf_file_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error ending SD access");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Setup the HDF Vgroup */
    hdf_id = Hopen ((char *)hdf_file, DFACC_RDWR, 0);
    if (hdf_id == HDF_ERROR) 
    {
        sprintf (errmsg, "Error opening the HDF file for Vgroup access: %s",
            hdf_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Start the Vgroup access */
    if (Vstart (hdf_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error starting Vgroup access");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Loop through the Grids, define them, and then assign appropriate SDSs
       to the Data Fields */
    for (igrid = 0; igrid < ngrids; igrid++)
    {
        /* Create Vgroup for current Grid */
        vgroup_id[0] = Vattach (hdf_id, -1, "w");
        if (vgroup_id[0] == HDF_ERROR) 
        {
            sprintf (errmsg, "Error getting Grid Vgroup ID");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* If this is the first grid, then don't use the pixel size in the grid
           name.  Otherwise use the pixel size in the grid name.  If processing
           geographic then use the count of the grids instead of the pixel
           size. */
        if (igrid == 0 &&
            xml_metadata->global.proj_info.proj_type != GCTP_GEO)
            count = snprintf (temp_name, sizeof (temp_name), "%s", grid_name);
        else
        {
            if (xml_metadata->global.proj_info.proj_type == GCTP_GEO)
                mycount = igrid+1;
            else
                mycount = (int) xml_metadata->band[grid[igrid]].pixel_size[0];
            count = snprintf (temp_name, sizeof (temp_name), "%s_%d", grid_name,
                mycount);
        }

        if (count < 0 || count >= sizeof (temp_name))
        {
            sprintf (errmsg, "Overflow of temp_name string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }


        if (Vsetname (vgroup_id[0], temp_name) == HDF_ERROR) 
        {
            sprintf (errmsg, "Error setting Grid Vgroup name: %s", temp_name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vsetclass (vgroup_id[0], "GRID") == HDF_ERROR) 
        {
            sprintf (errmsg, "Error setting Grid Vgroup class to GRID");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        /* Create Data Fields Vgroup */
        vgroup_id[1] = Vattach (hdf_id, -1, "w");
        if (vgroup_id[1] == HDF_ERROR) 
        {
            sprintf (errmsg, "Error getting Data Fields Vgroup ID");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vsetname (vgroup_id[1], "Data Fields") == HDF_ERROR) 
        {
            sprintf (errmsg, "Error setting Data Fields Vgroup name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vsetclass (vgroup_id[1], "GRID Vgroup") == HDF_ERROR) 
        {
            sprintf (errmsg, "Error setting Data Fields Vgroup class");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vinsert (vgroup_id[0], vgroup_id[1]) == HDF_ERROR) 
        {
            sprintf (errmsg, "Error inserting Data Fields Vgroup");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        /* Create Attributes Vgroup */
        vgroup_id[2] = Vattach (hdf_id, -1, "w");
        if (vgroup_id[2] == HDF_ERROR) 
        {
            sprintf (errmsg, "Error getting attributes Vgroup ID");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vsetname (vgroup_id[2], "Grid Attributes") == HDF_ERROR) 
        {
            sprintf (errmsg, "Error setting attributes Vgroup name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vsetclass (vgroup_id[2], "GRID Vgroup") == HDF_ERROR) 
        {
            sprintf (errmsg, "Error setting attributes Vgroup class");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vinsert (vgroup_id[0], vgroup_id[2]) == HDF_ERROR) 
        {
            sprintf (errmsg, "Error inserting attributes Vgroup");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        /* Attach SDSs to Data Fields Vgroup */
        hdf_file_id = SDstart ((char *)hdf_file, DFACC_RDWR);
        if (hdf_file_id == HDF_ERROR) 
        {
            sprintf (errmsg, "Error opening output file for SD access");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        /* Loop through the bands and attach those with the same resolution
           as the current grid */
        for (isds = 0; isds < xml_metadata->nbands; isds++)
        {
            if ((xml_metadata->band[isds].pixel_size[0] !=
                 xml_metadata->band[grid[igrid]].pixel_size[0]) ||
                (xml_metadata->band[isds].pixel_size[1] !=
                 xml_metadata->band[grid[igrid]].pixel_size[1]))
                continue;

            sds_index = SDnametoindex (hdf_file_id, 
                xml_metadata->band[isds].name);
            if (sds_index == HDF_ERROR) 
            {
                sprintf (errmsg, "Error getting SDS index for SDS[%d]: %s",
                    isds, xml_metadata->band[isds].name);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            sds_id = SDselect (hdf_file_id, sds_index);
            if (sds_id == HDF_ERROR) 
            {
                sprintf (errmsg, "Error getting SDS ID");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (Vaddtagref (vgroup_id[1], DFTAG_NDG, SDidtoref(sds_id)) == 
                HDF_ERROR) 
            {
                sprintf (errmsg, "Error adding reference tag to SDS");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (SDendaccess (sds_id) == HDF_ERROR) 
            {
                sprintf (errmsg, "Error ending access to SDS");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
        
        if (SDend (hdf_file_id) == HDF_ERROR) 
        {
            sprintf (errmsg, "Error ending SD access");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
  
        /* Detach Vgroups */
        if (Vdetach (vgroup_id[0]) == HDF_ERROR) 
        {
            sprintf (errmsg, "Error detaching from Grid Vgroup");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vdetach (vgroup_id[1]) == HDF_ERROR) 
        {
            sprintf (errmsg, "Error detaching from Data Fields Vgroup");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (Vdetach (vgroup_id[2]) == HDF_ERROR) 
        {
            sprintf (errmsg, "Error detaching from Attributes Vgroup");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Close access */
    if (Vend (hdf_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error ending Vgroup access");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    if (Hclose (hdf_id) == HDF_ERROR) 
    {
        sprintf (errmsg, "Error ending HDF access");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    return (SUCCESS);
}

