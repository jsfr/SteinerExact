
TARGETS = $(subst .json,.txt,$(subst ../Instances/,,$(wildcard ../Instances/*.json)))
TIMER = ../../helpers/timer.sh
STEINER = ../../../steinertree -iteration=smith -sort

%.txt: ../Instances/%.json
	$(TIMER) $(STEINER) $^ > $@

all: $(TARGETS)

clean:
	rm *.txt
