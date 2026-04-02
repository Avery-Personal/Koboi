#ifndef KOBOI_OPCODE_H
#define KOBOI_OPCODE_H

    typedef enum {
        KVM_OP_NOP = 0,

        KVM_OP_LOAD_CONST,
        KVM_OP_ADD,

        KVM_OP_PRINT,
        
        KVM_OP_HALT
    } KVMOpcode;

#endif