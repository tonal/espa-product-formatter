/****************************************************************************
Name: geographic

Purpose: Provides a "dummy" implementation of the geographic projection.
    Since the geographic projection is the base projection that everything
    goes through when being transformed, there isn't any transformation
    needed for it.

****************************************************************************/
#include "local.h"
#include "cproj.h"

/****************************************************************************
Name: geo_print_info

Purpose: Prints out the projection parameter information for the geographic
    projection (there aren't any).

****************************************************************************/
static void geo_print_info
(
    const TRANSFORMATION *trans
)
{
    gctp_print_title("GEOGRAPHIC");
}

/****************************************************************************
Name: gctp_geo_init

Purpose: Initialization routine for the geographic projection. 

Returns:
    GCTP_SUCCESS (can't fail, but needs to use the standard init routine
        prototype)

****************************************************************************/
int gctp_geo_init(TRANSFORMATION *trans)
{
    /* only need to set the print info routine here */
    trans->print_info = geo_print_info;
    return GCTP_SUCCESS;
}
