CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS ?= -Isrc
TARGET := build/minihv
SOURCES := $(shell find src -name '*.c' -print)
APP_SOURCES := $(filter-out src/main.c,$(SOURCES))
TEST_NAMES := util config loader vm cpu sched mem io ext log hv cli
TEST_BINS := $(addprefix build/test_,$(TEST_NAMES))

.PHONY: all run test valgrind perf clean
all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build out
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

run: $(TARGET)
	./$(TARGET) -c conf/hv.conf

build/test_%: tests/test_%.c $(APP_SOURCES)
	@mkdir -p build out
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(APP_SOURCES) -o $@

test: $(TARGET) $(TEST_BINS)
	@set -e; for test_bin in $(TEST_BINS); do ./$$test_bin; done
	sh tests/test.sh

valgrind: $(TARGET)
	sh tests/valgrind.sh

perf: $(TARGET)
	./$(TARGET) -c conf/performance-8vm.conf >/dev/null

clean:
	rm -rf build
	find out -mindepth 1 -maxdepth 1 ! -name .gitkeep -exec rm -rf {} +
	mkdir -p out
	touch out/.gitkeep
