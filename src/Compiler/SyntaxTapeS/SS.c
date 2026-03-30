#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SS.h"

static int LabelCounter = 0;

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

static void Emit(SSProgram *Program, SSOp Op, SSOperand Destination, SSOperand S1, SSOperand S2) {
    if (Program -> Count >= Program -> Capacity) {
        Program -> Capacity = Program -> Capacity ? Program -> Capacity * 2 : 64;
        Program -> Instructions = realloc(Program -> Instructions, sizeof(SSInstruction) * Program -> Capacity);
    }

    Program -> Instructions[Program -> Count++] = (SSInstruction){
        .Op = Op,
        .Destination = Destination,
        .Source1 = S1,
        .Source2 = S2
    };
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
        .Register = 0
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
