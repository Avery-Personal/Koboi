#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CompiletimeKVM.h"

CompileTimeKVM *CompileTimeKVMCreate(KVMContext *Context) {
    CompileTimeKVM *KVM = malloc(sizeof(CompileTimeKVM));
    if (!KVM)
        return NULL;

    memset(KVM, 0, sizeof(CompileTimeKVM));

    KVM -> Context = Context;

    return KVM;
}

void CompileTimeKVMDestroy(CompileTimeKVM *KVM) {
    free(KVM -> Environment);
    free(KVM);
}

int CompileTimeExecute(CompileTimeKVM *KVM, const uint8_t *Bytecode, uint32_t Size) {

}
