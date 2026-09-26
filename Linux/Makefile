
prefix ?= /usr/local
export prefix

mandir = $(prefix)/share/man
man1dir = $(mandir)/man1

TOPTARGETS := all clean

SUBDIRS := ../P372/Linux/ \
	../P533/Linux/ \
	../ITURHFProp/Linux \
	../ITURHFProp/Src/CircuitCSV \
	../P372/Src/ITURNoise

datarootdir = $(prefix)/share
datadir = $(datarootdir)

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

# Each library is linked against before the programs are, so under make -j a
# directory waits for the libraries it links against.
../P533/Linux/: ../P372/Linux/
../ITURHFProp/Linux ../ITURHFProp/Src/CircuitCSV: ../P533/Linux/ ../P372/Linux/
../P372/Src/ITURNoise: ../P372/Linux/

.PHONY: $(TOPTARGETS) $(SUBDIRS)

# Each directory in SUBDIRS is made with the same goal, so "make install"
# here installs the libraries and all three programs, CircuitCSV and
# ITURNoise included. install -D is GNU-only; create the directories with
# install -d so this also works with the BSD install on macOS.
install: $(SUBDIRS)
	install -d $(DESTDIR)$(man1dir)
	install -m 644 ITURHFProp.1 $(DESTDIR)$(man1dir)
	install -d $(DESTDIR)$(datadir)/iturhfprop/data
	install -m 644 ../P372/Data/*.txt $(DESTDIR)$(datadir)/iturhfprop/data
	install -m 644 ../P372/Data/*.bin $(DESTDIR)$(datadir)/iturhfprop/data
