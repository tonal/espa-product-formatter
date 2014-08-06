SUBDIRS	= py_modules scripts src

all:
	@for dir in $(SUBDIRS); do \
        echo "make all in $$dir..."; \
        (cd $$dir; $(MAKE)); done

install: all
	echo "Installing schema ..."; \
	install -d $(PREFIX)/schema
	install -m 644 ./schema/*.xsd $(PREFIX)/schema
	@for dir in $(SUBDIRS); do \
        echo "make install in $$dir..."; \
        (cd $$dir; $(MAKE) install); done

clean:
	@for dir in $(SUBDIRS); do \
        echo "make clean in $$dir..."; \
        (cd $$dir; $(MAKE) clean); done

