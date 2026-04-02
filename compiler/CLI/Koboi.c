#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "KoboiC.h"

char *ReadFile(const char *Path) {
    FILE *File = fopen(Path, "rb");
    if (!File)
        return NULL;

    fseek(File, 0, SEEK_END);

    long _Size = ftell(File);

    rewind(File);

    char *Buffer = malloc(_Size + 1);

    fread(Buffer, 1, _Size, File);

    Buffer[_Size] = '\0';

    fclose(File);

    return Buffer;
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("Usage: %s <FILE>", argv[0]);

        return 1;
    }

    char *Source = ReadFile(argv[1]);
    if (!Source) {
        printf("Failed to read file: %s\n", argv[1]);
        
        return 1;
    }

    Lexer __LEXER__ = LexerCreate(Source);

    __LEXER__.CurrentFile = argv[1];

    TokenStream __TOKENS__ = Tokenize(&__LEXER__);
    Parser __PARSER__ = CreateParser(&__TOKENS__);
    ASTProgram *__AST__ = ParseProgram(&__PARSER__);
    SSProgram *__SYNTAX_TAPES__ = LowerSS(__AST__);
    SSSSContext *__SYNTAX_TAPES_SEMANTICS_SYSTEM__ = CreateContext();

    //int __SSSS_ERROR__ = AnalyzeProgram(__SYNTAX_TAPES_SEMANTICS_SYSTEM__, __SYNTAX_TAPES__);

    if (__PARSER__.HasError) {
        return 1;
    }

    KVMConfiguration __KOBOI_VIRTUAL_MACHINE_CONFIGURATION__ = {0};

    __KOBOI_VIRTUAL_MACHINE_CONFIGURATION__.DEBUG = 0;
    __KOBOI_VIRTUAL_MACHINE_CONFIGURATION__.RegisterCount = 0;

    KoboiVM *__KOBOI_VIRTUAL_MACHINE__ = KVMCreate(__KOBOI_VIRTUAL_MACHINE_CONFIGURATION__);
    KVMBytecode *__KOBOI_VIRTUAL_MACHINE_BYTECODE__;

    KVMInitialize(__KOBOI_VIRTUAL_MACHINE__);
    KVMRun(__KOBOI_VIRTUAL_MACHINE__, __KOBOI_VIRTUAL_MACHINE_BYTECODE__);

    free(Source);

    return 0;
}
