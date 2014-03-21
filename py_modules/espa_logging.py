
'''
License:
  "NASA Open Source Agreement 1.3"

Description:
  This module implements logging routines for generating standard log messages.

Notes:
  By default all log messages will be written to STDOUT unless you choose to
  create a log file by using open_log_handler(...) and close_log_handler().

  This is not implemented to handle multiple output log files.

History:
  Created Oct/2013 by Ron Dilley, USGS/EROS
'''

import os
import inspect
import datetime

from espa_constants import *

'''
The global variable that is the file handle to the log file.  Only used when
using a file for the log.
'''
__LOG_HANDLER__ = None


'''
The global variable used for determining wether or not to output debug log
messages.
'''
__DEBUG_ON__ = False


def open_log_handler (file_name):
    '''
    Description:
      Opens the log file handle for appending

    Returns:
      ERROR - Failed to open the log file
      SUCCESS - The file opened
    '''
    global __LOG_HANDLER__

    if (__LOG_HANDLER__ == None) and (file_name != None):
        __LOG_HANDLER__ = open (file_name, 'a', buffering=1)

    if (__LOG_HANDLER__ == None):
        return ERROR

    return SUCCESS
# END open_log_handler


def close_log_handler():
    '''
    Description:
      Closes the log file handle

    Returns:
      ERROR - Failed to close the log file
      SUCCESS - The file closed
    '''
    global __LOG_HANDLER__

    if (__LOG_HANDLER__ != None):
        __LOG_HANDLER__.close()
        if not __LOG_HANDLER__.closed:
            return ERROR
        __LOG_HANDLER__ = None

    return SUCCESS
# END close_log_handler


def build_log_message (message, filename, line):
    '''
    Desacription:
      Builds a standardized log message

    Input:
      message - the message to write to the log
      filename - filename/module for where the message originates
      line - line for where the message originates

    Returns:
        Returns an ESPA standard log message
    '''
    now = datetime.datetime.now()
    pid = os.getpid()
    return "%s-%s-%s %s:%s.%s %d [%s]:%d %s" % (now.year,
        str(now.month).zfill(2),
        str(now.day).zfill(2),
        str(now.hour).zfill(2),
        str(now.minute).zfill(2),
        str(now.second).zfill(2),
        pid,
        filename,
        line,
        message)
# END build_log_message


def log (message, file=None, line=None):
    '''
    Description:
      Logs a message in the ESPA standard log format to STDOUT or a log file

    Input:
      message - the message to write to the log
      line(optional) - to override the location of the message
      file(optional) - to specify an alternative module name instead
    '''
    global __LOG_HANDLER__

    # Get information about the calling code for the filename and line_number
    (frame, filename, line_number, function_name, lines, index) = \
        inspect.getouterframes(inspect.currentframe())[1]
    filename = os.path.basename(filename)

    # Use the one provided
    if line != None:
        line_number = line

    # Use the one provided
    if file != None:
        filename = file

    # Build the message string
    message_string = build_log_message(message, filename, line_number)

    # Determine where to output the message
    if __LOG_HANDLER__ is None:
        print (message_string)
    else:
        __LOG_HANDLER__.write (message_string + '\n')
# END log


def set_debug (on=True):
    '''
    Description:
      Used for turning debug messaging on or off
    '''
    global __DEBUG_ON__

    __DEBUG_ON__ = on
# END set_debug


def debug (message, file=None, line=None):
    '''
    Description:
      Write the message to the log it debug is turned on
    '''
    global __DEBUG_ON__

    if __DEBUG_ON__:
        # Get information about the calling code for the filename and line_number
        (frame, filename, line_number, function_name, lines, index) = \
            inspect.getouterframes(inspect.currentframe())[1]
        filename = os.path.basename(filename)

        # Use the one provided
        if line != None:
            line_number = line

        # Use the one provided
        if file != None:
            filename = file

        log (message, filename, line_number)
# END debug

