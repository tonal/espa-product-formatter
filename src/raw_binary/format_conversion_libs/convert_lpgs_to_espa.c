/*****************************************************************************
FILE: convert_lpgs_to_espa.c
  
PURPOSE: Contains functions for reading LPGS input GeoTIFF products and
writing to ESPA raw binary format.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/30/2013   Gail Schmidt     Original development
6/17/2014    Gail Schmidt     Updated to support L8

NOTES:
  1. The XML metadata format written via this library follows the ESPA internal
     metadata format found in ESPA Raw Binary Format v1.0.doc.  The schema for
     the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/static/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/

#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include "convert_lpgs_to_espa.h"

/******************************************************************************
MODULE:  read_lpgs_mtl

PURPOSE: Read the LPGS MTL metadata file and populate the ESPA internal
metadata structure

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error reading the metadata file
SUCCESS         Successfully populated the ESPA metadata structure

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/30/2013   Gail Schmidt     Original development
2/28/2014    Gail Schmidt     Added support for the band category; band source
                              will be left blank for the L1 LPGS data
6/17/2014    Gail Schmidt     Updated for OLI_TIRS

NOTES:
1. The new MTL files contain the gain and bias coefficients for the TOA
   reflectance and brightness temp calculations.  These coefficients are
   parsed and written to our XML metadata file, if they exist.
2. When processing OLI_TIRS stack the 11 image bands first, then add the
   QA band to the list.
******************************************************************************/
int read_lpgs_mtl
(
    char *mtl_file,                  /* I: name of the MTL metadata file to
                                           be read */
    Espa_internal_meta_t *metadata,  /* I/O: input metadata structure to be
                                           populated from the MTL file */
    int *nlpgs_bands,                /* O: number of bands in LPGS product */
    char lpgs_bands[][STR_SIZE]      /* O: array containing the filenames of
                                           the LPGS bands */
)
{
    char FUNC_NAME[] = "read_lpgs_mtl";  /* function name */
    char errmsg[STR_SIZE];    /* error message */
    int i;                    /* looping variable */
    int count;                /* number of chars copied in snprintf */
    bool done_with_mtl;       /* are we done processing the MTL file? */
    bool gain_bias_available; /* are the TOA reflectance (brightness temp)
                                 gain/bias values available in the MTL file? */
    FILE *mtl_fptr=NULL;      /* file pointer to the MTL metadata file */
    Espa_global_meta_t *gmeta = &metadata->global;  /* pointer to the global
                                                       metadata structure */
    Espa_band_meta_t *bmeta;  /* pointer to the array of bands metadata */
    Espa_band_meta_t tmp_bmeta;    /* for temporary storage of the band-related
                                      metadata for reflective bands */
    Espa_band_meta_t tmp_bmeta_th; /* for temporary storage of the band-related
                                      metadata for thermal bands */
    Espa_band_meta_t tmp_bmeta_pan; /* for temporary storage of the band-
                                       related metadata for pan bands */
    Space_def_t geoloc_def;  /* geolocation space information */
    Geoloc_t *geoloc_map = NULL;  /* geolocation mapping information */
    Geo_bounds_t bounds;     /* image boundary for the scene */
    double ur_corner[2];     /* geographic UR lat, long */
    double ll_corner[2];     /* geographic LL lat, long */
    char *cptr = NULL;       /* pointer to the '_' in the band name */
    char scene_id[STR_SIZE]; /* LPGS scene ID */
    char band_fname[MAX_LPGS_BANDS][STR_SIZE];  /* filenames for each band */
    int band_min[MAX_LPGS_BANDS];  /* minimum value for each band */
    int band_max[MAX_LPGS_BANDS];  /* maximum value for each band */
    float band_gain[MAX_LPGS_BANDS]; /* gain values for TOA reflectance (or
                                        brightness temp) calculations */
    float band_bias[MAX_LPGS_BANDS]; /* bias values for TOA reflectance (or
                                        brightness temp) calculations */

    /* vars used in parameter parsing */
    char buffer[STR_SIZE] = "\0";          /* line buffer from MTL file */
    char *label = NULL;                    /* label value in the line */
    char *tokenptr = NULL;                 /* pointer to process each line */
    char *seperator = "=\" \t";            /* separator string */
    float fnum;                            /* temporary variable for floating
                                              point numbers */

    /* Open the metadata MTL file with read privelages */
    mtl_fptr = fopen (mtl_file, "r");
    if (mtl_fptr == NULL)
    {
        sprintf (errmsg, "Opening %s for read access.", mtl_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Process the MTL file line by line */
    gain_bias_available = false;
    done_with_mtl = false;
    while (fgets (buffer, STR_SIZE, mtl_fptr) != NULL)
    {
        /* If the last character is the end of line, then strip it off */
        if (buffer[strlen(buffer)-1] == '\n')
            buffer[strlen(buffer)-1] = '\0';

        /* Get string token */
        tokenptr = strtok (buffer, seperator);
        label = tokenptr;
 
        if (tokenptr != NULL)
        {
            tokenptr = strtok (NULL, seperator);

            /* Process each token; in some cases we are supporting both the
               old and the new LPGS metadata tags */
            if (!strcmp (label, "PROCESSING_SOFTWARE_VERSION") ||
                !strcmp (label, "PROCESSING_SOFTWARE"))
            {
                count = snprintf (tmp_bmeta.app_version,
                    sizeof (tmp_bmeta.app_version), "%s", tokenptr);
                if (count < 0 || count >= sizeof (tmp_bmeta.app_version))
                {
                    sprintf (errmsg, "Overflow of tmp_bmeta.app_version");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "DATA_TYPE") ||
                     !strcmp (label, "PRODUCT_TYPE"))
            {
                count = snprintf (tmp_bmeta.product, sizeof (tmp_bmeta.product),
                    "%s", tokenptr);
                if (count < 0 || count >= sizeof (tmp_bmeta.product))
                {
                    sprintf (errmsg, "Overflow of tmp_bmeta.product string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "SPACECRAFT_ID"))
            {
                if (strcmp (tokenptr, "LANDSAT_8") == 0 ||
                    strcmp (tokenptr, "Landsat8") == 0)
                    strcpy (gmeta->satellite, "LANDSAT_8");
                else if (strcmp (tokenptr, "LANDSAT_7") == 0 ||
                    strcmp (tokenptr, "Landsat7") == 0)
                    strcpy (gmeta->satellite, "LANDSAT_7");
                else if (strcmp (tokenptr, "LANDSAT_5") == 0 ||
                         strcmp (tokenptr, "Landsat5") == 0)
                    strcpy (gmeta->satellite, "LANDSAT_5");
                else if (strcmp (tokenptr, "LANDSAT_4") == 0 ||
                         strcmp (tokenptr, "Landsat4") == 0)
                    strcpy (gmeta->satellite, "LANDSAT_4");
                else
                {
                    sprintf (errmsg, "Unsupported satellite type: %s",
                        tokenptr);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "SENSOR_ID"))
            {
                count = snprintf (gmeta->instrument, sizeof (gmeta->instrument),
                    "%s", tokenptr);
                if (count < 0 || count >= sizeof (gmeta->instrument))
                {
                    sprintf (errmsg, "Overflow of gmeta->instrument string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "DATE_ACQUIRED") ||
                     !strcmp (label, "ACQUISITION_DATE"))
            {
                count = snprintf (gmeta->acquisition_date,
                    sizeof (gmeta->acquisition_date), "%s", tokenptr);
                if (count < 0 || count >= sizeof (gmeta->acquisition_date))
                {
                    sprintf (errmsg, "Overflow of gmeta->acquisition_date");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "SCENE_CENTER_TIME") ||
                     !strcmp (label, "SCENE_CENTER_SCAN_TIME"))
            {
                count = snprintf (gmeta->scene_center_time,
                    sizeof (gmeta->scene_center_time), "%s", tokenptr);
                if (count < 0 || count >= sizeof (gmeta->scene_center_time))
                {
                    sprintf (errmsg, "Overflow of gmeta->scene_center_time");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_DATE") ||
                     !strcmp (label, "PRODUCT_CREATION_TIME"))
            {
                count = snprintf (gmeta->level1_production_date,
                    sizeof (gmeta->level1_production_date), "%s", tokenptr);
                if (count < 0 ||
                    count >= sizeof (gmeta->level1_production_date))
                {
                    sprintf (errmsg,
                        "Overflow of gmeta->level1_production_date");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "SUN_ELEVATION"))
            {
                sscanf (tokenptr, "%f", &fnum);
                gmeta->solar_zenith = 90.0 - fnum;
            }
            else if (!strcmp (label, "SUN_AZIMUTH"))
                sscanf (tokenptr, "%f", &gmeta->solar_azimuth);
            else if (!strcmp (label, "WRS_PATH"))
                sscanf (tokenptr, "%d", &gmeta->wrs_path);
            else if (!strcmp (label, "WRS_ROW") ||
                     !strcmp (label, "STARTING_ROW"))
                sscanf (tokenptr, "%d", &gmeta->wrs_row);

            else if (!strcmp (label, "CORNER_UL_LAT_PRODUCT") ||
                     !strcmp (label, "PRODUCT_UL_CORNER_LAT"))
                sscanf (tokenptr, "%lf", &gmeta->ul_corner[0]);
            else if (!strcmp (label, "CORNER_UL_LON_PRODUCT") ||
                     !strcmp (label, "PRODUCT_UL_CORNER_LON"))
                sscanf (tokenptr, "%lf", &gmeta->ul_corner[1]);
            else if (!strcmp (label, "CORNER_LR_LAT_PRODUCT") ||
                     !strcmp (label, "PRODUCT_LR_CORNER_LAT"))
                sscanf (tokenptr, "%lf", &gmeta->lr_corner[0]);
            else if (!strcmp (label, "CORNER_LR_LON_PRODUCT") ||
                     !strcmp (label, "PRODUCT_LR_CORNER_LON"))
                sscanf (tokenptr, "%lf", &gmeta->lr_corner[1]);

            else if (!strcmp (label, "CORNER_UR_LAT_PRODUCT") ||
                     !strcmp (label, "PRODUCT_UR_CORNER_LAT"))
                sscanf (tokenptr, "%lf", &ur_corner[0]);
            else if (!strcmp (label, "CORNER_UR_LON_PRODUCT") ||
                     !strcmp (label, "PRODUCT_UR_CORNER_LON"))
                sscanf (tokenptr, "%lf", &ur_corner[1]);
            else if (!strcmp (label, "CORNER_LL_LAT_PRODUCT") ||
                     !strcmp (label, "PRODUCT_LL_CORNER_LAT"))
                sscanf (tokenptr, "%lf", &ll_corner[0]);
            else if (!strcmp (label, "CORNER_LL_LON_PRODUCT") ||
                     !strcmp (label, "PRODUCT_LL_CORNER_LON"))
                sscanf (tokenptr, "%lf", &ll_corner[1]);

            else if (!strcmp (label, "CORNER_UL_PROJECTION_X_PRODUCT") ||
                     !strcmp (label, "PRODUCT_UL_CORNER_MAPX") ||
                     !strcmp (label, "SCENE_UL_CORNER_MAPX"))
                sscanf (tokenptr, "%lf", &gmeta->proj_info.ul_corner[0]);
            else if (!strcmp (label, "CORNER_UL_PROJECTION_Y_PRODUCT") ||
                     !strcmp (label, "PRODUCT_UL_CORNER_MAPY") ||
                     !strcmp (label, "SCENE_UL_CORNER_MAPY"))
                sscanf (tokenptr, "%lf", &gmeta->proj_info.ul_corner[1]);
            else if (!strcmp (label, "CORNER_LR_PROJECTION_X_PRODUCT") ||
                     !strcmp (label, "PRODUCT_LR_CORNER_MAPX") ||
                     !strcmp (label, "SCENE_LR_CORNER_MAPX"))
                sscanf (tokenptr, "%lf", &gmeta->proj_info.lr_corner[0]);
            else if (!strcmp (label, "CORNER_LR_PROJECTION_Y_PRODUCT") ||
                     !strcmp (label, "PRODUCT_LR_CORNER_MAPY") ||
                     !strcmp (label, "SCENE_LR_CORNER_MAPY"))
                sscanf (tokenptr, "%lf", &gmeta->proj_info.lr_corner[1]);

            else if (!strcmp (label, "REFLECTIVE_SAMPLES") ||
                     !strcmp (label, "PRODUCT_SAMPLES_REF"))
                sscanf (tokenptr, "%d", &tmp_bmeta.nsamps);
            else if (!strcmp (label, "REFLECTIVE_LINES") ||
                     !strcmp (label, "PRODUCT_LINES_REF"))
                sscanf (tokenptr, "%d", &tmp_bmeta.nlines);
            else if (!strcmp (label, "THERMAL_SAMPLES") ||
                     !strcmp (label, "PRODUCT_SAMPLES_THM"))
                sscanf (tokenptr, "%d", &tmp_bmeta_th.nsamps);
            else if (!strcmp (label, "THERMAL_LINES") ||
                     !strcmp (label, "PRODUCT_LINES_THM"))
                sscanf (tokenptr, "%d", &tmp_bmeta_th.nlines);
            else if (!strcmp (label, "PANCHROMATIC_SAMPLES") ||
                     !strcmp (label, "PRODUCT_SAMPLES_PAN"))
                sscanf (tokenptr, "%d", &tmp_bmeta_pan.nsamps);
            else if (!strcmp (label, "PANCHROMATIC_LINES") ||
                     !strcmp (label, "PRODUCT_LINES_PAN"))
                sscanf (tokenptr, "%d", &tmp_bmeta_pan.nlines);

            else if (!strcmp (label, "MAP_PROJECTION"))
            {
                if (!strcmp (tokenptr, "UTM"))
                    gmeta->proj_info.proj_type = GCTP_UTM_PROJ;
                else if (!strcmp (tokenptr, "PS"))
                    gmeta->proj_info.proj_type = GCTP_PS_PROJ;
                else
                {
                    sprintf (errmsg, "Unsupported projection type: %s. "
                        "Only UTM and PS are supported for LPGS.", tokenptr);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "DATUM") ||
                     !strcmp (label, "REFERENCE_DATUM"))
            {
                if (!strcmp (tokenptr, "WGS84"))
                    gmeta->proj_info.datum_type = ESPA_WGS84;
                else
                {
                    sprintf (errmsg, "Unexpected datum type: %s", tokenptr);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "GRID_CELL_SIZE_REFLECTIVE") ||
                     !strcmp (label, "GRID_CELL_SIZE_REF"))
            {
                sscanf (tokenptr, "%lf", &tmp_bmeta.pixel_size[0]);
                tmp_bmeta.pixel_size[1] = tmp_bmeta.pixel_size[0];
            }
            else if (!strcmp (label, "GRID_CELL_SIZE_THERMAL") ||
                     !strcmp (label, "GRID_CELL_SIZE_THM"))
            {
                sscanf (tokenptr, "%lf", &tmp_bmeta_th.pixel_size[0]);
                tmp_bmeta_th.pixel_size[1] = tmp_bmeta_th.pixel_size[0];
            }
            else if (!strcmp (label, "GRID_CELL_SIZE_PANCHROMATIC") ||
                     !strcmp (label, "GRID_CELL_SIZE_PAN"))
            {
                sscanf (tokenptr, "%lf", &tmp_bmeta_pan.pixel_size[0]);
                tmp_bmeta_pan.pixel_size[1] = tmp_bmeta_pan.pixel_size[0];
            }
            else if (!strcmp (label, "UTM_ZONE") ||
                     !strcmp (label, "ZONE_NUMBER"))
                sscanf (tokenptr, "%d", &gmeta->proj_info.utm_zone);
            else if (!strcmp (label, "VERTICAL_LON_FROM_POLE") ||
                     !strcmp (label, "VERTICAL_LONGITUDE_FROM_POLE"))
                sscanf (tokenptr, "%lf", &gmeta->proj_info.longitude_pole);
            else if (!strcmp (label, "TRUE_SCALE_LAT") ||
                     !strcmp (label, "LATITUDE_OF_TRUE_SCALE"))
                sscanf (tokenptr, "%lf", &gmeta->proj_info.latitude_true_scale);
            else if (!strcmp (label, "FALSE_EASTING"))
                sscanf (tokenptr, "%lf", &gmeta->proj_info.false_easting);
            else if (!strcmp (label, "FALSE_NORTHING"))
                sscanf (tokenptr, "%lf", &gmeta->proj_info.false_northing);

            else if (!strcmp (label, "END"))
            {
                done_with_mtl = true;
                break;
            }

            /* Read the band names */
            else if (!strcmp (label, "FILE_NAME_BAND_1") ||
                     !strcmp (label, "BAND1_FILE_NAME"))
            {
                count = snprintf (band_fname[0], sizeof (band_fname[0]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[0]))
                {
                    sprintf (errmsg, "Overflow of band_fname[0] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_2") ||
                     !strcmp (label, "BAND2_FILE_NAME"))
            {
                count = snprintf (band_fname[1], sizeof (band_fname[1]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[1]))
                {
                    sprintf (errmsg, "Overflow of band_fname[1] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_3") ||
                     !strcmp (label, "BAND3_FILE_NAME"))
            {
                count = snprintf (band_fname[2], sizeof (band_fname[2]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[2]))
                {
                    sprintf (errmsg, "Overflow of band_fname[2] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_4") ||
                     !strcmp (label, "BAND4_FILE_NAME"))
            {
                count = snprintf (band_fname[3], sizeof (band_fname[3]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[3]))
                {
                    sprintf (errmsg, "Overflow of band_fname[3] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_5") ||
                     !strcmp (label, "BAND5_FILE_NAME"))
            {
                count = snprintf (band_fname[4], sizeof (band_fname[4]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[4]))
                {
                    sprintf (errmsg, "Overflow of band_fname[4] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_6") ||
                     !strcmp (label, "BAND6_FILE_NAME"))
            {
                count = snprintf (band_fname[5], sizeof (band_fname[5]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[5]))
                {
                    sprintf (errmsg, "Overflow of band_fname[5] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_7") ||
                     !strcmp (label, "BAND7_FILE_NAME"))
            {
                if (!strcmp (gmeta->instrument, "TM") ||
                    !strcmp (gmeta->instrument, "OLI_TIRS"))
                {  /* only a single band 6 so count is normal */
                    count = snprintf (band_fname[6], sizeof (band_fname[6]),
                        "%s", tokenptr);
                    if (count < 0 || count >= sizeof (band_fname[6]))
                    {
                        sprintf (errmsg, "Overflow of band_fname[6] string");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                }
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                {  /* band 6L and 6H throw off the count for ETM+ */
                    count = snprintf (band_fname[7], sizeof (band_fname[7]),
                        "%s", tokenptr);
                    if (count < 0 || count >= sizeof (band_fname[7]))
                    {
                        sprintf (errmsg, "Overflow of band_fname[7] string");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_8") ||
                     !strcmp (label, "BAND8_FILE_NAME"))
            {
                if (!strcmp (gmeta->instrument, "OLI_TIRS"))
                {  /* only a single band 6 so count is normal */
                    count = snprintf (band_fname[7], sizeof (band_fname[7]),
                        "%s", tokenptr);
                    if (count < 0 || count >= sizeof (band_fname[7]))
                    {
                        sprintf (errmsg, "Overflow of band_fname[7] string");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                }
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                {  /* band 6L and 6H throw off the count for ETM+ */
                    count = snprintf (band_fname[8], sizeof (band_fname[8]),
                        "%s", tokenptr);
                    if (count < 0 || count >= sizeof (band_fname[8]))
                    {
                        sprintf (errmsg, "Overflow of band_fname[8] string");
                        error_handler (true, FUNC_NAME, errmsg);
                        return (ERROR);
                    }
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_6_VCID_1") ||
                     !strcmp (label, "BAND61_FILE_NAME"))
            {
                count = snprintf (band_fname[5], sizeof (band_fname[5]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[5]))
                {
                    sprintf (errmsg, "Overflow of band_fname[5] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_6_VCID_2") ||
                     !strcmp (label, "BAND62_FILE_NAME"))
            {
                count = snprintf (band_fname[6], sizeof (band_fname[6]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[6]))
                {
                    sprintf (errmsg, "Overflow of band_fname[6] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_9") ||
                     !strcmp (label, "BAND9_FILE_NAME"))
            {
                count = snprintf (band_fname[8], sizeof (band_fname[8]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[8]))
                {
                    sprintf (errmsg, "Overflow of band_fname[8] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_10") ||
                     !strcmp (label, "BAND10_FILE_NAME"))
            {
                count = snprintf (band_fname[9], sizeof (band_fname[9]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[9]))
                {
                    sprintf (errmsg, "Overflow of band_fname[9] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_11") ||
                     !strcmp (label, "BAND11_FILE_NAME"))
            {
                count = snprintf (band_fname[10], sizeof (band_fname[10]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[10]))
                {
                    sprintf (errmsg, "Overflow of band_fname[10] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strcmp (label, "FILE_NAME_BAND_QUALITY"))
            {
                count = snprintf (band_fname[11], sizeof (band_fname[11]), "%s",
                    tokenptr);
                if (count < 0 || count >= sizeof (band_fname[11]))
                {
                    sprintf (errmsg, "Overflow of band_fname[11] string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }

            /* Read the min pixel values */
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_1") ||
                     !strcmp (label, "QCALMIN_BAND1"))
                sscanf (tokenptr, "%d", &band_min[0]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_2") ||
                     !strcmp (label, "QCALMIN_BAND2"))
                sscanf (tokenptr, "%d", &band_min[1]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_3") ||
                     !strcmp (label, "QCALMIN_BAND3"))
                sscanf (tokenptr, "%d", &band_min[2]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_4") ||
                     !strcmp (label, "QCALMIN_BAND4"))
                sscanf (tokenptr, "%d", &band_min[3]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_5") ||
                     !strcmp (label, "QCALMIN_BAND5"))
                sscanf (tokenptr, "%d", &band_min[4]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_6") ||
                     !strcmp (label, "QCALMIN_BAND6"))
                sscanf (tokenptr, "%d", &band_min[5]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_7") ||
                     !strcmp (label, "QCALMIN_BAND7"))
            {
                if (!strcmp (gmeta->instrument, "TM") ||
                    !strcmp (gmeta->instrument, "OLI_TIRS"))
                    sscanf (tokenptr, "%d", &band_min[6]);
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                    sscanf (tokenptr, "%d", &band_min[7]);
            }
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_8") ||
                     !strcmp (label, "QCALMIN_BAND8"))
            {
                if (!strcmp (gmeta->instrument, "OLI_TIRS"))
                    sscanf (tokenptr, "%d", &band_min[7]);
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                    sscanf (tokenptr, "%d", &band_min[8]);
            }
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_6_VCID_1") ||
                     !strcmp (label, "QCALMIN_BAND61"))
                sscanf (tokenptr, "%d", &band_min[5]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_6_VCID_2") ||
                     !strcmp (label, "QCALMIN_BAND62"))
                sscanf (tokenptr, "%d", &band_min[6]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_9"))
                sscanf (tokenptr, "%d", &band_min[8]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_10"))
                sscanf (tokenptr, "%d", &band_min[9]);
            else if (!strcmp (label, "QUANTIZE_CAL_MIN_BAND_11"))
                sscanf (tokenptr, "%d", &band_min[10]);

            /* Read the max pixel values */
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_1") ||
                     !strcmp (label, "QCALMAX_BAND1"))
                sscanf (tokenptr, "%d", &band_max[0]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_2") ||
                     !strcmp (label, "QCALMAX_BAND2"))
                sscanf (tokenptr, "%d", &band_max[1]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_3") ||
                     !strcmp (label, "QCALMAX_BAND3"))
                sscanf (tokenptr, "%d", &band_max[2]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_4") ||
                     !strcmp (label, "QCALMAX_BAND4"))
                sscanf (tokenptr, "%d", &band_max[3]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_5") ||
                     !strcmp (label, "QCALMAX_BAND5"))
                sscanf (tokenptr, "%d", &band_max[4]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_6") ||
                     !strcmp (label, "QCALMAX_BAND6"))
                sscanf (tokenptr, "%d", &band_max[5]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_7") ||
                     !strcmp (label, "QCALMAX_BAND7"))
            {
                if (!strcmp (gmeta->instrument, "TM") ||
                    !strcmp (gmeta->instrument, "OLI_TIRS"))
                    sscanf (tokenptr, "%d", &band_max[6]);
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                    sscanf (tokenptr, "%d", &band_max[7]);
            }
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_8") ||
                     !strcmp (label, "QCALMAX_BAND8"))
            {
                if (!strcmp (gmeta->instrument, "OLI_TIRS"))
                    sscanf (tokenptr, "%d", &band_max[7]);
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                    sscanf (tokenptr, "%d", &band_max[8]);
            }
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_6_VCID_1") ||
                     !strcmp (label, "QCALMAX_BAND61"))
                sscanf (tokenptr, "%d", &band_max[5]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_6_VCID_2") ||
                     !strcmp (label, "QCALMAX_BAND62"))
                sscanf (tokenptr, "%d", &band_max[6]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_9"))
                sscanf (tokenptr, "%d", &band_max[8]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_10"))
                sscanf (tokenptr, "%d", &band_max[9]);
            else if (!strcmp (label, "QUANTIZE_CAL_MAX_BAND_11"))
                sscanf (tokenptr, "%d", &band_max[10]);

            /* Read the reflectance gains */
            else if (!strcmp (label, "RADIANCE_MULT_BAND_1"))
            {
                sscanf (tokenptr, "%f", &band_gain[0]);

                /* Assume that if gain value for band 1 is available, then
                   the gain and bias values for all bands will be available */
                gain_bias_available = true;
            }
            else if (!strcmp (label, "RADIANCE_MULT_BAND_2"))
                sscanf (tokenptr, "%f", &band_gain[1]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_3"))
                sscanf (tokenptr, "%f", &band_gain[2]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_4"))
                sscanf (tokenptr, "%f", &band_gain[3]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_5"))
                sscanf (tokenptr, "%f", &band_gain[4]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_6"))
                sscanf (tokenptr, "%f", &band_gain[5]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_7"))
            {
                if (!strcmp (gmeta->instrument, "TM") ||
                    !strcmp (gmeta->instrument, "OLI_TIRS"))
                    sscanf (tokenptr, "%f", &band_gain[6]);
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                    sscanf (tokenptr, "%f", &band_gain[7]);
            }
            else if (!strcmp (label, "RADIANCE_MULT_BAND_8"))
            {
                if (!strcmp (gmeta->instrument, "OLI_TIRS"))
                    sscanf (tokenptr, "%f", &band_gain[7]);
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                    sscanf (tokenptr, "%f", &band_gain[8]);
            }
            else if (!strcmp (label, "RADIANCE_MULT_BAND_6_VCID_1"))
                sscanf (tokenptr, "%f", &band_gain[5]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_6_VCID_2"))
                sscanf (tokenptr, "%f", &band_gain[6]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_9"))
                sscanf (tokenptr, "%f", &band_gain[8]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_10"))
                sscanf (tokenptr, "%f", &band_gain[9]);
            else if (!strcmp (label, "RADIANCE_MULT_BAND_11"))
                sscanf (tokenptr, "%f", &band_gain[10]);

            /* Read the reflectance biases */
            else if (!strcmp (label, "RADIANCE_ADD_BAND_1"))
                sscanf (tokenptr, "%f", &band_bias[0]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_2"))
                sscanf (tokenptr, "%f", &band_bias[1]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_3"))
                sscanf (tokenptr, "%f", &band_bias[2]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_4"))
                sscanf (tokenptr, "%f", &band_bias[3]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_5"))
                sscanf (tokenptr, "%f", &band_bias[4]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_6"))
                sscanf (tokenptr, "%f", &band_bias[5]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_7"))
            {
                if (!strcmp (gmeta->instrument, "TM") ||
                    !strcmp (gmeta->instrument, "OLI_TIRS"))
                    sscanf (tokenptr, "%f", &band_bias[6]);
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                    sscanf (tokenptr, "%f", &band_bias[7]);
            }
            else if (!strcmp (label, "RADIANCE_ADD_BAND_8"))
            {
                if (!strcmp (gmeta->instrument, "OLI_TIRS"))
                    sscanf (tokenptr, "%f", &band_bias[7]);
                else if (!strncmp (gmeta->instrument, "ETM", 3))
                    sscanf (tokenptr, "%f", &band_bias[8]);
            }
            else if (!strcmp (label, "RADIANCE_ADD_BAND_6_VCID_1"))
                sscanf (tokenptr, "%f", &band_bias[5]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_6_VCID_2"))
                sscanf (tokenptr, "%f", &band_bias[6]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_9"))
                sscanf (tokenptr, "%f", &band_bias[8]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_10"))
                sscanf (tokenptr, "%f", &band_bias[9]);
            else if (!strcmp (label, "RADIANCE_ADD_BAND_11"))
                sscanf (tokenptr, "%f", &band_bias[10]);
        } /* end if tokenptr */

        /* If we are done */
        if (done_with_mtl)
            break;
    }  /* end while fgets */

    /* Set defaults that aren't in the MTL file */
    gmeta->wrs_system = 2;
    gmeta->orientation_angle = 0.0;
    strcpy (gmeta->data_provider, "USGS/EROS");
    strcpy (gmeta->solar_units, "degrees");

    count = snprintf (gmeta->lpgs_metadata_file,
        sizeof (gmeta->lpgs_metadata_file), "%s", mtl_file);
    if (count < 0 || count >= sizeof (gmeta->lpgs_metadata_file))
    {
        sprintf (errmsg, "Overflow of gmeta->lpgs_metadata_file string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (gmeta->proj_info.units, sizeof (gmeta->proj_info.units),
        "meters");
    if (count < 0 || count >= sizeof (gmeta->proj_info.units))
    {
        sprintf (errmsg, "Overflow of gmeta->proj_info.units string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* UL and LR corner projection coords in the MTL file are for the center
       of the pixel.  Given there are different resolution bands, leave the
       corners as the center of the pixel. */
    strcpy (gmeta->proj_info.grid_origin, "CENTER");

    /* Set up the number of total bands */
    if (!strcmp (gmeta->instrument, "TM"))
        metadata->nbands = 7;
    else if (!strncmp (gmeta->instrument, "ETM", 3))
        metadata->nbands = 9;
    else if (!strcmp (gmeta->instrument, "OLI_TIRS"))
        metadata->nbands = 12;  /* 11 image plus QA band */
    else
    {
        sprintf (errmsg, "Unsupported instrument type: %s. Only TM, ETM+, and "
            "OLI_TIRS are currently supported.", gmeta->instrument);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (allocate_band_metadata (metadata, metadata->nbands) != SUCCESS)
    {   /* Error messages already printed */
        return (ERROR);
    }
    bmeta = metadata->band;

    /* Strip the scene ID from the LPGS band name */
    count = snprintf (scene_id, sizeof (scene_id), "%s", band_fname[0]);
    if (count < 0 || count >= sizeof (scene_id))
    {
        sprintf (errmsg, "Overflow of scene_id string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    cptr = strrchr (scene_id, '_');
    if (cptr == NULL)
    {
        sprintf (errmsg, "Unsuspected format for the filename.  Expected "
            "{scene_id}_Bx.* however no '_' was found in the filename: %s.",
            scene_id);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    *cptr = '\0';

    /* Fill in the band-related metadata for each of the bands */
    *nlpgs_bands = metadata->nbands;
    for (i = 0; i < metadata->nbands; i++)
    {
        /* Handle the general metadata for each band */
        count = snprintf (lpgs_bands[i], sizeof (lpgs_bands[i]), "%s",
            band_fname[i]);
        if (count < 0 || count >= sizeof (lpgs_bands[i]))
        {
            sprintf (errmsg, "Overflow of lpgs_bands[i] string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (bmeta[i].product, sizeof (bmeta[i].product), "%s",
            tmp_bmeta.product);
        if (count < 0 || count >= sizeof (bmeta[i].product))
        {
            sprintf (errmsg, "Overflow of bmeta[i].product string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        if (!strcmp (gmeta->instrument, "OLI_TIRS") &&
            i == 11 /* band quality */)
        {
            /* This is a QA band and needs the class values set up */
            strcpy (bmeta[i].category, "qa");
        }
        else
        {
            strcpy (bmeta[i].category, "image");
        }

        count = snprintf (bmeta[i].app_version, sizeof (bmeta[i].app_version),
            "%s", tmp_bmeta.app_version);
        if (count < 0 || count >= sizeof (bmeta[i].app_version))
        {
            sprintf (errmsg, "Overflow of bmeta[i].app_version string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        bmeta[i].valid_range[0] = band_min[i];
        bmeta[i].valid_range[1] = band_max[i];

        if (gain_bias_available)
        {
            bmeta[i].toa_gain = band_gain[i];
            bmeta[i].toa_bias = band_bias[i];
        }

        count = snprintf (bmeta[i].data_units, sizeof (bmeta[i].data_units),
            "digital numbers");
        if (count < 0 || count >= sizeof (bmeta[i].data_units))
        {
            sprintf (errmsg, "Overflow of bmeta[i].data_units string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (bmeta[i].pixel_units, sizeof (bmeta[i].pixel_units),
            "meters");
        if (count < 0 || count >= sizeof (bmeta[i].pixel_units))
        {
            sprintf (errmsg, "Overflow of bmeta[i].pixel_units string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (bmeta[i].production_date,
            sizeof (bmeta[i].production_date), "%s",
            gmeta->level1_production_date);
        if (count < 0 || count >= sizeof (bmeta[i].production_date))
        {
            sprintf (errmsg, "Overflow of bmeta[i].production_date string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (!strcmp (gmeta->instrument, "TM"))
        {
            bmeta[i].data_type = ESPA_UINT8;
            bmeta[i].fill_value = 0;
            if (!strcmp (gmeta->satellite, "LANDSAT_4"))
                strcpy (bmeta[i].short_name, "LT4DN");
            else if (!strcmp (gmeta->satellite, "LANDSAT_5"))
                strcpy (bmeta[i].short_name, "LT5DN");
        }
        else if (!strncmp (gmeta->instrument, "ETM", 3))
        {
            bmeta[i].data_type = ESPA_UINT8;
            bmeta[i].fill_value = 0;
            strcpy (bmeta[i].short_name, "LE7DN");
        }
        else if (!strcmp (gmeta->instrument, "OLI_TIRS"))
        {
            bmeta[i].data_type = ESPA_UINT16;
            bmeta[i].fill_value = 0;
            strcpy (bmeta[i].short_name, "LC8DN");
        }

        /* Handle the ETM+ thermal bands */
        if ((!strncmp (gmeta->instrument, "ETM", 3) &&
            ((i == 5 /* band 61 */) || (i == 6 /* band 62 */))) ||
            (!strcmp (gmeta->instrument, "TM") &&
            (i == 5 /* band 6 */)) ||
            (!strcmp (gmeta->instrument, "OLI_TIRS") &&
            ((i == 9 /* band 10 */) || (i == 10 /* band 11 */))))
        {
            if (!strncmp (gmeta->instrument, "ETM", 3) &&
               (i == 5 /* band 61 */))
            {
                strcpy (bmeta[i].name, "band61");
                strcpy (bmeta[i].long_name, "band 61 digital numbers");
                count = snprintf (bmeta[i].file_name,
                    sizeof (bmeta[i].file_name), "%s_B61.img", scene_id);
                if (count < 0 || count >= sizeof (bmeta[i].file_name))
                {
                    sprintf (errmsg, "Overflow of bmeta[i].file_name");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else if (!strncmp (gmeta->instrument, "ETM", 3) &&
                (i == 6 /* band 62 */))
            {
                strcpy (bmeta[i].name, "band62");
                strcpy (bmeta[i].long_name, "band 62 digital numbers");
                count = snprintf (bmeta[i].file_name,
                    sizeof (bmeta[i].file_name), "%s_B62.img", scene_id);
                if (count < 0 || count >= sizeof (bmeta[i].file_name))
                {
                    sprintf (errmsg, "Overflow of bmeta[i].file_name");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else
            {
                sprintf (bmeta[i].name, "band%d", i+1);
                sprintf (bmeta[i].long_name, "band %d digital numbers", i+1);
                count = snprintf (bmeta[i].file_name,
                    sizeof (bmeta[i].file_name), "%s_B%d.img", scene_id, i+1);
                if (count < 0 || count >= sizeof (bmeta[i].file_name))
                {
                    sprintf (errmsg, "Overflow of bmeta[i].file_name string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
           
            bmeta[i].nlines = tmp_bmeta_th.nlines;
            bmeta[i].nsamps = tmp_bmeta_th.nsamps;
            bmeta[i].pixel_size[0] = tmp_bmeta_th.pixel_size[0];
            bmeta[i].pixel_size[1] = tmp_bmeta_th.pixel_size[1];
        }

        /* Handle the pan bands */
        else if ((!strncmp (gmeta->instrument, "ETM", 3) &&
            (i == 8 /* band 8 */)) ||
            (!strcmp (gmeta->instrument, "OLI_TIRS") &&
            ((i == 7 /* band 8 */))))
        {
            strcpy (bmeta[i].name, "band8");
            strcpy (bmeta[i].long_name, "band 8 digital numbers");
            count = snprintf (bmeta[i].file_name, sizeof (bmeta[i].file_name),
                "%s_B8.img", scene_id);
            if (count < 0 || count >= sizeof (bmeta[i].file_name))
            {
                sprintf (errmsg, "Overflow of bmeta[i].file_name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            bmeta[i].nlines = tmp_bmeta_pan.nlines;
            bmeta[i].nsamps = tmp_bmeta_pan.nsamps;
            bmeta[i].pixel_size[0] = tmp_bmeta_pan.pixel_size[0];
            bmeta[i].pixel_size[1] = tmp_bmeta_pan.pixel_size[1];
        }

        /* Handle the OLI_TIRS quality band */
        else if (!strcmp (gmeta->instrument, "OLI_TIRS") &&
            (i == 11 /* quality band */))
        {
            strcpy (bmeta[i].name, "qa");
            strcpy (bmeta[i].long_name, "band quality");
            count = snprintf (bmeta[i].file_name, sizeof (bmeta[i].file_name),
                "%s_BQA.img", scene_id);
            if (count < 0 || count >= sizeof (bmeta[i].file_name))
            {
                sprintf (errmsg, "Overflow of bmeta[i].file_name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            bmeta[i].nlines = tmp_bmeta.nlines;
            bmeta[i].nsamps = tmp_bmeta.nsamps;
            bmeta[i].pixel_size[0] = tmp_bmeta.pixel_size[0];
            bmeta[i].pixel_size[1] = tmp_bmeta.pixel_size[1];
        }

        /* Handle the TM/OLI thermal and all reflective bands */
        else
        {
            if (!strncmp (gmeta->instrument, "ETM", 3) && i > 6)
            {
                strcpy (bmeta[i].name, "band7");
                strcpy (bmeta[i].long_name, "band 7 digital numbers");
                count = snprintf (bmeta[i].file_name,
                    sizeof (bmeta[i].file_name), "%s_B7.img", scene_id);
                if (count < 0 || count >= sizeof (bmeta[i].file_name))
                {
                    sprintf (errmsg, "Overflow of bmeta[i].file_name string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            else  /* bands 1-5 */
            {
                sprintf (bmeta[i].name, "band%d", i+1);
                sprintf (bmeta[i].long_name, "band %d digital numbers", i+1);
                count = snprintf (bmeta[i].file_name,
                    sizeof (bmeta[i].file_name), "%s_B%d.img", scene_id, i+1);
                if (count < 0 || count >= sizeof (bmeta[i].file_name))
                {
                    sprintf (errmsg, "Overflow of bmeta[i].file_name string");
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
                }
            }
            bmeta[i].nlines = tmp_bmeta.nlines;
            bmeta[i].nsamps = tmp_bmeta.nsamps;
            bmeta[i].pixel_size[0] = tmp_bmeta.pixel_size[0];
            bmeta[i].pixel_size[1] = tmp_bmeta.pixel_size[1];
        }

        /* If this is the OLI_TIRS QA band, then overwrite some things for the
           QA band itself */
        if (!strcmp (gmeta->instrument, "OLI_TIRS") &&
            i == 11 /* band quality */)
        {
            count = snprintf (bmeta[i].data_units, sizeof (bmeta[i].data_units),
                "quality/feature classification");
            if (count < 0 || count >= sizeof (bmeta[i].data_units))
            {
                sprintf (errmsg, "Overflow of bmeta[i].data_units string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            bmeta[i].valid_range[0] = 0;
            bmeta[i].valid_range[1] = 65535;
            bmeta[i].toa_gain = ESPA_INT_META_FILL;
            bmeta[i].toa_bias = ESPA_INT_META_FILL;

            if (allocate_bitmap_metadata (&bmeta[i], 16) != SUCCESS)
            {
                sprintf (errmsg, "Allocating 16 bits for the bitmap");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            strcpy (bmeta[i].bitmap_description[0],
                "Data Fill Flag (0 = valid data, 1 = invalid data)");
            strcpy (bmeta[i].bitmap_description[1],
                "Dropped Frame (0 = not a dropped frame , 1 = dropped frame)");
            strcpy (bmeta[i].bitmap_description[2],
                "Terrain Occlusion (0 = not terrain occluded, "
                "1 = terrain occluded)");
            strcpy (bmeta[i].bitmap_description[3], "Reserved");
            strcpy (bmeta[i].bitmap_description[4], "Water Confidence");
            strcpy (bmeta[i].bitmap_description[5], "Water Confidence");
            strcpy (bmeta[i].bitmap_description[6],
                "Reserved for cloud shadow");
            strcpy (bmeta[i].bitmap_description[7],
                "Reserved for cloud shadow");
            strcpy (bmeta[i].bitmap_description[8], "Vegetation Confidence");
            strcpy (bmeta[i].bitmap_description[9], "Vegetation Confidence");
            strcpy (bmeta[i].bitmap_description[10], "Snow/Ice Confidence");
            strcpy (bmeta[i].bitmap_description[11], "Snow/Ice Confidence");
            strcpy (bmeta[i].bitmap_description[12], "Cirrus Confidence");
            strcpy (bmeta[i].bitmap_description[13], "Cirrus Confidence");
            strcpy (bmeta[i].bitmap_description[14], "Cloud Confidence");
            strcpy (bmeta[i].bitmap_description[15], "Cloud Confidence");
        }
    }

    /* Close the metadata file */
    fclose (mtl_fptr);

    /* Get geolocation information from the XML file to prepare for computing
       the bounding coordinates */
    if (!get_geoloc_info (metadata, &geoloc_def))
    {
        sprintf (errmsg, "Copying the geolocation information from the XML "
            "metadata structure.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Setup the mapping structure */
    geoloc_map = setup_mapping (&geoloc_def);
    if (geoloc_map == NULL)
    {
        sprintf (errmsg, "Setting up the geolocation mapping structure.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    
    /* Compute the geographic bounds using the reflectance band coordinates */
    /* For ascending scenes and scenes in the polar regions, the scenes are
       flipped upside down.  The bounding coords will be correct in North
       represents the northernmost latitude and South represents the
       southernmost latitude.  However, the UL corner in this case would be
       more south than the LR corner.  Comparing the UL and LR corners will
       allow the user to determine if the scene is flipped. */
    if (!compute_bounds (geoloc_map, tmp_bmeta.nlines, tmp_bmeta.nsamps,
        &bounds))
    {
        sprintf (errmsg, "Setting up the geolocation mapping structure.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    gmeta->bounding_coords[ESPA_WEST] = bounds.min_lon;
    gmeta->bounding_coords[ESPA_EAST] = bounds.max_lon;
    gmeta->bounding_coords[ESPA_NORTH] = bounds.max_lat;
    gmeta->bounding_coords[ESPA_SOUTH] = bounds.min_lat;

    /* Free the geolocation structure */
    free (geoloc_map);

    /* Successful read */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  convert_gtif_to_img

PURPOSE: Convert the LPGS GeoTIFF band to ESPA raw binary (.img) file and
writes the associated ENVI header for each band.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error converting the GeoTIFF file
SUCCESS         Successfully converterd GeoTIFF to raw binary

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/30/2013   Gail Schmidt     Original development
2/12/2014    Gail Schmidt     Updated to support int16
6/17/2014    Gail Schmidt     Updated to support uint16 for L8

NOTES:
1. TIFF read scanline only supports reading a single line at a time.  We will
   read a single line, stuff it into a large buffer, then write the entire
   image at one time.  This is about 40% faster than reading a single line
   then writing a single line.
******************************************************************************/
int convert_gtif_to_img
(
    char *gtif_file,           /* I: name of the input GeoTIFF file */
    Espa_band_meta_t *bmeta,   /* I: pointer to band metadata for this band */
    Espa_global_meta_t *gmeta  /* I: pointer to global metadata */
)
{
    char FUNC_NAME[] = "convert_gtif_to_img";  /* function name */
    char errmsg[STR_SIZE];    /* error message */
    char *cptr = NULL;        /* pointer to the file extension */
    char *img_file = NULL;    /* name of the output raw binary file */
    char envi_file[STR_SIZE]; /* name of the output ENVI header file */
    int i;                    /* looping variable for lines in image */
    int nbytes;               /* number of bytes in the data type */
    int count;                /* number of chars copied in snprintf */
    void *file_buf = NULL;    /* pointer to correct input file buffer */
    uint8 *file_buf_u8 = NULL;  /* buffer for uint8 TIFF data to be read */
    int16 *file_buf_i16 = NULL; /* buffer for int16 TIFF data to be read */
    int16 *file_buf_u16 = NULL; /* buffer for uint16 TIFF data to be read */
    TIFF *fp_tiff = NULL;     /* file pointer for the TIFF file */
    FILE *fp_rb = NULL;       /* file pointer for the raw binary file */
    Envi_header_t envi_hdr;   /* output ENVI header information */

    /* Open the TIFF file for reading */
    fp_tiff = XTIFFOpen (gtif_file, "r");
    if (fp_tiff == NULL)
    {
        sprintf (errmsg, "Opening the LPGS GeoTIFF file: %s", gtif_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Open the raw binary file for writing */
    img_file = bmeta->file_name;
    fp_rb = open_raw_binary (img_file, "wb");
    if (fp_rb == NULL)
    {
        sprintf (errmsg, "Opening the output raw binary file: %s", img_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Allocate memory for the entire image, based on the input data type */
    if (bmeta->data_type == ESPA_UINT8)
    {
        nbytes = sizeof (uint8);
        file_buf_u8 = calloc (bmeta->nlines * bmeta->nsamps, nbytes);
        if (file_buf_u8 == NULL)
        {
            sprintf (errmsg, "Allocating memory for the image of uint8 data "
                "containing %d lines x %d samples.", bmeta->nlines,
                bmeta->nsamps);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        file_buf = file_buf_u8;
    }
    else if (bmeta->data_type == ESPA_INT16)
    {
        nbytes = sizeof (int16);
        file_buf_i16 = calloc (bmeta->nlines * bmeta->nsamps, nbytes);
        if (file_buf_i16 == NULL)
        {
            sprintf (errmsg, "Allocating memory for the image of int16 data "
                "containing %d lines x %d samples.", bmeta->nlines,
                bmeta->nsamps);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        file_buf = file_buf_i16;
    }
    else if (bmeta->data_type == ESPA_UINT16)
    {
        nbytes = sizeof (uint16);
        file_buf_u16 = calloc (bmeta->nlines * bmeta->nsamps, nbytes);
        if (file_buf_u16 == NULL)
        {
            sprintf (errmsg, "Allocating memory for the image of uint16 data "
                "containing %d lines x %d samples.", bmeta->nlines,
                bmeta->nsamps);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        file_buf = file_buf_u16;
    }
    else
    {
        sprintf (errmsg, "Unsupported data type.  Currently only uint8, "
            "int16, and uint16 are supported.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Loop through the lines in the TIFF file, reading and stuffing in the
       image buffer */
    if (bmeta->data_type == ESPA_UINT8)
    {
        for (i = 0; i < bmeta->nlines; i++)
        {
            /* Read current line from the TIFF file */
            if (!TIFFReadScanline (fp_tiff, &file_buf_u8[i*bmeta->nsamps],
                i, 0))
            {
                sprintf (errmsg, "Reading line %d from the TIFF file: %s", i,
                    gtif_file);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }
    else if (bmeta->data_type == ESPA_INT16)
    {
        for (i = 0; i < bmeta->nlines; i++)
        {
            /* Read current line from the TIFF file */
            if (!TIFFReadScanline (fp_tiff, &file_buf_i16[i*bmeta->nsamps],
                i, 0))
            {
                sprintf (errmsg, "Reading line %d from the TIFF file: %s", i,
                    gtif_file);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }
    else if (bmeta->data_type == ESPA_UINT16)
    {
        for (i = 0; i < bmeta->nlines; i++)
        {
            /* Read current line from the TIFF file */
            if (!TIFFReadScanline (fp_tiff, &file_buf_u16[i*bmeta->nsamps],
                i, 0))
            {
                sprintf (errmsg, "Reading line %d from the TIFF file: %s", i,
                    gtif_file);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }

    /* Write entire image to the raw binary file */
    if (write_raw_binary (fp_rb, bmeta->nlines, bmeta->nsamps, nbytes,
        file_buf) != SUCCESS)
    {
        sprintf (errmsg, "Writing image to the raw binary file: %s", img_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Close the TIFF and raw binary files */
    XTIFFClose (fp_tiff);
    close_raw_binary (fp_rb);

    /* Free the memory */
    free (file_buf_u8);
    free (file_buf_i16);
    free (file_buf_u16);

    /* Create the ENVI header file this band */
    if (create_envi_struct (bmeta, gmeta, &envi_hdr) != SUCCESS)
    {
        sprintf (errmsg, "Creating the ENVI header structure for this file.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write the ENVI header */
    count = snprintf (envi_file, sizeof (envi_file), "%s", img_file);
    if (count < 0 || count >= sizeof (envi_file))
    {
        sprintf (errmsg, "Overflow of envi_file string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    cptr = strchr (envi_file, '.');
    strcpy (cptr, ".hdr");

    if (write_envi_hdr (envi_file, &envi_hdr) != SUCCESS)
    {
        sprintf (errmsg, "Writing the ENVI header file: %s.", envi_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Successful conversion */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  convert_lpgs_to_espa

PURPOSE: Converts the input LPGS GeoTIFF files (and associated MTL file) to
the ESPA internal raw binary file format (and associated XML file).

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error converting the GeoTIFF file
SUCCESS         Successfully converted GeoTIFF to raw binary

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/30/2013   Gail Schmidt     Original development
4/2/2014     Gail Schmidt     Added support for a flag to delete the source
                              .tif files
8/5/2014     Gail Schmidt     Obtain the location of the ESPA schema file from
                              an environment variable vs. the ESPA http site

NOTES:
  1. The LPGS GeoTIFF band files will be deciphered from the LPGS MTL file.
  2. The ESPA raw binary band files will be generated from the ESPA XML
     filename.
******************************************************************************/
int convert_lpgs_to_espa
(
    char *lpgs_mtl_file,   /* I: input LPGS MTL metadata filename */
    char *espa_xml_file,   /* I: output ESPA XML metadata filename */
    bool del_src           /* I: should the source .tif files be removed after
                                 conversion? */
)
{
    char FUNC_NAME[] = "convert_lpgs_to_espa";  /* function name */
    char errmsg[STR_SIZE];   /* error message */
    char *schema = NULL;     /* ESPA schema file */
    Espa_internal_meta_t xml_metadata;  /* XML metadata structure to be
                                populated by reading the MTL metadata file */
    int i;                   /* looping variable */
    int nlpgs_bands;         /* number of bands in the LPGS product */
    char lpgs_bands[MAX_LPGS_BANDS][STR_SIZE];  /* array containing the file
                                names of the LPGS bands */
    struct stat statbuf;     /* buffer for the file stat function */

    /* Get the ESPA schema environment variable which specifies the location
       of the XML schema to be used */
    schema = getenv ("ESPA_SCHEMA");
    if (schema == NULL)
    {  /* ESPA schema environment variable wasn't defined. Try the version in
          /usr/local... */
        schema = LOCAL_ESPA_SCHEMA;
        if (stat (schema, &statbuf) == -1)
        {  /* /usr/local ESPA schema file doesn't exist.  Try the version on
              the ESPA http site... */
            schema = ESPA_SCHEMA;
        }
    }

    /* Read the LPGS MTL file and populate our internal ESPA metadata
       structure */
    if (read_lpgs_mtl (lpgs_mtl_file, &xml_metadata, &nlpgs_bands,
        lpgs_bands) != SUCCESS)
    {
        sprintf (errmsg, "Reading the LPGS MTL file: %s", lpgs_mtl_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write the metadata from our internal metadata structure to the output
       XML filename */
    if (write_metadata (&xml_metadata, espa_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Validate the input metadata file */
    printf ("Validating schema with %s ...\n", schema);
    if (validate_xml_file (espa_xml_file, schema) != SUCCESS)
    {  /* Error messages already written */
        sprintf (errmsg, "Possible schema file not found.  ESPA_SCHEMA "
            "environment variable isn't defined.  The first default schema "
            "location of %s doesn't exist.  And the second default location of "
            "%s was used as the last default.", LOCAL_ESPA_SCHEMA, ESPA_SCHEMA);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Convert each of the LPGS GeoTIFF files to raw binary */
    for (i = 0; i < nlpgs_bands; i++)
    {
        printf ("  Band %d: %s to %s\n", i, lpgs_bands[i],
            xml_metadata.band[i].file_name);
        if (convert_gtif_to_img (lpgs_bands[i], &xml_metadata.band[i],
            &xml_metadata.global) != SUCCESS)
        {
            sprintf (errmsg, "Converting band %d: %s", i, lpgs_bands[i]);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Remove the source file if specified */
        if (del_src)
        {
            printf ("  Removing %s\n", lpgs_bands[i]);
            if (unlink (lpgs_bands[i]) != 0)
            {
                sprintf (errmsg, "Deleting source file: %s", lpgs_bands[i]);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }

    /* Free the metadata structure */
    free_metadata (&xml_metadata);

    /* Successful conversion */
    return (SUCCESS);
}

