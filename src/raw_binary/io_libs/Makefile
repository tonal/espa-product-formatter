#-----------------------------------------------------------------------
# Makefile for ESPA raw binary I/O libraries
# NOTE: when building with this library, make sure to include
#    LIBS = -L$(XML2LIB) -lxml2 -lm
#    INCS = -I$(XML2INC)
# with your make options.
#-----------------------------------------------------------------------

ARCHIVE = lib_espa_raw_binary.a
CC = gcc
CFLAGS = -Wall -g
RM = rm -f
AR = ar rcsv


#--------------------------
# Define the include files:
#--------------------------
INCS = -I$(XML2INC) -I../common
INC = envi_header.h espa_metadata.h meta_stack.h parse_metadata.h \
    raw_binary_io.h write_metadata.h subset_metadata.h gctp_defines.h

#-----------------------------------------
# Define the source code and object files:
#-----------------------------------------
SRC	= \
	envi_header.c espa_metadata.c meta_stack.c parse_metadata.c  \
    raw_binary_io.c write_metadata.c subset_metadata.c

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

