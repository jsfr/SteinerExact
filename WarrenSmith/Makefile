CC = gcc
CFLAGS = -lm -std=c89

all: steinertree_old

steinertree_old: wds_smt_counting.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f steinertree_old
