#ifndef GCTP_H
#define GCTP_H

/* Projection codes
   0 = Geographic
   1 = Universal Transverse Mercator (UTM)
   2 = State Plane Coordinates
   3 = Albers Conical Equal Area
   4 = Lambert Conformal Conic
   5 = Mercator
   6 = Polar Stereographic
   7 = Polyconic
   8 = Equidistant Conic
   9 = Transverse Mercator
  10 = Stereographic
  11 = Lambert Azimuthal Equal Area
  12 = Azimuthal Equidistant
  13 = Gnomonic
  14 = Orthographic
  15 = General Vertical Near-Side Perspective
  16 = Sinusiodal
  17 = Equirectangular
  18 = Miller Cylindrical
  19 = Van der Grinten
  20 = (Hotine) Oblique Mercator 
  21 = Robinson
  22 = Space Oblique Mercator (SOM)
  23 = Alaska Conformal
  24 = Interrupted Goode Homolosine 
  25 = Mollweide
  26 = Interrupted Mollweide
  27 = Hammer
  28 = Wagner IV
  29 = Wagner VII
  30 = Oblated Equal Area
  31 = Integerized Sinusiodal
  99 = User defined
*/

/* Define projection codes */
#define GEO 0
#define UTM 1
#define SPCS 2
#define ALBERS 3
#define LAMCC 4
#define MERCAT 5
#define PS 6
#define POLYC 7
#define EQUIDC 8
#define TM 9
#define STEREO 10
#define LAMAZ 11
#define AZMEQD 12
#define GNOMON 13
#define ORTHO 14
#define GVNSP 15
#define SNSOID 16
#define EQRECT 17
#define MILLER 18
#define VGRINT 19
#define HOM 20
#define ROBIN 21
#define SOM 22
#define ALASKA 23
#define GOOD 24
#define MOLL 25
#define IMOLL 26
#define HAMMER 27
#define WAGIV 28
#define WAGVII 29
#define OBEQA 30
#define ISIN 31
#define USDEF 99 

#define MAXPROJ 31		/* largest supported projection number */

/* Define the number of projection parameters */
#define GCTP_PROJECTION_PARAMETER_COUNT 15

/* Define the return values for error and success */
#define GCTP_ERROR -1
#define GCTP_SUCCESS 0

/* Define the return value for a coordinate that is in a "break" area of an
   interrupted projection like Goode's */
#define GCTP_IN_BREAK -2

/* Define the unit codes */
#define RADIAN  0
#define FEET    1
#define METER   2
#define SECOND  3
#define DEGREE  4
#define DMS     5

/* Define a structure to hold the information that defines a projection */
typedef struct gctp_projection
{
    int proj_code;      /* Projection code */
    int zone;           /* Projection zone number - only has meaning for
                           projections like UTM and stateplane */
    int units;          /* Units of coordinates */
    int spheroid;       /* Spheroid code for the projection */
    double parameters[GCTP_PROJECTION_PARAMETER_COUNT];
                        /* Array of projection parameters */

} GCTP_PROJECTION;

/* Forward declaration of the GCTP_TRANSFORMATION type.  This is done to
   prevent users of the library from seeing the details of what is stored in
   the structure. */
typedef struct gctp_transformation GCTP_TRANSFORMATION;

/* Routine to create a transformation between the input and output projections
   provided.  A pointer to the transformation is returned (NULL if an error
   occurs).  gctp_destroy_transformation should be called to clean up the
   transformation when it is no longer needed. */
GCTP_TRANSFORMATION *gctp_create_transformation
(
    const GCTP_PROJECTION *input_projection,    /* I: starting projection */
    const GCTP_PROJECTION *output_projection    /* I: ending projection */
);

/* Routine to free any resources allocated in the creation of the
   transformation. */
void gctp_destroy_transformation
(
    GCTP_TRANSFORMATION *trans
);

/* Routine to transform coordinates using a transformation set up by
   gctp_create_transformation. */
int gctp_transform
(
    const GCTP_TRANSFORMATION *trans, /* I: transformation to use */
    const double *in_coor, /* I: array of (lon, lat) or (x, y) */
    double * out_coor      /* O: array of (x, y) or (lon, lat) */
);

typedef enum gctp_message_type_enum
{
    GCTP_INFO_MESSAGE,
    GCTP_ERROR_MESSAGE,
    GCTP_MESSAGE_TYPE_COUNT
} GCTP_MESSAGE_TYPE_ENUM;

/* Define the type for a callback print routine to allow the library user
   to deal with output messages. */
typedef void (*GCTP_CALLBACK_PRINT_FUNC)
(
    GCTP_MESSAGE_TYPE_ENUM message_type, /* I: message type */
    const char *filename,     /* I: source code file name for input */
    int line_number,          /* I: source code line number for input */
    const char *format, ...   /* I: format string for message */
);
void gctp_set_message_callback(GCTP_CALLBACK_PRINT_FUNC callback);

/* Routine to print information about the transformation (such as the input
   and output projection parameters) */
void gctp_print_transformation_info
(
    const GCTP_TRANSFORMATION *trans
);

/* Routine to return the input projection defined in a transformation */
const GCTP_PROJECTION *gctp_get_input_proj
(
    const GCTP_TRANSFORMATION *trans
);

/* Routine to return the output projection defined in a transformation */
const GCTP_PROJECTION *gctp_get_output_proj
(
    const GCTP_TRANSFORMATION *trans
);

int gctp_calc_utm_zone
(
    double lon          /* I: longitude (in degrees) */
);

void gctp_only_allow_threadsafe_transforms();

#endif
