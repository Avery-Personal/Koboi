#ifndef LEXER_H
#define LEXER_H

#include "Tokens.h"

int IsDigit(int Character);
int IsAlphanumericNumber(int Character);
int IsAlphanumeric(int Character);

const char *TokenTypeToString(TokenType Type);

int LexerIsAtEnd(Lexer *_Lexer);
void LexerSkipWC(Lexer *_Lexer);

TokenType ResolveIdentifier(const char* Start, size_t Len);

Lexer LexerCreate(const char *Source);

char LexerPeek(Lexer *_Lexer);
char LexerNext(Lexer *_Lexer);

static void LexerErrorAt(Lexer *_Lexer, const char *Message);
static void LexingError(Lexer *_Lexer);

Token LexerNextToken(Lexer *_Lexer);
TokenStream Tokenize(Lexer *_Lexer);

#endif
