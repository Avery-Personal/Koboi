#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Tokens.h"
#include "Lexer.h"

int IsDigit(int Character) {
    if (Character >= '0' && Character <= '9')
        return 1;
    else
        return 0;
}

int IsAlphanumericNumber(int Character) {
    if ((Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || (Character >= '0' && Character <= '9'))
        return 1;
    else
        return 0;
}

int IsAlphanumeric(int Character) {
    if ((Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z'))
        return 1;
    else
        return 0;
}

const char *TokenTypeToString(TokenType Type) {
    switch (Type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";

        default: return "UNKNOWN";
    }
}

int LexerIsAtEnd(Lexer *_Lexer) {
    return _Lexer -> Cursor >= _Lexer -> Length;
}

void LexerSkipWC(Lexer *_Lexer) {
    while (!LexerIsAtEnd(_Lexer)) {
        char Character = LexerPeek(_Lexer);

        if (Character == ' ' || Character == '\t' || Character == '\r' || Character == '\n') {
            LexerNext(_Lexer);
        } else if (Character == '-' && _Lexer -> Cursor + 1 < _Lexer -> Length && _Lexer -> Source[_Lexer -> Cursor + 1] == '-') {
            LexerNext(_Lexer);
            LexerNext(_Lexer);

            while (!LexerIsAtEnd(_Lexer) && LexerPeek(_Lexer) != '\n')
                LexerNext(_Lexer);
        } else break;
    }
}

TokenType ResolveIdentifier(const char* Start, size_t Len) {
    if (Len == 8 && memcmp(Start, "function", 8) == 0) return TOKEN_FUNCTION;
    
    return TOKEN_IDENTIFIER;
}

Lexer LexerCreate(const char *Source) {
    Lexer _Lexer = {0};

    _Lexer.Source = Source;
    _Lexer.Length = strlen(Source);
    _Lexer.Cursor = 0;
    _Lexer.Line = 1;
    _Lexer.Column = 1;

    return _Lexer;
}

char LexerPeek(Lexer *_Lexer) {
    if (_Lexer -> Cursor >= _Lexer -> Length)
        return '\0';

    return _Lexer -> Source[_Lexer -> Cursor];
}

char LexerNext(Lexer *_Lexer) {
    char Character = LexerPeek(_Lexer);

    if (Character == '\n') {
        _Lexer -> Line++;
        _Lexer -> Column = 1;
    } else {
        _Lexer -> Column++;
    }

    if (_Lexer -> Cursor < _Lexer -> Length)
        _Lexer -> Cursor++;

    return Character;
}

static void LexerErrorAt(Lexer *_Lexer, const char *Message) {
    if (_Lexer -> HasError) return;

    _Lexer -> Error.Line = _Lexer -> Line;
    _Lexer -> Error.Column = _Lexer -> Column;

    _Lexer -> Error.Message = Message;
    _Lexer -> HasError = 1;
}

static void LexingError(Lexer *_Lexer) {
    fprintf(stderr, "[Lexer Error] Line %u:%u >> %s\n", _Lexer -> Error.Line, _Lexer -> Error.Column, _Lexer -> Error.Message);
}

Token LexerNextToken(Lexer *_Lexer) {
    LexerSkipWC(_Lexer);

    Token _Token = {0};

    _Token.Line = _Lexer -> Line;
    _Token.Column = _Lexer -> Column;

    char Character = LexerNext(_Lexer);

    if (_Lexer -> HasError) {
        Token NewToken = {0};
        NewToken.Type = TOKEN_EOF;

        return NewToken;
    }

    if (Character == '\0') {
        _Token.Type = TOKEN_EOF;
    } else if (IsAlphanumeric(Character) || Character == '_') {
        size_t Start = _Lexer -> Cursor - 1;

        while (IsAlphanumericNumber(LexerPeek(_Lexer)) || LexerPeek(_Lexer) == '_')
            LexerNext(_Lexer);

        _Token.Start = _Lexer -> Source + Start;
        _Token.Length = _Lexer -> Cursor - Start;

        _Token.Type = ResolveIdentifier(_Token.Start, _Token.Length);
    } else if (IsDigit(Character) || (Character == '.' && IsDigit(LexerPeek(_Lexer)))) {
        size_t Start = _Lexer -> Cursor - 1;

        int IsFloat = 0;

        while (IsDigit(LexerPeek(_Lexer)))
            LexerNext(_Lexer);

        if (LexerPeek(_Lexer) == '.') {
            IsFloat = 1;

            LexerNext(_Lexer);

            while (IsDigit(LexerPeek(_Lexer)))
                LexerNext(_Lexer);
        }

        _Token.Start  = _Lexer -> Source + Start;
        _Token.Length = _Lexer -> Cursor - Start;

        if (IsFloat) {
            _Token.Type = TOKEN_FLOAT;
            _Token.Literal.Float = strtod(_Token.Start, NULL);
        } else {
            _Token.Type = TOKEN_INT;
            _Token.Literal.Int = (uint64_t) strtoull(_Token.Start, NULL, 10);
        }
    } else if (Character == '"') {
        size_t Start = _Lexer -> Cursor;

        while (!LexerIsAtEnd(_Lexer)) {
            char NextCharacter = LexerNext(_Lexer);

            if (NextCharacter == '"')
                break;

            if (NextCharacter == '\n') {
                LexerErrorAt(_Lexer, "newline in string literal");
                LexingError(_Lexer);

                break;
            }

            if (NextCharacter == '\\')
                LexerNext(_Lexer);
        }

        if (LexerIsAtEnd(_Lexer)) {
            LexerErrorAt(_Lexer, "unterminated string literal");
            LexingError(_Lexer);
        }

        _Token.Start = _Lexer -> Source + Start;
        _Token.Length = _Lexer -> Cursor - Start - 1;

        _Token.Type = TOKEN_STRING_LITERAL;

        char *Copy = malloc(_Token.Length + 1);

        memcpy(Copy, _Token.Start, _Token.Length);

        Copy[_Token.Length] = '\0';

        _Token.Literal.String = Copy;
    } else if (Character == '\'') {
        char Value = LexerNext(_Lexer);

        if (LexerNext(_Lexer) != '\'') {
            LexerErrorAt(_Lexer, "invalid character literal");
            LexingError(_Lexer);
        }

        _Token.Type = TOKEN_CHAR_LITERAL;
        _Token.Literal.Int = (uint64_t) Value;
    } else {
        switch (Character) {
            case '+': _Token.Type = TOKEN_PLUS; break;
            case '-':
                if (LexerPeek(_Lexer) == '>') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_ARROW;
                } else {
                    _Token.Type = TOKEN_MINUS;
                }
                
                break;

            case '(': _Token.Type = TOKEN_LPAREN; break;
            case ')': _Token.Type = TOKEN_RPAREN; break;
            case '{': _Token.Type = TOKEN_LBRACE; break;
            case '}': _Token.Type = TOKEN_RBRACE; break;
            case '[': _Token.Type = TOKEN_LBRACKET; break;
            case ']': _Token.Type = TOKEN_RBRACKET; break;
            case ',': _Token.Type = TOKEN_COMMA; break;
            case ';': _Token.Type = TOKEN_SEMICOLON; break;

            default:
                _Token.Type = TOKEN_ERROR;

                LexerErrorAt(_Lexer, "unexpected character");
                LexingError(_Lexer);
        }
    }

    return _Token;
}

TokenStream Tokenize(Lexer *_Lexer) {
    TokenStream _TokenStream = {0};

    _TokenStream.Capacity = 64;
    _TokenStream.Data = malloc(sizeof(Token) * _TokenStream.Capacity);

    while (1) {
        Token _Token = LexerNextToken(_Lexer);

        if (_TokenStream.Count >= _TokenStream.Capacity) {
            _TokenStream.Capacity *= 2;
            _TokenStream.Data = realloc(_TokenStream.Data, sizeof(Token) * _TokenStream.Capacity);
        }

        // DEBUGGING USE | printf("[Lexer] Line %i:%i | %s | Symbol: %.*s\n", _Lexer -> Line, _Lexer -> Column, TokenTypeToString(_Token.Type), _Token.Length, _Token.Start ? _Token.Start : "");

        _TokenStream.Data[_TokenStream.Count++] = _Token;

        if (_Token.Type == TOKEN_EOF)
            break;
    }

    return _TokenStream;
}
