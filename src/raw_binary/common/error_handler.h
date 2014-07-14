/*****************************************************************************
FILE: error_handler.h
  
PURPOSE: Contains error handling related defines and structures

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/12/2013   Gail Schmidt     Original development

NOTES:
*****************************************************************************/

#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "common.h"

/* Prototypes */
void error_handler
(
    bool error_flag,  /* I: true for errors, false for warnings */
    char *module,     /* I: calling module name */
    char *errmsg      /* I: error/warning message to be printed, without
                            ending EOL */
);

#endif
