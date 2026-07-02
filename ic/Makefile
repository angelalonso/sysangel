CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -O2 -std=gnu11
LDFLAGS  ?=

TEST_DIR  := tests
BIN_DIR   := bin

TARGET    := $(BIN_DIR)/ic
DEMO      := $(BIN_DIR)/demo_target

.PHONY: all build test run run-demo clean help

all: build

help:
	@echo "Targets:"
	@echo "  make build      - compile ic into $(TARGET)"
	@echo "  make test       - build ic and a demo target, then trace the demo"
	@echo "  make run ARGS='PROGRAM [ARGS...]'  - build and run ic against PROGRAM"
	@echo "  make run-demo   - build and run ic against the bundled demo target"
	@echo "  make clean      - remove build artifacts"

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

build: $(BIN_DIR) $(TARGET)

$(TARGET): ic.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(DEMO): $(TEST_DIR)/demo_target.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Builds everything and runs ic against the bundled demo program so you
# can see disk opens/reads/writes/deletes and a network connect attempt.
test: build $(DEMO)
	@echo "== running ic against tests/demo_target =="
	./$(TARGET) -- ./$(DEMO)

# Run ic against an arbitrary program, e.g.:
#   make run ARGS="curl -s https://example.com"
run: build
	@if [ -z "$(ARGS)" ]; then \
		echo "usage: make run ARGS='PROGRAM [ARGS...]'"; exit 1; \
	fi
	./$(TARGET) -- $(ARGS)

run-demo: build $(DEMO)
	./$(TARGET) -- ./$(DEMO)

clean:
	rm -rf $(BIN_DIR)
