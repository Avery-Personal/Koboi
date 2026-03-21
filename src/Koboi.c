#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Lexer/Lexer.h"
#include "Parser/Parser.h"

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
    ASTProgram *__PROGRAM__ = ParseProgram(&__PARSER__);

    if (__PARSER__.HasError) {
        return 1;
    }

    free(Source);

    return 0;
}
