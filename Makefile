CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2
CPPFLAGS += -Iinclude
LDLIBS += -lm
BUILD ?= build
CORE = src/simulation.c src/analysis.c src/smoothing.c src/feedback.c

.PHONY: all run test
all: $(BUILD)/demo

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/demo: src/main.c $(CORE) include/demo.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c $(CORE) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD)/test_demo: tests/test_demo.c $(CORE) include/demo.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG tests/test_demo.c $(CORE) $(LDFLAGS) $(LDLIBS) -o $@

run: $(BUILD)/demo
	$(BUILD)/demo

test: $(BUILD)/test_demo
	$(BUILD)/test_demo
