TARGETS = simple simplesort smithnew smithnewsort smithold

MAKEOPTS = -j 8

all: $(TARGETS)

simple:
	cd Simple; make $(MAKEOPTS)

simplesort:
	cd SimpleSort; make $(MAKEOPTS)

smithnew:
	cd SmithNew; make $(MAKEOPTS)

smithnewsort:
	cd SmithNewSort; make $(MAKEOPTS)

smithold:
	cd SmithOld; make $(MAKEOPTS)

clean:
	cd Simple; make clean
	cd SimpleSort; make clean
	cd SmithNew; make clean
	cd SmithNewSort; make clean
	cd SmithOld; make clean
