#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "KVMContext.h"

KVMContext *KVMContextCreate(void) {    
    KVMContext *Context = malloc(sizeof(KVMContext));

    memset(Context, 0, sizeof(KVMContext));

    Context -> SafetyMode = KOBOI_POLICY_SAFE;

    return Context;
}

void KVMContextDestroy(KVMContext *Context) {
    free(Context -> CurrentWorld);
    free(Context);
}
