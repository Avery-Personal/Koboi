#ifndef BYTECODE_FORMAT_H
#define BYTECODE_FORMAT_H

    #include <stdint.h>

    typedef uint8_t Byte;

    typedef struct {
        uint32_t Magic;
        uint32_t Version;
        uint32_t CodeSize;
        uint32_t EntryPoint;
    } KVMBytecodeHeader;

    typedef struct {
        const Byte *Data;
        uint32_t Size;
    } KVMBytecode;

#endif
