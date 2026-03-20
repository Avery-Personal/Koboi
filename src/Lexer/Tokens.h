#ifndef TOKENS_H
#define TOKENS_H

#include <stdint.h>

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_IDENTIFIER_SPECIAL,

    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_BOOL_LITERAL,

    TOKEN_NONE,

    TOKEN_GLOBAL,
    TOKEN_STATIC,
    TOKEN_CONST,
    TOKEN_SILENT,
    TOKEN_PRIVATE,
    TOKEN_EXPORT,

    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_MODULE,

    TOKEN_ENV,
    TOKEN_REQUIRES,
    TOKEN_PROVIDES,
    TOKEN_SYS,

    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_BREAK,
    TOKEN_MATCH,

    TOKEN_UNSAFE,
    TOKEN_SAFE,
    TOKEN_TRUSTED,
    TOKEN_CHECK,
    TOKEN_ASSUME,
    TOKEN_DEFER,

    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_BOOL,
    TOKEN_STRING,
    TOKEN_VOID,

    TOKEN_ENUM,
    TOKEN_MACRO,
    TOKEN_EXPOSE,
    TOKEN_NOALIAS,

    TOKEN_EQUAL,
    TOKEN_PLUS_EQUAL,
    TOKEN_MINUS_EQUAL,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,

    TOKEN_EQUAL_EQUAL,
    TOKEN_NE,
    TOKEN_GT,
    TOKEN_GE,
    TOKEN_LT,
    TOKEN_LE,

    TOKEN_AND,
    TOKEN_OR,
    TOKEN_EXCLAMATION,

    TOKEN_DOT,
    TOKEN_DOT_DOT,

    TOKEN_AMPERSAND,
    TOKEN_AT,
    TOKEN_HASH,

    TOKEN_ARROW,
    TOKEN_COLON,
    TOKEN_DOUBLE_COLON,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    TOKEN_NEWLINE,

    TOKEN_EOF,
    TOKEN_ERROR,
} TokenType;

typedef enum {
    LEXER_MODE_NORMAL,
    LEXER_MODE_STRING,
    LEXER_MODE_CHAR,
    LEXER_MODE_COMMENT,
    LEXER_MODE_BLOCK_COMMENT,
} LexerMode;

typedef enum {
    LEXER_ERROR_NONE,
    LEXER_ERROR_INVALID_CHAR,
    LEXER_ERROR_UNTERMINATED_STRING,
    LEXER_ERROR_INVALID_NUMBER,
    LEXER_ERROR_INVALID_OPERATOR,
} LexerErrorType;

typedef enum {
    TOKEN_FLAG_NONE = 0,
    TOKEN_FLAG_KEYWORD = 1 << 0,
    TOKEN_FLAG_LITERAL = 1 << 1,
    TOKEN_FLAG_OPERATOR = 1 << 2,
    TOKEN_FLAG_ASSIGNMENT = 1 << 3,
    TOKEN_FLAG_UNARY = 1 << 4,
    TOKEN_FLAG_BINARY = 1 << 5,
} TokenFlag;

typedef struct {
    TokenType Type;

    const char *Start;
    uint32_t Length;

    uint32_t Line;
    uint32_t Column;

    uint32_t FileID;
    uint32_t ScopeID;

    uint32_t TokenIndex;
    uint32_t Flags;

    union {
        double Float;
        uint64_t Int;

        const char *String;
    } Literal;
} Token;

typedef struct {
    Token *Data;

    size_t Count;
    size_t Capacity;

    size_t Cursor;

    Token *Current;
    Token *Previous;

    int PanicMode;
} TokenStream;

typedef struct {
    LexerErrorType Type;

    const char *Message;

    uint32_t Line;
    uint32_t Column;

    const char *ContextStart;
    uint32_t ContextLength;

    const char *Hint;
} LexerError;

typedef struct {
    const char *Source;

    size_t Length;
    size_t Cursor;

    uint32_t Line;
    uint32_t Column;

    uint32_t FileID;
    
    LexerMode Mode;

    int BraceDepth;
    int ParenDepth;
    int BracketDepth;

    int InMacro;
    int InUnsafe;
    int InMatch;

    Token PeekedToken;
    int HasPeeked;

    LexerError Error;
    int HasError;
} Lexer;

#endif
