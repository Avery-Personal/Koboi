#ifndef AST_H
#define AST_H

    #include <stdint.h>
    #include <stddef.h>

    typedef enum {
        TYPE_INT,
        TYPE_FLOAT,
        TYPE_CHAR,
        TYPE_STRING,
        TYPE_BOOL,
        TYPE_VOID,
        TYPE_ARRAY,
        TYPE_SLICE,
        TYPE_NAMED
    } ASTTypeKind;

    typedef struct ASTType {
        ASTTypeKind Kind;

        struct ASTType *ElementType;
        size_t ArraySize;

        const char *Name;

        int IsSlice;
    } ASTType;

    typedef enum {
        MOD_NONE = 0,
        MOD_GLOBAL = 1 << 0,
        MOD_STATIC = 1 << 1,
        MOD_CONST = 1 << 2,
        MOD_EXPORT = 1 << 3,
        MOD_PRIVATE = 1 << 4,
        MOD_SILENT = 1 << 5,
        MOD_LINER = 1 << 6,
        MOD_HISTORY = 1 << 7,
        MOD_SYMBOLIC = 1 << 8,
    } ASTModifiers;

    typedef enum {
        OWN_MOVE,
        OWN_BORROW,
        OWN_COPY,
        OWN_TRAIL,
        OWN_ADDRESS
    } ASTOwnershipKind;

    typedef enum {
        OP_ADD,
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_MOD,

        OP_AND,
        OP_OR,
        OP_NOT,

        OP_EQ,
        OP_NE,
        OP_LT,
        OP_LE,
        OP_GT,
        OP_GE,
    } ASTOperator;

    typedef enum {
        EXPR_LITERAL,
        EXPR_IDENTIFIER,
        EXPR_BINARY,
        EXPR_UNARY,
        EXPR_CALL,
        EXPR_ARRAY,
        EXPR_INDEX,
        EXPR_MEMBER,
        EXPR_OWNERSHIP
    } ASTExpressionKind;

    typedef struct ASTExpression ASTExpression;

    typedef struct {
        ASTOwnershipKind Kind;
        ASTExpression *Target;
    } ASTOwnershipExpression;

    typedef struct {
        double Float;
        uint64_t Int;

        char *String;
        char Char;
        int Bool;

        ASTTypeKind LiteralKind;
    } ASTLiteral;

    typedef struct {
        ASTExpression *Left;
        ASTExpression *Right;
        ASTOperator Op;

        int IsAssignment;
    } ASTBinaryExpression;

    typedef struct {
        ASTOperator Op;
        ASTExpression *Operand;
    } ASTUnaryExpression;

    typedef struct {
        ASTExpression *Callee;

        ASTExpression **Arguments;
        size_t ArgumentCount;
    } ASTCallExpression;

    typedef struct {
        ASTExpression **Elements;
        size_t Count;
    } ASTArrayExpression;

    typedef struct {
        ASTExpression *Target;
        ASTExpression *Index;
    } ASTIndexExpression;

    struct ASTExpression {
        ASTExpressionKind Kind;

        union {
            const char *Identifier;

            ASTLiteral Literal;
            ASTBinaryExpression Binary;
            ASTUnaryExpression Unary;
            ASTCallExpression Call;
            ASTArrayExpression Array;
            ASTIndexExpression Index;

            ASTOwnershipExpression Ownership;
        };

        struct {
            ASTType *ResolvedType;

            int IsConstant;
            int IsLValue;
            int OwnershipState;

            int ScopeID;
        } Metadata;
    };

    typedef struct {
        const char *Name;

        ASTExpression *Value;
    } ASTEnvironment;

    typedef struct ASTStatement ASTStatement;

    typedef struct {
        ASTExpression *Pattern;
        ASTExpression *Guard;

        ASTStatement **Body;
        size_t BodyCount;
    } ASTMatchArm;

    typedef enum {
        STMT_VAR_DECL,
        STMT_ENUM_DECL,
        STMT_STATE_DECL,
        STMT_ASSIGN,
        STMT_IF,
        STMT_WHILE,
        STMT_FOR,
        STMT_RETURN,
        STMT_BREAK,
        STMT_CONTINUE,
        STMT_EXPR,
        STMT_BLOCK,

        STMT_MATCH,

        STMT_UNSAFE,
        STMT_SAFE,
        STMT_TRUSTED,

        STMT_CHECK,
        STMT_ASSUME,
        STMT_DEFER
    } ASTStmtKind;

    struct ASTStatement {
        ASTStmtKind Kind;

        union {
            struct {
                const char *Name;

                ASTType *Type;
                ASTExpression *Initializer;
                uint32_t Modifiers;
            } VariableDeclaration;

            struct {
                const char *Name;
                
                const char **Variants;
                size_t VariantCount;
            } EnumDecl;

            struct {
                ASTExpression *Target;
                ASTExpression *Value;
            } Assign;

            struct {
                ASTExpression *Condition;
                ASTStatement **ThenBlock;
                size_t ThenCount;

                ASTStatement **ElseBlock;
                size_t ElseCount;
            } If;

            struct {
                ASTExpression *Condition;
                ASTStatement **Body;

                size_t Count;
            } While;

            struct {
                const char *Iterator;

                ASTExpression *Start;
                ASTExpression *End;
                ASTExpression *Step;

                ASTStatement **Body;

                size_t Count;
            } For;

            struct {
                ASTExpression *Target;

                ASTMatchArm *Arms;
                size_t ArmCount;
            } Match;

            struct {
                ASTExpression *Value;
            } Return;

            struct {
                ASTExpression *Expression;
            } Expression;

            struct {
                ASTStatement **Statements;

                size_t Count;
            } Block;

            struct {
                ASTStatement **Body;

                size_t Count;
            } ScopedBlock;

            struct {
                ASTExpression *Condition;
            } ConditionBlock;

            struct {
                ASTStatement **Body;

                size_t Count;
            } Defer;
        };
    };

    typedef struct {
        const char *Name;
        ASTType *Type;
    } ASTParam;

    typedef struct ASTSubprogram {
        const char *Name;

        ASTParam *Parameters;
        size_t ParameterCount;

        ASTType *ReturnType;

        ASTStatement **Body;
        size_t BodyCount;

        ASTEnvironment *Requires;
        size_t RequireCount;

        ASTEnvironment *Provides;
        size_t ProvideCount;
    } ASTSubprogram;

    typedef struct {
        const char *Name;

        const char **Parameters;
        size_t ParameterCount;

        ASTStatement **Body;
        size_t BodyCount;
    } ASTMacro;

    typedef struct {
        ASTSubprogram **Subprograms;
        size_t SubprogramCount;

        ASTStatement **Statements;
        size_t StatementCount;

        ASTMacro **Macros;
        size_t MacroCount;
    } ASTProgram;

#endif