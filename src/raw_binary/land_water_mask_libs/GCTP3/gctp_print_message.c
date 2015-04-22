/****************************************************************************
Name: gctp_print_message

Purpose: Provides the means to print messages from GCTP.  By default a local
    routine is provided to print messages to the display.  However, by
    calling gctp_set_message_callback, the user of the library can provide
    their own routine to deal with messages from the library.

****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "gctp.h"
#include "local.h"

/* Keep track of the routine the user provided to override the normal print
   message routine. */
static GCTP_CALLBACK_PRINT_FUNC callback_print_func = NULL;

/****************************************************************************
Name: gctp_print_message

Purpose: Formats and prints a message to stdout.  If the user has provided a
    callback routine to handle messages, call that one instead.

Returns:
    nothing

****************************************************************************/
void gctp_print_message 
(
    GCTP_MESSAGE_TYPE_ENUM message_type, /* I: message type */
    const char *filename,     /* I: source code file name for input */
    int line_number,          /* I: source code line number for input */
    const char *format, ...   /* I: format string for message */
)
{
    static const char *message_type_string[GCTP_MESSAGE_TYPE_COUNT + 1]
        = {"GCTP Info", "GCTP Error", "GCTP Unknown Message"};
    char buffer[2000];
    char buffer2[2000];
    va_list ap;

    if (callback_print_func)
    {
        va_start(ap, format); 
        callback_print_func(message_type, filename, line_number, format, ap);
        va_end(ap);           
    }
    else
    {
        /* limit the message type to the defined types */
        if (message_type < 0)
            message_type = 0;
        else if (message_type > GCTP_MESSAGE_TYPE_COUNT)
            message_type = GCTP_MESSAGE_TYPE_COUNT;

        if (message_type == GCTP_INFO_MESSAGE)
        {
            /* Skip the file and line number for info messages */
            snprintf(buffer, sizeof(buffer), "%s: ",
                message_type_string[message_type]);
        }
        else
        {
            /* Default message formatting */
            snprintf(buffer, sizeof(buffer), "%s:%s:%d",
                message_type_string[message_type], filename, line_number);
        }

        /* Set arg_ptr to beginning of list of optional arguments */
        va_start(ap, format); 
        vsnprintf(buffer2, sizeof(buffer2), format, ap);
        /* Reset arg_ptr */
        va_end(ap);           

        printf("%s %s\n", buffer, buffer2);
    }
}

/****************************************************************************
Name: gctp_set_message_callback

Purpose: Allows the user to set their own routine to handling printing out
    messages.  This allows the user to control what goes to stdout and what
    format it uses if they want to.

Returns:
    nothing

****************************************************************************/
void gctp_set_message_callback(GCTP_CALLBACK_PRINT_FUNC callback)
{
    callback_print_func = callback;
}
