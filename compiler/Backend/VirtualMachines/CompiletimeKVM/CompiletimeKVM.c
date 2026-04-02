#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CompiletimeKVM.h"

CompileTimeKVM *CompileTimeKVMCreate() {
    CompileTimeKVM *KVM = malloc(sizeof(CompileTimeKVM));
    if (!KVM)
        return NULL;

    memset(KVM, 0, sizeof(CompileTimeKVM));

    return KVM;
}

void CompileTimeKVMDestroy(CompileTimeKVM *KVM) {
    free(KVM -> Environment);
    free(KVM);
}
