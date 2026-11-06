#ifndef clox_native_h
#define clox_native_h

#include "value.h"

bool sqrtNative(int argCount, Value* args, Value* result, const char** error);
bool sinNative(int argCount, Value* args, Value* result, const char** error);
bool lengthNative(int argCount, Value* args, Value* result, const char** error);

#endif