#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_RETURN,
} OpCode;

// Challenge 14.1 - Implement run-length encoding of line information
// Structure to store the start of a new line for run-length encoding
typedef struct {
    int offset;
    int line;
} LineStart;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
    
    // Challenge 14.1 - Implement run-length encoding of line information
    int lineCount; 
    int lineCapacity;
    LineStart* lines; // Array of line start information for run-length encoding
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

// Challenge 14.1 - Implement run-length encoding of line information
// Helper function to get the line number for a given instruction using run-length encoding
int getLine(Chunk* chunk, int instruction);

// Challenge 14.2 - Implement writeConstant
void writeConstant(Chunk* chunk, Value value, int line);

#endif