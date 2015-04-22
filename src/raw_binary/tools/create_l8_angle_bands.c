/*****************************************************************************
FILE: create_l8_angle_bands
  
PURPOSE: Creates the Landsat 8 solar and view/satellite per-pixel angles.
Both the zenith and azimuth angles are created for each angle type for each
Landsat 8 band.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/3/2015     Gail Schmidt     Original development

NOTES:
*****************************************************************************/
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "error_handler.h"
#include "l8_angles.h"

/******************************************************************************
MODULE: usage

PURPOSE: Prints the usage information for this application.

RETURN VALUE:
Type = None

HISTORY:
Date         Programmer       Reason
---------    ---------------  -------------------------------------
4/3/2015     Gail Schmidt     Original development

NOTES:
******************************************************************************/
void usage ()
{
    printf ("create_l8_angle_bands creates the Landsat 8 solar and view "
            "(satellite) per-pixel angles for each band.  Both the zenith "
            "and azimuth angles are created for each angle.  Values are "
            "written in degrees and scaled by 100.\n\n");
    printf ("usage: create_l8_angle_bands "
            "--ang=angle_coefficient_filename\n"
            "--outfile=base_output_filename\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -ang: input angle coefficient file\n");
    printf ("    -outfile: base filename of the output angle files\n");
    printf ("\nExample: create_l8_angle_bands "
            "--ang=LC80470272013287LGN00_ANG.txt "
            "--outfile=LC80470272013287LGN00\n");
}


/******************************************************************************
MODULE:  get_args

PURPOSE:  Gets the command-line arguments and validates that the required
arguments were specified.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error getting the command-line arguments or a command-line
                argument and associated value were not specified
SUCCESS         No errors encountered

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/3/2015     Gail Schmidt     Original development

NOTES:
  1. Memory is allocated for the input and output files.  All of these should
     be character pointers set to NULL on input.  The caller is responsible
     for freeing the allocated memory upon successful return.
******************************************************************************/
short get_args
(
    int argc,             /* I: number of cmd-line args */
    char *argv[],         /* I: string of cmd-line args */
    char **ang_infile,    /* O: address of input angle coefficient filename */
    char **outfile        /* O: address of output base filename */
)
{
    int c;                           /* current argument index */
    int option_index;                /* index for the command-line option */
    char errmsg[STR_SIZE];           /* error message */
    char FUNC_NAME[] = "get_args";   /* function name */
    static struct option long_options[] =
    {
        {"ang", required_argument, 0, 'a'},
        {"outfile", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* Loop through all the cmd-line options */
    opterr = 0;   /* turn off getopt_long error msgs as we'll print our own */
    while (1)
    {
        /* optstring in call to getopt_long is empty since we will only
           support the long options */
        c = getopt_long (argc, argv, "", long_options, &option_index);
        if (c == -1)
        {   /* Out of cmd-line options */
            break;
        }

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
     
            case 'h':  /* help */
                usage ();
                return (ERROR);
                break;

            case 'a':  /* input angle coefficient file */
                *ang_infile = strdup (optarg);
                break;
     
            case 'o':  /* base outfile */
                *outfile = strdup (optarg);
                break;
     
            case '?':
            default:
                sprintf (errmsg, "Unknown option %s", argv[optind-1]);
                error_handler (true, FUNC_NAME, errmsg);
                usage ();
                return (ERROR);
                break;
        }
    }

    /* Make sure the infiles and outfiles were specified */
    if (*ang_infile == NULL)
    {
        sprintf (errmsg, "Input angle coefficient file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    if (*outfile == NULL)
    {
        sprintf (errmsg, "Base output file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    return (SUCCESS);
}


/******************************************************************************
MODULE:  main

PURPOSE: Creates the Landsat 8 solar and view/satellite per-pixel angles.
Both the zenith and azimuth angles are created for each angle type for each
band (bands 1-11).

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error creating the angle bands
SUCCESS         No errors encountered

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/3/2015     Gail Schmidt     Original development

NOTES:
1. Angles are written in degrees and scaled by 100.
******************************************************************************/
int main (int argc, char** argv)
{
    int i;                       /* looping variable */
    int parm;                    /* looping variable */
    int nlines[L8_NBANDS];       /* number of lines for each band */
    int nsamps[L8_NBANDS];       /* number of samples for each band */
    char FUNC_NAME[] = "create_l8_angle_bands";  /* function name */
    char errmsg[STR_SIZE];       /* error message */
    char tmpfile[1024];          /* temporary filename */
    char *ang_infile = NULL;     /* input angle coefficient filename */
    char *outfile = NULL;        /* output base filename for angle bands */
    ANGLES_FRAME frame[L8_NBANDS];   /* image frame info for each band */
    short *solar_zenith[L8_NBANDS];  /* array of pointers for the solar zenith
                                        angle array, one per band */
    short *solar_azimuth[L8_NBANDS]; /* array of pointers for the solar azimuth
                                        angle array, one per band */
    short *sat_zenith[L8_NBANDS];    /* array of pointers for the satellite
                                        zenith angle array, one per band */
    short *sat_azimuth[L8_NBANDS];   /* array of pointers for the satellite
                                        azimuth angle array, one per band */
    FILE *fptr=NULL;             /* file pointer */
    Envi_header_t envi_hdr;   /* output ENVI header information */

    /* Read the command-line arguments */
    if (get_args (argc, argv, &ang_infile, &outfile) != SUCCESS)
    {   /* get_args already printed the error message */
        exit (ERROR);
    }

    /* Create the Landsat 8 angle bands for all bands.  Create a full
       resolution product with a fill value of -9999 to match the Landsat 8
       image data. */
    if (l8_per_pixel_angles (ang_infile, 1, -9999, "ALL", frame, solar_zenith,
        solar_azimuth, sat_zenith, sat_azimuth, nlines, nsamps) != SUCCESS)
//    if (l8_per_pixel_angles (ang_infile, 1, -9999, "1,2,3,4,5,6,7,9", solar_zenith,
//        solar_azimuth, sat_zenith, NULL, nlines, nsamps) != SUCCESS)
    {  /* Error messages already written */
        exit (ERROR);
    }

    /* Write the solar zenith output angles */
    printf ("Writing solar zenith angles ...\n");
    for (i = 0; i < L8_NBANDS; i++)
    {
        /* Open the output file for this band */
        sprintf (tmpfile, "%s_B%d_solar_zenith.img", outfile, i+1);
        fptr = open_raw_binary (tmpfile, "wb");
        if (!fptr)
        {
            sprintf (errmsg, "Unable to open the solar zenith file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Write the data for this band */
        if (write_raw_binary (fptr, nlines[i], nsamps[i], sizeof (short),
            &solar_zenith[i][0]) != SUCCESS)
        {
            sprintf (errmsg, "Unable to write to the solar zenith file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Close the file and free the pointer for this band */
        close_raw_binary (fptr);
        free (solar_zenith[i]);

        /* Create the ENVI header */
        sprintf (envi_hdr.description, "Solar angle file");
        envi_hdr.nlines = nlines[i];
        envi_hdr.nsamps = nsamps[i];
        envi_hdr.nbands = 1;
        envi_hdr.header_offset = 0;
        envi_hdr.byte_order = 0;
        sprintf (envi_hdr.file_type, "ENVI Standard");
        envi_hdr.data_type = 2;
        envi_hdr.data_ignore_value = -9999;
        sprintf (envi_hdr.interleave, "BSQ");
        sprintf (envi_hdr.sensor_type, "Landsat OLI/TIRS");

        if (frame[i].projection.spheroid == WGS84_SPHEROID)
            envi_hdr.datum_type = ESPA_WGS84;
        else
        {
            sprintf (errmsg, "Unsupported datum. Currently only expect WGS84.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }
 
        if (frame[i].projection.proj_code == UTM)
        {
            envi_hdr.proj_type = GCTP_UTM_PROJ;
            envi_hdr.utm_zone = frame[i].projection.zone;
        }
        else if (frame[i].projection.proj_code == PS)
        {
            envi_hdr.proj_type = GCTP_PS_PROJ;
        }
        else
        {
            sprintf (errmsg, "Unsupported projection. Currently only expect "
                "UTM or PS.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
            envi_hdr.proj_parms[parm] = frame[i].projection.parameters[parm];
        envi_hdr.pixel_size[0] = frame[i].pixel_size;
        envi_hdr.pixel_size[1] = frame[i].pixel_size;
        envi_hdr.ul_corner[0] = frame[i].ul_corner.x -
            frame[i].pixel_size * 0.5;
        envi_hdr.ul_corner[1] = frame[i].ul_corner.y +
            frame[i].pixel_size * 0.5;
        envi_hdr.xy_start[0] = 1;
        envi_hdr.xy_start[1] = 1;
        sprintf (envi_hdr.band_names[0], "Solar zenith angle");
 
        /* Write the ENVI header */
        sprintf (tmpfile, "%s_B%d_solar_zenith.hdr", outfile, i+1);
        if (write_envi_hdr (tmpfile, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Writing the ENVI header file: %s.", tmpfile);
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }
    }

    /* Write the solar azimuth output angles */
    printf ("Writing solar azimuth angles ...\n");
    for (i = 0; i < L8_NBANDS; i++)
    {
        /* Open the output file for this band */
        sprintf (tmpfile, "%s_B%d_solar_azimuth.img", outfile, i+1);
        fptr = open_raw_binary (tmpfile, "wb");
        if (!fptr)
        {
            sprintf (errmsg, "Unable to open the solar azimuth file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Write the data for this band */
        if (write_raw_binary (fptr, nlines[i], nsamps[i], sizeof (short),
            &solar_azimuth[i][0]) != SUCCESS)
        {
            sprintf (errmsg, "Unable to write to the solar azimuth file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Close the file and free the pointer for this band */
        close_raw_binary (fptr);
        free (solar_azimuth[i]);

        /* Create the ENVI header */
        sprintf (envi_hdr.description, "Solar angle file");
        envi_hdr.nlines = nlines[i];
        envi_hdr.nsamps = nsamps[i];
        envi_hdr.nbands = 1;
        envi_hdr.header_offset = 0;
        envi_hdr.byte_order = 0;
        sprintf (envi_hdr.file_type, "ENVI Standard");
        envi_hdr.data_type = 2;
        envi_hdr.data_ignore_value = -9999;
        sprintf (envi_hdr.interleave, "BSQ");
        sprintf (envi_hdr.sensor_type, "Landsat OLI/TIRS");

        if (frame[i].projection.spheroid == WGS84_SPHEROID)
            envi_hdr.datum_type = ESPA_WGS84;
        else
        {
            sprintf (errmsg, "Unsupported datum. Currently only expect WGS84.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }
 
        if (frame[i].projection.proj_code == UTM)
        {
            envi_hdr.proj_type = GCTP_UTM_PROJ;
            envi_hdr.utm_zone = frame[i].projection.zone;
        }
        else if (frame[i].projection.proj_code == PS)
        {
            envi_hdr.proj_type = GCTP_PS_PROJ;
        }
        else
        {
            sprintf (errmsg, "Unsupported projection. Currently only expect "
                "UTM or PS.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
            envi_hdr.proj_parms[parm] = frame[i].projection.parameters[parm];
        envi_hdr.pixel_size[0] = frame[i].pixel_size;
        envi_hdr.pixel_size[1] = frame[i].pixel_size;
        envi_hdr.ul_corner[0] = frame[i].ul_corner.x -
            frame[i].pixel_size * 0.5;
        envi_hdr.ul_corner[1] = frame[i].ul_corner.y +
            frame[i].pixel_size * 0.5;
        envi_hdr.xy_start[0] = 1;
        envi_hdr.xy_start[1] = 1;
        sprintf (envi_hdr.band_names[0], "Solar azimuth angle");
 
        /* Write the ENVI header */
        sprintf (tmpfile, "%s_B%d_solar_azimuth.hdr", outfile, i+1);
        if (write_envi_hdr (tmpfile, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Writing the ENVI header file: %s.", tmpfile);
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }
    }

    /* Write the sat zenith output angles */
    printf ("Writing view zenith angles ...\n");
    for (i = 0; i < L8_NBANDS; i++)
    {
        /* Open the output file for this band */
        sprintf (tmpfile, "%s_B%d_sensor_zenith.img", outfile, i+1);
        fptr = open_raw_binary (tmpfile, "wb");
        if (!fptr)
        {
            sprintf (errmsg, "Unable to open the sat zenith file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Write the data for this band */
        if (write_raw_binary (fptr, nlines[i], nsamps[i], sizeof (short),
            &sat_zenith[i][0]) != SUCCESS)
        {
            sprintf (errmsg, "Unable to write to the sat zenith file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Close the file and free the pointer for this band */
        close_raw_binary (fptr);
        free (sat_zenith[i]);

        /* Create the ENVI header */
        sprintf (envi_hdr.description, "Satellite/View angle file");
        envi_hdr.nlines = nlines[i];
        envi_hdr.nsamps = nsamps[i];
        envi_hdr.nbands = 1;
        envi_hdr.header_offset = 0;
        envi_hdr.byte_order = 0;
        sprintf (envi_hdr.file_type, "ENVI Standard");
        envi_hdr.data_type = 2;
        envi_hdr.data_ignore_value = -9999;
        sprintf (envi_hdr.interleave, "BSQ");
        sprintf (envi_hdr.sensor_type, "Landsat OLI/TIRS");

        if (frame[i].projection.spheroid == WGS84_SPHEROID)
            envi_hdr.datum_type = ESPA_WGS84;
        else
        {
            sprintf (errmsg, "Unsupported datum. Currently only expect WGS84.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }
 
        if (frame[i].projection.proj_code == UTM)
        {
            envi_hdr.proj_type = GCTP_UTM_PROJ;
            envi_hdr.utm_zone = frame[i].projection.zone;
        }
        else if (frame[i].projection.proj_code == PS)
        {
            envi_hdr.proj_type = GCTP_PS_PROJ;
        }
        else
        {
            sprintf (errmsg, "Unsupported projection. Currently only expect "
                "UTM or PS.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
            envi_hdr.proj_parms[parm] = frame[i].projection.parameters[parm];
        envi_hdr.pixel_size[0] = frame[i].pixel_size;
        envi_hdr.pixel_size[1] = frame[i].pixel_size;
        envi_hdr.ul_corner[0] = frame[i].ul_corner.x -
            frame[i].pixel_size * 0.5;
        envi_hdr.ul_corner[1] = frame[i].ul_corner.y +
            frame[i].pixel_size * 0.5;
        envi_hdr.xy_start[0] = 1;
        envi_hdr.xy_start[1] = 1;
        sprintf (envi_hdr.band_names[0], "View zenith angle");
 
        /* Write the ENVI header */
        sprintf (tmpfile, "%s_B%d_sensor_zenith.hdr", outfile, i+1);
        if (write_envi_hdr (tmpfile, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Writing the ENVI header file: %s.", tmpfile);
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }
    }

    /* Write the sat azimuth output angles */
    printf ("Writing view azimuth angles ...\n");
    for (i = 0; i < L8_NBANDS; i++)
    {
        /* Open the output file for this band */
        sprintf (tmpfile, "%s_B%d_sensor_azimuth.img", outfile, i+1);
        fptr = open_raw_binary (tmpfile, "wb");
        if (!fptr)
        {
            sprintf (errmsg, "Unable to open the sat azimuth file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Write the data for this band */
        if (write_raw_binary (fptr, nlines[i], nsamps[i], sizeof (short),
            &sat_azimuth[i][0]) != SUCCESS)
        {
            sprintf (errmsg, "Unable to write to the sat azimuth file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Close the file and free the pointer for this band */
        close_raw_binary (fptr);
        free (sat_azimuth[i]);

        /* Create the ENVI header */
        sprintf (envi_hdr.description, "Satellite/View angle file");
        envi_hdr.nlines = nlines[i];
        envi_hdr.nsamps = nsamps[i];
        envi_hdr.nbands = 1;
        envi_hdr.header_offset = 0;
        envi_hdr.byte_order = 0;
        sprintf (envi_hdr.file_type, "ENVI Standard");
        envi_hdr.data_type = 2;
        envi_hdr.data_ignore_value = -9999;
        sprintf (envi_hdr.interleave, "BSQ");
        sprintf (envi_hdr.sensor_type, "Landsat OLI/TIRS");

        if (frame[i].projection.spheroid == WGS84_SPHEROID)
            envi_hdr.datum_type = ESPA_WGS84;
        else
        {
            sprintf (errmsg, "Unsupported datum. Currently only expect WGS84.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }
 
        if (frame[i].projection.proj_code == UTM)
        {
            envi_hdr.proj_type = GCTP_UTM_PROJ;
            envi_hdr.utm_zone = frame[i].projection.zone;
        }
        else if (frame[i].projection.proj_code == PS)
        {
            envi_hdr.proj_type = GCTP_PS_PROJ;
        }
        else
        {
            sprintf (errmsg, "Unsupported projection. Currently only expect "
                "UTM or PS.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
            envi_hdr.proj_parms[parm] = frame[i].projection.parameters[parm];
        envi_hdr.pixel_size[0] = frame[i].pixel_size;
        envi_hdr.pixel_size[1] = frame[i].pixel_size;
        envi_hdr.ul_corner[0] = frame[i].ul_corner.x -
            frame[i].pixel_size * 0.5;
        envi_hdr.ul_corner[1] = frame[i].ul_corner.y +
            frame[i].pixel_size * 0.5;
        envi_hdr.xy_start[0] = 1;
        envi_hdr.xy_start[1] = 1;
        sprintf (envi_hdr.band_names[0], "View azimuth angle");
 
        /* Write the ENVI header */
        sprintf (tmpfile, "%s_B%d_sensor_azimuth.hdr", outfile, i+1);
        if (write_envi_hdr (tmpfile, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Writing the ENVI header file: %s.", tmpfile);
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }
    }

    /* Free the pointers */
    free (ang_infile);
    free (outfile);

    /* Successful completion */
    exit (SUCCESS);
}
