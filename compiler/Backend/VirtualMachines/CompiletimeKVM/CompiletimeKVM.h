#ifndef COMPILETIME_KVM_H
#define COMPILETIME_KVM_H

#include <stdint.h>

    struct KVMContext;

    typedef struct {
        uint32_t RegisterCount;
        int32_t *Registers;

        uint32_t InstructionPointer;

        const uint8_t *Bytecode;
        uint32_t BytecodeSize;

        int Running;

        void *Environment;
        void *MacroContext;

        struct KVMContext *Context;
    } CompileTimeKVM;
    
    CompileTimeKVM *CompileTimeKVMCreate();
    void CompileTimeKVMDestroy(CompileTimeKVM *KVM);

#endif
