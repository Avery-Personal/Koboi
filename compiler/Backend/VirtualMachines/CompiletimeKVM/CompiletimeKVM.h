#ifndef COMPILETIME_KVM_H
#define COMPILETIME_KVM_H

    #include <stdint.h>

    typedef struct KVMContext KVMContext;

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
    
    CompileTimeKVM *CompileTimeKVMCreate(KVMContext *Context);
    void CompileTimeKVMDestroy(CompileTimeKVM *KVM);

    int CompileTimeExecute(CompileTimeKVM *KVM, const uint8_t *Bytecode, uint32_t Size);

#endif
