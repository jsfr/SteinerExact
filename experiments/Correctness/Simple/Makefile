
TARGETS = $(subst .json,.txt,$(subst ../Instances/,,$(wildcard ../Instances/*.json)))
STEINER = ../../../steinertree -iteration=simple -sort

%.txt: ../Instances/%.json
	$(STEINER) $^ > $@

all: $(TARGETS)

clean:
	rm *.txt
