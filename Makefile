CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2
CPPFLAGS += -Iinclude
LDLIBS += -lm
BUILD ?= build
CORE = src/simulation.c src/analysis.c src/smoothing.c src/feedback.c
ESTIMATOR = src/estimator.c src/noise.c
PYTHON ?= python3
LIB_OBJECTS = $(patsubst src/%.c,$(BUILD)/%.o,$(CORE) $(ESTIMATOR))

.PHONY: all run test test-c test-cli experiment lib example benchmark-runtime
all: $(BUILD)/demo $(BUILD)/thermo $(BUILD)/libfluorescence.a

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/demo: src/main.c $(CORE) include/demo.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c $(CORE) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD)/test_demo: tests/test_demo.c $(CORE) include/demo.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG tests/test_demo.c $(CORE) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD)/thermo: src/cli.c src/frame_io.c $(CORE) $(ESTIMATOR) include/demo.h include/estimator.h include/frame_io.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/cli.c src/frame_io.c $(CORE) $(ESTIMATOR) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD)/test_estimator: tests/test_estimator.c $(CORE) $(ESTIMATOR) include/demo.h include/estimator.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG tests/test_estimator.c $(CORE) $(ESTIMATOR) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD)/%.o: src/%.c include/demo.h include/estimator.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/libfluorescence.a: $(LIB_OBJECTS)
	$(AR) rcs $@ $^

lib: $(BUILD)/libfluorescence.a

$(BUILD)/example: examples/estimate.c $(BUILD)/libfluorescence.a include/estimator.h include/demo.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(BUILD)/libfluorescence.a $(LDFLAGS) $(LDLIBS) -o $@

example: $(BUILD)/example
	$(BUILD)/example

$(BUILD)/bench_runtime: tools/bench_runtime.c $(BUILD)/libfluorescence.a include/estimator.h include/demo.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(BUILD)/libfluorescence.a $(LDFLAGS) $(LDLIBS) -o $@

benchmark-runtime: $(BUILD)/bench_runtime
	$(BUILD)/bench_runtime

run: $(BUILD)/demo
	$(BUILD)/demo

test-c: $(BUILD)/test_demo $(BUILD)/test_estimator
	$(BUILD)/test_demo
	$(BUILD)/test_estimator

test-cli: $(BUILD)/thermo
	$(PYTHON) tests/test_cli.py $(BUILD)/thermo
	$(PYTHON) tests/test_report.py $(BUILD)/thermo

test: test-c test-cli example

experiment: $(BUILD)/thermo
	$(BUILD)/thermo benchmark --trials 128 --seed 42 > $(BUILD)/benchmark.csv
	$(PYTHON) tools/report.py $(BUILD)/benchmark.csv --output $(BUILD)/report
