#-----------------------------------------------------------------------
# Makefile for ESPA per-pixel angle libraries
# NOTE: when building with this library, make sure to include
#    LIBS = -l_espa_l8_ang.a -l_espa_l8_angles.a
#    INCS = 
# with your make options.
#-----------------------------------------------------------------------

ARCHIVE = lib_espa_l8_angles.a
CC = gcc
CFLAGS = -Wall -g -O2
RM = rm -f
AR = ar rcsv

#--------------------------
# Define the include files:
#--------------------------
INCS = -I$(XML2INC) -I../include
INC = l8_angles.h

#-----------------------------------------
# Define the source code and object files:
#-----------------------------------------
SRC	= \
	l8_angles.c angles_api.c

OBJ = $(SRC:.c=.o)

#-------------------------------
# Rules for building the archive
#-------------------------------
$(ARCHIVE): $(OBJ) $(INC)
	$(AR) $(ARCHIVE) $(OBJ)
	install -d ../lib
	install -d ../include
	install -m 644 $(ARCHIVE) ../lib
	install -m 644 $(INC) ../include

all: $(ARCHIVE)

clean:
	$(RM) $(OBJ) $(ARCHIVE)

#-------------------------------------
# Rules for compiling the object files
#-------------------------------------

$(OBJ):

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $<

