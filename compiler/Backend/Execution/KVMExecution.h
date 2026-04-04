#ifndef KVM_EXECUTION_H
#define KVM_EXECUTION_H

    #include "../Core/KoboiVM.h"

    typedef enum {
        KVM_EXECUTION_OK = 1,
        KVM_EXECUTION_ERROR = 0
    } KVMExecutionResult;

    int KVMRun(KoboiVM *KVM, KVMBytecode *Program);
    int KVMStep(KoboiVM *KVM);

    int KVMPause(KoboiVM *KVM);
    int KVMResume(KoboiVM *KVM);
    int KVMStop(KoboiVM *KVM);

#endif
