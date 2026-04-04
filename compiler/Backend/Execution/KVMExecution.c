#include <stdio.h>

#include "KVMExecution.h"

int KVMRun(KoboiVM *KVM, KVMBytecode *Program) {
    if (!KVM || !Program)
        return KVM_EXECUTION_ERROR;

    if (KVM -> State != KVM_STATE_READY && KVM -> State != KVM_STATE_PAUSED)
        return KVM_EXECUTION_ERROR;

    KVM -> State = KVM_STATE_RUNNING;

    int Result = 0;

    if (KVM -> Mode == KOBOI_EXECUTION_MODE_RUNTIME) {
        if (!RuntimeKVMLoad(KVM -> RuntimeKVM, Program))
            goto Error;

        Result = RuntimeKVMRun(KVM -> RuntimeKVM);
    } else if (KVM -> Mode == KOBOI_EXECUTION_MODE_COMPILETIME) {
        Result = CompileTimeExecute(KVM -> CompileTimeKVM, Program -> Data, Program -> Size);
    } else {
        goto Error;
    }

    if (Result) {
        KVM -> State = KVM_STATE_HALTED;

        return KVM_EXECUTION_OK;
    }

    Error:
        KVM -> State = KVM_STATE_ERROR;

    return KVM_EXECUTION_ERROR;
}

int KVMStep(KoboiVM *KVM) {
    if (!KVM)
        return KVM_EXECUTION_ERROR;

    if (KVM -> State != KVM_STATE_RUNNING && KVM -> State != KVM_STATE_PAUSED)
        return KVM_EXECUTION_ERROR;

    KVM -> State = KVM_STATE_RUNNING;

    int Result = 0;

    if (KVM -> Mode == KOBOI_EXECUTION_MODE_RUNTIME) {
        Result = RuntimeKVMStep(KVM -> RuntimeKVM);
    } else {
        return KVM_EXECUTION_ERROR;
    }

    if (!Result) {
        KVM -> State = KVM_STATE_ERROR;

        return KVM_EXECUTION_ERROR;
    }

    return KVM_EXECUTION_OK;
}

int KVMPause(KoboiVM *KVM) {
    if (!KVM || KVM -> State != KVM_STATE_RUNNING)
        return KVM_EXECUTION_ERROR;

    KVM -> State = KVM_STATE_PAUSED;
    
    return KVM_EXECUTION_OK;
}

int KVMResume(KoboiVM *KVM) {
    if (!KVM || KVM -> State != KVM_STATE_PAUSED)
        return KVM_EXECUTION_ERROR;

    KVM -> State = KVM_STATE_RUNNING;

    return KVM_EXECUTION_OK;
}

int KVMStop(KoboiVM *KVM) {
    if (!KVM)
        return KVM_EXECUTION_ERROR;

    KVM -> State = KVM_STATE_HALTED;

    if (KVM -> RuntimeKVM)
        KVM -> RuntimeKVM -> Running = 0;

    return KVM_EXECUTION_OK;
}
