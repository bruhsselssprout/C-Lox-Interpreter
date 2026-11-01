// Simple Local Variable Lookup Benchmark
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BUCKETS 64
#define LOOKUPS 20000000

typedef struct {
    int length;
    uint32_t hash;
    int next;
} Local;

static Local locals[256];
static int buckets[BUCKETS];
static char names[256][8];

static uint32_t hashName(const char* s, int len) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++) h = (h ^ (uint8_t)s[i]) * 16777619;
    return h;
}

static int resolveLinear(int count, const char* s, int len) {
    for (int i = count - 1; i >= 0; i--) {
        if (locals[i].length == len && memcmp(names[i], s, len) == 0) return i;
    }
    return -1;
}

static int resolveHashed(const char* s, int len) {
    uint32_t h = hashName(s, len);
    for (int i = buckets[h % BUCKETS]; i != -1; i = locals[i].next) {
        if (locals[i].hash == h && locals[i].length == len &&
            memcmp(names[i], s, len) == 0) return i;
    }
    return -1;
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Always looks up the oldest local, the worst case for linear scan.
static double timeIt(int count, int hashed) {
    long sum = 0;
    double start = now();
    for (int n = 0; n < LOOKUPS; n++) {
        sum += hashed ? resolveHashed(names[0], locals[0].length)
                      : resolveLinear(count, names[0], locals[0].length);
    }
    double elapsed = now() - start;
    return sum < 0 ? -1 : elapsed;
}

int main(void) {
    int sizes[] = {4, 16, 64, 128, 255};

    printf("\n  Local variable lookup: %d lookups, worst case\n\n", LOOKUPS);
    printf("  %-8s %10s %10s %10s\n", "Locals", "Linear", "Hashed", "Speedup");
    printf("  %-8s %10s %10s %10s\n", "------", "------", "------", "-------");

    for (int s = 0; s < 5; s++) {
        int count = sizes[s];
        for (int i = 0; i < BUCKETS; i++) buckets[i] = -1;
        for (int i = 0; i < count; i++) {
            locals[i].length = snprintf(names[i], sizeof names[i], "var%d", i);
            locals[i].hash = hashName(names[i], locals[i].length);
            uint32_t b = locals[i].hash % BUCKETS;
            locals[i].next = buckets[b];
            buckets[b] = i;
        }

        double linear = timeIt(count, 0);
        double hashed = timeIt(count, 1);
        printf("  %-8d %9.3fs %9.3fs %9.1fx\n", count, linear, hashed, linear / hashed);
    }
    printf("\n");
    return 0;
}
