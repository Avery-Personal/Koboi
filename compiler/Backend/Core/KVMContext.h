#ifndef VIRTUAL_MACHINE_CONTEXT_H
#define VIRTUAL_MACHINE_CONTEXT_H

    typedef enum {
        KVM_STATE_UNINITIALIZED,
        KVM_STATE_READY,
        KVM_STATE_RUNNING,
        KVM_STATE_PAUSED,
        KVM_STATE_HALTED,

        KVM_STATE_ERROR
    } KoboiVirtualMachineState;
    
    typedef enum {
        KOBOI_POLICY_SAFE,
        KOBOI_POLICY_CHECKINGS,
        KOBOI_POLICY_UNSAFE,
        KOBOI_POLICY_TRUSTED_UNSAFE
    } KoboiSafetyMode;

    typedef struct {
        KoboiSafetyMode SafetyMode;

        void *CurrentWorld;
        void *ModuleRegistry;
    } KVMContext;

#endif
