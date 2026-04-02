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

    KVM -> Context = koboi_vmcontext_create();
    if (!KVM -> Context)
        return 0;

    KVM -> RuntimeVM = RuntimeKVM(KVM -> Configuration.RegisterCount);
    if (!KVM -> RuntimeVM)
        return 0;

    KVM -> CompileTimeVM = CompileTimeKVMCreate();
    KVM -> State = KVM_STATE_READY;

    return 1;
}

void KVMDestroy(KoboiVM *KVM) {
    if (!KVM)
        return;

    if (KVM -> RuntimeVM)
        RuntimeKVMDestroy(KVM -> RuntimeVM);

    if (KVM -> CompileTimeVM)
        CompileTimeVMDestroy(KVM -> CompileTimeVM);

    if (KVM -> Context)
        KVMContextDestroy(KVM -> Context);

    free(KVM);
}
