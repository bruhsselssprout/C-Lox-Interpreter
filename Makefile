CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -g
TARGET = clox
SOURCES = main.c chunk.c debug.c memory.c value.c vm.c scanner.c compiler.c object.c table.c

.PHONY: all run clean test bench bench_locals bench_vm

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

test:
	@set -eu; \
	test_dir=$$(mktemp -d "$${TMPDIR:-/tmp}/clox-native-arity.XXXXXX"); \
	trap 'rm -rf "$$test_dir"' EXIT HUP INT TERM; \
	for vm in vm optimal_vm; do \
		$(CC) $(CFLAGS) -DCLOX_BENCH -I. tests/native_arity.c \
			$(filter-out main.c vm.c,$(SOURCES)) "$$vm.c" -o "$$test_dir/$$vm"; \
		"$$test_dir/$$vm"; \
	done

bench: bench_table.c $(filter-out main.c,$(SOURCES))
	$(CC) -std=c11 -O2 -Wall -Wextra bench_table.c $(filter-out main.c,$(SOURCES)) -o bench_table
	./bench_table

bench_locals: bench_locals.c
	$(CC) -std=c11 -O2 -Wall -Wextra bench_locals.c -o bench_locals
	./bench_locals

bench_vm: bench_vm.c
	$(CC) -std=gnu11 -O2 -Wall -Wextra bench_vm.c -o bench_vm
	./bench_vm
