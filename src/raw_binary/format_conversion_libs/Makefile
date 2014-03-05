#-----------------------------------------------------------------------
# Makefile for ESPA format conversion libraries
# NOTE: when building with this library, make sure to include
#    LIBS = -L$(XML2LIB) -lxml2 -L$(GEOTIFF_LIB) -lgeotiff -L$(TIFFLIB) -ltiff
#           -L$(HDFEOS_LIB) -lhdfeos -L$(HDFLIB) -lmfhdf -ldf
#           -L$(HDFEOS_GCTPLIB) -lGctp -L$(JPEGLIB) -ljpeg -lz
#           
#    INCS = -I$(XML2INC) -I$(TIFFINC) -I$(GEOTIFF_INC) -I$(HDFINC)
#           -I$(HDFEOS_INC) -I$(HDFEOS_GCTPINC)
# with your make options.
#-----------------------------------------------------------------------

ARCHIVE = lib_espa_format_conversion.a
CC = gcc
CFLAGS = -Wall -g
RM = rm -f
AR = ar rcsv


#--------------------------
# Define the include files:
#--------------------------
INCS = -I$(XML2INC) -I../common -I../io_libs -I$(JPEGINC) -I$(TIFFINC) \
       -I$(GEOTIFF_INC) -I$(HDFINC) -I$(HDFEOS_INC) -I$(HDFEOS_GCTPINC)
INC = convert_lpgs_to_espa.h convert_espa_to_hdf.h espa_hdf.h espa_hdf_eos.h \
      convert_espa_to_gtif.h

#-----------------------------------------
# Define the source code and object files:
#-----------------------------------------
SRC	= \
	convert_lpgs_to_espa.c convert_espa_to_hdf.c espa_hdf.c espa_hdf_eos.c \
    convert_espa_to_gtif.c espa_geoloc.c

OBJ = $(SRC:.c=.o)

#-------------------------------
# Rules for building the archive
#-------------------------------
$(ARCHIVE): $(OBJ) $(INC)
	$(AR) $(ARCHIVE) $(OBJ)

all: $(ARCHIVE)

install:
	install -d ../lib
	install -d ../include
	install -m 644 $(ARCHIVE) ../lib
	install -m 644 $(INC) ../include

clean:
	$(RM) $(OBJ) $(ARCHIVE)

#-------------------------------------
# Rules for compiling the object files
#-------------------------------------

$(OBJ):

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $<

