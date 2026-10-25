#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

// Challenge 20.1
void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static uint32_t hashValue(Value value) {
    switch (value.type) {
        case VAL_BOOL: return AS_BOOL(value) ? 3 : 5;
        case VAL_NIL: return 7;
        case VAL_NUMBER: {
            double number = AS_NUMBER(value);
            if (number == 0) number = 0; // -0.0 == 0.0, so they must hash alike.
            uint64_t bits;
            memcpy(&bits, &number, sizeof(bits));
            bits ^= bits >> 33;
            bits *= 0xff51afd7ed558ccdULL;
            bits ^= bits >> 33;
            return (uint32_t)bits;
        }
        case VAL_OBJ:
            // Only strings have a content hash; other objects hash by identity.
            if (IS_STRING(value)) return AS_STRING(value)->hash;
            return (uint32_t)(((uintptr_t)AS_OBJ(value)) >> 4);
        default: return 0;
    }
}

// Challenge 20.1
static Entry* findEntry(Entry* entries, int capacity, Value key) {
    uint32_t index = hashValue(key) % capacity;
    Entry* tombstone = NULL;

    for (;;) {
        Entry* entry = &entries[index];

        if (IS_EMPTY(entry->key)) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (valuesEqual(entry->key, key)) {
            // We found the key.
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

// Challenge 20.1
bool tableGet(Table* table, Value key, Value* value) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (IS_EMPTY(entry->key)) return false;

    *value = entry->value;
    return true;
}

// Challenge 20.1
static void adjustCapacity(Table* table, int capacity) {
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = EMPTY_VAL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (IS_EMPTY(entry->key)) continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

// Challenge 20.1
bool tableSet(Table* table, Value key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    
    Entry* entry = findEntry(table->entries, table->capacity, key);

    bool isNewKey = IS_EMPTY(entry->key);
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

// Challenge 20.1
bool tableDelete(Table* table, Value key) {
    if (table->count == 0) return false;

    // Find the entry.
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (IS_EMPTY(entry->key)) return false;

    // Place a tombstone in the entry.
    entry->key = EMPTY_VAL;
    entry->value = BOOL_VAL(true);

    return true;
}

void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (!IS_EMPTY(entry->key)) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

// Challenge 20.1
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;

    for (;;) {
        Entry* entry = &table->entries[index];

        if (IS_EMPTY(entry->key)) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->value)) return NULL;
        } else if (IS_STRING(entry->key)) {
            ObjString* string = AS_STRING(entry->key);
            if (string->length == length && string->hash == hash && memcmp(string->chars, chars, length) == 0) {
                // We found it.
                return string;
            }
        }

        index = (index + 1) % table->capacity;
    }
}