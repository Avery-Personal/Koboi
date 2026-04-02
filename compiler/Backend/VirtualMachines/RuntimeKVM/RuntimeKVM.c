#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RuntimeKVM.h"

RuntimeKVM *RuntimeKVMCreate(uint32_t RegisterCount) {
    RuntimeKVM *KVM = malloc(sizeof(RuntimeKVM));
    if (!KVM)
        return NULL;

    memset(KVM, 0, sizeof(KVM));

    KVM -> RegisterCount = RegisterCount;
    KVM -> Registers = calloc(RegisterCount, sizeof(int32_t));

    KVM -> Running = 1;

    return KVM;
}

void RuntimeKVMDestroy(RuntimeKVM *KVM) {
    if (!KVM)
        return;

    free(KVM -> Registers);
    free(KVM);
}

int RuntimeKVMLoad(RuntimeKVM *KVM, const uint8_t *Bytecode, uint32_t Size) {
    if (!KVM || !Bytecode)
        return 0;

    KVM -> Bytecode = Bytecode;
    KVM -> BytecodeSize = Size;
    KVM -> InstructionPointer = 0;

    return 1;
}
