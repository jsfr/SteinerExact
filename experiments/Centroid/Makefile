
TARGETS = $(subst .json,.txt,$(subst ../Correctness/Instances/,,$(wildcard ../Correctness/Instances/*.json)))
STEINER = ../../steinertree -iteration=smith

%.txt: ../Correctness/Instances/%.json
	$(STEINER) $^ | grep -E "iterations|trees" > $@

all: $(TARGETS)

clean:
	rm *.txt
