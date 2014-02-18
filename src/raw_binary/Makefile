LIBDIRS	= common io_libs format_conversion_libs
EXEDIRS = tools

all:
# build the libraries, which also need to be installed before the executables
# can be compiled
	@for dir in $(LIBDIRS); do \
        echo "make all in $$dir..."; \
        (cd $$dir; $(MAKE); $(MAKE) install); done

# build the executables
	@for dir in $(EXEDIRS); do \
        echo "make all in $$dir..."; \
        (cd $$dir; $(MAKE)); done

install: all
# the libraries have already been installed so just the executables
	@for dir in $(EXEDIRS); do \
        echo "make install in $$dir..."; \
        (cd $$dir; $(MAKE) install); done

clean:
# all directories need to be cleaned
	@for dir in $(LIBDIRS) $(EXEDIRS); do \
        echo "make clean in $$dir..."; \
        (cd $$dir; $(MAKE) clean); done

