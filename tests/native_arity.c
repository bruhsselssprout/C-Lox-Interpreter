// Challenge 24.2
// Challenge 24.2
#include <assert.h>

#include "object.h"
#include "vm.h"

static int nativeCalls = 0;

static bool addNative(int argCount, Value* args, Value* result, const char** error) {
    assert(argCount == 2);
    nativeCalls++;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        *error = "addNative expects numbers (100% required).";
        return false;
    }
    *result = NUMBER_VAL(AS_NUMBER(args[0]) + AS_NUMBER(args[1]));
    return true;
}

static bool failNative(int argCount, Value* args, Value* result, const char** error) {
    (void)argCount;
    (void)args;
    (void)result;
    (void)error;
    return false;
}

int main(void) {
    initVM();
    push(OBJ_VAL(copyString("addNative", 9)));
    push(OBJ_VAL(newNative(addNative, 2)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
    push(OBJ_VAL(copyString("failNative", 10)));
    push(OBJ_VAL(newNative(failNative, 0)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();

    assert(interpret("var result = addNative(2, 3);") == INTERPRET_OK);
    Value result;
    assert(tableGet(&vm.globals, copyString("result", 6), &result));
    assert(IS_NUMBER(result) && AS_NUMBER(result) == 5);
    assert(nativeCalls == 1);

    assert(interpret("addNative();") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("addNative(1);") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("addNative(1, 2, 3);") == INTERPRET_RUNTIME_ERROR);
    assert(nativeCalls == 1);

    assert(interpret("clock();") == INTERPRET_OK);
    assert(interpret("clock(1);") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("fun wrapper() { addNative(1, \"wrong\"); } wrapper();") == INTERPRET_RUNTIME_ERROR);
    assert(nativeCalls == 2);
    assert(interpret("failNative();") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("addNative(4, 5);") == INTERPRET_OK);
    assert(nativeCalls == 3);

    assert(interpret("var root = sqrt(81); var sine = sin(0); var size = length(\"hello\");") == INTERPRET_OK);
    assert(tableGet(&vm.globals, copyString("root", 4), &result));
    assert(IS_NUMBER(result) && AS_NUMBER(result) == 9);
    assert(tableGet(&vm.globals, copyString("sine", 4), &result));
    assert(IS_NUMBER(result) && AS_NUMBER(result) == 0);
    assert(tableGet(&vm.globals, copyString("size", 4), &result));
    assert(IS_NUMBER(result) && AS_NUMBER(result) == 5);
    assert(interpret("sqrt(-1);") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("sqrt(\"bad\");") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("sin(\"bad\");") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("length(42);") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("length();") == INTERPRET_RUNTIME_ERROR);
    assert(interpret("var emptySize = length(\"\"); var byteSize = length(\"\xc3\xa9\");") == INTERPRET_OK);
    assert(tableGet(&vm.globals, copyString("emptySize", 9), &result));
    assert(IS_NUMBER(result) && AS_NUMBER(result) == 0);
    assert(tableGet(&vm.globals, copyString("byteSize", 8), &result));
    assert(IS_NUMBER(result) && AS_NUMBER(result) == 2);
    assert(interpret("sin(1 / 0);") == INTERPRET_RUNTIME_ERROR);

    freeVM();
    return 0;
}