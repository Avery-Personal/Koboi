#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RuntimeKVM.h"
#include "../../Bytecode/Opcode.h"
#include "../../Bytecode/Reader.h"

RuntimeKVM *RuntimeKVMCreate(uint32_t RegisterCount) {
    RuntimeKVM *KVM = malloc(sizeof(RuntimeKVM));
    if (!KVM)
        return NULL;

    memset(KVM, 0, sizeof(RuntimeKVM));

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

int RuntimeKVMLoad(RuntimeKVM *KVM, KVMBytecode *Bytecode) {
    if (!KVM || !Bytecode)
        return 0;

    KVM -> Bytecode = Bytecode -> Data;
    KVM -> BytecodeSize = Bytecode -> Size;
    KVM -> InstructionPointer = 0;

    return 1;
}

int RuntimeKVMRun(RuntimeKVM *KVM) {
    if (!KVM || !KVM -> Bytecode)
        return 0;

    KVM -> Running = 1;

    KVMReader Reader;

    Reader.Data = KVM -> Bytecode;
    Reader.Size = KVM -> BytecodeSize;
    Reader.InstructionPointer = KVM -> InstructionPointer;

    while (KVM -> Running && KVM -> InstructionPointer < KVM -> BytecodeSize) {
        uint8_t Op = KVMReaderReadU8(&Reader);

        switch (Op) {
            case KVM_OP_NOP:
                break;

            case KVM_OP_LOAD_CONST: {
                uint8_t Register = KVMReaderReadU8(&Reader);
                int32_t Value = KVMReaderReadI32(&Reader);

                if (Register < KVM -> RegisterCount)
                    KVM -> Registers[Register] = Value;

                break;
            }

            case KVM_OP_ADD: {
                uint8_t Destination = KVMReaderReadU8(&Reader);
                uint8_t Source1 = KVMReaderReadU8(&Reader);
                uint8_t Source2 = KVMReaderReadU8(&Reader);

                if (Destination < KVM -> RegisterCount && Source1 < KVM -> RegisterCount && Source2 < KVM -> RegisterCount) {
                    KVM -> Registers[Destination] = KVM -> Registers[Source1] + KVM -> Registers[Source2];
                }

                break;
            }

            case KVM_OP_PRINT: {
                uint8_t Register = KVMReaderReadU8(&Reader);

                if (Register < KVM -> RegisterCount)
                    printf("%d\n", KVM -> Registers[Register]);

                break;
            }

            case KVM_OP_HALT:
                KVM -> Running = 0;

                break;

            default:
                printf("Unknown opcode: %d\n", Op);

                return 0;
        }
    }

    KVM -> InstructionPointer = Reader.InstructionPointer;

    return 1;
}

int RuntimeKVMStep(RuntimeKVM *KVM) {
    
}
