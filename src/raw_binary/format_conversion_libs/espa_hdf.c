/*****************************************************************************
FILE: espa_hdf.c
  
PURPOSE: Contains functions and defines for writing attributes to the HDF
global and SDS metadata.

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
1/7/2014     Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#include "espa_hdf.h"
#include "error_handler.h"
#include "hdf.h"
#include "mfhdf.h"

/* Constants */
#define DIM_MAX_NCHAR (80)  /* Maximum size of a dimension name */

/* Possible ranges for data types */
#define MYHDF_CHAR8H     (        255  )
#define MYHDF_CHAR8L     (          0  )
#define MYHDF_INT8H      (        127  )
#define MYHDF_INT8L      (       -128  )
#define MYHDF_UINT8H     (        255  )
#define MYHDF_UINT8L     (          0  )
#define MYHDF_INT16H     (      32767  )
#define MYHDF_INT16L     (     -32768  )
#define MYHDF_UINT16H    (      65535u )
#define MYHDF_UINT16L    (          0u )
#define MYHDF_INT32H     ( 2147483647l )
#define MYHDF_INT32L     (-2147483648l )
#define MYHDF_UINT32H    ( 4294967295ul)
#define MYHDF_UINT32L    (          0ul)
#define MYHDF_FLOAT32H   ( 3.4e+38f)
#define MYHDF_FLOAT32L   (-3.4e+38f)
#define MYHDF_FLOAT64H   ( 1.7e+308)
#define MYHDF_FLOAT64L   (-1.7e-308)


/******************************************************************************
MODULE:  put_attr_double

PURPOSE:  Writes an attribute from a parameter type of double to the HDF file.
The double value is converted to the native data type before writing.

RETURN VALUE:
Type = int
Value      Description
-----      -----------
ERROR      Error occurred writing this attribute
SUCCESS    Successful completion

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
1/3/2012    Gail Schmidt     Original Development (based on input routines
                             from the LEDAPS lndsr application)
1/8/2013    Gail Schmidt     Modified to not add 0.5 to the floating point
                             attribute values before writing to the HDF file.
                             That is only appropriate when converting float to
                             int and not float to float.

NOTES:
******************************************************************************/
int put_attr_double
(
    int32 sds_id,          /* I: SDS ID to write attribute to */
    Espa_hdf_attr_t *attr, /* I: attribute data structure */
    double *val            /* I: array of values to be written as native type */
)
{
    char FUNC_NAME[] = "put_attr_double";   /* function name */
    char errmsg[STR_SIZE];   /* error message */
    int i;                   /* looping variable for the number of values */
    void *buf = NULL;        /* void pointer to actual data array */
    char8 val_char8[MYHDF_MAX_NATTR_VAL];
    int8 val_int8[MYHDF_MAX_NATTR_VAL];
    uint8 val_uint8[MYHDF_MAX_NATTR_VAL];
    int16 val_int16[MYHDF_MAX_NATTR_VAL];
    uint16 val_uint16[MYHDF_MAX_NATTR_VAL];
    int32 val_int32[MYHDF_MAX_NATTR_VAL];
    uint32 val_uint32[MYHDF_MAX_NATTR_VAL];
    float32 val_float32[MYHDF_MAX_NATTR_VAL];
    float64 val_float64[MYHDF_MAX_NATTR_VAL];

    if (attr->nval <= 0 || attr->nval > MYHDF_MAX_NATTR_VAL) 
    {
        sprintf (errmsg, "Invalid number of attribute values");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    /* Write the attribute based on its data type */
    switch (attr->type)
    {
    case DFNT_CHAR8:
        for (i = 0; i < attr->nval; i++)
        {
            if (val[i] >= ((double)MYHDF_CHAR8H))
                val_char8[i] = MYHDF_CHAR8H;
            else if (val[i] <= ((double)MYHDF_CHAR8L))
                val_char8[i] = MYHDF_CHAR8L;
            else if (val[i] >= 0.0)
                val_char8[i] = (char8)(val[i] + 0.5);
            else
                val_char8[i] = -((char8)(-val[i] + 0.5));
        }
        buf = (void *)val_char8;
        break;
  
    case DFNT_INT8:
        for (i = 0; i < attr->nval; i++)
        {
            if (val[i] >= ((double)MYHDF_INT8H))
                val_int8[i] = MYHDF_INT8H;
            else if (val[i] <= ((double)MYHDF_INT8L))
                val_int8[i] = MYHDF_INT8L;
            else if (val[i] >= 0.0)
                val_int8[i] = (int8)(val[i] + 0.5);
            else
                val_int8[i] = -((int8)(-val[i] + 0.5));
        }
        buf = (void *)val_int8;
        break;
  
    case DFNT_UINT8:
        for (i = 0; i < attr->nval; i++)
        {
            if (val[i] >= ((double)MYHDF_UINT8H))
                val_uint8[i] = MYHDF_UINT8H;
            else if (val[i] <= ((double)MYHDF_UINT8L))
                val_uint8[i] = MYHDF_UINT8L;
            else if (val[i] >= 0.0)
                val_uint8[i] = (uint8)(val[i] + 0.5);
            else
                val_uint8[i] = -((uint8)(-val[i] + 0.5));
        }
        buf = (void *)val_uint8;
        break;
  
    case DFNT_INT16:
        for (i = 0; i < attr->nval; i++)
        {
            if (val[i] >= ((double)MYHDF_INT16H))
                val_int16[i] = MYHDF_INT16H;
            else if (val[i] <= ((double)MYHDF_INT16L))
                val_int16[i] = MYHDF_INT16L;
            else if (val[i] >= 0.0)
                val_int16[i] = (int16)( val[i] + 0.5);
            else
                val_int16[i] = -((int16)(-val[i] + 0.5));
        }
        buf = (void *)val_int16;
        break;
  
    case DFNT_UINT16:
        for (i = 0; i < attr->nval; i++)
        {
            if (val[i] >= ((double)MYHDF_UINT16H))
                val_uint16[i] = MYHDF_UINT16H;
            else if (val[i] <= ((double)MYHDF_UINT16L))
                val_uint16[i] = MYHDF_UINT16L;
            else if (val[i] >= 0.0)
                val_uint16[i] = (uint16)( val[i] + 0.5);
            else
                val_uint16[i] = -((uint16)(-val[i] + 0.5));
        }
        buf = (void *)val_uint16;
        break;
  
    case DFNT_INT32:
        for (i = 0; i < attr->nval; i++)
        {
            if (val[i] >= ((double)MYHDF_INT32H))
                val_int32[i] = MYHDF_INT32H;
            else if (val[i] <= ((double)MYHDF_INT32L))
                val_int32[i] = MYHDF_INT32L;
            else if (val[i] >= 0.0)
                val_int32[i] = (int32)( val[i] + 0.5);
            else
                val_int32[i] = -((int32)(-val[i] + 0.5));
        }
        buf = (void *)val_int32;
        break;
  
    case DFNT_UINT32:
        for (i = 0; i < attr->nval; i++)
        {
            if (val[i] >= ((double)MYHDF_UINT32H))
                val_uint32[i] = MYHDF_UINT32H;
            else if (val[i] <= ((double)MYHDF_UINT32L))
                val_uint32[i] = MYHDF_UINT32L;
            else if (val[i] >= 0.0)
                val_uint32[i] = (uint32)( val[i] + 0.5);
            else
                val_uint32[i] = -((uint32)(-val[i] + 0.5));
        }
        buf = (void *)val_uint32;
        break;
  
    case DFNT_FLOAT32:
        for (i = 0; i < attr->nval; i++)
        {
            if (val[i] >= ((double)MYHDF_FLOAT32H))
                val_float32[i] = MYHDF_FLOAT32H;
            else if (val[i] <= ((double)MYHDF_FLOAT32L))
                val_float32[i] = MYHDF_FLOAT32L;
            else
                val_float32[i] = (float32) val[i];
        }
        buf = (void *)val_float32;
        break;
  
    case DFNT_FLOAT64:
        if (sizeof (float64) == sizeof (double))
            buf = (void *)val;
        else
        {
            for (i = 0; i < attr->nval; i++)
                val_float64[i] = val[i];
            buf = (void *)val_float64;
        }
        break;
  
    default: 
        sprintf (errmsg, "Unsupported attribute data type");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    if (SDsetattr(sds_id, attr->name, attr->type, attr->nval, buf) == HDF_ERROR)
    {
        sprintf (errmsg, "Error writing attribute");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }
  
    return (SUCCESS);
}


/******************************************************************************
MODULE:  put_attr_string

PURPOSE:  Writes a string attribute to the HDF file.

RETURN VALUE:
Type = int
Value      Description
-----      -----------
ERROR      Error occurred writing this attribute
SUCCESS    Successful completion

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
1/3/2012    Gail Schmidt     Original Development (based on input routines
                             from the LEDAPS lndsr application)

NOTES:
******************************************************************************/
int put_attr_string
(
    int32 sds_id,          /* I: SDS ID to write attribute to */
    Espa_hdf_attr_t *attr, /* I: attribute data structure */
    char *string           /* I: string value to be written */
)
{
    char FUNC_NAME[] = "put_attr_string";   /* function name */
    char errmsg[STR_SIZE];   /* error message */
    int i;                   /* looping variable for the number of values */
    void *buf = NULL;        /* void pointer to actual data array */
    char8 val_char8[MYHDF_MAX_STRING];

    if (attr->nval <= 0 || attr->nval > MYHDF_MAX_STRING) 
    {
        sprintf (errmsg, "Invalid number of attribute values");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Validate the data type is character */
    if (attr->type != DFNT_CHAR8) 
    {
        sprintf (errmsg, "Invalid data type - should be string (char8)");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    /* Set up the void buffer and write the attribute */
    if (sizeof(char8) == sizeof(char))
        buf = (void *)string;
    else
    {
        for (i = 0; i < attr->nval; i++) 
            val_char8[i] = (char8)string[i];
        buf = (void *)val_char8;
    }

    if (SDsetattr(sds_id, attr->name, attr->type, attr->nval, buf) == HDF_ERROR)
    {
        sprintf (errmsg, "Error writing attribute");
        error_handler (true, FUNC_NAME, errmsg);
        return (ERROR);
    }

    return (SUCCESS);
}
