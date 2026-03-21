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

int TokenIsOperator(TokenType Type) {
    switch (Type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT: return 1;

        default: return 0;
    }
}

const char *TokenTypeToString(TokenType Type) {
    switch (Type) {
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_IDENTIFIER_SPECIAL: return "IDENTIFIER_SPECIAL";

        case TOKEN_INT_LITERAL: return "INT_LITERAL";
        case TOKEN_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        case TOKEN_CHAR_LITERAL: return "CHAR_LITERAL";
        case TOKEN_BOOL_LITERAL: return "BOOL_LITERAL";

        case TOKEN_NONE: return "NONE";

        case TOKEN_GLOBAL: return "GLOBAL";
        case TOKEN_STATIC: return "STATIC";
        case TOKEN_CONST: return "CONST";
        case TOKEN_SILENT: return "SILENT";
        case TOKEN_PRIVATE: return "PRIVATE";
        case TOKEN_EXPORT: return "EXPORT";

        case TOKEN_FUNCTION: return "FUNCTION";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_MODULE: return "MODULE";

        case TOKEN_ENV: return "ENV";
        case TOKEN_REQUIRES: return "REQUIRES";
        case TOKEN_PROVIDES: return "PROVIDES";
        case TOKEN_SYS: return "SYS";

        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_MATCH: return "MATCH";

        case TOKEN_UNSAFE: return "UNSAFE";
        case TOKEN_SAFE: return "SAFE";
        case TOKEN_TRUSTED: return "TRUSTED";
        case TOKEN_CHECK: return "CHECK";
        case TOKEN_ASSUME: return "ASSUME";
        case TOKEN_DEFER: return "DEFER";

        case TOKEN_INT: return "INT";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_BOOL: return "BOOL";
        case TOKEN_STRING: return "STRING";
        case TOKEN_VOID: return "VOID";

        case TOKEN_ENUM: return "ENUM";
        case TOKEN_MACRO: return "MACRO";
        case TOKEN_EXPOSE: return "EXPOSE";
        case TOKEN_NOALIAS: return "NOALIAS";

        case TOKEN_EQUAL: return "EQUAL";
        case TOKEN_PLUS_EQUAL: return "PLUS_EQUAL";
        case TOKEN_MINUS_EQUAL: return "MINUS_EQUAL";

        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";

        case TOKEN_EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TOKEN_NE: return "NOT_EQUAL";
        case TOKEN_GT: return "GREATER_THAN";
        case TOKEN_GE: return "GREATER_EQUAL";
        case TOKEN_LT: return "LESS_THAN";
        case TOKEN_LE: return "LESS_EQUAL";

        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_EXCLAMATION: return "EXCLAMATION";

        case TOKEN_DOT: return "DOT";
        case TOKEN_DOT_DOT: return "DOT_DOT";

        case TOKEN_AMPERSAND: return "AMPERSAND";
        case TOKEN_AT: return "AT";
        case TOKEN_HASH: return "HASH";

        case TOKEN_ARROW: return "ARROW";
        case TOKEN_COLON: return "COLON";
        case TOKEN_DOUBLE_COLON: return "DOUBLE_COLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";

        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";

        case TOKEN_NEWLINE: return "NEWLINE";

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

        if (Character == ' ' || Character == '\t' || Character == '\r') {
            LexerNext(_Lexer);
        } else if (Character == '\n') {
            LexerNext(_Lexer);
        } else if (Character == '/' && _Lexer -> Cursor + 1 < _Lexer -> Length && _Lexer -> Source[_Lexer -> Cursor + 1] == '/') {
            LexerNext(_Lexer);
            LexerNext(_Lexer);

            while (!LexerIsAtEnd(_Lexer) && LexerPeek(_Lexer) != '\n')
                LexerNext(_Lexer);
        } else if (Character == '/' && _Lexer -> Cursor + 1 < _Lexer -> Length && _Lexer -> Source[_Lexer -> Cursor + 1] == '*') {
            LexerNext(_Lexer);
            LexerNext(_Lexer);

            while (!LexerIsAtEnd(_Lexer)) {
                if (LexerPeek(_Lexer) == '*' && _Lexer -> Cursor + 1 < _Lexer -> Length && _Lexer -> Source[_Lexer -> Cursor + 1] == '/') {
                    LexerNext(_Lexer);
                    LexerNext(_Lexer);

                    break;
                }

                LexerNext(_Lexer);
            }
        }

        else break;
    }
}

TokenType ResolveIdentifier(const char *Start, size_t Len) {
    switch (Start[0]) {
        case 'f':
            if (Len == 2 && memcmp(Start, "fn", 2) == 0) return TOKEN_FUNCTION;
            if (Len == 3 && memcmp(Start, "for", 3) == 0) return TOKEN_FOR;

            break;

        case 'r':
            if (Len == 6 && memcmp(Start, "return", 6) == 0) return TOKEN_RETURN;
            if (Len == 8 && memcmp(Start, "requires", 8) == 0) return TOKEN_REQUIRES;
            if (Len == 8 && memcmp(Start, "rollback", 8) == 0) return TOKEN_ROLLBACK;
            if (Len == 6 && memcmp(Start, "region", 6) == 0) return TOKEN_REGION;

            break;

        case 'p':
            if (Len == 8 && memcmp(Start, "provides", 8) == 0) return TOKEN_PROVIDES;
            if (Len == 7 && memcmp(Start, "private", 7) == 0) return TOKEN_PRIVATE;
            if (Len == 7 && memcmp(Start, "partial", 7) == 0) return TOKEN_PARTIAL;

            break;

        case 'e':
            if (Len == 4 && memcmp(Start, "else", 4) == 0) return TOKEN_ELSE;
            if (Len == 4 && memcmp(Start, "enum", 4) == 0) return TOKEN_ENUM;
            if (Len == 6 && memcmp(Start, "export", 6) == 0) return TOKEN_EXPORT;
            if (Len == 3 && memcmp(Start, "env", 3) == 0) return TOKEN_ENV;
            if (Len == 7 && memcmp(Start, "extends", 7) == 0) return TOKEN_ENV;

            break;

        case 'i':
            if (Len == 2 && memcmp(Start, "if", 2) == 0) return TOKEN_IF;
            if (Len == 3 && memcmp(Start, "int", 3) == 0) return TOKEN_INT;

            break;

        case 's':
            if (Len == 6 && memcmp(Start, "static", 6) == 0) return TOKEN_STATIC;
            if (Len == 6 && memcmp(Start, "silent", 6) == 0) return TOKEN_SILENT;
            if (Len == 8 && memcmp(Start, "symbolic", 8) == 0) return TOKEN_SYMBOLIC;
            if (Len == 3 && memcmp(Start, "sys", 3) == 0) return TOKEN_SYS;
            if (Len == 4 && memcmp(Start, "safe", 4) == 0) return TOKEN_SAFE;
            if (Len == 5 && memcmp(Start, "state", 5) == 0) return TOKEN_STATE;
            if (Len == 5 && memcmp(Start, "struct", 5) == 0) return TOKEN_STRUCT;

            break;

        case 'u':
            if (Len == 6 && memcmp(Start, "unsafe", 6) == 0) return TOKEN_UNSAFE;
            if (Len == 5 && memcmp(Start, "using", 5) == 0) return TOKEN_USING;

            break;

        case 't':
            if (Len == 7 && memcmp(Start, "trusted", 7) == 0) return TOKEN_TRUSTED;

            break;

        case 'c':
            if (Len == 5 && memcmp(Start, "const", 5) == 0) return TOKEN_CONST;
            if (Len == 5 && memcmp(Start, "check", 5) == 0) return TOKEN_CHECK;
            if (Len == 7 && memcmp(Start, "context", 7) == 0) return TOKEN_CONTEXT;
            if (Len == 11 && memcmp(Start, "compiletime", 11) == 0) return TOKEN_COMPTIME;

            break;

        case 'a':
            if (Len == 6 && memcmp(Start, "assume", 6) == 0) return TOKEN_ASSUME;

            break;

        case 'd':
            if (Len == 5 && memcmp(Start, "defer", 5) == 0) return TOKEN_DEFER;

            break;

        case 'm':
            if (Len == 5 && memcmp(Start, "match", 5) == 0) return TOKEN_MATCH;
            if (Len == 5 && memcmp(Start, "macro", 5) == 0) return TOKEN_MACRO;
            if (Len == 6 && memcmp(Start, "module", 6) == 0) return TOKEN_MODULE;

            break;

        case 'n':
            if (Len == 7 && memcmp(Start, "noalias", 7) == 0) return TOKEN_NOALIAS;

            break;

        case 'b':
            if (Len == 4 && memcmp(Start, "bool", 4) == 0) return TOKEN_BOOL;

            break;

        case 'v':
            if (Len == 4 && memcmp(Start, "void", 4) == 0) return TOKEN_VOID;

            break;

        case 'g':
            if (Len == 6 && memcmp(Start, "global", 6) == 0) return TOKEN_GLOBAL;

            break;

        case 'w':
            if (Len == 5 && memcmp(Start, "world", 5) == 0) return TOKEN_WORLD;
            if (Len == 4 && memcmp(Start, "with", 4) == 0) return TOKEN_WITH;

            break;

        case 'l':
            if (Len == 6 && memcmp(Start, "linear", 6) == 0) return TOKEN_LINEAR;

            break;

        case 'h':
            if (Len == 7 && memcmp(Start, "history", 7) == 0) return TOKEN_HISTORY;

            break;
    }

    if (Len == 4 && memcmp(Start, "true", 4) == 0) return TOKEN_BOOL_LITERAL;
    if (Len == 5 && memcmp(Start, "false", 5) == 0) return TOKEN_BOOL_LITERAL;

    if (Len == 4 && memcmp(Start, "None", 4) == 0) return TOKEN_NONE;
    
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

Token LexerPeekToken(Lexer *_Lexer) {
    if (!_Lexer -> HasPeeked) {
        _Lexer -> PeekedToken = LexerNextToken(_Lexer);
        _Lexer -> HasPeeked = 1;
    }

    return _Lexer -> PeekedToken;
}

Token LexerConsumePeek(Lexer *_Lexer) {
    if (_Lexer -> HasPeeked) {
        _Lexer -> HasPeeked = 0;
        return _Lexer -> PeekedToken;
    }

    return LexerNextToken(_Lexer);
}

char LexerNext(Lexer *_Lexer) {
    char Character = LexerPeek(_Lexer);

    switch (Character) {
        case '{': _Lexer -> BraceDepth++; break;
        case '}': _Lexer -> BraceDepth--; break;
        case '(': _Lexer -> ParenDepth++; break;
        case ')': _Lexer -> ParenDepth--; break;
        case '[': _Lexer -> BracketDepth++; break;
        case ']': _Lexer -> BracketDepth--; break;
    }

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
    fprintf(stderr, "[Lexer Error] Line %u:%u >> %s\n", _Lexer -> Error.Line, _Lexer -> Error.Column - 1, _Lexer -> Error.Message);
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

        if (Character == '0') {
            char Next = LexerPeek(_Lexer);

            if (Next == 'x') {
                LexerNext(_Lexer);

                size_t Start = _Lexer -> Cursor;

                while (IsDigit(LexerPeek(_Lexer)))
                    LexerNext(_Lexer);

                _Token.Type = TOKEN_INT_LITERAL;
                _Token.Literal.Int = strtoul(_Lexer -> Source + Start, NULL, 16);

                return _Token;
            }
        }

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
            _Token.Type = TOKEN_FLOAT_LITERAL;
            _Token.Literal.Float = strtod(_Token.Start, NULL);
        } else {
            _Token.Type = TOKEN_INT_LITERAL;
            _Token.Literal.Int = (uint64_t) strtoull(_Token.Start, NULL, 10);
        }
    } else if (Character == '"') {
        size_t Start = _Lexer -> Cursor;

        while (!LexerIsAtEnd(_Lexer)) {
            char NextCharacter = LexerNext(_Lexer);

            if (NextCharacter == '"')
                break;

            if (NextCharacter == '\n') {
                LexerErrorAt(_Lexer, "newline in string literal");\
                LexingError(_Lexer);

                break;
            }

            if (NextCharacter == '\\') {
                char Escape = LexerNext(_Lexer);

                switch (Escape) {
                    case 'n': break;
                    case 't': break;
                    case '\\': break;
                    case '"': break;

                    default: LexerErrorAt(_Lexer, "invalid escape sequence");
                }
            }
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
            case '+':
                if (LexerPeek(_Lexer) == '=') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_PLUS_EQUAL;
                } else {
                    _Token.Type = TOKEN_PLUS;
                }

                break;

            case '-':
                if (LexerPeek(_Lexer) == '=') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_MINUS_EQUAL;
                } else if (LexerPeek(_Lexer) == '>') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_ARROW;
                } else {
                    _Token.Type = TOKEN_MINUS;
                }

                break;

            case '*': _Token.Type = TOKEN_STAR; break;
            case '=':
                if (LexerPeek(_Lexer) == '=') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_EQUAL_EQUAL;
                } else {
                    _Token.Type = TOKEN_EQUAL;
                }

                break;

            case '!':
                if (LexerPeek(_Lexer) == '=') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_NE;
                } else {
                    _Token.Type = TOKEN_EXCLAMATION;
                }

                break;

            case '>':
                if (LexerPeek(_Lexer) == '=') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_GE;
                } else if (LexerPeek(_Lexer) == '>') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_MATCH_ARROW;
                } else {
                    _Token.Type = TOKEN_GT;
                }

                break;

            case '<':
                if (LexerPeek(_Lexer) == '=') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_LE;
                } else {
                    _Token.Type = TOKEN_LT;
                }

                break;

            case '.':
                if (LexerPeek(_Lexer) == '.') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_DOT_DOT;
                } else {
                    _Token.Type = TOKEN_DOT;
                }

                break;
            
            case ':':
                if (LexerPeek(_Lexer) == ':') {
                    LexerNext(_Lexer);

                    _Token.Type = TOKEN_DOUBLE_COLON;
                } else {
                    _Token.Type = TOKEN_COLON;
                }

                break;

            case '$': _Token.Type = TOKEN_DOLLAR; break;
            case '&': _Token.Type = TOKEN_AMPERSAND; break;
            case '@': _Token.Type = TOKEN_AT; break;
            case '#': _Token.Type = TOKEN_HASH; break;
            case '(': _Token.Type = TOKEN_LPAREN; break;
            case ')': _Token.Type = TOKEN_RPAREN; break;
            case '{': _Token.Type = TOKEN_LBRACE; break;
            case '}': _Token.Type = TOKEN_RBRACE; break;
            case '[': _Token.Type = TOKEN_LBRACKET; break;
            case ']': _Token.Type = TOKEN_RBRACKET; break;
            case '^': _Token.Type = TOKEN_ARROW; break;
            case ',': _Token.Type = TOKEN_COMMA; break;
            case ';': _Token.Type = TOKEN_SEMICOLON; break;

            default:
                _Token.Type = TOKEN_ERROR;

                LexerErrorAt(_Lexer, "unexpected character");
                LexingError(_Lexer);
        }
    }

    if (_Token.Type == TOKEN_MACRO) {
        _Lexer -> InMacro = 1;
    }

    if (_Token.Type == TOKEN_UNSAFE) {
        _Lexer -> InUnsafe = 1;
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

        if (_Token.Type == TOKEN_IDENTIFIER && _Token.Length > 3)
            printf("[Lexer] Line %i:%i | %s | Symbol: %.*s\n", _Lexer -> Line, _Lexer -> Column, TokenTypeToString(_Token.Type), _Token.Length, _Token.Start ? _Token.Start : "");

        _TokenStream.Data[_TokenStream.Count++] = _Token;

        if (_Token.Type == TOKEN_EOF)
            break;
    }

    return _TokenStream;
}
