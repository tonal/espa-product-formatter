#------------------------------------------------------------------------------
# Makefile for sample XML code
#------------------------------------------------------------------------------

# Set up compile options
CC = gcc
RM = rm -f
EXTRA = -Wall -g -O2

# Define the include files
INC = 
INCDIR  = -I. -I../include -I$(XML2INC) -I$(TIFFINC) -I$(GEOTIFF_INC) \
    -I$(HDFINC) -I$(HDFEOS_INC) -I$(JBIGINC) -I$(LZMAINC)
NCFLAGS = $(EXTRA) $(INCDIR)

# Define the source code and object files
SRC1 = convert_lpgs_to_espa.c
OBJ1 = $(SRC1:.c=.o)

SRC2 = convert_espa_to_hdf.c
OBJ2 = $(SRC2:.c=.o)

SRC3 = convert_espa_to_gtif.c
OBJ3 = $(SRC3:.c=.o)

SRC4 = espa_band_subset.c
OBJ4 = $(SRC4:.c=.o)

SRC5 = espa_product_subset.c
OBJ5 = $(SRC5:.c=.o)

SRC6 = convert_modis_to_espa.c
OBJ6 = $(SRC6:.c=.o)

SRC7 = create_l8_angle_bands.c
OBJ7 = $(SRC7:.c=.o)

# Define the object libraries
LIB1   = \
    -L../lib -l_espa_format_conversion -l_espa_raw_binary -l_espa_common \
    -L$(XML2LIB) -lxml2 -L$(GEOTIFF_LIB) -lgeotiff -L$(TIFFLIB) -ltiff \
    -L$(JPEGLIB) -ljpeg -L$(JBIGLIB) -ljbig -L$(HDFEOS_GCTPLIB) -lGctp -lz -lm

LIB2   = \
    -L../lib -l_espa_format_conversion -l_espa_raw_binary -l_espa_common \
    -L$(XML2LIB) -lxml2 \
    -L$(HDFEOS_LIB) -lhdfeos -L$(HDFEOS_GCTPLIB) -lGctp \
    -L$(HDFLIB) -lmfhdf -ldf -L$(JPEGLIB) -ljpeg -L$(JBIGLIB) -ljbig -lz -lm

LIB3   = \
    -L../lib -l_espa_format_conversion -l_espa_raw_binary -l_espa_common \
    -L$(XML2LIB) -lxml2 -L$(JPEGLIB) -ljpeg -L$(JBIGLIB) -ljbig -lz -lm

LIB4   = \
    -L../lib -l_espa_format_conversion -l_espa_raw_binary -l_espa_common \
    -L$(XML2LIB) -lxml2 -L$(JPEGLIB) -ljpeg -L$(JBIGLIB) -ljbig -lz -lm

LIB5   = \
    -L../lib -l_espa_format_conversion -l_espa_raw_binary -l_espa_common \
    -L$(XML2LIB) -lxml2 -L$(JPEGLIB) -ljpeg -L$(JBIGLIB) -ljbig -lz -lm

LIB6   = \
    -L../lib -l_espa_format_conversion -l_espa_raw_binary -l_espa_common \
    -L$(XML2LIB) -lxml2 \
    -L$(HDFEOS_LIB) -lhdfeos -L$(HDFEOS_GCTPLIB) -lGctp \
    -L$(HDFLIB) -lmfhdf -ldf -L$(JPEGLIB) -ljpeg -L$(JBIGLIB) -ljbig -lz -lm

LIB7   = \
    -L../lib -l_espa_common -l_espa_l8_angles -l_espa_l8_ang \
    -l_espa_raw_binary -lz -lm

# Define the executable
EXE = convert_lpgs_to_espa convert_espa_to_hdf convert_espa_to_gtif \
      espa_band_subset espa_product_subset convert_modis_to_espa \
      create_l8_angle_bands

# Target for the executable
all: $(EXE)
	install -d ../bin
	install -m 755 $(EXE) ../bin

convert_lpgs_to_espa: $(OBJ1) $(INC)
	$(CC) $(NCFLAGS) -o convert_lpgs_to_espa $(OBJ1) $(LIB1)

convert_espa_to_hdf: $(OBJ2) $(INC)
	$(CC) $(NCFLAGS) -o convert_espa_to_hdf $(OBJ2) $(LIB2)

convert_espa_to_gtif: $(OBJ3) $(INC)
	$(CC) $(NCFLAGS) -o convert_espa_to_gtif $(OBJ3) $(LIB3)

espa_band_subset: $(OBJ4) $(INC)
	$(CC) $(NCFLAGS) -o espa_band_subset $(OBJ4) $(LIB4)

espa_product_subset: $(OBJ5) $(INC)
	$(CC) $(NCFLAGS) -o espa_product_subset $(OBJ5) $(LIB5)

convert_modis_to_espa: $(OBJ6) $(INC)
	$(CC) $(NCFLAGS) -o convert_modis_to_espa $(OBJ6) $(LIB6)

create_l8_angle_bands: $(OBJ7) $(INC)
	$(CC) $(NCFLAGS) -o create_l8_angle_bands $(OBJ7) $(LIB7)

clean:
	$(RM) *.o $(EXE)

$(OBJ1): $(INC)
$(OBJ2): $(INC)
$(OBJ3): $(INC)
$(OBJ4): $(INC)
$(OBJ5): $(INC)
$(OBJ6): $(INC)
$(OBJ7): $(INC)
.c.o:
	$(CC) $(NCFLAGS) -c $<

