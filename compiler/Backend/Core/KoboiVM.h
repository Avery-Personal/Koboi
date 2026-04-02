#ifndef KOBOI_VIRTUAL_MACHINE_H
#define KOBOI_VIRTUAL_MACHINE_H

    #include <stdint.h>

    #include "KVMContext.h"
    #include "KVMState.h"
    #include "../VirtualMachines/RuntimeKVM/RuntimeKVM.h"
    #include "../VirtualMachines/CompiletimeKVM/CompiletimeKVM.h"

    //typedef struct RuntimeKVM RuntimeKVM;
    //typedef struct CompileTimeKVM CompileTimeKVM;

    typedef enum {
        KOBOI_EXECUTION_MODE_RUNTIME,
        KOBOI_EXECUTION_MODE_COMPILETIME
    } KoboiExecutionMode;

    typedef struct {
        int DEBUG;

        uint32_t RegisterCount;
    } KVMConfiguration;

    typedef struct KoboiVM {
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

    int KVMRun(KoboiVM *KVM, void *BYTECODE_PLACEHOLDER);

#endif
