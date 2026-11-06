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

    freeVM();
    return 0;
}