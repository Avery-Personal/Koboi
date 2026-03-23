#ifndef LOWER_H
#define LOWER_H

    #include "../Parser/AST.h"
    #include "IR/HIR.h"

    typedef struct {
        const char *Name;

        HIRValue *Value;

        int ScopeDepth;
        int ScopeID;

        int IsLinear;
        int IsSilent;
        int IsConst;
        int IsPrivate;
        int IsHistory;
    } LowerBinding;

    typedef struct LowerScope {
        LowerBinding *Bindings;

        size_t Count;
        size_t Capacity;

        struct LowerScope *Parent;

        int Depth;
        int ID;
    } LowerScope;

    typedef struct {
        HIRInstruction **Body;

        size_t Count;
        size_t Capacity;

        int ScopeDepth;
    } LowerDeferEntry;

    typedef struct LowerLoopContext {
        const char *HeaderLabel;
        const char *ExitLabel;

        int ScopeDepth;

        struct LowerLoopContext *Parent;
    } LowerLoopContext;

    typedef struct LowerTrailEntry {
        HIRValue *Source;
        HIRValue *Dependent;

        struct LowerTrailEntry *Next;
    } LowerTrailEntry;

    typedef enum {
        LOWER_DIAG_ERROR,
        LOWER_DIAG_WARNING,
        LOWER_DIAG_NOTE,
    } LowerDiagKind;

    typedef struct {
        LowerDiagKind Kind;
        const char *Message;
        uint32_t Line;
        uint32_t Column;
    } LowerDiagnostic;

    typedef struct {
        HIRProgram *Program;

        HIRFunction *CurrentFunction;
        HIRBlock *CurrentBlock;

        LowerScope *CurrentScope;

        int NextScopeID;
        int NextTempID;
        int NextLabelID;

        HIRSafetyLevel *SafetyStack;
        size_t SafetyStackCount;
        size_t SafetyStackCapacity;

        LowerDeferEntry *DeferStack;
        size_t DeferCount;
        size_t DeferCapacity;

        LowerLoopContext *LoopContext;

        LowerTrailEntry *TrailHead;

        const char *CurrentFunctionName;

        int InMacro;
        int MacroDepth;

        LowerDiagnostic *Diagnostics;

        size_t DiagCount;
        size_t DiagCapacity;

        int HasError;
    } LowerContext;

    HIRProgram *LowerASTToHIR(ASTProgram *Program);
    LowerContext *LowerCreateContext(HIRProgram *Program);
    void LowerDestroyContext(LowerContext *Context);

    void LowerProgram(LowerContext *Context, ASTProgram *Program);
    HIRFunction *LowerSubprogram(LowerContext *Context, ASTSubprogram *Subprogram);
    void LowerStatement(LowerContext *Context, ASTStatement *Statement);
    static HIRValue *LLowerMemberRead(LowerContext *Context, ASTExpression *ObjectExpression, ASTExpression *FieldExpression, uint32_t Line, uint32_t Column);
    static int LLowerMemberWrite(LowerContext *Context, ASTExpression *ObjectExpression, ASTExpression *FieldExpression, HIRValue *RHS, uint32_t Line, uint32_t Column);
    HIRValue *LowerExpression(LowerContext *Context, ASTExpression *Expression);
    HIRType *LowerType(ASTType *Type);

    int LowerHasErrors(LowerContext *Context);
    void LowerPrintDiagnostics(LowerContext *Context);

#endif
