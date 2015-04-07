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
int process_parameters (char *angle_coeff_name, int subsamp_fact,
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
                return EXIT_FAILURE;
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
                return EXIT_FAILURE;
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
                return EXIT_FAILURE;
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
                return EXIT_FAILURE;
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
                    sat_azimuth[band_index][index] = (short)floor(r2d *
                        sat_angles[IAS_ANGLE_GEN_AZIMUTH_INDEX] + 0.5);
                if (sat_zenith)
                    sat_zenith[band_index][index] = (short)floor(r2d *
                        sat_angles[IAS_ANGLE_GEN_ZENITH_INDEX] + 0.5);
                if (solar_azimuth)
                    solar_azimuth[band_index][index] = (short)floor(r2d *
                        sun_angles[IAS_ANGLE_GEN_AZIMUTH_INDEX] + 0.5);
                if (solar_zenith)
                    solar_zenith[band_index][index] = (short)floor(r2d *
                        sun_angles[IAS_ANGLE_GEN_ZENITH_INDEX] + 0.5);
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
int process_parameters
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
