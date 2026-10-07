CC ?= cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -ffreestanding

all: ribat-hash-test ribat-capability-test

ribat-hash-test: src/sha256.c src/sha256.h src/object_id.c src/object_id.h tests/test_sha256.c
	$(CC) $(CFLAGS) -Isrc -o $@ src/sha256.c src/object_id.c tests/test_sha256.c

ribat-capability-test: src/capability.c src/capability.h src/sha256.h tests/test_capability.c
	$(CC) $(CFLAGS) -Isrc -o $@ src/capability.c tests/test_capability.c

test: ribat-hash-test ribat-capability-test
	./ribat-hash-test
	./ribat-capability-test

clean:
	rm -f ribat-hash-test ribat-capability-test

.PHONY: all test clean
