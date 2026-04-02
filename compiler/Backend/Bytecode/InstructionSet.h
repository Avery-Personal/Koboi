#ifndef KVM_INSTRUCTION_SET_H
#define KVM_INSTRUCTION_SET_H

    #include <stdint.h>

    #include "Opcode.h"

    typedef struct {
        KVMOpcode Opcode;
        uint8_t OperandCount;
    } KVMInstructionInfo;

#endif
