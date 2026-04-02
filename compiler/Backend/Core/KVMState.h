#ifndef KVM_STATE_H
#define KVM_STATE_H

    typedef enum {
        KVM_STATE_UNINITIALIZED,
        KVM_STATE_READY,
        KVM_STATE_RUNNING,
        KVM_STATE_PAUSED,
        KVM_STATE_HALTED,

        KVM_STATE_ERROR
    } KoboiVirtualMachineState;

#endif
