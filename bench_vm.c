// Compares vm.c with optimal_vm.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RUNS 5
#define SRC "main.c chunk.c debug.c memory.c value.c scanner.c compiler.c object.c table.c"

static const char* names[] = {"loop", "fib(32)"};
static const char* sources[] = {
    "var s = 0; for (var i = 0; i < 30000000; i = i + 1) { s = s + i; } print s;",
    "fun fib(n) { if (n < 2) return n; return fib(n - 1) + fib(n - 2); } print fib(32);",
};

static void run(const char* cmd) {
    if (system(cmd) != 0) {
        fprintf(stderr, "Failed: %s\n", cmd);
        exit(1);
    }
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Average seconds per run of the given binary on /tmp/bench.lox.
static double timeBinary(const char* binary) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s /tmp/bench.lox > /dev/null", binary);
    double start = now();
    for (int i = 0; i < RUNS; i++) run(cmd);
    return (now() - start) / RUNS;
}

int main(void) {
    run("cc -std=c11 -O2 -w -DCLOX_BENCH " SRC " vm.c -o /tmp/clox_base");
    run("cc -std=c11 -O2 -w -DCLOX_BENCH " SRC " optimal_vm.c -o /tmp/clox_opt");

    printf("\n%-10s %10s %10s %9s %9s\n", "benchmark", "vm.c", "optimal", "speedup", "faster");
    printf("------------------------------------------------------\n");
    for (int i = 0; i < 2; i++) {
        FILE* f = fopen("/tmp/bench.lox", "w");
        fputs(sources[i], f);
        fclose(f);

        double base = timeBinary("/tmp/clox_base");
        double opt = timeBinary("/tmp/clox_opt");
        printf("%-10s %9.3fs %9.3fs %8.2fx %8.1f%%\n\n", names[i], base, opt, base / opt,
               (base - opt) / base * 100);
    }
    return 0;
}

