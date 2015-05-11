/*****************************************************************************
FILE: create_l8_angle_bands

PURPOSE: Creates the Landsat 8 solar and view/satellite per-pixel angles.
Both the zenith and azimuth angles are created for each angle type for each
Landsat 8 band or for the average of the L8 reflective bands.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/3/2015     Gail Schmidt     Original development
5/5/2015     Gail Schmidt     Updated to support writing the average of the
                              reflectance bands for each angle vs. all the
                              bands

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
            "(satellite) per-pixel angles for each band or for an average of "
            "the reflective bands.  Both the zenith and azimuth angles are "
            "created for each angle.  Values are written in degrees and scaled "
            "by 100.\n\n");
    printf ("usage: create_l8_angle_bands "
            "--ang=angle_coefficient_filename\n"
            "--outfile=base_output_filename "
            "{--average}");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -ang: input angle coefficient file\n");
    printf ("    -outfile: base filename of the output angle files\n");
    printf ("and the following parameters are optional:\n");
    printf ("    -average: write the reflectance band averages instead of "
            "writing each of the band angles.\n\n");

    printf ("\nExample: create_l8_angle_bands "
            "--ang=LC80470272013287LGN00_ANG.txt "
            "--outfile=LC80470272013287LGN00\n");
    printf ("This writes a single band file for each of the bands for the "
            "solar azimuth/zenith and the satellite azimuth/zenith angles.\n");

    printf ("\nExample: create_l8_angle_bands "
            "--ang=LC80470272013287LGN00_ANG.txt "
            "--outfile=LC80470272013287LGN00 --average\n");
    printf ("This writes an average band file for the solar azimuth/zenith "
            "and the satellite azimuth/zenith angles.\n");
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
    char **outfile,       /* O: address of output base filename */
    bool *band_avg        /* O: should the reflectance band average be
                                processed? */
)
{
    int c;                           /* current argument index */
    int option_index;                /* index for the command-line option */
    static int avg_flag = 0;         /* flag to indicate if the band average
                                        should be processed */
    char errmsg[STR_SIZE];           /* error message */
    char FUNC_NAME[] = "get_args";   /* function name */
    static struct option long_options[] =
    {
        {"average", no_argument, &avg_flag, 1},
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

    /* Check the band average flag */
    if (avg_flag)
        *band_avg = true;

    return (SUCCESS);
}


/******************************************************************************
MODULE:  main

PURPOSE: Creates the Landsat 8 solar and view/satellite per-pixel angles.
Both the zenith and azimuth angles are created for each angle type for each
band (bands 1-11).  An option is supported to write the average of the
reflectance bands for each angle instead of writing the angle for each band.

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
2. Note this is a memory hog.  The goal here was to have an application to
   be able to write out the per-pixel angle bands for testing the per-pixel
   angles.  In order to make this a less memory hog, then break it down to
   process the solar angles, write the solar angles, process the satellite
   angles, and write the satellite angles.
******************************************************************************/
int main (int argc, char** argv)
{
    bool band_avg;               /* should the reflectance band average be
                                    processed? */
    int i;                       /* looping variable */
    int parm;                    /* looping variable */
    int count;                   /* number of chars copied in snprintf */
    int nlines[L8_NBANDS];       /* number of lines for each band */
    int nsamps[L8_NBANDS];       /* number of samples for each band */
    int avg_nlines;              /* number of lines for band average */
    int avg_nsamps;              /* number of samples for band average */
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
    ANGLES_FRAME avg_frame;        /* image frame info for band average */
    short *avg_solar_zenith=NULL;  /* array for solar zenith angle average */
    short *avg_solar_azimuth=NULL; /* array for solar azimuth angle average */
    short *avg_sat_zenith=NULL;    /* array for satellite zenith angle avg */
    short *avg_sat_azimuth=NULL;   /* array for satellite azimuth angle avg */
    FILE *fptr=NULL;               /* file pointer */
    Envi_header_t envi_hdr;        /* output ENVI header information */

    /* Read the command-line arguments */
    if (get_args (argc, argv, &ang_infile, &outfile, &band_avg) != SUCCESS)
    {   /* get_args already printed the error message */
        exit (ERROR);
    }

    if (!band_avg)
    {
        /* Create the Landsat 8 angle bands for all bands.  Create a full
           resolution product with a fill value of -9999 to match the Landsat 8
           image data. */
        if (l8_per_pixel_angles (ang_infile, 1, -9999, "ALL", frame,
            solar_zenith, solar_azimuth, sat_zenith, sat_azimuth, nlines,
            nsamps) != SUCCESS)
        {  /* Error messages already written */
            exit (ERROR);
        }

        /* Write the solar zenith output angles */
        printf ("Writing solar zenith angles ...\n");
        for (i = 0; i < L8_NBANDS; i++)
        {
            /* Open the output file for this band */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "%s_B%d_solar_zenith.img", outfile, i+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                sprintf (errmsg, "Overflow of tmpfile");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

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
            count = snprintf (envi_hdr.description,
                sizeof (envi_hdr.description), "Solar angle file");
            if (count < 0 || count >= sizeof (envi_hdr.description))
            {
                sprintf (errmsg, "Overflow of envi_hdr.description");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            envi_hdr.nlines = nlines[i];
            envi_hdr.nsamps = nsamps[i];
            envi_hdr.nbands = 1;
            envi_hdr.header_offset = 0;
            envi_hdr.byte_order = 0;

            count = snprintf (envi_hdr.file_type, sizeof (envi_hdr.file_type),
                "ENVI Standard");
            if (count < 0 || count >= sizeof (envi_hdr.file_type))
            {
                sprintf (errmsg, "Overflow of envi_hdr.file_type");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            envi_hdr.data_type = 2;
            envi_hdr.data_ignore_value = -9999;
            count = snprintf (envi_hdr.interleave, sizeof (envi_hdr.interleave),
                "BSQ");
            if (count < 0 || count >= sizeof (envi_hdr.interleave))
            {
                sprintf (errmsg, "Overflow of envi_hdr.interleave");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            count = snprintf (envi_hdr.sensor_type,
                sizeof (envi_hdr.sensor_type), "Landsat OLI/TIRS");
            if (count < 0 || count >= sizeof (envi_hdr.sensor_type))
            {
                sprintf (errmsg, "Overflow of envi_hdr.interleave");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (frame[i].projection.spheroid == WGS84_SPHEROID)
                envi_hdr.datum_type = ESPA_WGS84;
            else
            {
                sprintf (errmsg, "Unsupported datum. Currently only expect "
                    "WGS84.");
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
                sprintf (errmsg, "Unsupported projection. Currently only "
                    "expect UTM or PS.");
                error_handler (true, FUNC_NAME, errmsg);
                exit (ERROR);
            }

            for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
                envi_hdr.proj_parms[parm] =
                    frame[i].projection.parameters[parm];
            envi_hdr.pixel_size[0] = frame[i].pixel_size;
            envi_hdr.pixel_size[1] = frame[i].pixel_size;
            envi_hdr.ul_corner[0] = frame[i].ul_corner.x -
                frame[i].pixel_size * 0.5;
            envi_hdr.ul_corner[1] = frame[i].ul_corner.y +
                frame[i].pixel_size * 0.5;
            envi_hdr.xy_start[0] = 1;
            envi_hdr.xy_start[1] = 1;
            count = snprintf (envi_hdr.band_names[0],
                sizeof (envi_hdr.band_names[0]), "Solar zenith angle");
            if (count < 0 || count >= sizeof (envi_hdr.band_names[0]))
            {
                sprintf (errmsg, "Overflow of envi_hdr.band_names[0]");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Write the ENVI header */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "%s_B%d_solar_zenith.hdr", outfile, i+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                sprintf (errmsg, "Overflow of tmpfile");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

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
            count = snprintf (tmpfile, sizeof (tmpfile),
                "%s_B%d_solar_azimuth.img", outfile, i+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                sprintf (errmsg, "Overflow of tmpfile");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

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
            count = snprintf (envi_hdr.description,
                sizeof (envi_hdr.description), "Solar angle file");
            if (count < 0 || count >= sizeof (envi_hdr.description))
            {
                sprintf (errmsg, "Overflow of envi_hdr.description");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            envi_hdr.nlines = nlines[i];
            envi_hdr.nsamps = nsamps[i];
            envi_hdr.nbands = 1;
            envi_hdr.header_offset = 0;
            envi_hdr.byte_order = 0;
            count = snprintf (envi_hdr.file_type, sizeof (envi_hdr.file_type),
                "ENVI Standard");
            if (count < 0 || count >= sizeof (envi_hdr.file_type))
            {
                sprintf (errmsg, "Overflow of envi_hdr.file_type");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            envi_hdr.data_type = 2;
            envi_hdr.data_ignore_value = -9999;
            count = snprintf (envi_hdr.interleave, sizeof (envi_hdr.interleave),
                "BSQ");
            if (count < 0 || count >= sizeof (envi_hdr.interleave))
            {
                sprintf (errmsg, "Overflow of envi_hdr.interleave");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            count = snprintf (envi_hdr.sensor_type,
                sizeof (envi_hdr.sensor_type), "Landsat OLI/TIRS");
            if (count < 0 || count >= sizeof (envi_hdr.sensor_type))
            {
                sprintf (errmsg, "Overflow of envi_hdr.interleave");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (frame[i].projection.spheroid == WGS84_SPHEROID)
                envi_hdr.datum_type = ESPA_WGS84;
            else
            {
                sprintf (errmsg, "Unsupported datum. Currently only expect "
                    "WGS84.");
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
                sprintf (errmsg, "Unsupported projection. Currently only "
                    "expect UTM or PS.");
                error_handler (true, FUNC_NAME, errmsg);
                exit (ERROR);
            }

            for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
                envi_hdr.proj_parms[parm] =
                    frame[i].projection.parameters[parm];
            envi_hdr.pixel_size[0] = frame[i].pixel_size;
            envi_hdr.pixel_size[1] = frame[i].pixel_size;
            envi_hdr.ul_corner[0] = frame[i].ul_corner.x -
                frame[i].pixel_size * 0.5;
            envi_hdr.ul_corner[1] = frame[i].ul_corner.y +
                frame[i].pixel_size * 0.5;
            envi_hdr.xy_start[0] = 1;
            envi_hdr.xy_start[1] = 1;
            count = snprintf (envi_hdr.band_names[0],
                sizeof (envi_hdr.band_names[0]), "Solar azimuth angle");
            if (count < 0 || count >= sizeof (envi_hdr.band_names[0]))
            {
                sprintf (errmsg, "Overflow of envi_hdr.band_names[0]");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Write the ENVI header */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "%s_B%d_solar_azimuth.hdr", outfile, i+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                sprintf (errmsg, "Overflow of tmpfile");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

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
            count = snprintf (tmpfile, sizeof (tmpfile),
                "%s_B%d_sensor_zenith.img", outfile, i+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                sprintf (errmsg, "Overflow of tmpfile");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

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
            count = snprintf (envi_hdr.description,
                sizeof (envi_hdr.description), "Satellite/View angle file");
            if (count < 0 || count >= sizeof (envi_hdr.description))
            {
                sprintf (errmsg, "Overflow of envi_hdr.description");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            envi_hdr.nlines = nlines[i];
            envi_hdr.nsamps = nsamps[i];
            envi_hdr.nbands = 1;
            envi_hdr.header_offset = 0;
            envi_hdr.byte_order = 0;

            count = snprintf (envi_hdr.file_type, sizeof (envi_hdr.file_type),
                "ENVI Standard");
            if (count < 0 || count >= sizeof (envi_hdr.file_type))
            {
                sprintf (errmsg, "Overflow of envi_hdr.file_type");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            envi_hdr.data_type = 2;
            envi_hdr.data_ignore_value = -9999;
            count = snprintf (envi_hdr.interleave, sizeof (envi_hdr.interleave),
                "BSQ");
            if (count < 0 || count >= sizeof (envi_hdr.interleave))
            {
                sprintf (errmsg, "Overflow of envi_hdr.interleave");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            count = snprintf (envi_hdr.sensor_type,
                sizeof (envi_hdr.sensor_type), "Landsat OLI/TIRS");
            if (count < 0 || count >= sizeof (envi_hdr.sensor_type))
            {
                sprintf (errmsg, "Overflow of envi_hdr.interleave");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (frame[i].projection.spheroid == WGS84_SPHEROID)
                envi_hdr.datum_type = ESPA_WGS84;
            else
            {
                sprintf (errmsg, "Unsupported datum. Currently only expect "
                    "WGS84.");
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
                sprintf (errmsg, "Unsupported projection. Currently only "
                    "expect UTM or PS.");
                error_handler (true, FUNC_NAME, errmsg);
                exit (ERROR);
            }

            for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
                envi_hdr.proj_parms[parm] =
                    frame[i].projection.parameters[parm];
            envi_hdr.pixel_size[0] = frame[i].pixel_size;
            envi_hdr.pixel_size[1] = frame[i].pixel_size;
            envi_hdr.ul_corner[0] = frame[i].ul_corner.x -
                frame[i].pixel_size * 0.5;
            envi_hdr.ul_corner[1] = frame[i].ul_corner.y +
                frame[i].pixel_size * 0.5;
            envi_hdr.xy_start[0] = 1;
            envi_hdr.xy_start[1] = 1;
            count = snprintf (envi_hdr.band_names[0],
                sizeof (envi_hdr.band_names[0]), "View zenith angle");
            if (count < 0 || count >= sizeof (envi_hdr.band_names[0]))
            {
                sprintf (errmsg, "Overflow of envi_hdr.band_names[0]");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Write the ENVI header */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "%s_B%d_sensor_zenith.hdr", outfile, i+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                sprintf (errmsg, "Overflow of tmpfile");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

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
            count = snprintf (tmpfile, sizeof (tmpfile),
                "%s_B%d_sensor_azimuth.img", outfile, i+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                sprintf (errmsg, "Overflow of tmpfile");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

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
            count = snprintf (envi_hdr.description,
                sizeof (envi_hdr.description), "Satellite/View angle file");
            if (count < 0 || count >= sizeof (envi_hdr.description))
            {
                sprintf (errmsg, "Overflow of envi_hdr.description");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            envi_hdr.nlines = nlines[i];
            envi_hdr.nsamps = nsamps[i];
            envi_hdr.nbands = 1;
            envi_hdr.header_offset = 0;
            envi_hdr.byte_order = 0;
            count = snprintf (envi_hdr.file_type, sizeof (envi_hdr.file_type),
                "ENVI Standard");
            if (count < 0 || count >= sizeof (envi_hdr.file_type))
            {
                sprintf (errmsg, "Overflow of envi_hdr.file_type");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            envi_hdr.data_type = 2;
            envi_hdr.data_ignore_value = -9999;
            count = snprintf (envi_hdr.interleave, sizeof (envi_hdr.interleave),
                "BSQ");
            if (count < 0 || count >= sizeof (envi_hdr.interleave))
            {
                sprintf (errmsg, "Overflow of envi_hdr.interleave");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            count = snprintf (envi_hdr.sensor_type,
                sizeof (envi_hdr.sensor_type), "Landsat OLI/TIRS");
            if (count < 0 || count >= sizeof (envi_hdr.sensor_type))
            {
                sprintf (errmsg, "Overflow of envi_hdr.interleave");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (frame[i].projection.spheroid == WGS84_SPHEROID)
                envi_hdr.datum_type = ESPA_WGS84;
            else
            {
                sprintf (errmsg, "Unsupported datum. Currently only expect "
                    "WGS84.");
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
                sprintf (errmsg, "Unsupported projection. Currently only "
                    "expect UTM or PS.");
                error_handler (true, FUNC_NAME, errmsg);
                exit (ERROR);
            }

            for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
                envi_hdr.proj_parms[parm] =
                    frame[i].projection.parameters[parm];
            envi_hdr.pixel_size[0] = frame[i].pixel_size;
            envi_hdr.pixel_size[1] = frame[i].pixel_size;
            envi_hdr.ul_corner[0] = frame[i].ul_corner.x -
                frame[i].pixel_size * 0.5;
            envi_hdr.ul_corner[1] = frame[i].ul_corner.y +
                frame[i].pixel_size * 0.5;
            envi_hdr.xy_start[0] = 1;
            envi_hdr.xy_start[1] = 1;
            count = snprintf (envi_hdr.band_names[0],
                sizeof (envi_hdr.band_names[0]), "View azimuth angle");
            if (count < 0 || count >= sizeof (envi_hdr.band_names[0]))
            {
                sprintf (errmsg, "Overflow of envi_hdr.band_names[0]");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Write the ENVI header */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "%s_B%d_sensor_azimuth.hdr", outfile, i+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                sprintf (errmsg, "Overflow of tmpfile");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            if (write_envi_hdr (tmpfile, &envi_hdr) != SUCCESS)
            {
                sprintf (errmsg, "Writing the ENVI header file: %s.", tmpfile);
                error_handler (true, FUNC_NAME, errmsg);
                exit (ERROR);
            }
        }
    }
    else
    {
        /* Create the average Landsat 8 angle bands over the reflectance bands.
           Create a full resolution product with a fill value of -9999 to match
           the Landsat 8 image data. */
        if (l8_per_pixel_avg_refl_angles (ang_infile, 1, -9999, &avg_frame,
            &avg_solar_zenith, &avg_solar_azimuth, &avg_sat_zenith,
            &avg_sat_azimuth, &avg_nlines, &avg_nsamps) != SUCCESS)
        {  /* Error messages already written */
            exit (ERROR);
        }

        /** Write the solar zenith output angle **/
        printf ("Writing solar zenith band average angle ...\n");

        /* Open the output file for this angle */
        count = snprintf (tmpfile, sizeof (tmpfile),
            "%s_avg_solar_zenith.img", outfile);
        if (count < 0 || count >= sizeof (tmpfile))
        {
            sprintf (errmsg, "Overflow of tmpfile");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        fptr = open_raw_binary (tmpfile, "wb");
        if (!fptr)
        {
            sprintf (errmsg, "Unable to open the average solar zenith file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Write the data for this band */
        if (write_raw_binary (fptr, avg_nlines, avg_nsamps, sizeof (short),
            avg_solar_zenith) != SUCCESS)
        {
            sprintf (errmsg, "Unable to write to average solar zenith file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Close the file and free the pointer for this angle */
        close_raw_binary (fptr);
        free (avg_solar_zenith);

        /* Create the ENVI header */
        count = snprintf (envi_hdr.description,
            sizeof (envi_hdr.description), "Solar angle file");
        if (count < 0 || count >= sizeof (envi_hdr.description))
        {
            sprintf (errmsg, "Overflow of envi_hdr.description");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        envi_hdr.nlines = avg_nlines;
        envi_hdr.nsamps = avg_nsamps;
        envi_hdr.nbands = 1;
        envi_hdr.header_offset = 0;
        envi_hdr.byte_order = 0;

        count = snprintf (envi_hdr.file_type, sizeof (envi_hdr.file_type),
            "ENVI Standard");
        if (count < 0 || count >= sizeof (envi_hdr.file_type))
        {
            sprintf (errmsg, "Overflow of envi_hdr.file_type");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        envi_hdr.data_type = 2;
        envi_hdr.data_ignore_value = -9999;
        count = snprintf (envi_hdr.interleave, sizeof (envi_hdr.interleave),
            "BSQ");
        if (count < 0 || count >= sizeof (envi_hdr.interleave))
        {
            sprintf (errmsg, "Overflow of envi_hdr.interleave");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (envi_hdr.sensor_type,
            sizeof (envi_hdr.sensor_type), "Landsat OLI/TIRS");
        if (count < 0 || count >= sizeof (envi_hdr.sensor_type))
        {
            sprintf (errmsg, "Overflow of envi_hdr.interleave");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (avg_frame.projection.spheroid == WGS84_SPHEROID)
            envi_hdr.datum_type = ESPA_WGS84;
        else
        {
            sprintf (errmsg, "Unsupported datum. Currently only expect "
                "WGS84.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        if (avg_frame.projection.proj_code == UTM)
        {
            envi_hdr.proj_type = GCTP_UTM_PROJ;
            envi_hdr.utm_zone = avg_frame.projection.zone;
        }
        else if (avg_frame.projection.proj_code == PS)
        {
            envi_hdr.proj_type = GCTP_PS_PROJ;
        }
        else
        {
            sprintf (errmsg, "Unsupported projection. Currently only "
                "expect UTM or PS.");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        for (parm = 0; parm < IAS_PROJ_PARAM_SIZE; parm++)
            envi_hdr.proj_parms[parm] =
                avg_frame.projection.parameters[parm];
        envi_hdr.pixel_size[0] = avg_frame.pixel_size;
        envi_hdr.pixel_size[1] = avg_frame.pixel_size;
        envi_hdr.ul_corner[0] = avg_frame.ul_corner.x -
            avg_frame.pixel_size * 0.5;
        envi_hdr.ul_corner[1] = avg_frame.ul_corner.y +
            avg_frame.pixel_size * 0.5;
        envi_hdr.xy_start[0] = 1;
        envi_hdr.xy_start[1] = 1;
        count = snprintf (envi_hdr.band_names[0],
            sizeof (envi_hdr.band_names[0]), "Solar zenith angle");
        if (count < 0 || count >= sizeof (envi_hdr.band_names[0]))
        {
            sprintf (errmsg, "Overflow of envi_hdr.band_names[0]");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Write the ENVI header */
        count = snprintf (tmpfile, sizeof (tmpfile),
            "%s_avg_solar_zenith.hdr", outfile);
        if (count < 0 || count >= sizeof (tmpfile))
        {
            sprintf (errmsg, "Overflow of tmpfile");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (write_envi_hdr (tmpfile, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Writing the ENVI header file: %s.", tmpfile);
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /** Write the solar azimuth output angle **/
        printf ("Writing solar azimuth band average angle ...\n");

        /* Open the output file for this angle */
        count = snprintf (tmpfile, sizeof (tmpfile),
            "%s_avg_solar_azimuth.img", outfile);
        if (count < 0 || count >= sizeof (tmpfile))
        {
            sprintf (errmsg, "Overflow of tmpfile");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        fptr = open_raw_binary (tmpfile, "wb");
        if (!fptr)
        {
            sprintf (errmsg, "Unable to open the average solar azimuth file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Write the data for this band */
        if (write_raw_binary (fptr, avg_nlines, avg_nsamps, sizeof (short),
            avg_solar_azimuth) != SUCCESS)
        {
            sprintf (errmsg, "Unable to write to average solar azimuth file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Close the file and free the pointer for this angle */
        close_raw_binary (fptr);
        free (avg_solar_azimuth);

        /* Create the ENVI header.  Many of the fields needed have already been
           filled in above with the solar zenith fields. */
        count = snprintf (envi_hdr.band_names[0],
            sizeof (envi_hdr.band_names[0]), "Solar azimuth angle");
        if (count < 0 || count >= sizeof (envi_hdr.band_names[0]))
        {
            sprintf (errmsg, "Overflow of envi_hdr.band_names[0]");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Write the ENVI header */
        count = snprintf (tmpfile, sizeof (tmpfile),
            "%s_avg_solar_azimuth.hdr", outfile);
        if (count < 0 || count >= sizeof (tmpfile))
        {
            sprintf (errmsg, "Overflow of tmpfile");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (write_envi_hdr (tmpfile, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Writing the ENVI header file: %s.", tmpfile);
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /** Write the satellite zenith output angle **/
        printf ("Writing view zenith band average angle ...\n");

        /* Open the output file for this angle */
        count = snprintf (tmpfile, sizeof (tmpfile),
            "%s_avg_sensor_zenith.img", outfile);
        if (count < 0 || count >= sizeof (tmpfile))
        {
            sprintf (errmsg, "Overflow of tmpfile");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        fptr = open_raw_binary (tmpfile, "wb");
        if (!fptr)
        {
            sprintf (errmsg, "Unable to open the average sensor zenith file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Write the data for this band */
        if (write_raw_binary (fptr, avg_nlines, avg_nsamps, sizeof (short),
            avg_sat_zenith) != SUCCESS)
        {
            sprintf (errmsg, "Unable to write to average sensor zenith file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Close the file and free the pointer for this angle */
        close_raw_binary (fptr);
        free (avg_sat_zenith);

        /* Create the ENVI header.  Many of the fields needed have already been
           filled in above with the solar zenith fields. */
        count = snprintf (envi_hdr.description,
            sizeof (envi_hdr.description), "Satellite/View angle file");
        if (count < 0 || count >= sizeof (envi_hdr.description))
        {
            sprintf (errmsg, "Overflow of envi_hdr.description");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        count = snprintf (envi_hdr.band_names[0],
            sizeof (envi_hdr.band_names[0]), "View zenith angle");
        if (count < 0 || count >= sizeof (envi_hdr.band_names[0]))
        {
            sprintf (errmsg, "Overflow of envi_hdr.band_names[0]");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Write the ENVI header */
        count = snprintf (tmpfile, sizeof (tmpfile),
            "%s_avg_sensor_zenith.hdr", outfile);
        if (count < 0 || count >= sizeof (tmpfile))
        {
            sprintf (errmsg, "Overflow of tmpfile");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        if (write_envi_hdr (tmpfile, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Writing the ENVI header file: %s.", tmpfile);
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /** Write the satellite azimuth output angle **/
        printf ("Writing view azimuth band average angle ...\n");

        /* Open the output file for this angle */
        count = snprintf (tmpfile, sizeof (tmpfile),
            "%s_avg_sensor_azimuth.img", outfile);
        if (count < 0 || count >= sizeof (tmpfile))
        {
            sprintf (errmsg, "Overflow of tmpfile");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        fptr = open_raw_binary (tmpfile, "wb");
        if (!fptr)
        {
            sprintf (errmsg, "Unable to open the average sensor azimuth file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Write the data for this band */
        if (write_raw_binary (fptr, avg_nlines, avg_nsamps, sizeof (short),
            avg_sat_azimuth) != SUCCESS)
        {
            sprintf (errmsg, "Unable to write to average sensor azimuth file");
            error_handler (true, FUNC_NAME, errmsg);
            exit (ERROR);
        }

        /* Close the file and free the pointer for this angle */
        close_raw_binary (fptr);
        free (avg_sat_azimuth);

        /* Create the ENVI header.  Many of the fields needed have already been
           filled in above with the solar zenith fields. */
        count = snprintf (envi_hdr.band_names[0],
            sizeof (envi_hdr.band_names[0]), "View azimuth angle");
        if (count < 0 || count >= sizeof (envi_hdr.band_names[0]))
        {
            sprintf (errmsg, "Overflow of envi_hdr.band_names[0]");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Write the ENVI header */
        count = snprintf (tmpfile, sizeof (tmpfile),
            "%s_avg_sensor_azimuth.hdr", outfile);
        if (count < 0 || count >= sizeof (tmpfile))
        {
            sprintf (errmsg, "Overflow of tmpfile");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

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
