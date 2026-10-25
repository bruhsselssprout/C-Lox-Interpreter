// Challenge 20.3
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "object.h"
#include "table.h"
#include "vm.h"

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static volatile double sink;

static uint64_t rngState = 88172645463325252ULL;
static uint64_t rnd(void) {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 7;
    rngState ^= rngState << 17;
    return rngState;
}

static void report(const char* name, double ms, long ops, const Table* t) {
    printf("%-34s %9.2f ms  %8.1f ns/op  (count=%d cap=%d)\n", name, ms,
           ms * 1e6 / ops, t->count, t->capacity);
}

static void fill(Table* t, Value* keys, int n) {
    for (int i = 0; i < n; i++) tableSet(t, keys[i], NUMBER_VAL(i));
}

static double lookupAll(Table* t, Value* keys, int n) {
    double acc = 0;
    Value v;
    for (int i = 0; i < n; i++)
        if (tableGet(t, keys[i], &v)) acc += AS_NUMBER(v);
    return acc;
}

// Runs insert then hit-lookup then miss-lookup for a key set
static void keysetBench(const char* name, Value* keys, Value* misses, int n) {
    Table t;
    initTable(&t);
    double t0 = now();
    fill(&t, keys, n);
    double t1 = now();
    report(name, t1 - t0, n, &t);

    t0 = now();
    sink = lookupAll(&t, keys, n);
    t1 = now();
    printf("  %-32s %9.2f ms  %8.1f ns/op\n", "lookup (hit)", t1 - t0, (t1 - t0) * 1e6 / n);

    t0 = now();
    sink = lookupAll(&t, misses, n);
    t1 = now();
    printf("  %-32s %9.2f ms  %8.1f ns/op\n", "lookup (miss)", t1 - t0, (t1 - t0) * 1e6 / n);
    freeTable(&t);
}

static void keysets(int n) {
    Value* keys = malloc(sizeof(Value) * n);
    Value* misses = malloc(sizeof(Value) * n);

    puts("\n[1] Sequential integers");
    for (int i = 0; i < n; i++) { keys[i] = NUMBER_VAL(i); misses[i] = NUMBER_VAL(i + n); }
    keysetBench("insert", keys, misses, n);

    puts("\n[2] Random doubles");
    for (int i = 0; i < n; i++) {
        keys[i] = NUMBER_VAL((double)(rnd() % (1ULL << 52)) / 7.0);
        misses[i] = NUMBER_VAL(-1.0 - (double)i);
    }
    keysetBench("insert", keys, misses, n);

    puts("\n[3] Strided integers (multiples of 4096)");
    for (int i = 0; i < n; i++) { keys[i] = NUMBER_VAL((double)i * 4096); misses[i] = NUMBER_VAL((double)(i + n) * 4096); }
    keysetBench("insert", keys, misses, n);

    puts("\n[4] Interned strings");
    char buf[32];
    // Strings are interned in vm.strings, so building them also exercises tableFindString
    double t0 = now();
    for (int i = 0; i < n; i++) {
        int len = snprintf(buf, sizeof buf, "key_%d", i);
        keys[i] = OBJ_VAL(copyString(buf, len));
    }
    double t1 = now();
    printf("  %-32s %9.2f ms  %8.1f ns/op  (vm.strings cap=%d)\n", "intern new strings", t1 - t0,
           (t1 - t0) * 1e6 / n, vm.strings.capacity);
    t0 = now();
    for (int i = 0; i < n; i++) {
        int len = snprintf(buf, sizeof buf, "key_%d", i);
        sink = (double)(uintptr_t)copyString(buf, len);
    }
    t1 = now();
    printf("  %-32s %9.2f ms  %8.1f ns/op\n", "intern existing strings", t1 - t0, (t1 - t0) * 1e6 / n);
    for (int i = 0; i < n; i++) {
        int len = snprintf(buf, sizeof buf, "miss_%d", i);
        misses[i] = OBJ_VAL(copyString(buf, len));
    }
    keysetBench("insert", keys, misses, n);

    free(keys);
    free(misses);
}

// Steady-state: keep a sliding window of `window` live keys, insert one and delete one per step
static void churn(int window, int steps) {
    puts("\n[5] Delete churn (sliding window)");
    Table t;
    initTable(&t);
    for (int i = 0; i < window; i++) tableSet(&t, NUMBER_VAL(i), NIL_VAL);
    double t0 = now();
    for (int i = window; i < window + steps; i++) {
        tableSet(&t, NUMBER_VAL(i), NIL_VAL);
        tableDelete(&t, NUMBER_VAL(i - window));
    }
    double t1 = now();
    char name[64];
    snprintf(name, sizeof name, "window=%d steps=%d", window, steps);
    report(name, t1 - t0, steps, &t);
    printf("  live keys = %d, capacity needed ~ %d\n", window, (int)(window / 0.75));
    freeTable(&t);
}

// Lookup cost as the table outgrows each cache level
static void sizeSweep(void) {
    puts("\n[6] Lookup latency vs table size (random hit lookups)");
    for (int shift = 10; shift <= 22; shift += 2) {
        int n = 1 << shift;
        Value* keys = malloc(sizeof(Value) * n);
        for (int i = 0; i < n; i++) keys[i] = NUMBER_VAL((double)(rnd() % (1ULL << 40)));
        Table t;
        initTable(&t);
        fill(&t, keys, n);

        int lookups = 4000000;
        double acc = 0;
        Value v;
        double t0 = now();
        for (int i = 0; i < lookups; i++)
            if (tableGet(&t, keys[rnd() % n], &v)) acc += AS_NUMBER(v);
        double t1 = now();
        sink = acc;
        char name[32];
        snprintf(name, sizeof name, "n=%d", n);
        report(name, t1 - t0, lookups, &t);
        freeTable(&t);
        free(keys);
    }
}

// Cost of lookups as the load factor approaches the 0.75 maximum
static void loadSweep(void) {
    puts("\n[7] Miss-lookup cost vs load factor (linear probing)");
    int cap = 1 << 20;
    int n = (int)(cap * 0.75) - 1;
    Value* keys = malloc(sizeof(Value) * n);
    Value* misses = malloc(sizeof(Value) * n);
    for (int i = 0; i < n; i++) {
        keys[i] = NUMBER_VAL((double)(rnd() % (1ULL << 40)));
        misses[i] = NUMBER_VAL(-1.0 - i);
    }
    Table t;
    initTable(&t);
    int inserted = 0;
    for (double load = 0.25; load <= 0.75; load += 0.125) {
        int target = (int)(cap * load);
        if (target > n) target = n;
        for (; inserted < target; inserted++) tableSet(&t, keys[inserted], NIL_VAL);
        int m = 1000000;
        Value v;
        double acc = 0;
        double t0 = now();
        for (int i = 0; i < m; i++) acc += tableGet(&t, misses[i % n], &v);
        double t1 = now();
        sink = acc;
        printf("  load ~%.3f (cap=%d)  miss lookup %6.1f ns/op\n", (double)t.count / t.capacity,
               t.capacity, (t1 - t0) * 1e6 / m);
    }
    freeTable(&t);
    free(keys);
    free(misses);
}

int main(void) {
    initVM();
    keysets(1000000);
    churn(1000, 5000000);
    churn(100000, 5000000);
    sizeSweep();
    loadSweep();
    freeVM();
    return 0;
}
