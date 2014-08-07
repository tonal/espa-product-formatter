/*****************************************************************************
FILE: convert_modis_to_espa.c
  
PURPOSE: Contains functions for reading MODIS HDF products and writing to ESPA
raw binary format.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/22/2014    Gail Schmidt     Original development

NOTES:
  1. The XML metadata format written via this library follows the ESPA internal
     metadata format found in ESPA Raw Binary Format v1.0.doc.  The schema for
     the ESPA internal metadata format is available at
     http://espa.cr.usgs.gov/static/schema/espa_internal_metadata_v1_0.xsd.
*****************************************************************************/
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include "convert_modis_to_espa.h"

/******************************************************************************
MODULE:  doy_to_month_day

PURPOSE: Convert the DOY to month and day.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error converting from DOY to month and day
SUCCESS         Successfully converted from the DOY to the month and day

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/30/2014    Gail Schmidt     Original development

NOTES:
******************************************************************************/
int doy_to_month_day
(
    int year,            /* I: year of the DOY to be converted */
    int doy,             /* I: DOY to be converted */
    int *month,          /* O: month of the DOY */
    int *day             /* O: day of the DOY */
)
{
    char FUNC_NAME[] = "doy_to_month_day";  /* function name */
    char errmsg[STR_SIZE];    /* error message */
    bool leap;                /* is this a leap year? */
    int i;                    /* looping variable */
    int nday_lp[12] = {31, 29, 31, 30,  31,  30,  31,  31,  30,  31,  30,  31};
        /* number of days in each month (for leap year) */
    int idoy_lp[12] = { 1, 32, 61, 92, 122, 153, 183, 214, 245, 275, 306, 336};
        /* starting DOY for each month (for leap year) */
    int nday[12] = {31, 28, 31, 30,  31,  30,  31,  31,  30,  31,  30,  31};
        /* number of days in each month (with Feb being a leap year) */
    int idoy[12] = { 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
        /* starting DOY for each month */

    /* Is this a leap year? */
    leap = (bool) (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

    /* Determine which month the DOY falls in */
    *month = 0;
    if (leap)
    {  /* leap year -- start with February */
        for (i = 1; i < 12; i++)
        {
            if (idoy_lp[i] > doy)
            {
                *month = i;
                *day = doy - idoy_lp[i-1] + 1;
                break;
            }
        }

        /* if the month isn't set, then it's a December scene */
        if (*month == 0)
        {
            *month = 12;
            *day = doy - idoy_lp[11] + 1;
        }
    }
    else
    {  /* non leap year -- start with February */
        for (i = 1; i < 12; i++)
        {
            if (idoy[i] > doy)
            {
                *month = i;
                *day = doy - idoy[i-1] + 1;
                break;
            }
        }

        /* if the month isn't set, then it's a December scene */
        if (*month == 0)
        {
            *month = 12;
            *day = doy - idoy[11] + 1;
        }
    }

    /* Validate the month and day */
    if (*month < 1 || *month > 12)
    {
        sprintf (errmsg, "Invalid month: %d\n", *month);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    if (leap)
    {  /* leap year */
        if (*day < 1 || *day > nday_lp[(*month)-1])
        {
            sprintf (errmsg, "Invalid day: %d-%d-%d\n", year, *month, *day);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else
    {  /* non leap year */
        if (*day < 1 || *day > nday[(*month)-1])
        {
            sprintf (errmsg, "Invalid day: %d-%d-%d\n", year, *month, *day);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Successful conversion */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  cleanup_file_name

PURPOSE:  Cleans up the filenames by replacing blank spaces in the filename
          with underscores.

RETURN VALUE:
Type = None

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
5/8/2014    Gail Schmidt      Original development

NOTES:
******************************************************************************/
void cleanup_file_name
(
    char instr[]         /* I: input string to be cleaned up */
)

{
    char *inptr = instr;     /* pointer to the input string */

    while (*inptr != '\0')
    {
        /* Change ' ' to '_' */
        if (*inptr == ' ')
            *inptr = '_';

        /* Next character */
        inptr++;
    }
}


/******************************************************************************
MODULE:  cleanup_qa_desc

PURPOSE:  Cleans up the QA description string by replacing the '>' and '<'
          characters.  These mess up the XML attribute fields and therefore
          need to be removed/modified.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Final QA string exceeds str_size
SUCCESS         Successfully processed the string

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
5/8/2014    Gail Schmidt      Original development

NOTES:
******************************************************************************/
int cleanup_qa_desc
(
    char instr[],         /* I: input string to be cleaned up */
    int str_size,         /* I: size of the output string */
    char outstr[]         /* O: output string */
)

{
    char *inptr = NULL;       /* pointer to the input string */
    char *outptr = NULL;      /* pointer to the output string */

    inptr = instr;
    outptr = outstr;
    while (*inptr != '\0')
    {
        /* Change '<=' to 'le' */
        if (!strncmp (inptr, "<=", 2))
        {
            outptr[0] = 'l';
            outptr[1] = 'e';
            outptr += 2;
            inptr += 2;
        }

        /* Change '>=' to 'ge' */
        else if (!strncmp (inptr, ">=", 2))
        {
            outptr[0] = 'g';
            outptr[1] = 'e';
            outptr += 2;
            inptr += 2;
        }

        /* Change '<' to 'lt' */
        else if (*inptr == '<')
        {
            outptr[0] = 'l';
            outptr[1] = 't';
            outptr += 2;
            inptr++;
        }

        /* Change '>' to 'gt' */
        else if (*inptr == '>')
        {
            outptr[0] = 'g';
            outptr[1] = 't';
            outptr += 2;
            inptr++;
        }

        /* Copy the character as-is */
        else
        {
            *outptr = *inptr;
            outptr++;
            inptr++;
        }
    }

    /* Terminate with the end of string character */
    *outptr = '\0'; 

    /* Make sure the size isn't too large for the output string */
    if (strlen (outstr) > str_size)
        return (ERROR);

    return (SUCCESS);
}


/******************************************************************************
MODULE:  get_sds_values

PURPOSE:  Reads SDS metadata values like valid range, background fill, etc.
          These are read from HDF-EOS file as HDF attributes.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error reading the SDS values
SUCCESS         Successfully read the SDS value information

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/30/2014    Gail Schmidt     Original development (pulled some code from MRT)

NOTES:
******************************************************************************/
int get_sds_values
(
    int32 sd_id,             /* I: file ID for the HDF file */
    int band,                /* I: band/SDS to be read (0-based) */
    int32 datatype,          /* I: datatype of this SDS */
    double *scalevalue,      /* O: value of scale factor */
    double *offsetvalue,     /* O: band offset value */
    double *minvalue,        /* O: minimum band value */
    double *maxvalue,        /* O: minimum band value */
    double *fillvalue,       /* O: fill value for this SDS */
    char longname[],         /* O: SDS long name */
    char units[],            /* O: units of the SDS */
    char qa_desc[]           /* O: qa_desc of the SDS */
)

{
    char FUNC_NAME[] = "get_sds_values";  /* function name */
    char errmsg[STR_SIZE];    /* error message */
    char tmpstr[HUGE_STR_SIZE]; /* temporary string for reading attributes */
    int count;                /* number of chars copied in snprintf */
    int32 sds_id = -1;        /* ID for the current SDS */
    int32 attr_indx = -1;     /* index for the current attribute */
    int32 range_status, fill_status;  /* return status values */
    int32 status;             /* return status values */
    int32 num_type;           /* data type - not used */
    int32 attr_size;          /* size of the current attribute */
    double cal, cal_error, offset, offset_error;  /* error values */

    /* Specific variable types to replace the void * types */
    unsigned char ucmin_value, ucmax_value, ucbackground_fill;
    char cmin_value, cmax_value, cbackground_fill;
    unsigned short usmin_value, usmax_value, usbackground_fill;
    short smin_value, smax_value, sbackground_fill;
    unsigned int umin_value, umax_value, ubackground_fill;
    int imin_value, imax_value, ibackground_fill;
    float fmin_value, fmax_value, fbackground_fill;

    /* Initialize the SDS attribute values to fill */
    *scalevalue = ESPA_FLOAT_META_FILL;
    *offsetvalue = ESPA_FLOAT_META_FILL;
    *minvalue = ESPA_FLOAT_META_FILL;
    *maxvalue = ESPA_FLOAT_META_FILL;
    *fillvalue = ESPA_FLOAT_META_FILL;
    strcpy (longname, ESPA_STRING_META_FILL);
    strcpy (units, ESPA_STRING_META_FILL);
    strcpy (qa_desc, ESPA_STRING_META_FILL);

    /* Open the specified SDS/band */
    sds_id = SDselect (sd_id, band);
    if (sds_id < 0)
    {
        sprintf (errmsg, "Unable to select SDS band %i", band );
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Get scale factor and offset */
    status = SDgetcal (sds_id, &cal, &cal_error, &offset, &offset_error,
        &num_type);
    if (status != -1)
    {
        *scalevalue = cal;
        *offsetvalue = offset;
    }
    else
    {
        /* Try the scale_factor alone */
        attr_indx = SDfindattr (sds_id, "scale_factor");
        if (attr_indx != -1)
        {
            /* Read the attribute value */
            status = SDreadattr (sds_id, attr_indx, &cal);
            if (status != -1)
                *scalevalue = cal;
        }
    }

    /* Sometimes MODIS conflicts itself with the scale_factor.  Sometimes it's
       reported as 1/scale and other times it's listed as the scale value.  To
       be consistent, if the scale_factor is greater than 1, then we'll divide
       one by the factor to get a fractional value. */
    if (*scalevalue > 1.0)
        *scalevalue = 1.0 / *scalevalue;

    /* Get valid range and background fill */
    switch (datatype)
    {
        case DFNT_INT8:
            /* Get range and background fill value */
            range_status = SDgetrange (sds_id, (void *) &cmax_value,
                (void *) &cmin_value);
            fill_status = SDgetfillvalue (sds_id, (void *) &cbackground_fill);

            /* Check for missing valid range values */
            if (range_status != -1)
            {
                *minvalue = cmin_value;
                *maxvalue = cmax_value;
            }

            /* Check for missing background fill value */
            if (fill_status != -1)
                *fillvalue = cbackground_fill;
            break;

        case DFNT_UINT8:
            /* Get range and background fill value */
            range_status = SDgetrange (sds_id, (void *) &ucmax_value,
                (void *) &ucmin_value);
            fill_status = SDgetfillvalue (sds_id, (void *) &ucbackground_fill);

            /* Check for missing valid range values */
            if (range_status != -1)
            {
                *minvalue = (double) ucmin_value;
                *maxvalue = (double) ucmax_value;
            }

            /* Check for missing background fill value */
            if (fill_status != -1)
                *fillvalue = (double) ucbackground_fill;
            break;

        case DFNT_INT16:
            /* Get range and background fill value */
            range_status = SDgetrange (sds_id, (void *) &smax_value,
                (void *) &smin_value);
            fill_status = SDgetfillvalue (sds_id, (void *) &sbackground_fill);

            /* Check for missing valid range values */
            if (range_status != -1)
            {
                *minvalue = smin_value;
                *maxvalue = smax_value;
            }

            /* Check for missing background fill value */
            if ( fill_status != -1 )
                *fillvalue = sbackground_fill;
            break;

        case DFNT_UINT16:
            /* Get range and background fill value */
            range_status = SDgetrange (sds_id, (void *) &usmax_value,
                (void *) &usmin_value);
            fill_status = SDgetfillvalue (sds_id, (void *) &usbackground_fill);

            /* Check for missing valid range values */
            if (range_status != -1)
            {
                *minvalue = (double) usmin_value;
                *maxvalue = (double) usmax_value;
            }

            /* Check for missing background fill value */
            if (fill_status != -1)
                *fillvalue = (double) usbackground_fill;
            break;

        case DFNT_INT32:
            /* Get range and background fill value */
            range_status = SDgetrange (sds_id, (void *) &imax_value,
                (void *) &imin_value);
            fill_status = SDgetfillvalue (sds_id, (void *) &ibackground_fill);

            /* Check for missing valid range values */
            if (range_status != -1)
            {
                *minvalue = imin_value;
                *maxvalue = imax_value;
            }

            /* Check for missing background fill value */
            if (fill_status != -1)
                *fillvalue = ibackground_fill;
            break;

        case DFNT_UINT32:
            /* Get range and background fill value */
            range_status = SDgetrange (sds_id, (void *) &umax_value,
                (void *) &umin_value);
            fill_status = SDgetfillvalue (sds_id, (void *) &ubackground_fill);

            /* Check for missing valid range values */
            if (range_status != -1)
            {
                *minvalue = (double) umin_value;
                *maxvalue = (double) umax_value;
            }

            /* Check for missing background fill value */
            if (fill_status != -1)
                *fillvalue = (double) ubackground_fill;
            break;

        case DFNT_FLOAT32:
            /* Get range and background fill value */
            range_status = SDgetrange (sds_id, (void *) &fmax_value,
                (void *) &fmin_value);
            fill_status = SDgetfillvalue (sds_id, (void *) &fbackground_fill);

            /* Check for missing valid range values */
            if (range_status != -1)
            {
                *minvalue = fmin_value;
                *maxvalue = fmax_value;
            }

            /* Check for missing background fill value */
            if (fill_status != -1)
                *fillvalue = fbackground_fill;
            break;

        default:
            sprintf (errmsg, "Unsupported MODIS HDF data type: %d", datatype);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
    }

    /* Get the long name */
    attr_indx = SDfindattr (sds_id, "long_name");
    if (attr_indx != -1)
    {
        /* Use SDattrinfo to get the size of the attr string */
        status = SDattrinfo (sds_id, attr_indx, tmpstr, &num_type, &attr_size);
        if (status != 0)
        {
            sprintf (errmsg, "Cannot obtain attribute info for long_name");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Read the attribute string */
        status = SDreadattr (sds_id, attr_indx, tmpstr);
        if (status != -1)
        {
            tmpstr[attr_size] = '\0';
            count = snprintf (longname, STR_SIZE, "%s", tmpstr);
            if (count < 0 || count >= STR_SIZE)
            {
                sprintf (errmsg, "Overflow of longname string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }

    /* Get the units */
    attr_indx = SDfindattr (sds_id, "units");
    if (attr_indx != -1)
    {
        /* Use SDattrinfo to get the size of the attr string */
        status = SDattrinfo (sds_id, attr_indx, tmpstr, &num_type, &attr_size);
        if (status != 0)
        {
            sprintf (errmsg, "Cannot obtain attribute info for units");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Read the attribute string */
        status = SDreadattr (sds_id, attr_indx, tmpstr);
        if (status != -1)
        {
            tmpstr[attr_size] = '\0';
            count = snprintf (units, STR_SIZE, "%s", tmpstr);
            if (count < 0 || count >= STR_SIZE)
            {
                sprintf (errmsg, "Overflow of units string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }

    /* Get the QA description via 'QA bitmap index', 'QA index', or 'Legend' */
    attr_indx = SDfindattr (sds_id, "QA bitmap index");
    if (attr_indx == -1)
    {
        attr_indx = SDfindattr (sds_id, "QA index");
        if (attr_indx == -1)
            attr_indx = SDfindattr (sds_id, "Legend");
    }

    if (attr_indx != -1)
    {
        /* Use SDattrinfo to get the size of the attr string */
        status = SDattrinfo (sds_id, attr_indx, tmpstr, &num_type, &attr_size);
        if (status != 0)
        {
            sprintf (errmsg, "Cannot obtain attribute info for 'QA bitmap "
                "index', 'QA index', or 'Legend'.");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Read the attribute string */
        status = SDreadattr (sds_id, attr_indx, tmpstr);
        if (status != -1)
        {
            tmpstr[attr_size] = '\0';

            /* The QA description cannot have '<' or '>' as it screws up the
               XML file and tags.  Therefore loop through the string and copy
               the "fixed" string to the qa description for the XML. */
            status = cleanup_qa_desc (tmpstr, HUGE_STR_SIZE, qa_desc);
            if (status != SUCCESS)
            {
                sprintf (errmsg, "Overflow of qa_desc string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
        }
    }

    /* Terminate access to the SDS */
    SDendaccess (sds_id);

    return (SUCCESS);
}


/******************************************************************************
MODULE:  read_core_metadata

PURPOSE: Reads the core metadata, searching for the desired fields.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error reading the core metadata
SUCCESS         Successfully read the core metadata

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
5/6/2014     Gail Schmidt     Original development

NOTES:
******************************************************************************/
int read_core_metadata
(
    int32 sd_id,              /* I: file ID for the HDF file */
    char prod_date_time[],    /* O: production date/time */
    char pge_version[]        /* O: PGE version */
)
{
    char FUNC_NAME[] = "read_core_metadata";  /* function name */
    char errmsg[STR_SIZE];      /* error message */
    char attr_name[STR_SIZE];   /* attribute name */
    char attrname[STR_SIZE];    /* holds the file_name string */
    char *file_data = NULL;     /* character string used for reading the
                                   CoreMetadata */
    char *file_data_ptr = NULL; /* pointer to file_data for scanning */
    int j;                  /* looping variable */
    int count;              /* number of chars copied in snprintf */
    int32 attr_indx = -1;   /* index for the current attribute */
    int32 data_type;        /* attribute's data type */
    int32 n_values;         /* number of vals of the attribute */
    int32 status;           /* return status */
    bool prod_date_found;   /* production date/time has been found */
    bool pge_version_found; /* PGE version has been found */
    int num_chars;          /* number of characters read in the line */
    char token_buffer[STR_SIZE]; /* holds the current token */

    /* Look for the CoreMetadata in the HDF file */
    attr_indx = SDfindattr (sd_id, "CoreMetadata");

    /* Only proceed if the attribute was found */
    if (attr_indx == -1)
    {
        /* If not found then try concatenating sequence numbers */
        for (j = 0; j <= 9; j++)
        {
            sprintf (attrname, "CoreMetadata.%d", j);
            attr_indx = SDfindattr (sd_id, attrname);
            if (attr_indx != -1)
                break;
        }

        if (attr_indx == -1)
        {
            sprintf (errmsg, "Unable to locate CoreMetadata for reading");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Get size of HDF file attribute */
    status = SDattrinfo (sd_id, attr_indx, attr_name, &data_type, &n_values);
    if (status == -1)
    {
        sprintf (errmsg, "Unable to get the size of CoreMetadata attribute");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Attempt to allocate memory for HDF file attribute contents (add one
       character for the end of string character) */
    file_data = calloc (n_values+1, sizeof (char));
    if (file_data == NULL)
    {
        sprintf (errmsg, "Unable to allocate %d bytes for %s", n_values,
            attr_name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Read attribute from the HDF file */
    status = SDreadattr (sd_id, attr_indx, file_data);
    if (status == -1 || !strcmp (file_data, ""))
    {
        free (file_data);
        sprintf (errmsg, "Unable to read the CoreMetadata HDF attributes");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Walk through the file_data string one token at a time looking for the
       production date and PGE version */
    prod_date_found = false;
    pge_version_found = false;
    file_data_ptr = file_data;
    status = ERROR;
    while (sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars) != EOF)
    {
        /* If this token is END, then we are done with the metadata */
        if (!strcmp (token_buffer, "END"))
            break;

        /* If the tokens have already been found, don't waste time with the
           rest of the metadata */
        if (prod_date_found && pge_version_found)
            break;

        /* Increment the file_data_ptr pointer to point to the next token */
        file_data_ptr += num_chars;

        /* Look for the PRODUCTIONDATETIME token */
        if (!strcmp (token_buffer, "PRODUCTIONDATETIME") && !prod_date_found)
        {
            /* Read the next three tokens, this should be the
               NUM_VAL = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Read the next three tokens, this should be the
               VALUE = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Store the value token (without the surrounding ""s) */
            count = snprintf (prod_date_time, STR_SIZE, "%s", &token_buffer[1]);
            if (count < 0 || count >= STR_SIZE)
            {
                sprintf (errmsg, "Overflow of prod_date_time string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            prod_date_time[strlen(prod_date_time)-1] = '\0';

            /* Production date/time was found */
            prod_date_found = true;
        }

        /* Look for the PGEVERSION token */
        if (!strcmp (token_buffer, "PGEVERSION") && !pge_version_found)
        {
            /* Read the next three tokens, this should be the
               NUM_VAL = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Read the next three tokens, this should be the
               VALUE = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Store the value token (without the surrounding ""s) */
            count = snprintf (pge_version, STR_SIZE, "%s", &token_buffer[1]);
            if (count < 0 || count >= STR_SIZE)
            {
                sprintf (errmsg, "Overflow of pge_version string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            pge_version[strlen(pge_version)-1] = '\0';

            /* PGE version was found */
            pge_version_found = true;
        }
    }  /* end while */

    /* Free dynamically allocated memory */
    free (file_data);

    return (SUCCESS);
}


/******************************************************************************
MODULE:  read_archive_metadata

PURPOSE: Reads the archive metadata, searching for the desired fields.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error reading the archive metadata
SUCCESS         Successfully read the archive metadata

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
5/6/2014     Gail Schmidt     Original development

NOTES:
******************************************************************************/
int read_archive_metadata
(
    int32 sd_id,              /* I: file ID for the HDF file */
    double *bound_coords      /* O: bounding coordinates */
)
{
    char FUNC_NAME[] = "read_archive_metadata";  /* function name */
    char errmsg[STR_SIZE];      /* error message */
    char attr_name[STR_SIZE];   /* attribute name */
    char attrname[STR_SIZE];    /* holds the file_name string */
    char *file_data = NULL;     /* character string used for reading the
                                   ArchiveMetadata */
    char *file_data_ptr = NULL; /* pointer to file_data for scanning */
    int j;                  /* looping variable */
    int32 attr_indx = -1;   /* index for the current attribute */
    int32 data_type;        /* attribute's data type */
    int32 n_values;         /* number of vals of the attribute */
    int32 status;           /* return status */
    bool north_bound_found; /* north bounding rectangle coord been found? */
    bool south_bound_found; /* south bounding rectangle coord been found? */
    bool east_bound_found;  /* east bounding rectangle coord been found? */
    bool west_bound_found;  /* west bounding rectangle coord been found? */
    int num_chars;          /* number of characters read in the line */
    char token_buffer[STR_SIZE]; /* holds the current token */

    /* Look for the ArchiveMetadata in the HDF file */
    attr_indx = SDfindattr (sd_id, "ArchiveMetadata");

    /* Only proceed if the attribute was found */
    if (attr_indx == -1)
    {
        /* If not found then try concatenating sequence numbers */
        for (j = 0; j <= 9; j++)
        {
            sprintf (attrname, "ArchiveMetadata.%d", j);
            attr_indx = SDfindattr (sd_id, attrname);
            if (attr_indx != -1)
                break;
        }

        if (attr_indx == -1)
        {
            sprintf (errmsg, "Unable to locate ArchiveMetadata for reading");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Get size of HDF file attribute */
    status = SDattrinfo (sd_id, attr_indx, attr_name, &data_type, &n_values);
    if (status == -1)
    {
        sprintf (errmsg, "Unable to get the size of ArchiveMetadata attribute");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Attempt to allocate memory for HDF file attribute contents (add one
       character for the end of string character) */
    file_data = calloc (n_values+1, sizeof (char));
    if (file_data == NULL)
    {
        sprintf (errmsg, "Unable to allocate %d bytes for %s", n_values,
            attr_name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Read attribute from the HDF file */
    status = SDreadattr (sd_id, attr_indx, file_data);
    if (status == -1 || !strcmp (file_data, ""))
    {
        free (file_data);
        sprintf (errmsg, "Unable to read the ArchiveMetadata HDF attributes");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Walk through the file_data string one token at a time looking for the
       bounding coordinates */
    north_bound_found = false;
    south_bound_found = false;
    east_bound_found = false;
    west_bound_found = false;
    file_data_ptr = file_data;
    status = ERROR;
    while (sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars) != EOF)
    {
        /* If this token is END, then we are done with the metadata */
        if (!strcmp (token_buffer, "END"))
            break;

        /* If all the bounding coords have been found, don't waste time with
           the rest of the metadata */
        if (north_bound_found && south_bound_found && east_bound_found &&
            west_bound_found)
            break;

        /* Increment the file_data_ptr pointer to point to the next token */
        file_data_ptr += num_chars;

        /* Look for the NORTHBOUNDINGCOORDINATE token */
        if (!strcmp (token_buffer, "NORTHBOUNDINGCOORDINATE") &&
            !north_bound_found)
        {
            /* Read the next three tokens, this should be the
               NUM_VAL = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Read the next three tokens, this should be the
               VALUE = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Store the value of the bounding coordinate */
            bound_coords[ESPA_NORTH] = atof (token_buffer);
            north_bound_found = true;
            continue;  /* don't waste time looking for the other
                          bounding rectangles with this token */
        }

        /* Look for the SOUTHBOUNDINGCOORDINATE token */
        if (!strcmp (token_buffer, "SOUTHBOUNDINGCOORDINATE") &&
            !south_bound_found)
        {
            /* Read the next three tokens, this should be the
               NUM_VAL = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Read the next three tokens, this should be the
               VALUE = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Store the value of the bounding coordinate */
            bound_coords[ESPA_SOUTH] = atof (token_buffer);
            south_bound_found = true;
            continue;  /* don't waste time looking for the other
                          bounding rectangles with this token */
        }

        /* Look for the EASTBOUNDINGCOORDINATE token */
        if (!strcmp (token_buffer, "EASTBOUNDINGCOORDINATE") &&
            !east_bound_found)
        {
            /* Read the next three tokens, this should be the
               NUM_VAL = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Read the next three tokens, this should be the
               VALUE = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Store the value of the bounding coordinate */
            bound_coords[ESPA_EAST] = atof (token_buffer);
            east_bound_found = true;
            continue;  /* don't waste time looking for the other
                          bounding rectangles with this token */
        }

        /* Look for the WESTBOUNDINGCOORDINATE token */
        if (!strcmp (token_buffer, "WESTBOUNDINGCOORDINATE") &&
            !west_bound_found)
        {
            /* Read the next three tokens, this should be the
               NUM_VAL = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Read the next three tokens, this should be the
               VALUE = ... line */
            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            sscanf (file_data_ptr, "%s%n", token_buffer, &num_chars);
            file_data_ptr += num_chars;

            /* Store the value of the bounding coordinate */
            bound_coords[ESPA_WEST] = atof (token_buffer);
            west_bound_found = true;
            continue;  /* don't waste time looking for the other
                          bounding rectangles with this token */
        }
    }  /* end while */

    /* Free dynamically allocated memory */
    free (file_data);

    return (SUCCESS);
}


/******************************************************************************
MODULE:  read_modis_hdf

PURPOSE: Read the metadata from the MODIS HDF file and populate the ESPA
internal metadata structure

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error reading the MODIS file
SUCCESS         Successfully populated the ESPA metadata structure

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/22/2014    Gail Schmidt     Original development

NOTES:
******************************************************************************/
int read_modis_hdf
(
    char *modis_hdf_name,            /* I: name of MODIS file to be read */
    Espa_internal_meta_t *metadata   /* I/O: input metadata structure to be
                                           populated from the MODIS file */
)
{
    char FUNC_NAME[] = "read_modis_hdf";  /* function name */
    char errmsg[STR_SIZE];    /* error message */
    char basename[STR_SIZE];  /* filename without path (uppercase) */
    char core_basename[STR_SIZE]; /* filename without path and extension */
    char strbuf[STR_SIZE];    /* temporary buffer to string data */
    char gridstr[STR_SIZE];   /* list of comma-separated grids in HDF file */
    char fieldstr[STR_SIZE];  /* list of comma-separated fields in HDF file */
    char prod_date_time[STR_SIZE];  /* production date/time */
    char pge_version[STR_SIZE];     /* PGE version */
    char grid_names[MAX_MODIS_GRIDS][STR_SIZE];  /* array of grid names vs.
                                 comma-separated list of grids */
    char modis_bands[MAX_MODIS_BANDS][STR_SIZE]; /* array containing names of
                                 the MODIS bands/SDSs to be written to the
                                 ESPA XML file */
    char longname[MAX_MODIS_BANDS][STR_SIZE]; /* array long_name attributes */
    char qa_desc[MAX_MODIS_BANDS][HUGE_STR_SIZE]; /* array qa description
                                                     attributes */
    char units[MAX_MODIS_BANDS][STR_SIZE];    /* array units attributes */
    char yearstr[5];          /* string to hold the acquisition year */
    char doystr[4];           /* string to hold the acquisition DOY */
    char htile[3];            /* string to hold the horiztonal tile */
    char vtile[3];            /* string to hold the vertical tile */
    char *cptr = NULL;        /* character pointer for strings */
    char *gridname = NULL;    /* pointer to the current grid name */
    char *gridend = NULL;     /* pointer to end of current grid name */
    char *gridlist = NULL;    /* list of grids in the HDF file */
    char *fieldname = NULL;   /* pointer to the current field name */
    char *fieldend = NULL;    /* pointer to end of current field name */
    char *fieldlist = NULL;   /* list of fields in the HDF file */
    char *dimname = NULL;     /* pointer to the current dimension name */

    int i, j, k;              /* looping variables */
    int count;                /* number of chars copied in snprintf */
    int acq_doy;              /* acquisition DOY */
    int acq_year;             /* acquisition year */
    int acq_month;            /* acquisition month */
    int acq_day;              /* acquisition day */
    int nmodis_bands;         /* number of bands that will be in the ESPA
                                 product from the MODIS file */
    int sdsfield = 0;         /* current SDS field to be processed */
    int32 fid;                /* file ID for the HDF-EOS file */
    int32 gid;                /* grid ID for the HDF-EOS file */
    int32 sd_id;              /* file ID for the HDF file */
    int32 ngrids;             /* number of grids in the HDF-EOS file */
    int32 nfields;            /* number of fields/SDSs in the current grid */
    int32 rank;               /* rank for the current SDS */
    int32 dtype;              /* datatype for the current SDS */
    int32 bufsize;            /* size of the returned buffer */
    int32 status;             /* return status */
    int32 projcode;           /* projection code */
    int32 zonecode;           /* UTM zone code */
    int32 spherecode;         /* sphere code */
    int32 origincode;         /* grid origin for corner points */
    int32 xdimsize;           /* x-dimension */
    int32 ydimsize;           /* y-dimension */
    int32 tmprank[256];       /* temporary array of dimensions */
    int32 tmpdtype[256];    /* temporary array of data types */
    int32 dims[MAX_MODIS_DIMS]; /* dimensions read from the SDS */
    int32 grid_dims[MAX_MODIS_GRIDS][2];  /* x,y dimensions of current grid */
    int32 data_type[MAX_MODIS_BANDS];     /* data type for each SDS */
    double projparm[15];      /* projection parameters */
    double central_meridian;  /* central meridian for the sinusoidal projection
                                 (in DMS) */
    double scalevalue[MAX_MODIS_BANDS];    /* scale factor for current SDS */
    double offsetvalue[MAX_MODIS_BANDS];   /* offset for current SDS */
    double minvalue[MAX_MODIS_BANDS];  /* minimum band value for current SDS */
    double maxvalue[MAX_MODIS_BANDS];  /* maximum band value for current SDS */
    double fillvalue[MAX_MODIS_BANDS]; /* fill value for current SDS */

    Img_coord_float_t img;        /* image coordinates for current pixel */
    Geo_coord_t geo;              /* geodetic coordinates (note radians) */
    Space_def_t geoloc_def;       /* geolocation space information */
    Geoloc_t *geoloc_map = NULL;  /* geolocation mapping information */
    Espa_global_meta_t *gmeta = &metadata->global;  /* pointer to the global
                                                       metadata structure */
    Espa_band_meta_t *bmeta;      /* pointer to the array of bands metadata */

    /* Get the basename of the input HDF file */
    cptr = strrchr (modis_hdf_name, '/');
    if (cptr != NULL)
    {
        /* Copy the basename from the cptr, after moving off of the '/' */
        cptr++;
        count = snprintf (basename, sizeof (basename), "%s", cptr);
        if (count < 0 || count >= sizeof (basename))
        {
            sprintf (errmsg, "Overflow of basename string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }
    else
    {
        /* Copy the filename itself as the basename since it doesn't have a
           path in the filename */
        count = snprintf (basename, sizeof (basename), "%s", modis_hdf_name);
        if (count < 0 || count >= sizeof (basename))
        {
            sprintf (errmsg, "Overflow of basename string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Strip the extension off the basename */
    count = snprintf (core_basename, sizeof (core_basename), "%s", basename);
    if (count < 0 || count >= sizeof (core_basename))
    {
        sprintf (errmsg, "Overflow of core_basename string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    cptr = strrchr (core_basename, '.');
    if (cptr != NULL)
        *cptr = '\0';

    /* Make sure the basename is uppercase */
    cptr = basename;
    while (*cptr != '\0')
    {
        *cptr = toupper ((unsigned char) *cptr);
        cptr++;
    }

    /* Use the HDF filename to determine the data_provider, satellite,
       instrument, and product. */
    strcpy (gmeta->data_provider, "USGS/EROS LPDAAC");
    strcpy (gmeta->instrument, "MODIS");
    if (!strncmp (basename, "MOD", 3))
        strcpy (gmeta->satellite, "TERRA");
    else if (!strncmp (basename, "MYD", 3))
        strcpy (gmeta->satellite, "AQUA");
    else
    {
        sprintf (errmsg, "Unknown MODIS filename: %s. Filenames are expected "
            "to start with MOD for TERRA and MYD for AQUA.", basename);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Use the HDF filename to determine the acquisition date as yyyyddd.
       Example - MOD09A1.A2013241.h08v05.005.2013252120055.hdf */
    if (strncpy (yearstr, &basename[9], 4) == NULL)
    {
        sprintf (errmsg, "Error pulling the acquisition year from the base "
            "filename: %s", basename);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    yearstr[4] = '\0';
    acq_year = atoi (yearstr);

    if (strncpy (doystr, &basename[13], 3) == NULL)
    {
        sprintf (errmsg, "Error pulling the acquisition DOY from the base "
            "filename: %s", basename);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    doystr[3] = '\0';
    acq_doy = atoi (doystr);

    /* Year and DOY need to be converted to yyyy-mm-dd */
    if (doy_to_month_day (acq_year, acq_doy, &acq_month, &acq_day) != SUCCESS)
    {
        sprintf (errmsg, "Error converting %d-%d to yyyy-mm-dd", acq_year,
            acq_doy);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    count = snprintf (gmeta->acquisition_date, sizeof (gmeta->acquisition_date),
        "%04d-%02d-%02d", acq_year, acq_month, acq_day);
    if (count < 0 || count >= sizeof (gmeta->acquisition_date))
    {
        sprintf (errmsg, "Overflow of gmeta->acquisition_date string");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Use the HDF filename to determine the horizontal and vertical tile
       numbers.  Example - MOD09A1.A2013241.h08v05.005.2013252120055.hdf */
    if (strncpy (htile, &basename[19], 2) == NULL)
    {
        sprintf (errmsg, "Error pulling the horizontal tile number from the "
            "base filename: %s", basename);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    htile[2] = '\0';
    gmeta->htile = atoi (htile);

    if (strncpy (vtile, &basename[22], 2) == NULL)
    {
        sprintf (errmsg, "Error pulling the vertical tile number from the "
            "base filename: %s", basename);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    vtile[2] = '\0';
    gmeta->vtile = atoi (vtile);

    /* Open HDF-EOS file for reading */
    fid = GDopen (modis_hdf_name, DFACC_READ);
    if (fid < 0)
    {
        sprintf (errmsg, "Unable to open %s", modis_hdf_name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Open as HDF file for reading */
    sd_id = SDstart (modis_hdf_name, DFACC_RDONLY);
    if (sd_id < 0)
    {
        sprintf (errmsg, "Unable to open %s for reading as SDS",
            modis_hdf_name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Read the production date/time and PGE version from the core metadata */
    status = read_core_metadata (sd_id, prod_date_time, pge_version);
    if (status != SUCCESS)
    {
        sprintf (errmsg, "Reading the core metadata from the HDF file.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Read the bounding coordinates from the core metadata */
    status = read_archive_metadata (sd_id, gmeta->bounding_coords);
    if (status != SUCCESS)
    {
        sprintf (errmsg, "Reading the archive metadata from the HDF file.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Get the list of grids in the HDF-EOS file */
    ngrids = GDinqgrid (modis_hdf_name, gridstr, &bufsize);
    gridlist = gridstr;

    /* Check if there are any grids. If no data in the grid then most likely
       this is swath or point data. This application only supports grid data. */
    if (ngrids < 1 || strlen (gridlist) < 1)
    {
        sprintf (errmsg, "No grid data found in %s. This application only "
            "supports gridded HDF-EOS data, not swath or point HDF-EOS data.",
            modis_hdf_name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Parse the grids */
    nmodis_bands = 0;
    for (i = 0; i < ngrids; i++)
    {
        /* Get next grid name from comma-separated gridlist */
        gridname = gridlist;
        gridend = strchr (gridlist, ',');
        if (gridend != NULL)
        {
            gridlist = gridend + 1;
            *gridend = '\0';
        }
        count = snprintf (grid_names[i], sizeof(grid_names[i]), "%s", gridname);
        if (count < 0 || count >= sizeof (grid_names[i]))
        {
            sprintf (errmsg, "Overflow of grid_names[i] string");
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Attach to next grid */
        gid = GDattach (fid, gridname);
        if (gid < 0)
        {
            sprintf (errmsg, "Unable to attach to grid %s", gridname);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Get the list of fields in this grid */
        nfields = GDinqfields (gid, fieldstr, tmprank, tmpdtype);
        fieldlist = fieldstr;

        /* Loop through fields/SDSs in grid */
        for (j = 0; j < nfields; j++, sdsfield++)
        {
            /* Get name of next field */
            fieldname = fieldlist;
            fieldend = strchr (fieldlist, ',');
            if (fieldend != NULL)
            {
                fieldlist = fieldend + 1;
                *fieldend = '\0';
            }

            /* Get field info */
            status = GDfieldinfo (gid, fieldname, &rank, dims, &dtype, strbuf);
            if (status != 0)
            {
                sprintf (errmsg, "Reading field info for current field.");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* If this is the first field/SDS, then get the x/y dimensions for
               the SDSs in this grid */
            if (j == 0)
            {
                /* Get the X and Y dimensions */
                for (k = 0, dimname = strtok (strbuf, ",");
                    k < rank && dimname != NULL; k++,
                    dimname = strtok (NULL, ","))
                {
                    if (!strcmp (dimname, "XDim"))
                        grid_dims[i][0] = dims[k];
                    else if (!strcmp (dimname, "YDim"))
                        grid_dims[i][1] = dims[k];
                }
            }

            /* Only support 2D SDSs at this time */
            if (rank > 2)
            {
                sprintf (errmsg, "Skipping SDS %s in grid %s since its "
                    "rank is greater than 2D (%d).", fieldname, gridname,
                    rank);
                error_handler (false, FUNC_NAME, errmsg);
                continue;
            }

            /* Store the band/SDS information */
            data_type[nmodis_bands] = dtype;
            count = snprintf (modis_bands[nmodis_bands],
                sizeof (modis_bands[nmodis_bands]), "%s", fieldname);
            if (count < 0 || count >= sizeof (modis_bands[nmodis_bands]))
            {
                sprintf (errmsg, "Overflow of modis_bands[] string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Get SDS values from the HDF file. Ignore the datum value since
               it will get pulled later. */
            status = get_sds_values (sd_id, sdsfield, dtype,
                &scalevalue[nmodis_bands], &offsetvalue[nmodis_bands],
                &minvalue[nmodis_bands], &maxvalue[nmodis_bands],
                &fillvalue[nmodis_bands], longname[nmodis_bands],
                units[nmodis_bands], qa_desc[nmodis_bands]);
            if (status != SUCCESS)
            {
                sprintf (errmsg, "Reading SDS-specific values from HDF file");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Increment the band/SDS count */
            nmodis_bands++;
        }  /* end for j loop through nfields */

        /* Close grid */
        GDdetach (gid);
    }  /* end for i loop through ngrids */

    /* Close the HDF file */
    status = SDend (sd_id);
    if (status != 0)
    {
        sprintf (errmsg, "Closing HDF file.");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Allocate bands for the XML structure */
    metadata->nbands = nmodis_bands;
    if (allocate_band_metadata (metadata, metadata->nbands) != SUCCESS)
    {   /* Error messages already printed */
        return (ERROR);
    }
    bmeta = metadata->band;

    /* Loop back through the grids and SDSs, fill in the information from
       above */
    nmodis_bands = 0;
    for (i = 0; i < ngrids; i++)
    {
        /* Attach to next grid */
        gid = GDattach (fid, grid_names[i]);
        if (gid < 0)
        {
            sprintf (errmsg, "Unable to attach to grid %s", grid_names[i]);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Get the grid information if this is the first grid.  We will assume
           that the projection and other related global metadata is the same
           for both grids. */
        if (i == 0)
        {
            /* Get the projection information for this grid */
            status = GDprojinfo (gid, &projcode, &zonecode, &spherecode,
                projparm);
            if (status != 0)
            {
                sprintf (errmsg, "Reading grid projection information from HDF "
                    "header");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Store the projection info */
            if (projcode != GCTP_SIN_PROJ)
            {
                sprintf (errmsg, "Invalid projection type.  MODIS data is "
                    "expected to be in the Sinusoidal projection.");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            gmeta->proj_info.proj_type = GCTP_SIN_PROJ;

            if (spherecode != ESPA_NODATUM)
            {
                sprintf (errmsg, "Invalid sphere code.  MODIS data is expected "
                    "to be in the Sinusoidal projection and have a sphere code "
                    "of %d.", ESPA_NODATUM);
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            gmeta->proj_info.datum_type = ESPA_NODATUM;
            strcpy (gmeta->proj_info.units, "meters");

            /* Store the input projection parameters for Sinusoidal */
            gmeta->proj_info.sphere_radius = projparm[0];
            central_meridian = projparm[4];
            gmeta->proj_info.false_easting = projparm[6];
            gmeta->proj_info.false_northing = projparm[7];

            /* According to HDF-EOS documentation angular projection parameters
               in HDF-EOS structural metadata are in DMS.  Convert the central
               meridian from DMS to decimal degrees. */
            dmsdeg (central_meridian, &gmeta->proj_info.central_meridian);

            /* Get the grid pixel size and corner info. Projection coords are
               in meters since this should be the Sinusoidal projection. */
            status = GDgridinfo (gid, &xdimsize, &ydimsize,
                gmeta->proj_info.ul_corner, gmeta->proj_info.lr_corner);
            if (status != 0)
            {
                sprintf (errmsg, "Reading dimension and corner information "
                    "from HDF header");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Get the coordinate system origin.  If the grid origin isn't
               specified then default to the UL which is standard for HDF. */
            status = GDorigininfo (gid, &origincode);
            if (status != 0)
                strcpy (gmeta->proj_info.grid_origin, "UL");
            else if (origincode == HDFE_GD_UL)
                strcpy (gmeta->proj_info.grid_origin, "UL");
            else
                strcpy (gmeta->proj_info.grid_origin, "CENTER");
        }  /* end for i == 0 (i.e. first grid) */

        /* Get the list of fields in this grid */
        nfields = GDinqfields (gid, fieldstr, tmprank, tmpdtype);
        fieldlist = fieldstr;

        /* Loop through fields/SDSs in grid */
        for (j = 0; j < nfields; j++, sdsfield++)
        {
            /* Get name of next field */
            fieldname = fieldlist;
            fieldend = strchr (fieldlist, ',');
            if (fieldend != NULL)
            {
                fieldlist = fieldend + 1;
                *fieldend = '\0';
            }

            /* Get field info */
            status = GDfieldinfo (gid, fieldname, &rank, dims, &dtype, strbuf);
            if (status != 0)
            {
                sprintf (errmsg, "Reading field info for current field.");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Only support 2D SDSs at this time */
            if (rank > 2)
                continue;

            /* Fill in the SDS/band level information already obtained. Use
               'sr_refl', 'land_surf_temp', or 'spectral_indices' for the
               product type.  Copy the first 7 characters of the basename as
               the short name.  Use 'image' for the category. */
            if (!strncmp (basename, "MOD09", 5) ||
                !strncmp (basename, "MYD09", 5))
                strcpy (bmeta[nmodis_bands].product, "sr_refl");
            else if (!strncmp (basename, "MOD11", 5) ||
                !strncmp (basename, "MYD11", 5))
                strcpy (bmeta[nmodis_bands].product, "land_surf_temp");
            else if (!strncmp (basename, "MOD13", 5) ||
                !strncmp (basename, "MYD13", 5))
                strcpy (bmeta[nmodis_bands].product, "spectral_indices");

            strncpy (bmeta[nmodis_bands].short_name, basename, 7);
            bmeta[nmodis_bands].short_name[7] = '\0';
            strcpy (bmeta[nmodis_bands].category, "image");
            bmeta[nmodis_bands].nsamps = grid_dims[i][0];
            bmeta[nmodis_bands].nlines = grid_dims[i][1];

            /* Use the SDS name as the band name as well as the file name */
            count = snprintf (bmeta[nmodis_bands].name,
                sizeof (bmeta[nmodis_bands].name), "%s",
                modis_bands[nmodis_bands]);
            if (count < 0 || count >= sizeof (bmeta[nmodis_bands].name))
            {
                sprintf (errmsg, "Overflow of bmeta[].name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Set up the filename, but replace any blank spaces in the
               filename (due to the SDS names) with underscores */
            count = snprintf (bmeta[nmodis_bands].file_name,
                sizeof (bmeta[nmodis_bands].file_name), "%s.%s.img",
                core_basename, modis_bands[nmodis_bands]);
            if (count < 0 || count >= sizeof (bmeta[nmodis_bands].file_name))
            {
                sprintf (errmsg, "Overflow of bmeta[].file_name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            cleanup_file_name (bmeta[nmodis_bands].file_name);

            /* Convert the data type to the ESPA data type */
            switch (data_type[nmodis_bands])
            {
                case DFNT_INT8:
                    bmeta[nmodis_bands].data_type = ESPA_INT8; break;
                case DFNT_UINT8:
                    bmeta[nmodis_bands].data_type = ESPA_UINT8; break;
                case DFNT_INT16:
                    bmeta[nmodis_bands].data_type = ESPA_INT16; break;
                case DFNT_UINT16:
                    bmeta[nmodis_bands].data_type = ESPA_UINT16; break;
                case DFNT_INT32:
                    bmeta[nmodis_bands].data_type = ESPA_INT32; break;
                case DFNT_UINT32:
                    bmeta[nmodis_bands].data_type = ESPA_UINT32; break;
                case DFNT_FLOAT32:
                    bmeta[nmodis_bands].data_type = ESPA_FLOAT32; break;
                case DFNT_FLOAT64:
                    bmeta[nmodis_bands].data_type = ESPA_FLOAT64; break;
                default:
                    sprintf (errmsg, "Unsupported MODIS HDF data type in grid "
                        "%d, field %d, modis band %d: %d", i, j, nmodis_bands,
                        data_type[nmodis_bands]);
                    error_handler (true, FUNC_NAME, errmsg);
                    return (ERROR);
            }

            /* Compute the pixel size */
            bmeta[nmodis_bands].pixel_size[1] = (gmeta->proj_info.ul_corner[1] -
                gmeta->proj_info.lr_corner[1]) / bmeta[nmodis_bands].nlines;
            bmeta[nmodis_bands].pixel_size[0] = (gmeta->proj_info.lr_corner[0] -
                gmeta->proj_info.ul_corner[0]) / bmeta[nmodis_bands].nsamps;
            strcpy (bmeta[nmodis_bands].pixel_units, "meters");

            /* Assign the scale, offset, min/max, and fill values.  Fill value
               is required, so assign it as 0 if it doesn't exist. */
            if (fillvalue[nmodis_bands] == ESPA_INT_META_FILL)
                bmeta[nmodis_bands].fill_value = 0;
            else
                bmeta[nmodis_bands].fill_value = fillvalue[nmodis_bands];
            bmeta[nmodis_bands].scale_factor = scalevalue[nmodis_bands];
            bmeta[nmodis_bands].add_offset = offsetvalue[nmodis_bands];
            bmeta[nmodis_bands].valid_range[0] = minvalue[nmodis_bands];
            bmeta[nmodis_bands].valid_range[1] = maxvalue[nmodis_bands];

            /* Assign the long_name and data_units values */
            count = snprintf (bmeta[nmodis_bands].long_name,
                sizeof (bmeta[nmodis_bands].long_name), "%s",
                longname[nmodis_bands]);
            if (count < 0 || count >= sizeof (bmeta[nmodis_bands].long_name))
            {
                sprintf (errmsg, "Overflow of bmeta[].long_name string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            count = snprintf (bmeta[nmodis_bands].data_units,
                sizeof (bmeta[nmodis_bands].data_units), "%s",
                units[nmodis_bands]);
            if (count < 0 || count >= sizeof (bmeta[nmodis_bands].data_units))
            {
                sprintf (errmsg, "Overflow of bmeta[].data_units string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Get the QA description information.  If it's not fill, then
               this is a QA band and change the category to flag that.  The
               MOD13 products also use Legend for "MIR reflectance" SDSs to
               provide additional information, but these aren't QA bands. */
            count = snprintf (bmeta[nmodis_bands].qa_desc,
                sizeof (bmeta[nmodis_bands].qa_desc), "%s",
                qa_desc[nmodis_bands]);
            if (count < 0 || count >= sizeof (bmeta[nmodis_bands].qa_desc))
            {
                sprintf (errmsg, "Overflow of bmeta[].qa_desc string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }
            if (strcmp (bmeta[nmodis_bands].qa_desc, ESPA_STRING_META_FILL) &&
                !strstr (bmeta[nmodis_bands].name, "MIR reflectance"))
                strcpy (bmeta[nmodis_bands].category, "qa");

            /* Add the production date/time and PGE version from the core
               metadata */
            count = snprintf (bmeta[nmodis_bands].production_date,
                sizeof (bmeta[nmodis_bands].production_date), "%s",
                prod_date_time);
            if (count < 0 ||
                count >= sizeof (bmeta[nmodis_bands].production_date))
            {
                sprintf (errmsg, "Overflow of bmeta[].production_date string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            count = snprintf (bmeta[nmodis_bands].app_version,
                sizeof (bmeta[nmodis_bands].app_version), "PGE Version %s",
                pge_version);
            if (count < 0 ||
                count >= sizeof (bmeta[nmodis_bands].app_version))
            {
                sprintf (errmsg, "Overflow of bmeta[].app_version string");
                error_handler (true, FUNC_NAME, errmsg);
                return (ERROR);
            }

            /* Increment the overall band/SDS count */
            nmodis_bands++;
        }  /* end for j loop through nfields */

        /* Close grid */
        GDdetach (gid);
    }  /* end for i (loop through the grids) */

    /* Close the HDF-EOS file */
    GDclose (fid);

    /* Set the orientation angle to 0.0 */
    gmeta->orientation_angle = 0.0;

    /* Get geolocation information from the XML file (using the first band) to
       prepare for computing the bounding coordinates */
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

    /* Get the geographic coords for the UL corner */
    img.l = 0.0;
    img.s = 0.0;
    img.is_fill = false;
    if (!from_space (geoloc_map, &img, &geo))
    {
        sprintf (errmsg, "Mapping UL corner to lat/long");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    gmeta->ul_corner[0] = geo.lat * DEG;
    gmeta->ul_corner[1] = geo.lon * DEG;

    /* Get the geographic coords for the LR corner */
    img.l = bmeta[0].nlines-1;
    img.s = bmeta[0].nsamps-1;
    img.is_fill = false;
    if (!from_space (geoloc_map, &img, &geo))
    {
        sprintf (errmsg, "Mapping UL corner to lat/long");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
    gmeta->lr_corner[0] = geo.lat * DEG;
    gmeta->lr_corner[1] = geo.lon * DEG;

    /* Free the geolocation structure */
    free (geoloc_map);

    /* Successful read */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  convert_hdf_to_img

PURPOSE: Convert the MODIS HDF SDS to an ESPA raw binary (.img) file and writes
the associated ENVI header for each band.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error converting the MODIS SDS
SUCCESS         Successfully converted MODIS SDS to raw binary

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/22/2014    Gail Schmidt     Original development

NOTES:
******************************************************************************/
int convert_hdf_to_img
(
    char *modis_hdf_name,      /* I: name of MODIS file to be processed */
    Espa_internal_meta_t *xml_metadata /* I: metadata structure for HDF file */
)
{
    char FUNC_NAME[] = "convert_hdf_to_img";  /* function name */
    char errmsg[STR_SIZE];    /* error message */
    char *cptr = NULL;        /* pointer to the file extension */
    char *img_file = NULL;    /* name of the output raw binary file */
    char envi_file[STR_SIZE]; /* name of the output ENVI header file */
    int i;                    /* looping variable for bands in XML file */
    int nbytes;               /* number of bytes in the data type */
    int count;                /* number of chars copied in snprintf */
    int32 sd_id;              /* file ID for the HDF file */
    int32 sds_id;             /* SDS ID in the HDF file */
    int32 sds_index;          /* index of current SDS name */
    int32 start[2];           /* starting point to read SDS data */
    int32 edges[2];           /* number of values to read in SDS data */
    int32 status;             /* return status of the HDF function */
    void *file_buf = NULL;    /* pointer to correct input file buffer */
    FILE *fp_rb = NULL;       /* file pointer for the raw binary file */
    Envi_header_t envi_hdr;   /* output ENVI header information */
    Espa_band_meta_t *bmeta = NULL;  /* pointer to band metadata */
    Espa_global_meta_t *gmeta = &xml_metadata->global;  /* global metadata */

    /* Open as HDF file for reading */
    sd_id = SDstart (modis_hdf_name, DFACC_RDONLY);
    if (sd_id < 0)
    {
        sprintf (errmsg, "Unable to open %s for reading as SDS",
            modis_hdf_name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Loop through the bands in the metadata file and convert each on to
       the ESPA format */
    for (i = 0; i < xml_metadata->nbands; i++)
    {
        /* Set up the band metadata pointer */
        bmeta = &xml_metadata->band[i];

        /* Find the SDS name */
        sds_index = SDnametoindex (sd_id, bmeta->name);
        if (sds_index == -1)
        {
            sprintf (errmsg, "Unable to find %s in the HDF file", bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
        printf ("  SDS %d: %s -- index: %d\n", i, bmeta->name, sds_index);

        /* Open the current band as an SDS */
        sds_id = SDselect (sd_id, sds_index);
        if (sds_id < 0)
        {
            sprintf (errmsg, "Unable to access %s for reading", bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Open the raw binary file for writing */
        img_file = bmeta->file_name;
        fp_rb = open_raw_binary (img_file, "wb");
        if (fp_rb == NULL)
        {
            sprintf (errmsg, "Opening the output raw binary file: %s",
                img_file);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Allocate memory for the entire image, based on the input data type.
           Since HDF reading works off of a void pointer and the raw binary
           write works off of a void pointer, there's no need to use a data
           type specific pointer for reading/writing memory.  Just make sure
           there are enough bytes for reading the data, based on the data
           type. */
        if (bmeta->data_type == ESPA_UINT8 || bmeta->data_type == ESPA_INT8)
            nbytes = sizeof (uint8);
        else if (bmeta->data_type == ESPA_UINT16 ||
            bmeta->data_type == ESPA_INT16)
            nbytes = sizeof (uint16);
        else if (bmeta->data_type == ESPA_UINT32 ||
            bmeta->data_type == ESPA_INT32)
            nbytes = sizeof (uint32);
        else if (bmeta->data_type == ESPA_FLOAT32)
            nbytes = sizeof (float32);
        else if (bmeta->data_type == ESPA_FLOAT64)
            nbytes = sizeof (float64);
        else
        {
            sprintf (errmsg, "Unsupported data type %d.", bmeta->data_type);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        file_buf = calloc (bmeta->nlines * bmeta->nsamps, nbytes);
        if (file_buf == NULL)
        {
            sprintf (errmsg, "Allocating memory for the image data containing "
                "%d lines x %d samples.", bmeta->nlines, bmeta->nsamps);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Read the entire tile */
        start[0] = 0;
        start[1] = 0;
        edges[0] = bmeta->nlines;
        edges[1] = bmeta->nsamps;
        status = SDreaddata (sds_id, start, NULL, edges, file_buf);
        if (status == -1)
        {
            sprintf (errmsg, "Reading data from the SDS: %s", bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Write entire image to the raw binary file */
        if (write_raw_binary (fp_rb, bmeta->nlines, bmeta->nsamps, nbytes,
            file_buf) != SUCCESS)
        {
            sprintf (errmsg, "Writing image to the raw binary file: %s",
                img_file);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Close the HDF SDS and raw binary file */
        close_raw_binary (fp_rb);
        status = SDendaccess (sds_id);
        if (status == -1)
        {
            sprintf (errmsg, "Ending access to SDS: %s", bmeta->name);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }

        /* Free the memory */
        free (file_buf);

        /* Create the ENVI header file this band */
        if (create_envi_struct (bmeta, gmeta, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Creating the ENVI header structure for this "
                "file: %s", bmeta->file_name);
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
        cptr = strrchr (envi_file, '.');
        strcpy (cptr, ".hdr");

        if (write_envi_hdr (envi_file, &envi_hdr) != SUCCESS)
        {
            sprintf (errmsg, "Writing the ENVI header file: %s.", envi_file);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }  /* end for */

    /* Close the HDF file */
    status = SDend (sd_id);
    if (status == -1)
    {
        sprintf (errmsg, "Ending access to HDF file: %s", modis_hdf_name);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Successful conversion */
    return (SUCCESS);
}


/******************************************************************************
MODULE:  convert_modis_to_espa

PURPOSE: Converts the input MODIS HDF file to the ESPA internal raw binary
file format (and associated XML file).

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error converting the MODIS file
SUCCESS         Successfully converted MODIS to ESPA format

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
4/22/2014    Gail Schmidt     Original development

NOTES:
  1. The ESPA raw binary band files will be generated from the ESPA XML
     filename.
******************************************************************************/
int convert_modis_to_espa
(
    char *modis_hdf_file,  /* I: input MODIS HDF filename */
    char *espa_xml_file,   /* I: output ESPA XML metadata filename */
    bool del_src           /* I: should the source .tif files be removed after
                                 conversion? */
)
{
    char FUNC_NAME[] = "convert_modis_to_espa";  /* function name */
    char errmsg[STR_SIZE];   /* error message */
    Espa_internal_meta_t xml_metadata;  /* XML metadata structure to be
                                populated by reading the MTL metadata file */

    /* Initialize the metadata structure */
    init_metadata_struct (&xml_metadata);

    /* Read the MODIS HDF file and populate our internal ESPA metadata
       structure */
    if (read_modis_hdf (modis_hdf_file, &xml_metadata) != SUCCESS)
    {
        sprintf (errmsg, "Reading the MODIS HDF file: %s", modis_hdf_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Write the metadata from our internal metadata structure to the output
       XML filename */
    if (write_metadata (&xml_metadata, espa_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Validate the output metadata file */
    if (validate_xml_file (espa_xml_file) != SUCCESS)
    {  /* Error messages already written */
        return (ERROR);
    }

    /* Convert each of the MODIS HDF bands/SDSs to raw binary */
    if (convert_hdf_to_img (modis_hdf_file, &xml_metadata) != SUCCESS)
    {
        sprintf (errmsg, "Converting %s to ESPA", modis_hdf_file);
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Remove the source file if specified */
    if (del_src)
    {
        printf ("  Removing %s\n", modis_hdf_file);
        if (unlink (modis_hdf_file) != 0)
        {
            sprintf (errmsg, "Deleting source file: %s", modis_hdf_file);
            error_handler (true, FUNC_NAME, errmsg);
            return (ERROR);
        }
    }

    /* Free the metadata structure */
    free_metadata (&xml_metadata);

    /* Successful conversion */
    return (SUCCESS);
}

