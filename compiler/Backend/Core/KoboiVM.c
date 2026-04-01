#include <stdio.h>

#include <stdlib.h>

#include "KoboiVM.h"

KoboiVM *KVMCreate(KVMConfiguration Configuration) {
    KoboiVM *KVM = malloc(sizeof(KoboiVM));

    KVM -> State = KVM_STATE_UNINITIALIZED;
    KVM -> Configuration = Configuration;

    return KVM;
}
