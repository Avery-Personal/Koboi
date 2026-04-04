#ifndef BYTECODE_FORMAT_H
#define BYTECODE_FORMAT_H

    #include <stdint.h>

    typedef uint8_t Byte;
    typedef const Byte CONST_BYTE;

    typedef struct {
        uint32_t Magic;
        uint32_t Version;
        
        uint32_t EntryPoint;
        uint32_t CodeSize;

        uint32_t ConstPoolOffset;
        uint32_t ConstPoolSize;

        uint32_t SymbolTableOffset;
        uint32_t SymbolTableSize;
    } KVMBytecodeHeader;

    typedef struct {
        CONST_BYTE *Data;
        uint32_t Size;
    } KVMBytecode;

    typedef struct {
        KVMBytecodeHeader Header;
        KVMBytecode Bytecode;
    } KVMBytecodeProgram;

#endif
