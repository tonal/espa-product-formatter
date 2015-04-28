#-----------------------------------------------------------------------
# Makefile for ESPA land water mask library
# NOTE: when building with this library, make sure to include
#    LIBS = -L$(XML2LIB) -lxml2  \
#           -L$(GCTP3LIB) -lgctp3 -L$(ZLIBLIB) -lz
#
#    INCS = -I$(XML2INC)
# with your make options.
#-----------------------------------------------------------------------

ARCHIVE = lib_espa_land_water_mask.a
CC = gcc
CFLAGS = -g -Wall -O2 
RM = rm -f
AR = ar rcsv

#--------------------------
# Define the include files:
#--------------------------
INC = -I../include -I$(XML2INC) -I$(HDFINC) -I$(HDFEOS_INC)
INCS = ias_lw_geo.h generate_land_water_mask.h

#-----------------------------------------
# Define the source code and object files:
#-----------------------------------------
SRC	= \
	generate_land_water_mask.c \
	deg_to_dms.c \
	ias_math_point_in_closed_polygon.c \
	ias_geo_convert_dms2deg.c \
	ias_geo_convert_deg2dms.c \
	ias_geo_find_deg.c \
	ias_geo_find_min.c \
	ias_geo_find_sec.c \
	ias_geo_handle_180.c \
	ias_geo_projection_transformation.c \
	ias_geo_shape_file.c \
	ias_geo_shape_mask.c

OBJ	= $(SRC:.c=.o)

#------------------------------------------------------------------------------
# Targets for each executable. 
#------------------------------------------------------------------------------
all: $(ARCHIVE)

$(ARCHIVE): $(OBJ)
	$(AR) $(ARCHIVE) $(OBJ) 
	install -d ../lib
	install -d ../include
	install -m 644 $(ARCHIVE) ../lib
	install -m 644 $(INCS) ../include

install: land_no_buf.ply
	install -d $(PREFIX)/static_data
	install -m 644 land_no_buf.ply $(PREFIX)/static_data

clean:
	$(RM) $(OBJ) $(ARCHIVE)

#---------------------------------------
# Target to distribute the executable(s)
#---------------------------------------
.c.o:
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

