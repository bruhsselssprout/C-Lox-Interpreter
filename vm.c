#include <stdarg.h>

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);

    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;

    // Challenge 21.2
    initTable(&vm.globalNames);
    initValueArray(&vm.globalValues);
    initTable(&vm.strings);
}

void freeVM() {
    // Challenge 21.2
    freeTable(&vm.globalNames);
    freeValueArray(&vm.globalValues);
    freeTable(&vm.strings);
    freeObjects();
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    chars[length] = '\0';
    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

// Challenge 21.2
// Only used on the error path, so a linear reverse lookup is fine.
static ObjString* globalName(Value* slot) {
    int index = (int)(slot - vm.globalValues.values);
    for (int i = 0; i < vm.globalNames.capacity; i++) {
        Entry* entry = &vm.globalNames.entries[i];
        if (entry->key != NULL && (int)AS_NUMBER(entry->value) == index) {
            return entry->key;
        }
    }
    return NULL;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// Challenge 21.2
#define READ_GLOBAL_SLOT() (&vm.globalValues.values[READ_BYTE()])
#define GLOBAL_NAME(slot) (globalName(slot))

#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, 
                                (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;

            // Challenge 21.2
            case OP_GET_GLOBAL: {
                Value* slot = READ_GLOBAL_SLOT();
                if (IS_UNDEFINED(*slot)) {
                    runtimeError("Undefined variable '%s'.", GLOBAL_NAME(slot)->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(*slot);
                break;
            }

            // Challenge 21.2
            case OP_DEFINE_GLOBAL: {
                *READ_GLOBAL_SLOT() = peek(0);
                pop();
                break;
            }

            case OP_SET_GLOBAL: {
                Value* slot = READ_GLOBAL_SLOT();
                if (IS_UNDEFINED(*slot)) {
                    runtimeError("Undefined variable '%s'.", GLOBAL_NAME(slot)->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                *slot = peek(0);
                break;
            }

            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }

            case OP_GREATER:    BINARY_OP(NUMBER_VAL, >); break;
            case OP_LESS:       BINARY_OP(NUMBER_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;

            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }

            case OP_RETURN: {
                // Exit interpreter.
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}