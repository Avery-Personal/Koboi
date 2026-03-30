#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SS.h"

#define SS_DEBUG 1

static int LabelCounter = 0;
static int TemporaryCounter = 0;

static int LoopStackTop = 0;

const char *SSOpToString(SSOp Op) {
    switch (Op) {
        case SS_OP_NOP: return "NOP";

        case SS_OP_CONST: return "CONST";
        case SS_OP_LOAD: return "LOAD";
        case SS_OP_STORE: return "STORE";

        case SS_OP_LOAD_MEM: return "LOAD_MEM";
        case SS_OP_STORE_MEM: return "STORE_MEM";
        case SS_OP_ADDR_OF: return "ADDR_OF";
        case SS_OP_COPY: return "COPY";
        case SS_OP_MOVE: return "MOVE";
        case SS_OP_FREE: return "FREE";

        case SS_OP_ADD: return "ADD";
        case SS_OP_SUB: return "SUB";
        case SS_OP_MUL: return "MUL";
        case SS_OP_DIV: return "DIV";
        case SS_OP_MOD: return "MOD";

        case SS_OP_EQ: return "EQ";
        case SS_OP_NE: return "NE";
        case SS_OP_GT: return "GT";
        case SS_OP_GE: return "GE";
        case SS_OP_LT: return "LT";
        case SS_OP_LE: return "LE";

        case SS_OP_AND: return "AND";
        case SS_OP_OR: return "OR";
        case SS_OP_NOT: return "NOT";

        case SS_OP_LABEL: return "LABEL";
        case SS_OP_JMP: return "JMP";
        case SS_OP_JMP_IF: return "JMP_IF";
        case SS_OP_JMP_IF_NOT: return "JMP_IF_NOT";

        case SS_OP_CALL: return "CALL";
        case SS_OP_CALL_INDIRECT: return "CALL_INDIRECT";
        case SS_OP_RET: return "RET";

        case SS_OP_SYS: return "SYS";

        case SS_OP_PARAM: return "PARAM";
        case SS_OP_ARG: return "ARG";

        case SS_OP_SCOPE_BEGIN: return "SCOPE_BEGIN";
        case SS_OP_SCOPE_END: return "SCOPE_END";
        case SS_OP_REGION_BEGIN: return "REGION_BEGIN";
        case SS_OP_REGION_END: return "REGION_END";

        case SS_OP_ASSUME: return "ASSUME";
        case SS_OP_CHECK: return "CHECK";
        case SS_OP_DEFER: return "DEFER";
        case SS_OP_TRAP: return "TRAP";

        case SS_OP_LINE: return "LINE";
        case SS_OP_NOP_DEBUG: return "NOP_DEBUG";

        default: return "UNKNOWN";
    }
}

static SSOp OpFromAST(ASTOperator Op) {
    switch (Op) {
        case OP_ADD: return SS_OP_ADD;
        case OP_SUB: return SS_OP_SUB;
        case OP_MUL: return SS_OP_MUL;
        case OP_DIV: return SS_OP_DIV;
        case OP_MOD: return SS_OP_MOD;

        case OP_EQ: return SS_OP_EQ;
        case OP_NE: return SS_OP_NE;
        case OP_GT: return SS_OP_GT;
        case OP_GE: return SS_OP_GE;
        case OP_LT: return SS_OP_LT;
        case OP_LE: return SS_OP_LE;

        case OP_AND: return SS_OP_AND;
        case OP_OR: return SS_OP_OR;

        default: return SS_OP_NOP;
    }
}

static int NewTemporary() {
    return TemporaryCounter++;
}
 
static void PushLoop(const char *Start, const char *End) {
    if (LoopStackTop < LOOP_STACK_MAX) {
        LoopStack[LoopStackTop].StartLabel = Start;
        LoopStack[LoopStackTop].EndLabel   = End;
        
        LoopStackTop++;
    }
}
 
static void PopLoop(void) {
    if (LoopStackTop > 0)
        LoopStackTop--;
}
 
static const char *CurrentLoopStart(void) {
    return LoopStackTop > 0 ? LoopStack[LoopStackTop - 1].StartLabel : NULL;
}
 
static const char *CurrentLoopEnd(void) {
    return LoopStackTop > 0 ? LoopStack[LoopStackTop - 1].EndLabel : NULL;
}

static SSOperand MakeRegister(void) {
    SSOperand Op = {0};

    Op.Type = SS_OPERAND_REGISTER;
    Op.Register  = NewTemporary();

    return Op;
}

static SSOperand MakeVariable(const char *Name) {
    SSOperand Op = {0};

    Op.Type = SS_OPERAND_VARIABLE;
    Op.Variable = Name;

    return Op;
}

static SSOperand MakeConstInt(int64_t Value) {
    SSOperand Op = {0};

    Op.Type = SS_OPERAND_CONSTANT;
    Op.Constant.Int = Value;

    return Op;
}

static SSOperand MakeLabel(const char *Name) {
    SSOperand Op = {0};

    Op.Type = SS_OPERAND_LABEL;
    Op.Label = Name;

    return Op;
}

static const char *NewLabel() {
    static char Buffer[64];
    
    sprintf(Buffer, "L%d", LabelCounter++);
    
    return strdup(Buffer);
}

static void Emit(SSProgram *Program, SSOp Op, SSOperand Destination, SSOperand Source1, SSOperand Source2) {
    if (Program -> Count >= Program -> Capacity) {
        Program -> Capacity = Program -> Capacity ? Program -> Capacity * 2 : 64;
        Program -> Instructions = realloc(Program -> Instructions, sizeof(SSInstruction) * Program -> Capacity);
    }

    SSInstruction Instruction = {
        .Op = Op,
        .Destination = Destination,
        .Source1 = Source1,
        .Source2 = Source2
    };

    Program -> Instructions[Program -> Count++] = Instruction;


    #if SS_DEBUG
        SSPrintInstruction(&Instruction, Program -> Count - 1);
    #endif
}

static SSOperand LowerLiteral(ASTLiteral *Literal) {
    SSOperand Op = {0};

    Op.Type = SS_OPERAND_CONSTANT;

    switch (Literal -> LiteralKind) {
        case TYPE_INT:
            Op.Constant.Int = Literal -> Int;

            break;
        case TYPE_FLOAT:
            Op.Constant.Float = Literal -> Float;

            break;
        case TYPE_STRING:
            Op.Constant.String = Literal -> String;

            break;

        default: break;
    }

    return Op;
}

static SSOperand LowerBinary(ASTBinaryExpression *Binary, SSProgram *Program) {
    SSOperand Left = LowerExpression(Binary -> Left, Program);
    SSOperand Right = LowerExpression(Binary -> Right, Program);

    SSOperand Temporary = {
        .Type = SS_OPERAND_REGISTER,
        .Register = NewTemporary()
    };

    Emit(Program, OpFromAST(Binary -> Op), Temporary, Left, Right);

    return Temporary;
}

static SSOperand LowerOwnership(ASTOwnershipExpression *Ownership, SSProgram *Program) {
    SSOperand Target = LowerExpression(Ownership -> Target, Program);

    switch (Ownership -> Kind) {
        case OWN_MOVE:
            Emit(Program, SS_OP_MOVE, Target, Target, (SSOperand){0});

            break;

        case OWN_COPY:
            Emit(Program, SS_OP_COPY, Target, Target, (SSOperand){0});

            break;

        case OWN_ADDRESS:
            Emit(Program, SS_OP_ADDR_OF, Target, Target, (SSOperand){0});

            break;

        default: break;
    }

    return Target;
}

static SSOperand LowerExpression(ASTExpression *Expression, SSProgram *Program) {
    switch (Expression -> Kind) {
        case EXPR_LITERAL: return LowerLiteral(&Expression -> Literal);
        case EXPR_IDENTIFIER: return MakeVariable(Expression -> Identifier);
        case EXPR_BINARY: return LowerBinary(&Expression -> Binary, Program);

        case EXPR_UNARY: {
            SSOperand Value = LowerExpression(Expression -> Unary.Operand, Program);

            if (Expression -> Unary.Op == OP_NOT)
                Emit(Program, SS_OP_NOT, Value, Value, (SSOperand){0});

            return Value;
        }

        case EXPR_OWNERSHIP: return LowerOwnership(&Expression -> Ownership, Program);

        default: return (SSOperand){0};
    }
}

static void LowerAssign(ASTStatement *Statement, SSProgram *Program) {
    SSOperand Target = LowerExpression(Statement -> Assign.Target, Program);
    SSOperand Value  = LowerExpression(Statement -> Assign.Value, Program);

    Emit(Program, SS_OP_STORE, Target, Value, (SSOperand){0});
}

static void LowerVariableDeclaration(ASTStatement *Statement, SSProgram *Program) {
    if (Statement -> VariableDeclaration.Initializer) {
        SSOperand Value = LowerExpression(Statement -> VariableDeclaration.Initializer, Program);
        SSOperand Variable = MakeVariable(Statement -> VariableDeclaration.Name);

        Emit(Program, SS_OP_STORE, Variable, Value, (SSOperand){0});
    }
}

static void LowerIf(ASTStatement *Statement, SSProgram *Program) {
    const char *LabelElse = NewLabel();
    const char *LabelEnd  = NewLabel();

    SSOperand Cond = LowerExpression(Statement -> If.Condition, Program);

    Emit(Program, SS_OP_JMP_IF_NOT, MakeLabel(LabelElse), Cond, (SSOperand){0});

    for (size_t i = 0; i < Statement -> If.ThenCount; i++)
        LowerStatement(Statement -> If.ThenBlock[i], Program);

    Emit(Program, SS_OP_JMP, MakeLabel(LabelEnd), (SSOperand){0}, (SSOperand){0});
    Emit(Program, SS_OP_LABEL, MakeLabel(LabelElse), (SSOperand){0}, (SSOperand){0});

    for (size_t i = 0; i < Statement -> If.ElseCount; i++)
        LowerStatement(Statement -> If.ElseBlock[i], Program);

    Emit(Program, SS_OP_LABEL, MakeLabel(LabelEnd), (SSOperand){0}, (SSOperand){0});
}

static void LowerWhile(ASTStatement *Statement, SSProgram *Program) {
    const char *LabelStart = NewLabel();
    const char *LabelEnd = NewLabel();

    Emit(Program, SS_OP_LABEL, MakeLabel(LabelStart), (SSOperand){0}, (SSOperand){0});

    SSOperand Cond = LowerExpression(Statement -> While.Condition, Program);

    Emit(Program, SS_OP_JMP_IF_NOT, MakeLabel(LabelEnd), Cond, (SSOperand){0});

    for (size_t i = 0; i < Statement -> While.Count; i++)
        LowerStatement(Statement -> While.Body[i], Program);

    Emit(Program, SS_OP_JMP, MakeLabel(LabelStart), (SSOperand){0}, (SSOperand){0});
    Emit(Program, SS_OP_LABEL, MakeLabel(LabelEnd), (SSOperand){0}, (SSOperand){0});
}

void LowerStatement(ASTStatement *Statement, SSProgram *Program) {
    switch (Statement -> Kind) {
        case STMT_VAR_DECL:
            LowerVariableDeclaration(Statement, Program);

            break;

        case STMT_ASSIGN:
            LowerAssign(Statement, Program);

            break;

        case STMT_IF:
            LowerIf(Statement, Program);

            break;

        case STMT_WHILE:
            LowerWhile(Statement, Program);

            break;

        case STMT_EXPR:
            LowerExpression(Statement -> Expression.Expression, Program);

            break;

        default: break;
    }
}

SSProgram *LowerSS(ASTProgram *AST) {
    SSProgram *Program = calloc(1, sizeof(SSProgram));

    for (size_t i = 0; i < AST -> StatementCount; i++) {
        LowerStatement(AST -> Statements[i], Program);
    }

    return Program;
}

void SSOperandToString(SSOperand Op, char *Buffer, size_t Size) {
    switch (Op.Type) {
        case SS_OPERAND_NONE:
            snprintf(Buffer, Size, "_");

            break;

        case SS_OPERAND_REGISTER:
            snprintf(Buffer, Size, "r%d", Op.Register);

            break;

        case SS_OPERAND_VARIABLE:
            snprintf(Buffer, Size, "%s", Op.Variable);

            break;

        case SS_OPERAND_LABEL:
            snprintf(Buffer, Size, "%s", Op.Label);

            break;

        case SS_OPERAND_CONSTANT:
            if (Op.Constant.String)
                snprintf(Buffer, Size, "\"%s\"", Op.Constant.String);
            else
                snprintf(Buffer, Size, "%lld", (long long) Op.Constant.Int);
            break;

        case SS_OPERAND_OFFSET:
            snprintf(Buffer, Size, "offset(%d)", Op.Offset);

            break;

        default:
            snprintf(Buffer, Size, "?");

            break;
    }
}

void SSPrintInstruction(SSInstruction *Instruction, size_t Index) {
    char Destination[64], Source1[64], Source2[64];

    SSOperandToString(Instruction -> Destination, Destination, sizeof(Destination));
    SSOperandToString(Instruction -> Source1, Source1, sizeof(Source1));
    SSOperandToString(Instruction -> Source2, Source2, sizeof(Source2));

    printf("[SS] #%zu | %-12s | %s, %s, %s\n", Index, SSOpToString(Instruction -> Op), Destination, Source1, Source2);
}
