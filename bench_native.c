// Challenge 24.3
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>

#include "object.h"
#include "vm.h"

#define CALLS 10000000
#define RUNS 5

static bool noopNative(int argCount, Value* args, Value* result, const char** error) {
    (void)argCount;
    (void)args;
    (void)error;
    *result = NIL_VAL;
    return true;
}

static double now(void) {
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    return timestamp.tv_sec + timestamp.tv_nsec / 1e9;
}

int main(void) {
    initVM();
    push(OBJ_VAL(copyString("noop", 4)));
    push(OBJ_VAL(newNative(noopNative, 0)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();

    for (int run = 0; run < RUNS; run++) {
        double start = now();
        if (interpret("for (var i = 0; i < 10000000; i = i + 1) noop();") != INTERPRET_OK) {
            freeVM();
            return 1;
        }
        double elapsed = now() - start;
        printf("%.3f s, %.1f ns/call (including loop)\n", elapsed, elapsed * 1e9 / CALLS);
    }

    freeVM();
    return 0;
}