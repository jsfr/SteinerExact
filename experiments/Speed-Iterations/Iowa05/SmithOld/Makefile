
TARGETS = $(subst .json,.txt,$(subst ../Instances/,,$(wildcard ../Instances/*.json)))
TIMER = ../../helpers/timer.sh
STEINER = ../../../WarrenSmith/steinertree_old
CONVERT = ../../helpers/convert_json.sh

%.txt: ../Instances/%.json
	$(CONVERT) $^ | $(TIMER) $(STEINER) > $@

all: $(TARGETS)

clean:
	rm *.txt
