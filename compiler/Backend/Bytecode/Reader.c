#include <string.h>

#include "Reader.h"

uint8_t KVMReaderReadU8(KVMReader *Reader) {
    return Reader -> Data[Reader -> InstructionPointer++];
}

int32_t KVMReaderReadI32(KVMReader *Reader) {
    int32_t Value;

    memcpy(&Value, &Reader -> Data[Reader -> InstructionPointer], sizeof(int32_t));

    Reader -> InstructionPointer += sizeof(int32_t);

    return Value;
}