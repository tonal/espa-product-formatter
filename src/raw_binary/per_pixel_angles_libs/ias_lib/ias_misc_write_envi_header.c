
/* Standard C Includes */
#include <stdio.h>
#include <limits.h>
#include <string.h>

/* IAS Includes */
#include "ias_angle_gen_includes.h"
#include "config.h" /* This is for the endian define */
#include "ias_const.h"
#include "ias_types.h"
#include "ias_logging.h"
#include "ias_geo.h"
#include "ias_miscellaneous.h"

/*****************************************************************************
 NAME: ias_misc_write_envi_header

 PURPOSE: A simple routine to write an ENVI header for a specified image file.

 RETURN VALUE: Type = int
    Value    Description
    -------  ---------------------
    SUCCESS  Successful completion
    ERROR    Operation failed

 NOTES: The following are non-supported ENVI data types.
        6 = 2x32-bit complex, real-imaginary pair of double precision
        9 = 2x64-bit double precision complex, real-imaginary pair of double
                precision
        14 = 64-bit signed long integer
        15 = 64-bit unsigned long integer

        If projection information is provided, the upper left corner point is
        adjusted from the center of the pixel to the upper left of the pixel.

        The following ENVI file provides some information about ENVI
        projection formats:
            <path_to_envi>/envi48/map_proj/map_proj.txt

*****************************************************************************/
int ias_misc_write_envi_header
(
    const char *image_filename, /* I: Full path name of the image file */
    const IAS_PROJECTION *proj_info, /* I: Optional projection info, set to 
                                           NULL if not known or needed */
    const char *description,    /* I: Optional description, set to NULL if not 
                                      known or needed */
    int lines,                  /* I: Number of lines in the data */
    int samples,                /* I: Number of samples in the data */
    int bands,                  /* I: Number of bands in the data */
    double upper_left_x,        /* I: Optional upper-left X coordinate, set to 
                                      0.0 if not known or needed (requires
                                      proj_info) */
    double upper_left_y,        /* I: Optional upper-left Y coordinate, set to 
                                      0.0 if not known or needed (requires
                                      proj_info) */
    double projection_distance_x, /* I: Optional pixel size in X projection, 
                                     set to 0.0 if not known or needed (requires
                                     proj_info) */
    double projection_distance_y, /* I: Optional pixel size in Y projection,
                                     set to 0.0 if not known or needed (requires
                                     proj_info) */
    const char *band_names,     /* I: Optional single string for all band names,
                                      set to NULL if not known or needed */
    IAS_DATA_TYPE data_type     /* I: The IAS type of the data */
)
{
    char output_filename[PATH_MAX]; /* ENVI header filename */
    FILE *output_fd; /* File descriptor for the output file */
    int envi_data_type; /* The IAS_DATA_TYPE converted to the ENVI equivalent */
    int count;          /* The number of characters printed */
    int status;
    int byte_order;     

    /* Check to make sure image_filename is not NULL */
    if (image_filename == NULL)
    {
        IAS_LOG_ERROR("The image_filename is not correctly provided");
        return ERROR;
    }

    switch (data_type)
    {
        case IAS_BYTE:
            envi_data_type = 1;  /*  8-bit byte */
            break;
        case IAS_I2:
            envi_data_type = 2;  /* 16-bit signed integer */
            break;
        case IAS_I4:
            envi_data_type = 3;  /* 32-bit signed long integer */
            break;
        case IAS_R4:
            envi_data_type = 4;  /* 32-bit floating point */
            break;
        case IAS_R8:
            envi_data_type = 5;  /* 64-bit double precision floating point */
            break;
        case IAS_UI2:
            envi_data_type = 12; /* 16-bit unsigned integer */
            break;
        case IAS_UI4:
            envi_data_type = 13; /* 32-bit unsigned long integer */
            break;
        default:
            IAS_LOG_ERROR("Unsupported IAS_DATA_TYPE specified for ENVI"
                " header file creation %d", data_type);
            return ERROR;
    }

    /* HAVE_LITTLE_ENDIAN is defined in ias_lib/config.h */
    if (HAVE_LITTLE_ENDIAN)
        byte_order = 0;
    else
        byte_order = 1;

    /* Create output envi header filename */
    count = snprintf(output_filename, sizeof(output_filename),
            "%s.hdr", image_filename);
    if ((count < 0) || (count >= sizeof(output_filename)))
    {
        IAS_LOG_ERROR("Buffer for ENVI header filename requires %d "
                      "characters", count);
        return ERROR;
    }

    /* Create and write out the header file */
    output_fd = fopen(output_filename, "w");
    if (output_fd == NULL)
    {
        IAS_LOG_ERROR("Opening ENVI header file '%s' for writing",
            output_filename);
        return ERROR;
    }

    if (fprintf(output_fd, "ENVI\n") < 0)
    {
        IAS_LOG_ERROR("Writing ENVI header to: %s", output_filename);
        fclose(output_fd);
        return ERROR;
    }

    if (description != NULL)
    {
        status = fprintf(output_fd, "description = { %s }\n", 
                 description);
    }
    else
    {
        status = fprintf(output_fd, "description = { ENVI Header "
                 "for Image File %s}\n", image_filename);
    }
    if (status < 0)
    {
        IAS_LOG_ERROR("Writing ENVI header description for "
                 "header_file: %s", output_filename);
        fclose(output_fd);
        return ERROR;
    }

    status = fprintf(output_fd,
            "lines = %d\n"
            "samples = %d\n"
            "bands = %d\n"
            "header offset = 0\n"
            "file type = ENVI Standard\n"
            "data type = %d\n"
            "interleave = bsq\n"
            "byte order = %d\n"
            "x start = 0\n"
            "y start = 0\n",
            lines, samples, bands, envi_data_type, byte_order);
    if (status < 0)
    {
        IAS_LOG_ERROR("Writing ENVI header content for "
                      "header_file: %s",  output_filename);
        fclose(output_fd);
        return ERROR;
    }

    if (proj_info != NULL)
    {
        /* Adjust from the center of the pixel to the upper left corner */
        double corner_x = upper_left_x - (projection_distance_x / 2.0);
        double corner_y = upper_left_y + (projection_distance_y / 2.0);

        /* Only supporting UTM and PS map projections at this time */
        if (proj_info->proj_code == UTM)
        {
            char hemisphere[20];            
            char envi_proj[20];
            /* Always claim to be the northern hemisphere since we 
               aren't using the negative UTM zone code convention 
               for the southern hemisphere */
            strcpy(hemisphere, "North");
            strcpy(envi_proj, "UTM");

            if (fprintf(output_fd,"map info = {%s, 1.0, "
                "1.0, %f, %f, %f, %f, %d, %s, WGS-84, units=Meters}\n",
                envi_proj, corner_x, corner_y, 
                projection_distance_x, projection_distance_y, 
                proj_info->zone, hemisphere) < 0)
            {
                IAS_LOG_ERROR("Error writing ENVI header file %s", 
                              output_filename);
                fclose(output_fd);
                return ERROR;
            }
        }
        else if (proj_info->proj_code == PS)
        {
            char envi_proj[20];
            int envi_code = 31;
            double semi_major = proj_info->parameters[0];
            double semi_minor = proj_info->parameters[1];
            double standard_parallel_1;
            double central_meridian;
            double false_easting = proj_info->parameters[6];
            double false_northing = proj_info->parameters[7];

            status = ias_geo_convert_dms2deg(proj_info->parameters[5],
                &standard_parallel_1, "LAT");
            if (status != SUCCESS)
            {
                IAS_LOG_ERROR("Error writing ENVI header file %s", 
                              output_filename);
                fclose(output_fd);
                return ERROR;
            }

            status = ias_geo_convert_dms2deg(proj_info->parameters[4],
                &central_meridian, "LON");
            if (status != SUCCESS)
            {
                IAS_LOG_ERROR("Error writing ENVI header file %s", 
                              output_filename);
                fclose(output_fd);
                return ERROR;
            }

            strcpy(envi_proj, "Polar Stereographic");

            if (fprintf(output_fd,"map info = {%s, 1.0, "
                "1.0, %f, %f, %f, %f, WGS-84, units=Meters}\n",
                envi_proj, corner_x, corner_y, 
                projection_distance_x, projection_distance_y) < 0)
            {
                IAS_LOG_ERROR("Error writing ENVI header file %s", 
                              output_filename);
                fclose(output_fd);
                return ERROR;
            }

            /* Output the ENVI "required for PS" projection_info parameter */
            if (fprintf(output_fd,"projection info ="
                " {%d, %lf, %lf, %lf, %lf, %lf, %lf,"
                " WGS-84, Polar Stereographic, units=Meters}\n",
                envi_code,
                semi_major, semi_minor,
                standard_parallel_1, central_meridian,
                false_easting, false_northing) < 0)
            {
                IAS_LOG_ERROR("Error writing ENVI header file %s", 
                              output_filename);
                fclose(output_fd);
                return ERROR;
            }

        }
        else
        {
            IAS_LOG_WARNING("Projection is not supported, so not "
                "including projection information in the ENVI header file");
        }
    }

    /* Write optional band names */
    if (band_names != NULL)
    {
        status = fprintf(output_fd, "band names = {%s}\n", band_names); 
        if (status < 0)
        {
            IAS_LOG_ERROR("Writing ENVI header band names for header_file: %s",
                output_filename);
            fclose(output_fd);
            return ERROR;
        }
    }

    if (fclose(output_fd) != 0)
    {
        IAS_LOG_ERROR("Closing the ENVI header file: %s", 
                      output_filename);
        return ERROR;
    }

    return SUCCESS;
}

