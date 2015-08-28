TARGETS = geosteiner simple smithnew simplesort smithnewsort
MAKE = make
MAKE_OPTS = -j 8

all: $(TARGETS)

simple:
	cd Simple; $(MAKE) $(MAKE_OPTS)

simplesort:
	cd SimpleSort; $(MAKE) $(MAKE_OPTS)

geosteiner:
	cd GeoSteiner; $(MAKE)

smithnew:
	cd SmithNew; $(MAKE) $(MAKE_OPTS)

smithnewsort:
	cd SmithNewSort; $(MAKE) $(MAKE_OPTS)

clean:
	cd Simple; $(MAKE) clean
	cd SimpleSort; $(MAKE) clean
	cd GeoSteiner; $(MAKE) clean
	cd SmithNew; $(MAKE) clean
	cd SmithNewSort; $(MAKE) clean
