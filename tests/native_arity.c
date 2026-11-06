// Challenge 24.2
#include <assert.h>

#include "object.h"
#include "vm.h"

static int nativeCalls = 0;

static Value addNative(int argCount, Value* args) {
    assert(argCount == 2);
    nativeCalls++;
    return NUMBER_VAL(AS_NUMBER(args[0]) + AS_NUMBER(args[1]));
}

int main(void) {
    initVM();
    push(OBJ_VAL(copyString("addNative", 9)));
    push(OBJ_VAL(newNative(addNative, 2)));
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
    assert(interpret("addNative(4, 5);") == INTERPRET_OK);
    assert(nativeCalls == 2);

    freeVM();
    return 0;
}