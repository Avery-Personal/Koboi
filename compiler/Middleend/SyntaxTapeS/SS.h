#ifndef SS_H
#define SS_H

    #include <stdint.h>
    #include <stddef.h>

    #include "../../Frontend/Parser/Parser.h"

    #define SS_FLOAT_SENTINEL INT64_MIN

    #define LOOP_STACK_MAX 64

    typedef enum {
        SS_OP_NOP,

        SS_OP_CONST,
        SS_OP_LOAD,
        SS_OP_STORE,

        SS_OP_LOAD_MEM,
        SS_OP_STORE_MEM,
        SS_OP_ADDR_OF,
        SS_OP_COPY,
        SS_OP_MOVE,
        SS_OP_FREE,

        SS_OP_ADD,
        SS_OP_SUB,
        SS_OP_MUL,
        SS_OP_DIV,
        SS_OP_MOD,

        SS_OP_EQ,
        SS_OP_NE,
        SS_OP_GT,
        SS_OP_GE,
        SS_OP_LT,
        SS_OP_LE,

        SS_OP_AND,
        SS_OP_OR,
        SS_OP_NOT,

        SS_OP_LABEL,
        SS_OP_JMP,
        SS_OP_JMP_IF,
        SS_OP_JMP_IF_NOT,

        SS_OP_CALL,
        SS_OP_CALL_INDIRECT,
        SS_OP_RET,

        SS_OP_SYS,

        SS_OP_PARAM,
        SS_OP_ARG,

        SS_OP_SCOPE_BEGIN,
        SS_OP_SCOPE_END,
        SS_OP_REGION_BEGIN,
        SS_OP_REGION_END,

        SS_OP_ASSUME,
        SS_OP_CHECK,
        SS_OP_DEFER,
        SS_OP_TRAP,

        SS_OP_LINE,
        SS_OP_NOP_DEBUG
    } SSOp;

    typedef enum {
        SS_OPERAND_NONE,

        SS_OPERAND_REGISTER,
        SS_OPERAND_VARIABLE,
        SS_OPERAND_CONSTANT,
        SS_OPERAND_LABEL,

        SS_OPERAND_OFFSET,
        SS_OPERAND_TYPE,
        SS_OPERAND_SYMBOL
    } SSOperandType;

    typedef enum {
        CONST_NONE,
        CONST_INT,
        CONST_FLOAT,
        CONST_STRING
    } SSConstantKind;

    typedef struct {
        SSOperandType Type;

        union {
            int Register;

            const char *Variable;
            const char *Label;

            struct {
                SSConstantKind Kind;

                int64_t Int;
                double Float;
                
                const char *String;
            } Constant;

            int Offset;
        };
    } SSOperand;

    typedef struct {
        SSOp Op;

        SSOperand Destination;
        SSOperand Source1;
        SSOperand Source2;

        uint32_t Line;
        uint32_t Column;
    } SSInstruction;

    typedef struct {
        SSInstruction *Instructions;

        size_t Count;
        size_t Capacity;
    } SSProgram;

    static struct {
        const char *StartLabel;
        const char *EndLabel;
    } LoopStack[LOOP_STACK_MAX];

    static SSOp OpFromAST(ASTOperator Op);
    
    static SSOperand MakeVariable(const char *Name);
    static SSOperand MakeConstInt(int64_t Value);
    static SSOperand MakeLabel(const char *Name);

    static const char *NewLabel();

    static void Emit(SSProgram *Program, SSOp Op, SSOperand Destination, SSOperand S1, SSOperand S2);

    static SSOperand LowerLiteral(ASTLiteral *Literal);
    static SSOperand LowerBinary(ASTBinaryExpression *Binary, SSProgram *Program);
    static SSOperand LowerOwnership(ASTOwnershipExpression *Ownership, SSProgram *Program);
    static SSOperand LowerExpression(ASTExpression *Expression, SSProgram *Program);
    static void LowerAssign(ASTStatement *Statement, SSProgram *Program);
    static void LowerVariableDeclaration(ASTStatement *Statement, SSProgram *Program);
    static void LowerIf(ASTStatement *Statement, SSProgram *Program);
    static void LowerWhile(ASTStatement *Statement, SSProgram *Program);
    void LowerStatement(ASTStatement *Statement, SSProgram *Program);

    SSProgram *LowerSS(ASTProgram *AST);

    void SSOperandToString(SSOperand Op, char *Buffer, size_t Size);
    void SSPrintInstruction(SSInstruction *Instruction, size_t Index);

#endif