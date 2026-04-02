#include <stdio.h>
#include <stdlib.h>

#include "KoboiVM.h"

KoboiVM *KVMCreate(KVMConfiguration Configuration) {
    KoboiVM *KVM = malloc(sizeof(KoboiVM));
    if (!KVM)
        return NULL;
    
    memset(KVM, 0, sizeof(KoboiVM));

    KVM -> State = KVM_STATE_UNINITIALIZED;
    KVM -> Mode = KOBOI_EXECUTION_MODE_RUNTIME;
    
    KVM -> Configuration = Configuration;

    return KVM;
}

int KVMInitialize(KoboiVM *KVM) {
    if (!KVM)
        return 0;

    if (KVM -> State != KVM_STATE_UNINITIALIZED)
        return 0;

    KVM -> Context = KVMContextCreate();
    if (!KVM -> Context)
        return 0;

    KVM -> RuntimeKVM = RuntimeKVMCreate(KVM -> Configuration.RegisterCount);
    if (!KVM -> RuntimeKVM)
        return 0;

    KVM -> CompileTimeKVM = CompileTimeKVMCreate();
    KVM -> State = KVM_STATE_READY;

    return 1;
}

void KVMDestroy(KoboiVM *KVM) {
    if (!KVM)
        return;

    if (KVM -> RuntimeKVM)
        RuntimeKVMDestroy(KVM -> RuntimeKVM);

    if (KVM -> CompileTimeKVM)
        CompileTimeVMDestroy(KVM -> CompileTimeKVM);

    if (KVM -> Context)
        KVMContextDestroy(KVM -> Context);

    free(KVM);
}
