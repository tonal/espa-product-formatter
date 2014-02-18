#-----------------------------------------------------------------------
# Makefile for ESPA common libraries
#-----------------------------------------------------------------------

ARCHIVE = lib_espa_common.a
CC = gcc
CFLAGS = -Wall -g
RM = rm -f
AR = ar rcsv

#--------------------------
# Define the include files:
#--------------------------
INCS =
INC = common.h error_handler.h

#-----------------------------------------
# Define the source code and object files:
#-----------------------------------------
SRC	= \
	error_handler.c

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

