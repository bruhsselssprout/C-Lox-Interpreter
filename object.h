#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

// Challenge 19.2 - Optimize ObjString
struct ObjString {
    Obj obj;
    int length;
    bool ownsChars;
    const char* chars;
    char data[];  // Inline storage, used only when ownsChars is true.
};

ObjString* makeString(int length);
ObjString* copyString(const char* chars, int length);
ObjString* makeConstantString(const char* chars, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif