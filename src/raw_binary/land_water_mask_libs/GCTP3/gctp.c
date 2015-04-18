/*******************************************************************************
NAME                           GCTP 

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*******************************************************************************/
#include "cproj.h"
#include "gctp.h"
#include "local.h"

#define TRUE 1
#define FALSE 0

static long iter = 0;			/* First time flag		*/
static long inpj[MAXPROJ + 1];		/* input projection array	*/
static long indat[MAXPROJ + 1];		/* input dataum array		*/
static long inzn[MAXPROJ + 1];		/* input zone array		*/
static double pdin[MAXPROJ + 1][COEFCT];/* input projection parm array	*/
static long outpj[MAXPROJ + 1];		/* output projection array	*/
static long outdat[MAXPROJ + 1];	/* output dataum array		*/
static long outzn[MAXPROJ + 1];		/* output zone array		*/
static double pdout[MAXPROJ+1][COEFCT];	/* output projection parm array	*/
static long (*for_trans[MAXPROJ + 1])(double,double,double*,double*);
                                    /* forward function pointer array*/
static long (*inv_trans[MAXPROJ + 1])(double,double,double*,double*);
                                    /* inverse function pointer array*/

void gctp
(
    const double *incoor,   /* input coordinates */
    const long *insys,      /* input projection code */
    const long *inzone,     /* input zone number */
    const double *inparm,   /* input projection parameter array */
    long *inunit,           /* input units */
    const long *inspheroid, /* input spheroid */
    double *outcoor,        /* output coordinates */
    const long *outsys,     /* output projection code */
    const long *outzone,    /* output zone */
    const double *outparm,  /* output projection array */
    long *outunit,          /* output units */
    const long *outspheroid,/* output spheroid */
    long *iflg              /* error flag */
)
{
double x;		/* x coordinate 				*/
double y;		/* y coordinate					*/
double factor;		/* conversion factor				*/
double lon;		/* longitude					*/
double lat;		/* latitude					*/
long i,j;		/* loop counters				*/
long ininit_flag;	/* input initilization flag			*/
long outinit_flag;	/* output initilization flag			*/
long unit;		/* temporary unit variable			*/

/* setup initilization flags and output message flags
---------------------------------------------------*/
ininit_flag = FALSE;
outinit_flag = FALSE;
*iflg = 0;


/* check to see if initilization is required
only the first 13 projection parameters are currently used.
If more are added the loop should be increased.
---------------------------------------------------------*/
if (iter == 0)
   {
   for (i = 0; i < MAXPROJ + 1; i++)
      {
      inpj[i] = 0;
      indat[i] = 0;
      inzn[i] = 0;
      outpj[i] = 0;
      outdat[i] = 0;
      outzn[i] = 0;
      for (j = 0; j < COEFCT; j++)
         {
         pdin[i][j] = 0.0;
         pdout[i][j] = 0.0;
         }
      }
   ininit_flag = TRUE;
   outinit_flag = TRUE;
   iter = 1;
   }
else
   {
   if (*insys != GEO)
     {
     if ((inzn[*insys] != *inzone) || (indat[*insys] != *inspheroid) || 
         (inpj[*insys] != *insys))
        {
        ininit_flag = TRUE;
        }
     else
     for (i = 0; i < 13; i++)
        if (pdin[*insys][i] != inparm[i])
          {
          ininit_flag = TRUE;
          break;
          }
     }
   if (*outsys != GEO)
     {
     if ((outzn[*outsys] != *outzone) || (outdat[*outsys] != *outspheroid) || 
         (outpj[*outsys] != *outsys))
        {
        outinit_flag = TRUE;
        }
     else
     for (i = 0; i < 13; i++)
        if (pdout[*outsys][i] != outparm[i])
          {
          outinit_flag = TRUE;
          break;
          }
     }
   }

/* Check input and output projection numbers
------------------------------------------*/
if ((*insys < GEO) || (*insys > MAXPROJ))
   {
   GCTP_PRINT_ERROR("Insys is illegal");
   *iflg = 1;
   return;
   }
if ((*outsys < GEO) || (*outsys > MAXPROJ))
   {
   GCTP_PRINT_ERROR("Outsys is illegal");
   *iflg = 2;
   return;
   }

/* find the correct conversion factor for units
---------------------------------------------*/
unit = *inunit;

/* find the factor unit conversions--all transformations are in radians
   or meters
  --------------------------------*/
if (*insys == GEO)
   *iflg = untfz(unit,RADIAN,&factor); 
else
   *iflg = untfz(unit,METER,&factor); 
if (*iflg != 0)
   {
   return;
   }
 
x = incoor[0] * factor;
y = incoor[1] * factor;

/* Initialize inverse transformation
----------------------------------*/
if (ininit_flag)
   {
   inpj[*insys] = *insys;
   indat[*insys] = *inspheroid;
   inzn[*insys] = *inzone;
   for (i = 0;i < COEFCT; i++)
      pdin[*insys][i] = inparm[i];

   /* Call the initialization function
   ----------------------------------*/
   inv_init(*insys,*inzone,inparm,*inspheroid,iflg,inv_trans);
   if (*iflg != 0)
      {
      return;
      }
   }

/* Do actual transformations
--------------------------*/

/* Inverse transformations
------------------------*/
if (*insys == GEO)
   {
   lon = x;
   lat = y;
   }
else
if ((*iflg = inv_trans[*insys](x, y, &lon, &lat)) != 0)
   {
   return;
   }

/* DATUM conversion should go here
--------------------------------*/

/* 
   The datum conversion facilities should go here 
*/

/* Initialize forward transformation
----------------------------------*/
if (outinit_flag)
   {
   outpj[*outsys] = *outsys;
   outdat[*outsys] = *outspheroid;
   outzn[*outsys] = *outzone;
   for (i = 0;i < COEFCT; i++)
      pdout[*outsys][i] = outparm[i];

   /* Call the forward initialization function */
   for_init(*outsys,*outzone,outparm,*outspheroid,iflg,for_trans);
   if (*iflg != 0)
      {
      return;
      }
   }

/* Forward transformations
------------------------*/
if (*outsys == GEO)
   {
   outcoor[0] = lon;
   outcoor[1] = lat;
   }
else
if ((*iflg = for_trans[*outsys](lon, lat, &outcoor[0], &outcoor[1])) != 0)
   {
   return;
   }

/* find the correct conversion factor for units
---------------------------------------------*/
unit = *outunit;

if (*outsys == GEO)
   *iflg = untfz(RADIAN,unit,&factor); 
else
   *iflg = untfz(METER,unit,&factor); 

outcoor[0] *= factor;
outcoor[1] *= factor;
return;
}
