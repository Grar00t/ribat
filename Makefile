CC ?= cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -ffreestanding

all: ribat-hash-test

ribat-hash-test: src/sha256.c src/sha256.h src/object_id.c src/object_id.h tests/test_sha256.c
	$(CC) $(CFLAGS) -Isrc -o $@ src/sha256.c src/object_id.c tests/test_sha256.c

test: ribat-hash-test
	./ribat-hash-test

clean:
	rm -f ribat-hash-test

.PHONY: all test clean
