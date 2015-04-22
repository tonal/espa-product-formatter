LIBDIRS	= common io_libs format_conversion_libs per_pixel_angles_libs/ias_lib per_pixel_angles_libs land_water_mask_libs/GCTP3 land_water_mask_libs
EXEDIRS = tools

all:
# build the libraries
	@for dir in $(LIBDIRS); do \
        echo "make all in $$dir..."; \
        (cd $$dir; $(MAKE)); done

# build the executables
	@for dir in $(EXEDIRS); do \
        echo "make all in $$dir..."; \
        (cd $$dir; $(MAKE)); done

install: all
# install the libraries and executables
	@for dir in $(EXEDIRS); do \
        echo "installing all in $$dir..."; \
        (cd $$dir; $(MAKE) install); done

# if the ESPALIB environment variable points to the 'lib' directory, then
# there is no need to install anything
ifneq ($(ESPALIB), $(CURDIR)/lib)
	@for file in `ls lib`; do \
		echo "installing $$file to $(ESPALIB)"; \
		install -d $(ESPALIB); \
		install -m 644 lib/$$file $(ESPALIB); done
else
	echo "$(ESPALIB) is the same as the lib directory. Libraries already installed."
endif

# if the ESPAINC environment variable points to the 'include' directory, then
# there is no need to install anything
ifneq ($(ESPAINC), $(CURDIR)/include)
	@for file in `ls include`; do \
        echo "installing $$file to $(ESPAINC)"; \
        install -d $(ESPAINC); \
        install -m 644 include/$$file $(ESPAINC); done
else
	echo "$(ESPAINC) is the same as the include directory. Include files already installed."
endif

clean:
# all directories need to be cleaned
	@for dir in $(LIBDIRS) $(EXEDIRS); do \
        echo "make clean in $$dir..."; \
        (cd $$dir; $(MAKE) clean); done
	rm -r include lib
