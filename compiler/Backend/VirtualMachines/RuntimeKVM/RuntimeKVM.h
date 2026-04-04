#ifndef RUNTIME_KVM_H
#define RUNTIME_KVM_H

    #include <stdint.h>

    #include "../../Bytecode/BytecodeFormat.h"

    typedef struct {
        uint32_t RegisterCount;
        int32_t *Registers;

        uint32_t InstructionPointer;

        const uint8_t *Bytecode;
        uint32_t BytecodeSize;

        int Running;
    } RuntimeKVM;

    RuntimeKVM *RuntimeKVMCreate(uint32_t RegisterCount);
    void RuntimeKVMDestroy(RuntimeKVM *KVM);

    int RuntimeKVMLoad(RuntimeKVM *KVM, KVMBytecode *Bytecode);
    int RuntimeKVMRun(RuntimeKVM *KVM);

#endif