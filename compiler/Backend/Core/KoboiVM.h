#ifndef KOBOI_VIRTUAL_MACHINE_H
#define KOBOI_VIRTUAL_MACHINE_H

    #include <stdint.h>

    #include "KVMContext.h"

    typedef struct RuntimeKVM RuntimeKVM;
    typedef struct CompileTimeKVM CompileTimeKVM;

    typedef enum {
        KVM_STATE_UNINITIALIZED,
        KVM_STATE_READY,
        KVM_STATE_RUNNING,
        KVM_STATE_PAUSED,
        KVM_STATE_HALTED,

        KVM_STATE_ERROR
    } KoboiVirtualMachineState;

    typedef enum {
        KOBOI_EXECUTION_MODE_RUNTIME,
        KOBOI_EXECUTION_MODE_COMPILETIME
    } KoboiExecutionMode;

    typedef struct {
        int DEBUG;

        uint32_t RegisterCount;
    } KVMConfiguration;

    typedef struct {
        KoboiVirtualMachineState State;

        KoboiExecutionMode Mode;
        KVMConfiguration Configuration;

        KVMContext *Context;

        RuntimeKVM *RuntimeKVM;
        CompileTimeKVM *CompileTimeKVM;
    } KoboiVM;

    KoboiVM *KVMCreate(KVMConfiguration Configuration);
    void KVMDestroy(KoboiVM *KVM);

    int KVMInitialize(KoboiVM *KVM);
    void KVMReset(KoboiVM *KVM);

#endif
