// Challenge 24.4
#include <math.h>

#include "native.h"
#include "object.h"

bool sqrtNative(int argCount, Value* args, Value* result, const char** error) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) {
        *error = "sqrt() expects a number.";
        return false;
    }
    if (AS_NUMBER(args[0]) < 0) {
        *error = "sqrt() expects a non-negative number.";
        return false;
    }
    *result = NUMBER_VAL(sqrt(AS_NUMBER(args[0])));
    return true;
}

bool sinNative(int argCount, Value* args, Value* result, const char** error) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !isfinite(AS_NUMBER(args[0]))) {
        *error = "sin() expects a finite number.";
        return false;
    }
    *result = NUMBER_VAL(sin(AS_NUMBER(args[0])));
    return true;
}

bool lengthNative(int argCount, Value* args, Value* result, const char** error) {
    (void)argCount;
    if (!IS_STRING(args[0])) {
        *error = "length() expects a string.";
        return false;
    }
    *result = NUMBER_VAL(AS_STRING(args[0])->length);
    return true;
}