/* Standard Library Includes */
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* IAS Library Includes */
#include "ias_logging.h"
#include "ias_const.h"
#include "ias_angle_gen_distro.h"
#include "ias_miscellaneous.h"

/* Local Includes */
#include "l8_angles.h"

/* Prototypes */
static int process_parameters (char *angle_coeff_name, int subsamp_fact,
    short fill_pix_value, char *band_list, L8_ANGLES_PARAMETERS *parameters);

/**************************************************************************
NAME: l8_per_pixel_angles

PURPOSE:   Uses the coefficients in the angle coefficients file to generate
the satellite viewing angle and/or solar angle values, for the specified
list of bands.

RETURN VALUE:
    Type = int
    Value           Description
    -----           -----------
    ERROR           An error occurred generating the per-pixel solar and/or
                    view angles
    SUCCESS         Angle band generation was successful

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
             Landsat team     Original development
4/3/2015     Gail Schmidt     Modified to work in the ESPA environment
5/5/2015     Gail Schmidt     Fixed a rounding bug that was incorrect for
                              negative values

NOTES:
  1. The band pointers for solar zenith/azimuth and satellite zenith/azimuth
     will have space allocated for the entire band
     (nlines * nsamps * sizeof (short)), unless NULL is passed in for the
     address.  Thus these pointers are an array of NBANDS pointers, where each
     pointer contains the angle values for the entire band as a 1D product.
  2. If any of the above pointers are NULL, then those per-pixel angles will
     not be calculated.
  3. It will be up to the calling routine to delete the memory allocated
     for these per band angle arrays.
  4. The angles that are returned are in degrees and have been scaled by 100.
***************************************************************************/
int l8_per_pixel_angles
(
    char *angle_coeff_name, /* I: Angle coefficient filename */
    int subsamp_fact,       /* I: Subsample factor used when calculating the
                                  angles (1=full resolution). OW take every Nth
                                  sample from the line, where N=subsamp_fact */
    short fill_pix_value,   /* I: Fill pixel value to use (-32768:32767) */
    char *band_list,        /* I: Band list used to calculate angles for.
                                  "ALL" - defaults to all bands 1 - 11.
                                  Must be comma separated with no spaces in
                                  between.  Example: 1,2,3,4,5,6,7,8,9
                                  The solar/sat_zenith/azimuth arrays should
                                  will have angles processed for these bands */
    ANGLES_FRAME frame[L8_NBANDS],   /* O: Image frame info for each band */
    short *solar_zenith[L8_NBANDS],  /* O: Array of pointers for the solar
                                           zenith angle array, one per band
                                           (if NULL, don't process), degrees
                                           scaled by 100 */
    short *solar_azimuth[L8_NBANDS], /* O: Array of pointers for the solar
                                           azimuth angle array, one per band
                                           (if NULL, don't process), degrees
                                           scaled by 100 */
    short *sat_zenith[L8_NBANDS],    /* O: Array of pointers for the satellite
                                           zenith angle array, one per band
                                           (if NULL, don't process), degrees
                                           scaled by 100 */
    short *sat_azimuth[L8_NBANDS],   /* O: Array of pointers for the satellite
                                           azimuth angle array, one per band
                                           (if NULL, don't process), degrees
                                           scaled by 100 */
    int nlines[L8_NBANDS],  /* O: Number of lines for each band, based on the
                                  subsample factor */
    int nsamps[L8_NBANDS]   /* O: Number of samples for each band, based on the
                                  subsample factor */
)
{
    int band_index;                   /* Metadata band index */
    int sub_sample;                   /* Subsampling factor */
    int num_lines;                    /* Lines in output angle band */
    int num_samps;                    /* Samps in output angle band */
    size_t angle_size;                /* Malloc angle size */
    L8_ANGLES_PARAMETERS parameters;  /* Parameters read in from file */
    IAS_ANGLE_GEN_METADATA metadata;  /* Angle metadata structure */ 
    char root_filename[PATH_MAX];     /* Root filename */
    char *base_ptr;                   /* Basename pointer */
    double r2d = 4500.0 / atan(1.0);  /* Conversion to hundredths of degrees */

    /* Make sure there is something to process */
    if (solar_zenith == NULL && solar_azimuth == NULL &&
        sat_zenith == NULL && sat_azimuth == NULL)
    {
        IAS_LOG_ERROR("Solar and Satellite zenith/azimuth pointer arrays are "
            "NULL. Nothing to process.");
        return ERROR;
    }

    /* Initialize the logging library */
    if (ias_log_initialize("L8 Angles") != SUCCESS)
    {
        IAS_LOG_ERROR("Error initializing logging library");
        return ERROR;
    }

    /* Initialize the satellite attributes */
    if (ias_sat_attr_initialize(IAS_L8) != SUCCESS)
    {
        IAS_LOG_ERROR("Initializing satellite attributes library");
        return ERROR;
    }

    /* Process the arguments */
    if (process_parameters(angle_coeff_name, subsamp_fact, fill_pix_value,
        band_list, &parameters) != SUCCESS)
    {
        IAS_LOG_ERROR("Invalid input parameters");
        return ERROR;
    }

    /* Setup local sub sampling factor variable */
    sub_sample = parameters.sub_sample_factor;

    /* Read the metadata file */
    if (ias_angle_gen_read_ang(parameters.metadata_filename, &metadata) 
        != SUCCESS)
    {
        IAS_LOG_ERROR("Reading the metadata file %s", 
            parameters.metadata_filename);
        return ERROR;
    }

    /* Extract the basename from the file path */
    base_ptr = strrchr(parameters.metadata_filename, '/');
    if (base_ptr)
    {
        base_ptr++; /* Move past the the last forward slash */
    }
    else
    {
        base_ptr = parameters.metadata_filename;
    }

    /* Check that the base buffer won't overflow the target buffer */
    if ((strlen(base_ptr) * sizeof(char)) > sizeof(root_filename))
    {
        IAS_LOG_ERROR("Angle coefficient filename too long");
        return ERROR;
    }
    strcpy(root_filename, base_ptr);

    /* Strip off the extension */
    base_ptr = strrchr(root_filename, '.');
    if (base_ptr)
    {
        *base_ptr = '\0'; /* Strip off the '.' by putting ending char */
    }

    /* Extract the root file name */
    base_ptr = strrchr(root_filename, '_');
    if (base_ptr)
    {
        *base_ptr = '\0'; /* Strip off the '_' by putting ending char */
    }

    /* Use the solar_azimuth, solar_zenith, sat_azimuth, sat_zenith arrays to
       determine the parameters.angle_type.  If either one of the azimuth or
       zenith angles are specified, then turn that angle type on. */
    parameters.angle_type = AT_UNKNOWN;
    if (solar_azimuth || solar_zenith)
    {
        if (sat_azimuth || sat_zenith)
            parameters.angle_type = AT_BOTH;
        else
            parameters.angle_type = AT_SOLAR;
    }
    else if (sat_azimuth || sat_zenith)
        parameters.angle_type = AT_SATELLITE;

    /* Process the angles for each band */
    for (band_index = 0; band_index < IAS_MAX_NBANDS; band_index++)
    {
        int line;                       /* Line index */
        int samp;                       /* Sample index */
        int index;                      /* Current sample index*/
        int tmp_percent;                /* Current percentage for printing
                                           status */
        int curr_tmp_percent;           /* Percentage for current line */
        IAS_MISC_LINE_EXTENT *trim_lut; /* Image trim lookup table */
        int band_number;                /* Band number */ 

        /* Retrieve the band number for current index */
        band_number = ias_sat_attr_convert_band_index_to_number(band_index);
        if (band_number == ERROR)
        {
            IAS_LOG_ERROR("Getting band number for band index %d", band_index);
            ias_angle_gen_free(&metadata);
            return ERROR;
        }

        /* Check if this band should be processed */
        if (!parameters.process_band[band_index])
            continue;

        /* Get framing information for this band if return is not successful
           then band is not present in metadata so continue */
        if (get_frame(&metadata, band_index, &frame[band_index]) != SUCCESS)
        {
            IAS_LOG_WARNING("Band not present in metadata for band number %d",
                band_number);
            continue;
        }

        /* Calculate size of subsampled output image */
        num_lines = (frame[band_index].num_lines - 1) / sub_sample + 1;
        num_samps = (frame[band_index].num_samps - 1) / sub_sample + 1;
        IAS_LOG_INFO("Processing band number %d using %d as subsampling "
            "factor", band_number, sub_sample);

        /* Calculate the angle sizes */
        nlines[band_index] = num_lines;
        nsamps[band_index] = num_samps;
        angle_size = num_lines * num_samps * sizeof(short);

        /* Allocate the satellite buffers if needed */
        if (sat_zenith != NULL)
        {
            sat_zenith[band_index] = (short *)malloc(angle_size);
            if (!sat_zenith[band_index])
            {
                IAS_LOG_ERROR("Allocating satellite zenith angle array for "
                    "band number %d", band_number);
                ias_angle_gen_free(&metadata);
                return ERROR;
            }
        }

        if (sat_azimuth != NULL)
        {
            sat_azimuth[band_index] = (short *)malloc(angle_size);
            if (!sat_azimuth[band_index])
            {
                IAS_LOG_ERROR("Allocating satellite azimuth angle array for "
                    "band number %d", band_number);
                ias_angle_gen_free(&metadata);
                return ERROR;
            }
        }

        /* Allocate the solar buffers if needed */
        if (solar_zenith != NULL)
        {
            solar_zenith[band_index] = (short *)malloc(angle_size);
            if (!solar_zenith[band_index])
            {
                IAS_LOG_ERROR("Allocating solar zenith angle array for band "
                    "number %d", band_number);
                ias_angle_gen_free(&metadata);
                return ERROR;
            }
        }

        if (solar_azimuth != NULL)
        {
            solar_azimuth[band_index] = (short *)malloc(angle_size);
            if (!solar_azimuth[band_index])
            {
                IAS_LOG_ERROR("Allocating solar azimuth angle array for band "
                    "number %d", band_number);
                ias_angle_gen_free(&metadata);
                return ERROR;
            }
        }

        /* Retrieve the trim look up table to remove the scene crenulation */
        trim_lut = ias_misc_create_output_image_trim_lut(
            get_active_lines(&metadata, band_index), 
            get_active_samples(&metadata, band_index),
            frame[band_index].num_lines, frame[band_index].num_samps);
        if (!trim_lut)
        {
            IAS_LOG_ERROR("Creating the scene trim lookup table for band "
                "number %d", band_number);
            ias_angle_gen_free(&metadata);
            return ERROR;
        }

        /* Loop through the L1T lines and samples, and just fill everything
           with fill. */
        if (sat_zenith)
        {
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                    sat_zenith[band_index][index] = parameters.background;
        }

        if (sat_azimuth)
        {
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                    sat_azimuth[band_index][index] = parameters.background;
        }

        if (solar_zenith)
        {
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                    solar_zenith[band_index][index] = parameters.background;
        }

        if (solar_azimuth)
        {
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                    solar_azimuth[band_index][index] =
                        parameters.background;
        }

        /* Loop through the L1T lines and samples */
        tmp_percent = 0;
        for (line = 0, index = 0; line < frame[band_index].num_lines;
             line += sub_sample)
        {
            double sun_angles[2];   /* Solar angles */
            double sat_angles[2];   /* Viewing angles */

            /* update status? */
            curr_tmp_percent = 100 * line / frame[band_index].num_lines;
            if (curr_tmp_percent > tmp_percent)
            {
                tmp_percent = curr_tmp_percent;
                if (tmp_percent % 10 == 0)
                {
                    printf ("%d%% ", tmp_percent);
                    fflush (stdout);
                }
            }

            for (samp = 0; samp < frame[band_index].num_samps;
                 samp += sub_sample, index++)
            {
                /* If the current sample falls outside the actual range of
                   image data in this scene, then goto the next pixel.  Fill
                   pixels are already handled. */
                if (samp <= trim_lut[line].start_sample || 
                    samp >= trim_lut[line].end_sample)
                {
                    continue;
                }

                /* Calculate the satellite and solar azimuth and zenith */
                if (calculate_angles (&metadata, line, samp, band_index, 
                    parameters.angle_type, sat_angles, sun_angles) != SUCCESS)
                {
                    IAS_LOG_ERROR("Evaluating angles in band %d", band_number);
                    free(trim_lut);
                    ias_angle_gen_free(&metadata);
                    return ERROR;
                }

                /* Quantize the angles by converting from radians to degrees 
                   and scaling by a factor of 100 so it can be stored in the
                   short integer image */
                if (sat_azimuth)
                    sat_azimuth[band_index][index] = (short) round (r2d *
                        sat_angles[IAS_ANGLE_GEN_AZIMUTH_INDEX]);
                if (sat_zenith)
                    sat_zenith[band_index][index] = (short) round (r2d *
                        sat_angles[IAS_ANGLE_GEN_ZENITH_INDEX]);
                if (solar_azimuth)
                    solar_azimuth[band_index][index] = (short) round (r2d *
                        sun_angles[IAS_ANGLE_GEN_AZIMUTH_INDEX]);
                if (solar_zenith)
                    solar_zenith[band_index][index] = (short) round (r2d *
                        sun_angles[IAS_ANGLE_GEN_ZENITH_INDEX]);
            }  /* for samp */
        }  /* for line */

        /* update status */
        printf ("100%%\n");
        fflush (stdout);

        /* Free the lookup table */
        free(trim_lut);
        trim_lut = NULL;
    }  /* for band */

    /* Release the metadata */
    ias_angle_gen_free(&metadata);

    return SUCCESS;
}


#define WRITE_BAND_DIFF_FROM_AVG 1
/**************************************************************************
NAME: l8_per_pixel_avg_refl_angles

PURPOSE:   Uses the coefficients in the angle coefficients file to generate
the satellite viewing angle and/or solar angle values, for the specified
list of bands.  The average of the reflective bands will be calculated, on a
per-pixel basis, and returned.

RETURN VALUE:
    Type = int
    Value           Description
    -----           -----------
    ERROR           An error occurred generating the per-pixel solar and/or
                    view angles
    SUCCESS         Angle band generation was successful

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
5/3/2015     Gail Schmidt     Original development

NOTES:
  1. The band pointers for solar zenith/azimuth and satellite zenith/azimuth
     averages will have space allocated for the entire band
     (nlines * nsamps * sizeof (short)), unless NULL is passed in for the
     address.
  2. If any of the above pointers are NULL, then those per-pixel angles will
     not be calculated.
  3. It will be up to the calling routine to delete the memory allocated
     for this reflectance band average angle array.
  4. The angles that are returned are in degrees and have been scaled by 100.
***************************************************************************/
int l8_per_pixel_avg_refl_angles
(
    char *angle_coeff_name, /* I: Angle coefficient filename */
    int subsamp_fact,       /* I: Subsample factor used when calculating the
                                  angles (1=full resolution). OW take every Nth
                                  sample from the line, where N=subsamp_fact */
    short fill_pix_value,   /* I: Fill pixel value to use (-32768:32767) */
    ANGLES_FRAME *avg_frame,  /* O: Image frame info for the scene */
    short **avg_solar_zenith, /* O: Addr of pointer for the average solar zenith
                                    angle array (if NULL, don't process),
                                    degrees scaled by 100 */
    short **avg_solar_azimuth,/* O: Addr of pointer for the average solar
                                    azimuth angle array (if NULL, don't
                                    process), degrees scaled by 100 */
    short **avg_sat_zenith,   /* O: Addr of pointer for the average satellite
                                    zenith angle array (if NULL, don't process),
                                    degrees scaled by 100 */
    short **avg_sat_azimuth,  /* O: Addr of pointer for the average satellite
                                    azimuth angle array (if NULL, don't
                                    process), degrees scaled by 100 */
    int *avg_nlines,          /* O: Number of lines for the bands, based on
                                    on the subsample factor */
    int *avg_nsamps           /* O: Number of samples for the bands, based on
                                    the subsample factor */
)
{
    char tmpfile[1024];               /* temporary filename for band diffs */
    int count;                        /* number of chars copied in snprintf */
    FILE *fptr=NULL;                  /* file pointer for band diffs */
    int band_index;                   /* band index */
    int sub_sample;                   /* subsampling factor */
    int line;                         /* line index */
    int samp;                         /* sample index */
    int index;                        /* current sample index */
    int angle_size;                   /* memory alloc angle size */
    int center_pixel;                 /* location of the scene center pixel */
    int min, max;                     /* minimum & maximum differences for the
                                         current band (across all pixels)
                                         between the band average and band
                                         angle */
    int min4, max4;                   /* minimum & maximum differences between
                                         band 4 (across all pixels) and the
                                         current band angle */
    short *bdiff = NULL;              /* array to hold the difference of the
                                         average from the actual band angle */
    short bdiff4;                     /* variable to hold the difference of
                                         band 4 from the actual band angle */
    ushort *pix_count = NULL;         /* array to carry the count of the
                                         non-zero pixels used for the sum */
    long *sum = NULL;                 /* array to carry the sum of the angles
                                         for all the bands */
    ANGLES_FRAME frame[L8_NBANDS];    /* image frame info for each band */
    short *solar_zenith[L8_NBANDS];   /* array of pointers for the solar zenith
                                         angle array, one per reflectance band.
                                         Degrees scaled by 100 */
    short *solar_azimuth[L8_NBANDS];  /* array of pointers for the solar
                                         azimuth angle array, one per
                                         reflectance band. Degrees scaled by
                                         100 */
    short *sat_zenith[L8_NBANDS];     /* array of pointers for the satellite
                                         zenith angle array, one per
                                         reflectance band. Degrees scaled by
                                         100 */
    short *sat_azimuth[L8_NBANDS];    /* array of pointers for the satellite
                                         azimuth angle array, one per
                                         reflectance band. Degrees scaled by
                                         100 */
    int BAND4 = 3;                    /* index for band 4 */
    char refl_band_list[] = "1,2,3,4,5,6,7,9"; /* list of reflectance bands to
                                         be used in the average */
    bool process_band[L8_NBANDS] = {true, true, true, true, true, true, true,
        false /* pan */, true, false /* band 10 */, false /* band 11 */};
                                         /* which bands will be processed as
                                            the average of reflectance bands */
    int nlines[L8_NBANDS];  /* number of lines for each band, based on the
                               subsample factor */
    int nsamps[L8_NBANDS];  /* number of samples for each band, based on the
                               subsample factor */

    /* Create the Landsat 8 angle bands for the reflectance bands.  Create a
       sub_sample product with a fill value of -9999 to match the Landsat 8
       image data.  This call also allocates memory for the solar angles and
       satellite/view angles for the reflectance band list. */
    sub_sample = subsamp_fact;
    if (l8_per_pixel_angles (angle_coeff_name, sub_sample, -9999,
        refl_band_list, frame, solar_zenith, solar_azimuth, sat_zenith,
        sat_azimuth, nlines, nsamps) != SUCCESS)
    {  /* Error messages already written */
        IAS_LOG_ERROR("Creating the per-pixel angles for the reflective bands");
        return ERROR;
    }

    /* Allocate memory for the angle bands.  Just use the first band (band 1)
       for the size of the band. */
    angle_size = nlines[0] * nsamps[0];
    if (avg_solar_zenith != NULL)
    {
        *avg_solar_zenith = calloc (angle_size, sizeof (short));
        if (*avg_solar_zenith == NULL)
        {
            IAS_LOG_ERROR("Allocating average solar zenith angle array");
            return ERROR;
        }
    }

    if (avg_solar_azimuth != NULL)
    {
        *avg_solar_azimuth = calloc (angle_size, sizeof (short));
        if (*avg_solar_azimuth == NULL)
        {
            IAS_LOG_ERROR("Allocating average solar azimuth angle array");
            return ERROR;
        }
    }

    if (avg_sat_zenith != NULL)
    {
        *avg_sat_zenith = calloc (angle_size, sizeof (short));
        if (*avg_sat_zenith == NULL)
        {
            IAS_LOG_ERROR("Allocating average satellite zenith angle array");
            return ERROR;
        }
    }

    if (avg_sat_azimuth != NULL)
    {
        *avg_sat_azimuth = calloc (angle_size, sizeof (short));
        if (*avg_sat_azimuth == NULL)
        {
            IAS_LOG_ERROR("Allocating average satellite azimuth angle array");
            return ERROR;
        }
    }

    /* Allocate memory to hold the sum of the angles from all the bands.
       Just use the first band (band 1) for the size of the band.  Also keep
       an array for counting the non-zero pixels in the sum which will be used
       to compute the average. */
    angle_size = nlines[0] * nsamps[0];
    sum = calloc (angle_size, sizeof (long));
    if (sum == NULL)
    {
        IAS_LOG_ERROR("Allocating array for holding the sum of the bands");
        return ERROR;
    }

    pix_count = calloc (angle_size, sizeof (ushort));
    if (pix_count == NULL)
    {
        IAS_LOG_ERROR("Allocating array for holding the pixel count");
        return ERROR;
    }

    if (avg_sat_zenith != NULL)
    {
        /* Loop through the reflectance bands and compute the average of these
           bands. Some of the pixels on the scene edge will be zero and throws
           off the average, so don't count the zero values. */
        printf ("Computing average for satellite zenith ...\n");
        for (band_index = 0; band_index < L8_NBANDS; band_index++)
        {
            /* Check if this band be added to the average */
            if (!process_band[band_index])
                continue;

            /* Add this band to the sum.  Skip values of 0 since they occur on
               the scene edges and we don't want to count them. */
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                    if (sat_zenith[band_index][index] != 0)
                    {
                        sum[index] += sat_zenith[band_index][index];
                        pix_count[index]++;
                    }

#ifndef WRITE_BAND_DIFF_FROM_AVG
            /* Release the memory */
            free (sat_zenith[band_index]);
#endif
        }

        /* Get the average */
        band_index = 0;
        for (line = 0, index = 0; line < frame[band_index].num_lines;
             line += sub_sample)
            for (samp = 0; samp < frame[band_index].num_samps;
                 samp += sub_sample, index++)
                (*avg_sat_zenith)[index] =
                    (short) (round ((float) sum[index] / pix_count[index]));
    }

    if (avg_sat_azimuth != NULL)
    {
        /* Clear the sum and pixel count arrays back to 0 */
        memset (sum, 0, angle_size * sizeof (long));
        memset (pix_count, 0, angle_size * sizeof (ushort));

        /* Loop through the reflectance bands and compute the average of these
           bands. Some of the pixels on the scene edge will be zero and throws
           off the average, so don't count the zero values. */
        printf ("Computing average for satellite azimuth ...\n");
        for (band_index = 0; band_index < L8_NBANDS; band_index++)
        {
            /* Check if this band be added to the average */
            if (!process_band[band_index])
                continue;

            /* Add this band to the sum */
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                    if (sat_azimuth[band_index][index] != 0)
                    {
                        sum[index] += sat_azimuth[band_index][index];
                        pix_count[index]++;
                    }

#ifndef WRITE_BAND_DIFF_FROM_AVG
            /* Release the memory */
            free (sat_azimuth[band_index]);
#endif
        }

        /* Get the average */
        band_index = 0;
        for (line = 0, index = 0; line < frame[band_index].num_lines;
             line += sub_sample)
            for (samp = 0; samp < frame[band_index].num_samps;
                 samp += sub_sample, index++)
                (*avg_sat_azimuth)[index] =
                    (short) (round ((float) sum[index] / pix_count[index]));
    }

    if (avg_solar_zenith != NULL)
    {
        /* Clear the sum and pixel count arrays back to 0 */
        memset (sum, 0, angle_size * sizeof (long));
        memset (pix_count, 0, angle_size * sizeof (ushort));

        /* Loop through the reflectance bands and compute the average of these
           bands. Some of the pixels on the scene edge will be zero and throws
           off the average, so don't count the zero values. */
        printf ("Computing average for solar zenith ...\n");
        for (band_index = 0; band_index < L8_NBANDS; band_index++)
        {
            /* Check if this band be added to the average */
            if (!process_band[band_index])
                continue;

            /* Add this band to the sum */
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                    if (solar_zenith[band_index][index] != 0)
                    {
                        sum[index] += solar_zenith[band_index][index];
                        pix_count[index]++;
                    }

#ifndef WRITE_BAND_DIFF_FROM_AVG
            /* Release the memory */
            free (solar_zenith[band_index]);
#endif
        }

        /* Get the average */
        band_index = 0;
        for (line = 0, index = 0; line < frame[band_index].num_lines;
             line += sub_sample)
            for (samp = 0; samp < frame[band_index].num_samps;
                 samp += sub_sample, index++)
            {
                (*avg_solar_zenith)[index] =
                    (short) (round ((float) sum[index] / pix_count[index]));
            }
    }

    if (avg_solar_azimuth != NULL)
    {
        /* Clear the sum and pixel count arrays back to 0 */
        memset (sum, 0, angle_size * sizeof (long));
        memset (pix_count, 0, angle_size * sizeof (ushort));

        /* Loop through the reflectance bands and compute the average of these
           bands. Some of the pixels on the scene edge will be zero and throws
           off the average, so don't count the zero values. */
        printf ("Computing average for solar azimuth ...\n");
        for (band_index = 0; band_index < L8_NBANDS; band_index++)
        {
            /* Check if this band be added to the average */
            if (!process_band[band_index])
                continue;

            /* Add this band to the sum */
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                    if (solar_azimuth[band_index][index] != 0)
                    {
                        sum[index] += solar_azimuth[band_index][index];
                        pix_count[index]++;
                    }

#ifndef WRITE_BAND_DIFF_FROM_AVG
            /* Release the memory */
            free (solar_azimuth[band_index]);
#endif
        }

        /* Get the average */
        band_index = 0;
        for (line = 0, index = 0; line < frame[band_index].num_lines;
             line += sub_sample)
            for (samp = 0; samp < frame[band_index].num_samps;
                 samp += sub_sample, index++)
                (*avg_solar_azimuth)[index] =
                    (short) (round ((float) sum[index] / pix_count[index]));
    }

    /* Free memory */
    free (sum);
    free (pix_count);

#ifdef WRITE_BAND_DIFF_FROM_AVG
    /* Allocate memory to hold the difference from the average for each band.
       Just use the first band (band 1) for the size of the band. */
    bdiff = calloc (angle_size, sizeof (short));
    if (bdiff == NULL)
    {
        IAS_LOG_ERROR("Allocating array for holding the difference from the "
            "average for each band.");
        return ERROR;
    }

    /* Write out the difference for each band from the band average */
    center_pixel = (nlines[0]/2) * nsamps[0] + (nsamps[0]/2);
    if (avg_sat_zenith != NULL)
    {
        /* Loop through the reflectance bands and compute the difference from
           the average for each of these bands */
        for (band_index = 0; band_index < L8_NBANDS; band_index++)
        {
            /* Check if this band be added to the average */
            if (!process_band[band_index])
                continue;

            /* Compute the differences, but skip the scene edge pixels with
               an angle value of 0 */
            min = min4 = 65536;
            max = max4 = -65536;
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                {
                    if (sat_zenith[band_index][index] != 0 &&
                        sat_zenith[BAND4][index] != 0)
                    {
                        bdiff[index] = (*avg_sat_zenith)[index] -
                            sat_zenith[band_index][index];
                        bdiff4 = sat_zenith[BAND4][index] -
                            sat_zenith[band_index][index];
                    }
                    else
                    {
                        bdiff[index] = 0;
                        bdiff4 = 0;
                    }

                    if (bdiff[index] < min)
                        min = bdiff[index];
                    if (bdiff[index] > max)
                        max = bdiff[index];

                    if (bdiff4 < min4)
                        min4 = bdiff4;
                    if (bdiff4 > max4)
                        max4 = bdiff4;
                }

            /* Print the stats for this band */
            printf ("*** Band %d -- Satellite Zenith ***\n", band_index+1);
            printf ("    average at scene center: %d\n",
                (*avg_sat_zenith)[center_pixel]);
            printf ("    angle at scene center: %d\n",
                sat_zenith[band_index][center_pixel]);
            printf ("    minimum difference from band avg: %d\n", min);
            printf ("    maximum difference from band avg: %d\n", max);
            printf ("    minimum difference from band4: %d\n", min4);
            printf ("    maximum difference from band4: %d\n", max4);

            /* Release the memory */
            if (band_index != BAND4)
                free (sat_zenith[band_index]);

            /* Open the difference file for this band */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "satellite_zenith_band_diff_from_avg_B%d.img", band_index+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                IAS_LOG_ERROR("Overflow of tmpfile");
                return ERROR;
            }

            fptr = open_raw_binary (tmpfile, "wb");
            if (!fptr)
            {
                IAS_LOG_ERROR("Unable to open the satellite zenith band "
                    "difference file");
                return ERROR;
            }
    
            /* Write the data for this band.  Just use nlines/nsamps for the
               first band since that's the assumption we've used all along. */
            if (write_raw_binary (fptr, nlines[0], nsamps[0], sizeof (short),
                bdiff) != SUCCESS)
            {
                IAS_LOG_ERROR("Unable to write to the band difference file");
                return ERROR;
            }
    
            /* Close the file */
            close_raw_binary (fptr);
        }  /* for band_index */
    }

    if (avg_sat_azimuth != NULL)
    {
        /* Loop through the reflectance bands and compute the difference from
           the average for each of these bands */
        for (band_index = 0; band_index < L8_NBANDS; band_index++)
        {
            /* Check if this band be added to the average */
            if (!process_band[band_index])
                continue;

            /* Compute the differences, but skip the scene edge pixels with
               an angle value of 0 */
            min = min4 = 65536;
            max = max4 = -65536;
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                {
                    if (sat_azimuth[band_index][index] != 0 &&
                        sat_azimuth[BAND4][index] != 0)
                    {
                        bdiff[index] = (*avg_sat_azimuth)[index] -
                            sat_azimuth[band_index][index];
                        bdiff4 = sat_azimuth[BAND4][index] -
                            sat_azimuth[band_index][index];
                    }
                    else
                    {
                        bdiff[index] = 0;
                        bdiff4 = 0;
                    }

                    if (bdiff[index] < min)
                        min = bdiff[index];
                    if (bdiff[index] > max)
                        max = bdiff[index];

                    if (bdiff4 < min4)
                        min4 = bdiff4;
                    if (bdiff4 > max4)
                        max4 = bdiff4;
                }

            /* Print the stats for this band */
            printf ("*** Band %d -- Satellite Azimuth ***\n", band_index+1);
            printf ("    average at scene center: %d\n",
                (*avg_sat_azimuth)[center_pixel]);
            printf ("    angle at scene center: %d\n",
                sat_azimuth[band_index][center_pixel]);
            printf ("    minimum difference from band avg: %d\n", min);
            printf ("    maximum difference from band avg: %d\n", max);
            printf ("    minimum difference from band4: %d\n", min4);
            printf ("    maximum difference from band4: %d\n", max4);

            /* Release the memory */
            if (band_index != BAND4)
                free (sat_azimuth[band_index]);

            /* Open the difference file for this band */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "satellite_azimuth_band_diff_from_avg_B%d.img", band_index+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                IAS_LOG_ERROR("Overflow of tmpfile");
                return ERROR;
            }

            fptr = open_raw_binary (tmpfile, "wb");
            if (!fptr)
            {
                IAS_LOG_ERROR("Unable to open the satellite azimuth band "
                    "difference file");
                return ERROR;
            }
    
            /* Write the data for this band.  Just use nlines/nsamps for the
               first band since that's the assumption we've used all along. */
            if (write_raw_binary (fptr, nlines[0], nsamps[0], sizeof (short),
                bdiff) != SUCCESS)
            {
                IAS_LOG_ERROR("Unable to write to the band difference file");
                return ERROR;
            }
    
            /* Close the file */
            close_raw_binary (fptr);
        }  /* for band_index */
    }

    if (avg_solar_zenith != NULL)
    {
        /* Loop through the reflectance bands and compute the difference from
           the average for each of these bands */
        for (band_index = 0; band_index < L8_NBANDS; band_index++)
        {
            /* Check if this band be added to the average */
            if (!process_band[band_index])
                continue;

            /* Compute the differences, but skip the scene edge pixels with
               an angle value of 0 */
            min = min4 = 65536;
            max = max4 = -65536;
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                {
                    if (solar_zenith[band_index][index] != 0 &&
                        solar_zenith[BAND4][index] != 0)
                    {
                        bdiff[index] = (*avg_solar_zenith)[index] -
                            solar_zenith[band_index][index];
                        bdiff4 = solar_zenith[BAND4][index] -
                            solar_zenith[band_index][index];
                    }
                    else
                    {
                        bdiff[index] = 0;
                        bdiff4 = 0;
                    }

                    if (bdiff[index] < min)
                        min = bdiff[index];
                    if (bdiff[index] > max)
                        max = bdiff[index];

                    if (bdiff4 < min4)
                        min4 = bdiff4;
                    if (bdiff4 > max4)
                        max4 = bdiff4;
                }

            /* Print the stats for this band */
            printf ("*** Band %d -- Solar Zenith ***\n", band_index+1);
            printf ("    average at scene center: %d\n",
                (*avg_solar_zenith)[center_pixel]);
            printf ("    angle at scene center: %d\n",
                solar_zenith[band_index][center_pixel]);
            printf ("    minimum difference from band avg: %d\n", min);
            printf ("    maximum difference from band avg: %d\n", max);
            printf ("    minimum difference from band4: %d\n", min4);
            printf ("    maximum difference from band4: %d\n", max4);

            /* Release the memory */
            if (band_index != BAND4)
                free (solar_zenith[band_index]);

            /* Open the difference file for this band */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "solar_zenith_band_diff_from_avg_B%d.img", band_index+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                IAS_LOG_ERROR("Overflow of tmpfile");
                return ERROR;
            }

            fptr = open_raw_binary (tmpfile, "wb");
            if (!fptr)
            {
                IAS_LOG_ERROR("Unable to open the solar zenith band "
                    "difference file");
                return ERROR;
            }
    
            /* Write the data for this band.  Just use nlines/nsamps for the
               first band since that's the assumption we've used all along. */
            if (write_raw_binary (fptr, nlines[0], nsamps[0], sizeof (short),
                bdiff) != SUCCESS)
            {
                IAS_LOG_ERROR("Unable to write to the band difference file");
                return ERROR;
            }
    
            /* Close the file */
            close_raw_binary (fptr);
        }  /* for band_index */
    }

    if (avg_solar_azimuth != NULL)
    {
        /* Loop through the reflectance bands and compute the difference from
           the average for each of these bands */
        for (band_index = 0; band_index < L8_NBANDS; band_index++)
        {
            /* Check if this band be added to the average */
            if (!process_band[band_index])
                continue;

            /* Compute the differences, but skip the scene edge pixels with
               an angle value of 0 */
            min = min4 = 65536;
            max = max4 = -65536;
            for (line = 0, index = 0; line < frame[band_index].num_lines;
                 line += sub_sample)
                for (samp = 0; samp < frame[band_index].num_samps;
                     samp += sub_sample, index++)
                {
                    if (solar_azimuth[band_index][index] != 0 &&
                        solar_azimuth[BAND4][index] != 0)
                    {
                        bdiff[index] = (*avg_solar_azimuth)[index] -
                            solar_azimuth[band_index][index];
                        bdiff4 = solar_azimuth[BAND4][index] -
                            solar_azimuth[band_index][index];
                    }
                    else
                    {
                        bdiff[index] = 0;
                        bdiff4 = 0;
                    }

                    if (bdiff[index] < min)
                        min = bdiff[index];
                    if (bdiff[index] > max)
                        max = bdiff[index];

                    if (bdiff4 < min4)
                        min4 = bdiff4;
                    if (bdiff4 > max4)
                        max4 = bdiff4;
                }

            /* Print the stats for this band */
            printf ("*** Band %d -- Solar Azimuth ***\n", band_index+1);
            printf ("    average at scene center: %d\n",
                (*avg_solar_azimuth)[center_pixel]);
            printf ("    angle at scene center: %d\n",
                solar_azimuth[band_index][center_pixel]);
            printf ("    minimum difference from band avg: %d\n", min);
            printf ("    maximum difference from band avg: %d\n", max);
            printf ("    minimum difference from band4: %d\n", min4);
            printf ("    maximum difference from band4: %d\n", max4);

            /* Release the memory */
            if (band_index != BAND4)
                free (solar_azimuth[band_index]);

            /* Open the difference file for this band */
            count = snprintf (tmpfile, sizeof (tmpfile),
                "solar_azimuth_band_diff_from_avg_B%d.img", band_index+1);
            if (count < 0 || count >= sizeof (tmpfile))
            {
                IAS_LOG_ERROR("Overflow of tmpfile");
                return ERROR;
            }

            fptr = open_raw_binary (tmpfile, "wb");
            if (!fptr)
            {
                IAS_LOG_ERROR("Unable to open the solar azimuth band "
                    "difference file");
                return ERROR;
            }
    
            /* Write the data for this band.  Just use nlines/nsamps for the
               first band since that's the assumption we've used all along. */
            if (write_raw_binary (fptr, nlines[0], nsamps[0], sizeof (short),
                bdiff) != SUCCESS)
            {
                IAS_LOG_ERROR("Unable to write to the band difference file");
                return ERROR;
            }
    
            /* Close the file */
            close_raw_binary (fptr);
        }  /* for band_index */
    }

    /* Free memory */
    free (bdiff);
    free (sat_zenith[BAND4]);
    free (sat_azimuth[BAND4]);
    free (solar_zenith[BAND4]);
    free (solar_azimuth[BAND4]);
#endif

    /* Populate the band average frame using the first band (band 1) from the
       band frames */
    *avg_frame = frame[0];
    *avg_nlines = nlines[0];
    *avg_nsamps = nsamps[0];

    return SUCCESS;
}

/******************************************************************************
NAME: process_parameters

PURPOSE: This fuction processes the l8_angles parameters to specify which
angles to process and some of the processing information.

RETURN VALUE: Type = int
    Value     Description
    -----     -----------
    SUCCESS   The parameters were successfully processed
    ERROR     An error occurred processing the parameters
******************************************************************************/
static int process_parameters
(
    char *angle_coeff_name,     /* I: Angle coefficient filename */
    int subsamp_fact,           /* I: Subsample factor used when calculating
                                      the angles (1=full resolution). OW take
                                      every Nth sample from the line, where
                                      N=subsamp_fact */
    short fill_pix_value,       /* I: Fill pixel value to use (-32768:32767) */
    char *band_list,            /* I: Band list used to calculate angles for.
                                      "ALL" - defaults to all bands 1 - 11.
                                      Must be comma separated with no spaces in
                                      between.  Example: 1,2,3,4,5,6,7,8,9 */
    L8_ANGLES_PARAMETERS *parameters /* O: Generation parameters */
)
{
    int local_band_count;     /* Number of bands read */
    int local_band_numbers[IAS_MAX_NBANDS] = {ERROR}; /* Read in band numbers */
    int index;                /* Generic loop variable */
    int band_list_init_value; /* Band list initialize value */
    int band_already_in_list[IAS_MAX_NBANDS];
    int status;

    /* Copy the angle coefficient filename */
    status = snprintf(parameters->metadata_filename, 
        sizeof(parameters->metadata_filename), "%s", angle_coeff_name); 
    if (status < 0 || status >= sizeof(parameters->metadata_filename))
    {
        IAS_LOG_ERROR("Copying the metadata filename");
        return ERROR;
    }   

    /* Parse the sub sample factor */
    parameters->sub_sample_factor = subsamp_fact;

    /* Parse the fill pixel value */
    parameters->background = fill_pix_value;

    /* Parse the band list */
    band_list_init_value = 1;
    local_band_count = 0;

    /* Parse the fill pixel value and band list */
    int offset = 0;
    int character_count = 0;
    int band_length = strlen(band_list);

    /* Check for the use of ALL in the band list.  If ALL was specified then
       process bands 1-11.  Otherwise, parse the band list provided. */
    if ((!strcmp (band_list, "ALL")) || (!strcmp (band_list, "all")))
    {
        for (index = 0; index < IAS_MAX_NBANDS; index++)
            parameters->process_band[index] = 1;
    }
    else
    {
        /* Read in the band list */
        while (character_count <= band_length)
        {
            if (sscanf(band_list, "%d%n", &local_band_numbers[local_band_count],
                &offset) < 1)
            {
                IAS_LOG_ERROR("Parsing the band list");
                return ERROR;
            }

            character_count += offset + 1;
            band_list += offset + 1; /* Move pointer ahead the number length 
                                         + 1 (for the comma) */
            local_band_count++;
            if (local_band_count > IAS_MAX_NBANDS)
            {
                IAS_LOG_ERROR("Too many bands specified");
                return ERROR;   
            }
        }

        /* Initialize band list */
        band_list_init_value = 0;
        for (index = 0; index < IAS_MAX_NBANDS; index++)
        {
            parameters->process_band[index] = band_list_init_value;
            band_already_in_list[index] = band_list_init_value;
        }

        for (index = 0; index < local_band_count; index++)
        {
            /* Convert the band number entered into a band index */
            int temp_band_index = ias_sat_attr_convert_band_number_to_index(    
                local_band_numbers[index]);
            if (temp_band_index == ERROR)
            {
                /* No bands numbers entered, so it is an error */
                IAS_LOG_ERROR("Illegal band number %d",
                    local_band_numbers[index]);
                return ERROR;
            }

            /* Make sure the band isn't already in the list */
            if (temp_band_index != ERROR)
            {
                if (!band_already_in_list[temp_band_index])
                {
                    /* Put band in list and flag that it has been added to
                       the list */
                    parameters->process_band[temp_band_index] = 1;
                    band_already_in_list[temp_band_index] += 1;
                }
                else if (band_already_in_list[temp_band_index] == 1)
                {
                     /* Only print the warning message once for each band */
                     band_already_in_list[temp_band_index] += 1;

                     IAS_LOG_WARNING("Warning: Band number %d appears multiple "
                         "times in the BAND_LIST, only the first one was used",
                         local_band_numbers[index]);
                }
            }
        }
    }

    return SUCCESS;
}
