
TARGETS = $(subst .json,.txt,$(subst ../Instances/,,$(wildcard ../Instances/*.json)))
STEINER = ../../../steinertree -iteration=smith -sort

%.txt: ../Instances/%.json
	$(STEINER) $^ > $@

all: $(TARGETS)

clean:
	rm *.txt
