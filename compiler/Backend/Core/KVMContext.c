#include <stdio.h>
#include <stdlib.h>

#include "KVMContext.h"

KVMContext *KVMContextCreate(void) {    
    KVMContext *Context = malloc(sizeof(KVMContext));

    Context -> SafetyMode = KOBOI_POLICY_SAFE;
}

void KVMContextDestroy(KVMContext *Context) {
    free(Context -> CurrentWorld);
    free(Context);
}
