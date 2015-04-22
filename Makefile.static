SUBDIRS	= py_modules scripts src

all:
	@for dir in $(SUBDIRS); do \
        echo "make all in $$dir..."; \
        (cd $$dir; $(MAKE)); done

install: all
ifeq ($(PREFIX),)
	echo "WARNING: PREFIX environment variable is not defined!  Schema will not be installed."
else ifneq ($(PREFIX), $(CURDIR))
	echo "Installing schema ..."; \
	install -d $(PREFIX)/schema
	install -m 644 ./schema/*.xsd $(PREFIX)/schema
else
	echo "$(PREFIX) is the same as the current directory. Schema file is already installed."
endif

	@for dir in $(SUBDIRS); do \
        echo "make install in $$dir..."; \
        (cd $$dir; $(MAKE) install); done

clean:
	@for dir in $(SUBDIRS); do \
        echo "make clean in $$dir..."; \
        (cd $$dir; $(MAKE) clean); done

