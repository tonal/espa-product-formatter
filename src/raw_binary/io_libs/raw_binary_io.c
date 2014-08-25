/*****************************************************************************
FILE: raw_binary_io.c
  
PURPOSE: Contains functions for opening/closing raw binary files as well as
reading/writing to raw binary files N lines at a time.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#include "raw_binary_io.h"

/* define the read/write formats to be used for opening a file */
typedef enum {
  RB_READ_FORMAT,
  RB_WRITE_FORMAT,
  RB_READ_WRITE_FORMAT,
} Raw_binary_format_t;
const char raw_binary_format[][4] = {"rb", "wb", "rb+"};

/******************************************************************************
MODULE: open_raw_binary

PURPOSE: Opens a raw binary file for specified read/write/both binary access.
 
RETURN VALUE:
Type = FILE *
Value        Description
-----        -----------
NULL         Error opening the specified file for read specified access
non-NULL     FILE pointer to the opened file

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/
FILE *open_raw_binary
(
    char *infile,        /* I: name of the input file to be opened */
    char *access_type    /* I: string for the access type for reading the
                               input file; use the raw_binary_format
                               array at the top of this file */
)
{
    char FUNC_NAME[] = "open_raw_binary"; /* function name */
    char errmsg[STR_SIZE];   /* error message */
    FILE *rb_fptr = NULL;    /* pointer to the raw binary file */

    /* Open the file with the specified access type */
    rb_fptr = fopen (infile, access_type);
    if (rb_fptr == NULL)
    {
        sprintf (errmsg, "Opening raw binary file %s with %s access.",
            infile, access_type);
        error_handler (true, FUNC_NAME, errmsg);
        return NULL;
    }

    /* Return the file pointer */
    return rb_fptr;
}


/******************************************************************************
MODULE: close_raw_binary

PURPOSE: Close the raw binary file
 
RETURN VALUE:
Type = N/A

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/
void close_raw_binary
(
    FILE *fptr      /* I: pointer to raw binary file to be closed */
)
{
    fclose (fptr);
}


/******************************************************************************
MODULE: write_raw_binary

PURPOSE: Writes nlines of data to the raw binary file
 
RETURN VALUE:
Type = int
Value        Description
-----        -----------
ERROR        An error occurred writing data to the raw binary file
SUCCESS      Writing was successful

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/
int write_raw_binary
(
    FILE *rb_fptr,      /* I: pointer to the raw binary file */
    int nlines,         /* I: number of lines to write to the file */
    int nsamps,         /* I: number of samples to write to the file */
    int size,           /* I: number of bytes per pixel (ex. sizeof(uint8)) */
    void *img_array     /* I: array of nlines * nsamps * size to be written
                              to the raw binary file */
)
{
    char FUNC_NAME[] = "write_raw_binary"; /* function name */
    char errmsg[STR_SIZE];   /* error message */
    int nvals;               /* number of values written to the file */

    /* Write the data to the raw binary file */
    nvals = fwrite (img_array, size, nlines * nsamps, rb_fptr);
    if (nvals != nlines * nsamps)
    {
        sprintf (errmsg, "Writing %d elements of %d bytes in size to the "
            "raw binary file.", nlines * nsamps, size);
        error_handler (true, FUNC_NAME, errmsg);
        return ERROR;
    }

    return SUCCESS;
}


/******************************************************************************
MODULE: read_raw_binary

PURPOSE: Reads nlines of data from the raw binary file
 
RETURN VALUE:
Type = int
Value        Description
-----        -----------
ERROR        An error occurred reading data from the raw binary file
SUCCESS      Reading was successful

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/
int read_raw_binary
(
    FILE *rb_fptr,      /* I: pointer to the raw binary file */
    int nlines,         /* I: number of lines to read from the file */
    int nsamps,         /* I: number of samples to read from the file */
    int size,           /* I: number of bytes per pixel (ex. sizeof(uint8)) */
    void *img_array     /* O: array of nlines * nsamps * size to be read from
                              the raw binary file (sufficient space should
                              already have been allocated) */
)
{
    char FUNC_NAME[] = "read_raw_binary"; /* function name */
    char errmsg[STR_SIZE];   /* error message */
    int nvals;               /* number of values read from the file */

    /* Read the data from the raw binary file */
    nvals = fread (img_array, size, nlines * nsamps, rb_fptr);
    if (nvals != nlines * nsamps)
    {
        sprintf (errmsg, "Reading %d elements of %d bytes in size from the "
            "raw binary file.", nlines * nsamps, size);
        error_handler (true, FUNC_NAME, errmsg);
        return ERROR;
    }

    return SUCCESS;
}

