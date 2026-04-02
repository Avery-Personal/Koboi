#ifndef KVM_BYTECODE_READER_H
#define KVM_BYTECODE_READER_H

    #include <stdint.h>

    typedef struct {
        const uint8_t *Data;

        uint32_t Size;
        uint32_t InstructionPointer;
    } KVMReader;

    uint8_t KVMReaderReadU8(KVMReader *Reader);
    int32_t KVMReaderReadI32(KVMReader *Reader);

#endif
