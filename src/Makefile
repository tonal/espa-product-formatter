#------------------------------------------------------------------------------
# Makefile
#------------------------------------------------------------------------------

SUBDIRS = raw_binary

# Build the common libraries and applications
all:
	@for dir in $(SUBDIRS); do \
        echo "make all in $$dir..."; \
        (cd $$dir; $(MAKE)); done

# Install the common libraries and applications
install: all
	@for dir in $(SUBDIRS); do \
        echo "make install in $$dir..."; \
        (cd $$dir; $(MAKE) install); done

# Cleanup
clean:
	@for dir in $(SUBDIRS); do \
        echo "make clean in $$dir..."; \
        (cd $$dir; $(MAKE) clean); done

