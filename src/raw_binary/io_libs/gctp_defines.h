/*****************************************************************************
FILE: gctp_defines.h
  
PURPOSE: Contains ESPA internal defines for GCTP projections

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

LICENSE TYPE:  NASA Open Source Agreement Version 1.3

HISTORY:
Date         Programmer       Reason
----------   --------------   -------------------------------------
12/13/2013   Gail Schmidt     Original development
4/17/2014    Gail Schmidt     Updated to support additional projections
4/22/2014    Gail Schmidt     Updated to support additional datums

NOTES:
*****************************************************************************/

#ifndef GCTP_DEFINES_H
#define GCTP_DEFINES_H

/* GCTP projection numbers for ESPA projections (match proj.h from GCTP) */
#define GCTP_GEO_PROJ    0
#define GCTP_UTM_PROJ    1
#define GCTP_ALBERS_PROJ 3
#define GCTP_PS_PROJ     6
#define GCTP_SIN_PROJ    16

/* GCTP spheroid numbers (match sphdz.c in GCTP) */
#define GCTP_CLARKE_1866  0
#define GCTP_GRS80        8
#define GCTP_WGS84        12
#define GCTP_MODIS_SPHERE 31

/* Local defines to use for the datum */
#define ESPA_NAD27   225
#define ESPA_NAD83   219
#define ESPA_WGS84   317
#define ESPA_NODATUM -1

/* Define the semi-major axis (meters), semi-minor axis (meters), and the
   inverse flattening for each of the spheroids.
   Obtained from NGA Reference Ellipsoid Parameters via
   http://geoengine.nga.mil/geospatial/SW_TOOLS/NIMAMUSE/webinter/geotrans2/help/elliptab.htm */
#define GCTP_CLARKE_1866_SEMI_MAJOR 6378206.4
#define GCTP_CLARKE_1866_SEMI_MINOR 6356583.8
#define GCTP_CLARKE_1866_INV_FLATTENING 294.9786982

#define GCTP_GRS80_SEMI_MAJOR 6378137.0
#define GCTP_GRS80_SEMI_MINOR 6356752.3141
#define GCTP_GRS80_INV_FLATTENING 298.257222101

#define GCTP_WGS84_SEMI_MAJOR 6378137.0
#define GCTP_WGS84_SEMI_MINOR 6356752.3142
#define GCTP_WGS84_INV_FLATTENING 298.257223563

#endif
