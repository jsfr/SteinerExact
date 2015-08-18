TARGETS = $(subst .json,.txt,$(subst ../Instances/,,$(wildcard ../Instances/*.json)))
GEOPATH = ../../../GeoSteiner

%.txt: ../Instances/%.json
	(../helpers/jsonToGeoSteiner.sh $^ | $(GEOPATH)/efst | $(GEOPATH)/bb) > $@

all: $(TARGETS)

clean:
	rm *.txt
