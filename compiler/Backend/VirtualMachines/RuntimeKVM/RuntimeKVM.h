#ifndef RUNTIME_VM_H
#define RUNTIME_VM_H

    #include <stdint.h>
    #include <stdbool.h>

    typedef struct {
        uint32_t RegisterCount;
        int32_t *Registers;

        uint32_t InstructionPointer;

        const uint8_t *Bytecode;
        uint32_t BytecodeSize;

        int Running;
    } RuntimeVM;

    RuntimeVM *RuntimeKVMCreate(uint32_t RegisterCount);
    void RuntimeKVMDestroy(RuntimeVM *KVM);

    int RuntimeKVMLoad(RuntimeVM *KVM, const uint8_t *Bytecode, uint32_t Size);
    int RuntimeKVMRun(RuntimeVM *KVM);

#endif