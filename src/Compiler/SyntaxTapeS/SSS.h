#ifndef SSS_H
#define SSS_H

    #include <stdint.h>
    #include <stddef.h>

    #include "../../Lexer/Tokens.h"

    typedef enum {
        SSS_PART_IDENTIFIER,
        SSS_PART_SPECIAL_IDENTIFIER,
        SSS_PART_LITERAL_INT,
        SSS_PART_LITERAL_FLOAT,
        SSS_PART_LITERAL_STRING,
        SSS_PART_LITERAL_CHAR,
        SSS_PART_LITERAL_BOOL,
        SSS_PART_LITERAL_NONE,

        SSS_PART_KEYWORD,
        SSS_PART_OPERATOR,
        SSS_PART_PUNCTUATION,
        SSS_PART_DELIMITER_OPEN,
        SSS_PART_DELIMITER_CLOSE,

        SSS_PART_LABEL_DEF,
        SSS_PART_LABEL_REF,

        SSS_PART_TYPE_NAME,
        SSS_PART_MODIFIER,
        SSS_PART_SECTION,
        SSS_PART_ATTRIBUTE,
        SSS_PART_SIGIL,

        SSS_PART_PATH,
        SSS_PART_ACCESS,
        SSS_PART_PATTERN,
        SSS_PART_MATCH_ARM,
        SSS_PART_MACRO_BODY,
        SSS_PART_BLOCK_BODY,

        SSS_PART_RAW,
        SSS_PART_ERROR
    } SSSPartType;

    typedef enum {
        SSS_LINE_UNKNOWN,
        SSS_LINE_EMPTY,

        SSS_LINE_USING,
        SSS_LINE_MODULE,
        SSS_LINE_CONTEXT,
        SSS_LINE_CONCEPT,
        SSS_LINE_WORLD,

        SSS_LINE_STRUCT,
        SSS_LINE_PARTIAL,
        SSS_LINE_ENUM,
        SSS_LINE_STATE,
        SSS_LINE_OBJECT,
        SSS_LINE_MACRO,

        SSS_LINE_ENV,
        SSS_LINE_FUNCTION,
        SSS_LINE_PARAM_LIST,
        SSS_LINE_RETURN,

        SSS_LINE_VAR_DECL,
        SSS_LINE_CONST_DECL,
        SSS_LINE_STATIC_DECL,
        SSS_LINE_GLOBAL_DECL,
        SSS_LINE_PRIVATE_DECL,
        SSS_LINE_SILENT_DECL,
        SSS_LINE_LINEAR_DECL,
        SSS_LINE_HISTORY_DECL,
        SSS_LINE_SYMBOLIC_DECL,
        SSS_LINE_EXPORT_DECL,

        SSS_LINE_ASSIGN,
        SSS_LINE_AUG_ASSIGN,
        SSS_LINE_BINDING,
        SSS_LINE_CALL,
        SSS_LINE_INDEX,
        SSS_LINE_FIELD_ACCESS,
        SSS_LINE_DEREF_LOAD,
        SSS_LINE_DEREF_STORE,
        SSS_LINE_ADDR_OF,
        SSS_LINE_COPY_OF,
        SSS_LINE_FREE_OF,

        SSS_LINE_IF,
        SSS_LINE_ELSE,
        SSS_LINE_FOR,
        SSS_LINE_WHILE,
        SSS_LINE_BREAK,
        SSS_LINE_MATCH,
        SSS_LINE_MATCH_ARM,
        SSS_LINE_GUARD,
        SSS_LINE_GOTO,
        SSS_LINE_LABEL,

        SSS_LINE_CHECK,
        SSS_LINE_ASSUME,
        SSS_LINE_DEFER,
        SSS_LINE_UNSAFE,
        SSS_LINE_SAFE,
        SSS_LINE_TRUSTED_UNSAFE,
        SSS_LINE_COMPTIME,
        SSS_LINE_TRANSACTION,
        SSS_LINE_REACTIVE,
        SSS_LINE_REGION,

        SSS_LINE_REQUIRES,
        SSS_LINE_PROVIDES,
        SSS_LINE_SYS,

        SSS_LINE_EXPOSE,
        SSS_LINE_EXTENDS,
        SSS_LINE_WITH,
        SSS_LINE_RAW
    } SSSLineType;

    typedef enum {
        SSS_SECTION_NONE,

        SSS_SECTION_USING,
        SSS_SECTION_MODULE,
        SSS_SECTION_CONTEXT,
        SSS_SECTION_CONCEPT,
        SSS_SECTION_WORLD,

        SSS_SECTION_STRUCT,
        SSS_SECTION_PARTIAL,
        SSS_SECTION_ENUM,
        SSS_SECTION_STATE,
        SSS_SECTION_OBJECT,
        SSS_SECTION_MACRO,

        SSS_SECTION_ENV,
        SSS_SECTION_FUNCTION,

        SSS_SECTION_IF,
        SSS_SECTION_ELSE,
        SSS_SECTION_FOR,
        SSS_SECTION_WHILE,
        SSS_SECTION_MATCH,
        SSS_SECTION_TRANSACTION,
        SSS_SECTION_CHECK,
        SSS_SECTION_ASSUME,
        SSS_SECTION_DEFER,
        SSS_SECTION_UNSAFE,
        SSS_SECTION_SAFE,
        SSS_SECTION_TRUSTED_UNSAFE,
        SSS_SECTION_COMPTIME,
        SSS_SECTION_REACTIVE,
        SSS_SECTION_REGION,
        SSS_SECTION_WITH,

        SSS_SECTION_CUSTOM
    } SSSSectionType;

    typedef enum {
        SSS_BLOCK_NONE,

        SSS_BLOCK_FILE,
        SSS_BLOCK_MODULE,
        SSS_BLOCK_CONTEXT,
        SSS_BLOCK_CONCEPT,
        SSS_BLOCK_WORLD,

        SSS_BLOCK_STRUCT,
        SSS_BLOCK_PARTIAL,
        SSS_BLOCK_ENUM,
        SSS_BLOCK_STATE,
        SSS_BLOCK_OBJECT,
        SSS_BLOCK_MACRO,

        SSS_BLOCK_ENV,
        SSS_BLOCK_FUNCTION,

        SSS_BLOCK_IF,
        SSS_BLOCK_ELSE,
        SSS_BLOCK_FOR,
        SSS_BLOCK_WHILE,
        SSS_BLOCK_MATCH,
        SSS_BLOCK_MATCH_ARM,
        SSS_BLOCK_TRANSACTION,
        SSS_BLOCK_CHECK,
        SSS_BLOCK_ASSUME,
        SSS_BLOCK_DEFER,
        SSS_BLOCK_UNSAFE,
        SSS_BLOCK_SAFE,
        SSS_BLOCK_TRUSTED_UNSAFE,
        SSS_BLOCK_COMPTIME,
        SSS_BLOCK_REACTIVE,
        SSS_BLOCK_REGION,
        SSS_BLOCK_WITH,

        SSS_BLOCK_RAW
    } SSSBlockType;

    typedef enum {
        SSS_DECLFLAG_NONE = 0,
        SSS_DECLFLAG_GLOBAL = 1 << 0,
        SSS_DECLFLAG_STATIC = 1 << 1,
        SSS_DECLFLAG_CONST = 1 << 2,
        SSS_DECLFLAG_SILENT = 1 << 3,
        SSS_DECLFLAG_PRIVATE = 1 << 4,
        SSS_DECLFLAG_LINEAR = 1 << 5,
        SSS_DECLFLAG_HISTORY = 1 << 6,
        SSS_DECLFLAG_SYMBOLIC = 1 << 7,
        SSS_DECLFLAG_EXPORT = 1 << 8,
        SSS_DECLFLAG_NOALIAS = 1 << 9,
        SSS_DECLFLAG_EXPOSE = 1 << 10,
        SSS_DECLFLAG_UNSAFE = 1 << 11,
        SSS_DECLFLAG_TRUSTED = 1 << 12,
        SSS_DECLFLAG_SAFE = 1 << 13,
        SSS_DECLFLAG_COMPTIME = 1 << 14,
        SSS_DECLFLAG_REACTIVE = 1 << 15
    } SSSDeclFlag;

    typedef struct {
        SSSPartType Type;

        const Token *Tokens;
        size_t TokenCount;
    } SSSPart;

    typedef struct {
        SSSLineType Type;
        SSSSectionType Section;
        SSSBlockType Block;

        SSSPart *Parts;
        size_t PartCount;
        size_t PartCapacity;

        uint32_t Line;
        uint32_t Column;

        uint32_t DeclarationFlags;
    } SSSLine;

    typedef struct {
        SSSLine *Lines;
        size_t LineCount;
        size_t LineCapacity;
    } SSSProgram;

    typedef struct {
        const char *Message;
        
        uint32_t Line;
        uint32_t Column;
    } SSSError;

#endif