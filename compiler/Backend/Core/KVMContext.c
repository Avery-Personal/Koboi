#include <stdio.h>
#include <stdlib.h>

#include "KVMContext.h"

KVMContext *KVMContextCreate(void) {
    KVMContext *Context = malloc(sizeof(Context));

    Context -> SafetyMode = KOBOI_POLICY_SAFE;
}
