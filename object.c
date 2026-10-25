#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;
    return object;
}

// Challenge 19.1 - Implement flexible array members
ObjString* makeString(int length) {
    ObjString* string =
        (ObjString*)allocateObject(sizeof(ObjString) + length + 1, OBJ_STRING);
    string->length = length;
    string->ownsChars = true;
    string->chars = string->data;
    string->data[length] = '\0';
    return string;
}

// Challenge 19.2 - Optimize ObjString
ObjString* makeConstantString(const char* chars, int length) {
    ObjString* string =
        (ObjString*)allocateObject(sizeof(ObjString), OBJ_STRING);
    string->length = length;
    string->ownsChars = false;
    string->chars = chars;
    return string;
}

// Challenge 19.1 - Implement flexible array members
ObjString* copyString(const char* chars, int length) {
    ObjString* string = makeString(length);
    memcpy(string->data, chars, length);
    return string;
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%.*s", AS_STRING(value)->length, AS_CSTRING(value));
            break;
    }
}