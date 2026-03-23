#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "Lower.h"

static const char *LStrDup(const char *String) {
    if (!String) return NULL;

    size_t Len = strlen(String) + 1;
    char *Out = malloc(Len);

    memcpy(Out, String, Len);

    return Out;
}

static const char *LGenLabel(LowerContext *Context, const char *Prefix) {
    char Buffer[256];

    snprintf(Buffer, sizeof(Buffer), "__%s_%d", Prefix, Context -> NextLabelID++);

    return LStrDup(Buffer);
}

static HIRValue *LMakeTemp(LowerContext *Context, HIRType *Type) {
    HIRValue *Value = HIRCreateTemp(Type);

    Value -> TempID   = Context -> NextTempID++;

    return Value;
}

static void LEmit(LowerContext *Context, HIRInstruction *Instruction) {
    assert(Context -> CurrentBlock && "LEmit: no current block");

    HIRAddInstruction(Context -> CurrentBlock, Instruction);
}

static HIRBlock *LNewBlock(LowerContext *Context, const char *LabelHint) {
    const char *Label = LabelHint ? LabelHint : LGenLabel(Context, "blk");

    HIRBlock *Block = HIRCreateBlock(Label);

    Block -> Safety = (Context -> SafetyStackCount > 0) ? Context -> SafetyStack[Context -> SafetyStackCount - 1] : HIR_SAFETY_SAFE;

    HIRAddBlock(Context -> CurrentFunction, Block);

    Context -> CurrentBlock = Block;

    return Block;
}

static void LCFGEdge(HIRBlock *Pred, HIRBlock *Succ) {
    Pred -> Successors = realloc(Pred -> Successors, (Pred -> SuccessorCount + 1) * sizeof(HIRBlock *));
    Pred -> Successors[Pred -> SuccessorCount++] = Succ;

    Succ -> Predecessors = realloc(Succ -> Predecessors, (Succ -> PredecessorCount + 1) * sizeof(HIRBlock *));
    Succ -> Predecessors[Succ -> PredecessorCount++] = Pred;
}

static HIRSafetyLevel LCurrentSafety(LowerContext *Context) {
    return (Context -> SafetyStackCount > 0) ? Context -> SafetyStack[Context -> SafetyStackCount - 1] : HIR_SAFETY_SAFE;
}

static void LPushSafety(LowerContext *Context, HIRSafetyLevel Level) {
    if (Context -> SafetyStackCount == Context -> SafetyStackCapacity) {
        Context -> SafetyStackCapacity = Context -> SafetyStackCapacity ? Context -> SafetyStackCapacity * 2 : 8;
        Context -> SafetyStack = realloc(Context -> SafetyStack, Context -> SafetyStackCapacity * sizeof(HIRSafetyLevel));
    }
    
    Context -> SafetyStack[Context -> SafetyStackCount++] = Level;
}

static void LPopSafety(LowerContext *Context) {
    assert(Context -> SafetyStackCount > 0 && "LPopSafety: underflow");

    Context -> SafetyStackCount--;
}

static LowerScope *LPushScope(LowerContext *Context) {
    LowerScope *String = calloc(1, sizeof(LowerScope));

    String -> Parent = Context -> CurrentScope;
    String -> Depth = Context -> CurrentScope ? Context -> CurrentScope -> Depth + 1 : 0;
    String -> ID = Context -> NextScopeID++;

    Context -> CurrentScope = String;

    return String;
}

static void LPopScope(LowerContext *Context) {
    assert(Context -> CurrentScope && "LPopScope: no scope to pop");

    LowerScope *String = Context -> CurrentScope;

    for (size_t I = 0; I < String -> Count; I++) {
        LowerBinding *Block = &String -> Bindings[I];
        HIRValue *Value = Block -> Value;
        if (!Value)
            continue;

        if (Value -> Ownership.Kind == HIR_OWN_MOVE && !Value -> Ownership.IsMoved && !Value -> Ownership.IsFreed) {
            LEmit(Context, HIRInstDrop(Value));
        }

        if (Value -> Ownership.IsBorrowed && Value -> Ownership.Owner) {
            LEmit(Context, HIRInstBorrowEnd(Value -> Ownership.Owner));

            Value -> Ownership.Owner -> Ownership.IsBorrowed = 0;
        }

        if (Value -> Ownership.Kind == HIR_OWN_TRAIL && Value -> Ownership.Owner) {
            LEmit(Context, HIRInstTrailBreak(Value -> Ownership.Owner));
        }
    }

    Context -> CurrentScope = String -> Parent;

    free(String -> Bindings);
    free(String);
}

static void LBind(LowerContext *Context, const char *Name, HIRValue *Value) {
    assert(Context -> CurrentScope && "LBind: no current scope");

    LowerScope *String = Context -> CurrentScope;

    if (String -> Count == String -> Capacity) {
        String -> Capacity = String -> Capacity ? String -> Capacity * 2 : 8;
        String -> Bindings = realloc(String -> Bindings, String -> Capacity * sizeof(LowerBinding));
    }

    LowerBinding *Block = &String -> Bindings[String -> Count++];

    Block -> Name = Name;
    Block -> Value = Value;
    Block -> ScopeDepth = String -> Depth;
    Block -> ScopeID = String -> ID;
    Block -> IsLinear = (Value -> Modifiers & HIR_MOD_LINEAR) != 0;
    Block -> IsSilent = (Value -> Modifiers & HIR_MOD_SILENT) != 0;
    Block -> IsConst = (Value -> Modifiers & HIR_MOD_CONST) != 0;
    Block -> IsPrivate = (Value -> Modifiers & HIR_MOD_PRIVATE) != 0;
    Block -> IsHistory = (Value -> Modifiers & HIR_MOD_HISTORY) != 0;
}

static void LError(LowerContext *Context, uint32_t Line, uint32_t Column, const char *Format, ...);

static HIRValue *LLookup(LowerContext *Context, const char *Name, uint32_t Line, uint32_t Column) {
    for (LowerScope *String = Context -> CurrentScope; String; String = String -> Parent) {
        for (size_t I = 0; I < String -> Count; I++) {
            LowerBinding *Block = &String -> Bindings[I];
            if (strcmp(Block -> Name, Name) != 0)
                continue;

            HIRValue *Value = Block -> Value;

            if (Value -> Ownership.IsMoved) {
                LError(Context, Line, Column, "use of moved value '%s'", Name);

                return NULL;
            }

            if (Value -> Ownership.IsFreed) {
                LError(Context, Line, Column, "use of freed value '%s'", Name);

                return NULL;
            }

            if (Block -> IsLinear) {
                if (Value -> HasBeenUsed) {
                    LError(Context, Line, Column, "linear variable '%s' used more than once", Name);

                    return NULL;
                }

                Value -> HasBeenUsed = 1;
                Value -> Ownership.IsMoved = 1;
            }

            if (Block -> IsSilent) {
                if (Context -> CurrentScope -> ID != Block -> ScopeID) {
                    LError(Context, Line, Column, "silent variable '%s' accessed outside its declaring scope", Name);

                    return NULL;
                }

                if (Value -> HasBeenUsed) {
                    LError(Context, Line, Column, "silent variable '%s' already consumed (single-use)", Name);
                    
                    return NULL;
                }

                Value -> HasBeenUsed = 1;
            }

            return Value;
        }
    }

    for (size_t I = 0; I < Context -> Program -> GlobalCount; I++) {
        HIRValue *Global = Context -> Program -> Globals[I];

        if (Global -> Name && strcmp(Global -> Name, Name) == 0)
            return Global;
    }

    LError(Context, Line, Column, "undefined variable '%s'", Name);

    return NULL;
}

static void LDeferRegister(LowerContext *Context, HIRInstruction **Body, size_t Count) {
    if (Context -> DeferCount == Context -> DeferCapacity) {
        Context -> DeferCapacity = Context -> DeferCapacity ? Context -> DeferCapacity * 2 : 4;
        Context -> DeferStack = realloc(Context -> DeferStack, Context -> DeferCapacity * sizeof(LowerDeferEntry));
    }

    LowerDeferEntry *Entry  = &Context -> DeferStack[Context -> DeferCount++];

    Entry -> Body = Body;
    Entry -> Count = Count;
    Entry -> ScopeDepth = Context -> CurrentScope ? Context -> CurrentScope -> Depth : 0;

    LEmit(Context, HIRInstDeferPush(Body, Count));
}

static void LDeferRunAll(LowerContext *Context) {
    if (Context -> DeferCount > 0) {
        LEmit(Context, HIRInstDeferRun());
    }
}

static void LDeferRunForExit(LowerContext *Context, int MinDepth) {
    for (int I = (int)Context -> DeferCount - 1; I >= 0; I--) {
        if (Context -> DeferStack[I].ScopeDepth >= MinDepth) {
            LEmit(Context, HIRInstDeferRun());
        }
    }
}

static void LPushLoop(LowerContext *Context, const char *Header, const char *Exit) {
    LowerLoopContext *LoopContext = calloc(1, sizeof(LowerLoopContext));

    LoopContext -> HeaderLabel = Header;
    LoopContext -> ExitLabel   = Exit;
    LoopContext -> ScopeDepth  = Context -> CurrentScope ? Context -> CurrentScope -> Depth : 0;
    LoopContext -> Parent      = Context -> LoopContext;

    Context -> LoopContext = LoopContext;
}

static void LPopLoop(LowerContext *Context) {
    assert(Context -> LoopContext && "LPopLoop: no loop context");

    LowerLoopContext *Old = Context -> LoopContext;

    Context -> LoopContext = Old -> Parent;

    free(Old);
}

static void LTrailAdd(LowerContext *Context, HIRValue *Source, HIRValue *Depth) {
    LowerTrailEntry *Entry = calloc(1, sizeof(LowerTrailEntry));

    Entry -> Source = Source;
    Entry -> Dependent = Depth;
    Entry -> Next = Context -> TrailHead;

    Context -> TrailHead = Entry;
}

static void LTrailPropagate(LowerContext *Context, HIRValue *Source) {
    for (LowerTrailEntry *Entry = Context -> TrailHead; Entry; Entry = Entry -> Next) {
        if (Entry -> Source == Source) {
            LEmit(Context, HIRInstReactiveUpdate(Source));
        }
    }
}

static void LDiag(LowerContext *Context, LowerDiagKind Kind, uint32_t Line, uint32_t Column, const char *Format, va_list Arguments) {
    char Buffer[1024];

    vsnprintf(Buffer, sizeof(Buffer), Format, Arguments);

    if (Context -> DiagCount == Context -> DiagCapacity) {
        Context -> DiagCapacity = Context -> DiagCapacity ? Context -> DiagCapacity * 2 : 8;
        Context -> Diagnostics = realloc(Context -> Diagnostics, Context -> DiagCapacity * sizeof(LowerDiagnostic));
    }

    LowerDiagnostic *Diagnostic = &Context -> Diagnostics[Context -> DiagCount++];

    Diagnostic -> Kind    = Kind;
    Diagnostic -> Message = LStrDup(Buffer);
    Diagnostic -> Line    = Line;
    Diagnostic -> Column  = Column;

    if (Kind == LOWER_DIAG_ERROR)
        Context -> HasError = 1;
}

static void LError(LowerContext *Context, uint32_t Line, uint32_t Column, const char *Format, ...) {
    va_list Arguments;

    va_start(Arguments, Format);

    LDiag(Context, LOWER_DIAG_ERROR, Line, Column, Format, Arguments);

    va_end(Arguments);
}

static void LWarn(LowerContext *Context, uint32_t Line, uint32_t Column, const char *Format, ...) {
    va_list Arguments;

    va_start(Arguments, Format);

    LDiag(Context, LOWER_DIAG_WARNING, Line, Column, Format, Arguments);

    va_end(Arguments);
}

HIRType *LowerType(ASTType *Type) {
    if (!Type)
        return HIRMakeVoidType();

    switch (Type -> Kind) {
        case TYPE_INT: return HIRMakeIntType();
        case TYPE_FLOAT: return HIRMakeFloatType();
        case TYPE_CHAR: return HIRMakeCharType();
        case TYPE_STRING: return HIRMakeStringType();
        case TYPE_BOOL: return HIRMakeBoolType();
        case TYPE_VOID: return HIRMakeVoidType();
        case TYPE_SLICE: return HIRMakeSliceType(LowerType(Type -> ElementType));
        case TYPE_ARRAY: {
            HIRType *Element = LowerType(Type -> ElementType);
            
            return Type -> IsSlice ? HIRMakeSliceType(Element) : HIRMakeArrayType(Element, Type -> ArraySize);
        }

        case TYPE_NAMED: return HIRMakeNamedType(Type -> Name);

        default: return HIRMakeVoidType();
    }
}

static uint32_t LLowerMods(uint32_t ASTMods) {
    uint32_t Modifier = HIR_MOD_NONE;

    if (ASTMods & MOD_GLOBAL) Modifier |= HIR_MOD_GLOBAL;
    if (ASTMods & MOD_STATIC) Modifier |= HIR_MOD_STATIC;
    if (ASTMods & MOD_CONST) Modifier |= HIR_MOD_CONST;
    if (ASTMods & MOD_EXPORT) Modifier |= HIR_MOD_EXPORT;
    if (ASTMods & MOD_PRIVATE) Modifier |= HIR_MOD_PRIVATE;
    if (ASTMods & MOD_SILENT) Modifier |= HIR_MOD_SILENT;
    if (ASTMods & MOD_LINER) Modifier |= HIR_MOD_LINEAR;
    if (ASTMods & MOD_HISTORY) Modifier |= HIR_MOD_HISTORY;
    if (ASTMods & MOD_SYMBOLIC) Modifier |= HIR_MOD_SYMBOLIC;

    return Modifier;
}

static HIROpcode LLowerBinOp(ASTOperator Op) {
    switch (Op) {
        case OP_ADD: return HIR_ADD;
        case OP_SUB: return HIR_SUB;
        case OP_MUL: return HIR_MUL;
        case OP_DIV: return HIR_DIV;
        case OP_MOD: return HIR_MOD;
        case OP_AND: return HIR_AND;
        case OP_OR: return HIR_OR;
        case OP_EQ: return HIR_EQ;
        case OP_NE: return HIR_NE;
        case OP_LT: return HIR_LT;
        case OP_LE: return HIR_LE;
        case OP_GT: return HIR_GT;
        case OP_GE: return HIR_GE;

        default: return HIR_NOP;
    }
}

static int LIsBoolOp(ASTOperator Op) {
    return Op == OP_EQ || Op == OP_NE || Op == OP_LT || Op == OP_LE || Op == OP_GT || Op == OP_GE || Op == OP_AND || Op == OP_OR;
}

static HIRValue *LLowerOwnership(LowerContext *Context, ASTOwnershipExpression *Own) {
    HIRValue *Source = LowerExpression(Context, Own -> Target);
    if (!Source)
        return NULL;

    switch (Own -> Kind) {
        case OWN_MOVE: {
            if (Source -> Ownership.IsBorrowed) {
                LError(Context, 0, 0, "cannot move a borrowed value");

                return NULL;
            }
            
            if (Source -> Ownership.IsMoved) {
                LError(Context, 0, 0, "use of moved value");

                return NULL;
            }

            HIRValue *Destination = LMakeTemp(Context, Source -> Type);

            Destination -> Ownership.Kind  = HIR_OWN_MOVE;
            Destination -> Ownership.Owner = Source;

            Source -> Ownership.IsMoved = 1;

            LEmit(Context, HIRInstMove(Destination, Source));

            return Destination;
        }

        case OWN_BORROW: {
            if (Source -> Ownership.IsBorrowed) {
                LError(Context, 0, 0, "value is already borrowed");

                return NULL;
            }

            HIRValue *Destination = LMakeTemp(Context, Source -> Type);

            Destination -> Ownership.Kind = HIR_OWN_BORROW;
            Destination -> Ownership.Owner = Source;
            Destination -> Ownership.IsBorrowed = 1;

            Source -> Ownership.IsBorrowed  = 1;

            LEmit(Context, HIRInstBorrowBegin(Source));
            LEmit(Context, HIRInstOwn(HIR_OWN_BORROW, Destination, Source));

            return Destination;
        }

        case OWN_COPY: {
            HIRValue *Destination = LMakeTemp(Context, Source -> Type);

            Destination -> Ownership.Kind = HIR_OWN_COPY;

            LEmit(Context, HIRInstCopy(Destination, Source));

            return Destination;
        }

        case OWN_TRAIL: {
            if (Source -> Ownership.IsMoved) {
                LError(Context, 0, 0, "cannot trail a moved value");

                return NULL;
            }

            HIRValue *Destination = LMakeTemp(Context, Source -> Type);

            Destination -> Ownership.Kind  = HIR_OWN_TRAIL;
            Destination -> Ownership.Owner = Source;

            LTrailAdd(Context, Source, Destination);

            Source -> Dependents = realloc(Source -> Dependents, (Source -> DependentCount + 1) * sizeof(HIRValue *));
            Source -> Dependents[Source -> DependentCount++] = Destination;

            LEmit(Context, HIRInstTrailLink(Destination, Source));

            return Destination;
        }

        case OWN_ADDRESS: {
            if (LCurrentSafety(Context) == HIR_SAFETY_SAFE) {
                LError(Context, 0, 0, "raw address operator '#' requires an unsafe context");

                return NULL;
            }

            HIRType *PointerType = HIRMakePtrType(Source -> Type);
            HIRValue *Destination  = LMakeTemp(Context, PointerType);

            Destination -> Ownership.Kind = HIR_OWN_ADDRESS;

            LEmit(Context, HIRInstAddr(Destination, Source));

            return Destination;
        }

        default:
            LError(Context, 0, 0, "unknown ownership kind");

            return NULL;
    }
}

static HIRValue *LLowerAssignment(LowerContext *Context, ASTBinaryExpression *Bin, uint32_t Line, uint32_t Column);

HIRValue *LowerExpression(LowerContext *Context, ASTExpression *Expression) {
    if (!Expression) return NULL;

    uint32_t Line = Expression -> Metadata.ScopeID;
    uint32_t Column = 0;

    switch (Expression -> Kind) {
        case EXPR_LITERAL: {
            ASTLiteral *Literal = &Expression -> Literal;

            HIRValue *Constant = NULL;
            HIRType *Type = NULL;

            switch (Literal -> LiteralKind) {
                case TYPE_INT: {
                    Type = HIRMakeIntType();
                    Constant = HIRCreateIntConst((int64_t)Literal -> Int, Type);

                    break;
                }

                case TYPE_FLOAT: {
                    Type = HIRMakeFloatType();
                    Constant = HIRCreateFloatConst(Literal -> Float);

                    break;
                }

                case TYPE_STRING: {
                    Type = HIRMakeStringType();
                    Constant = HIRCreateStringConst(Literal -> String);

                    break;
                }

                case TYPE_CHAR: {
                    Type = HIRMakeCharType();
                    Constant = HIRCreateCharConst(Literal -> Char);

                    break;
                }

                case TYPE_BOOL: {
                    Type = HIRMakeBoolType();
                    Constant = HIRCreateBoolConst(Literal -> Bool);

                    break;
                }

                default:
                    LError(Context, Line, Column, "unrecognised literal kind");

                    return NULL;
            }

            HIRValue *Destination = LMakeTemp(Context, Type);

            LEmit(Context, HIRInstConst(Destination, Constant));

            return Destination;
        }

        case EXPR_IDENTIFIER: {
            const char *Name = Expression -> Identifier;

            if (strncmp(Name, "sys.", 4) == 0) {
                const char *SysVar = Name + 4;

                HIRType *Type = HIRMakeBoolType();
                HIRValue *Destination = LMakeTemp(Context, Type);

                LEmit(Context, HIRInstSysQuery(Destination, SysVar));

                return Destination;
            }

            return LLookup(Context, Expression -> Identifier, Line, Column);
        }

        case EXPR_BINARY: {
            ASTBinaryExpression *Bin = &Expression -> Binary;
            if (Bin -> IsAssignment) {
                return LLowerAssignment(Context, Bin, Line, Column);
            }

            HIRValue *Left = LowerExpression(Context, Bin -> Left);
            HIRValue *Right = LowerExpression(Context, Bin -> Right);
            if (!Left || !Right)
                return NULL;

            HIROpcode Op = LLowerBinOp(Bin -> Op);
            HIRType *ResultType = LIsBoolOp(Bin -> Op) ? HIRMakeBoolType() : (Left -> Type ? Left -> Type : HIRMakeIntType());
            HIRValue *Destination = LMakeTemp(Context, ResultType);

            LEmit(Context, HIRInstBinary(Op, Destination, Left, Right));

            return Destination;
        }

        case EXPR_UNARY: {
            ASTUnaryExpression *Unary = &Expression -> Unary;
            HIRValue *Operand = LowerExpression(Context, Unary -> Operand);
            if (!Operand)
                return NULL;

            HIROpcode Op;
            HIRType *RightType;

            if (Unary -> Op == OP_NOT) {
                Op = HIR_NOT;

                RightType = HIRMakeBoolType();
            } else {
                Op = HIR_NEG;

                RightType = Operand -> Type ? Operand -> Type : HIRMakeIntType();
            }

            HIRValue *Destination = LMakeTemp(Context, RightType);

            LEmit(Context, HIRInstUnary(Op, Destination, Operand));

            return Destination;
        }

        case EXPR_CALL: {
            ASTCallExpression *Call = &Expression -> Call;

            const char *Target = NULL;

            if (Call -> Callee -> Kind == EXPR_IDENTIFIER) {
                Target = Call -> Callee -> Identifier;
            } else {
                LError(Context, Line, Column, "indirect function calls not yet supported in HIR lowering");
                return NULL;
            }

            HIRValue **Arguments = calloc(Call -> ArgumentCount, sizeof(HIRValue *));
            
            for (size_t I = 0; I < Call -> ArgumentCount; I++) {
                Arguments[I] = LowerExpression(Context, Call -> Arguments[I]);
                if (!Arguments[I]) {
                    free(Arguments);

                    return NULL;
                }
            }

            HIRType *ReturnType = (Expression -> Metadata.ResolvedType) ? LowerType(Expression -> Metadata.ResolvedType) : HIRMakeVoidType();
            HIRValue *Destination = LMakeTemp(Context, ReturnType);

            LEmit(Context, HIRInstCall(Destination, Target, Arguments, Call -> ArgumentCount));

            free(Arguments);

            return Destination;
        }

        case EXPR_ARRAY: {
            ASTArrayExpression *Array = &Expression -> Array;

            HIRType *ElementType = HIRMakeVoidType();
            if (Array -> Count > 0 && Array -> Elements[0] -> Metadata.ResolvedType) {
                ElementType = LowerType(Array -> Elements[0] -> Metadata.ResolvedType);
            }

            HIRType *ArrayType = HIRMakeArrayType(ElementType, Array -> Count);
            HIRValue *SizeConst = HIRCreateIntConst((int64_t)Array -> Count, HIRMakeIntType());
            HIRValue *SizeTemp = LMakeTemp(Context, HIRMakeIntType());

            LEmit(Context, HIRInstConst(SizeTemp, SizeConst));

            HIRValue *ArrayValue = LMakeTemp(Context, ArrayType);

            LEmit(Context, HIRInstArrayAlloc(ArrayValue, ElementType, SizeTemp));

            for (size_t I = 0; I < Array -> Count; I++) {
                HIRValue *Element = LowerExpression(Context, Array -> Elements[I]);
                if (!Element)
                    continue;

                HIRValue *IndexConstant = HIRCreateIntConst((int64_t)I, HIRMakeIntType());
                HIRValue *IndexType = LMakeTemp(Context, HIRMakeIntType());

                LEmit(Context, HIRInstConst(IndexType, IndexConstant));

                HIRValue *ElementPointer = LMakeTemp(Context, HIRMakePtrType(ElementType));

                LEmit(Context, HIRInstArrayIndex(ElementPointer, ArrayValue, IndexType));
                LEmit(Context, HIRInstStore(ElementPointer, Element));
            }

            return ArrayValue;
        }


        case EXPR_INDEX: {
            ASTIndexExpression *Index = &Expression -> Index;

            HIRValue *Target = LowerExpression(Context, Index -> Target);
            HIRValue *IndexValue = LowerExpression(Context, Index -> Index);
            if (!Target || !IndexValue)
                return NULL;
                
            HIRType *ElementType = (Target -> Type && Target -> Type -> ElementType) ? Target -> Type -> ElementType : HIRMakeVoidType();
            HIRValue *Destination = LMakeTemp(Context, ElementType);

            LEmit(Context, HIRInstArrayIndex(Destination, Target, IndexValue));

            return Destination;
        }

        case EXPR_OWNERSHIP: return LLowerOwnership(Context, &Expression -> Ownership);

        default:
            LError(Context, Line, Column, "unrecognised expression kind (%d)", (int) Expression -> Kind);

            return NULL;
    }
}

static HIRValue *LLowerAssignment(LowerContext *Context, ASTBinaryExpression *Bin, uint32_t Line, uint32_t Column) {
    HIRValue *RHS = LowerExpression(Context, Bin -> Right);
    if (!RHS)
        return NULL;

    ASTExpression *LHSExpression = Bin -> Left;

    if (LHSExpression -> Kind == EXPR_IDENTIFIER) {
        const char *Name   = LHSExpression -> Identifier;

        HIRValue *Target = LLookup(Context, Name, Line, Column);
        if (!Target)
            return NULL;

        if (Target -> Modifiers & HIR_MOD_CONST) {
            LError(Context, Line, Column, "cannot assign to const variable '%s'", Name);

            return NULL;
        }

        if (Target -> Ownership.IsBorrowed) {
            LError(Context, Line, Column, "cannot assign to '%s' while it is borrowed", Name);

            return NULL;
        }
        
        HIRValue *NewValue = RHS;

        if (Bin -> Op != OP_EQ) {
            HIROpcode BinOp  = LLowerBinOp(Bin -> Op);
            HIRType *ResultType  = Target -> Type ? Target -> Type : HIRMakeIntType();
            HIRValue *OldValue = Target;

            NewValue = LMakeTemp(Context, ResultType);

            LEmit(Context, HIRInstBinary(BinOp, NewValue, OldValue, RHS));
        }

        LEmit(Context, HIRInstMove(Target, NewValue));

        if (Target -> Modifiers & HIR_MOD_HISTORY) {
            LEmit(Context, HIRInstHistorySnap(Target));
        }

        LTrailPropagate(Context, Target);

        if (Target -> DependentCount > 0) {
            LEmit(Context, HIRInstReactiveUpdate(Target));
        }

        return Target;
    }

    else if (LHSExpression -> Kind == EXPR_INDEX) {
        HIRValue *ArrayValue = LowerExpression(Context, LHSExpression -> Index.Target);
        HIRValue *IndexValue = LowerExpression(Context, LHSExpression -> Index.Index);
        if (!ArrayValue || !IndexValue)
            return NULL;

        HIRType *ElementType = (ArrayValue -> Type && ArrayValue -> Type -> ElementType) ? ArrayValue -> Type -> ElementType : HIRMakeVoidType();
        HIRType *PointerType  = HIRMakePtrType(ElementType);
        HIRValue *ElementPointer = LMakeTemp(Context, PointerType);

        LEmit(Context, HIRInstArrayIndex(ElementPointer, ArrayValue, IndexValue));

        HIRValue *FinalValue = RHS;
        if (Bin -> Op != OP_EQ) {
            HIRValue *OldElem = LMakeTemp(Context, ElementType);

            LEmit(Context, HIRInstLoad(OldElem, ElementPointer));

            HIROpcode BinOp = LLowerBinOp(Bin -> Op);

            FinalValue = LMakeTemp(Context, ElementType);

            LEmit(Context, HIRInstBinary(BinOp, FinalValue, OldElem, RHS));
        }

        LEmit(Context, HIRInstStore(ElementPointer, FinalValue));

        return FinalValue;
    }

    else {
        LError(Context, Line, Column, "invalid assignment target");

        return NULL;
    }
}

static HIRPattern *LLowerPattern(LowerContext *Context, ASTExpression *PatternExpression, ASTExpression *GuardExpression) {
    if (!PatternExpression)
        return HIRPatternWildcard();

    HIRPattern *BasePattern = NULL;

    switch (PatternExpression -> Kind) {
        case EXPR_LITERAL: {
            HIRValue *Literal = LowerExpression(Context, PatternExpression);

            BasePattern = HIRPatternLiteral(Literal);

            break;
        }

        case EXPR_IDENTIFIER: {
            const char *Name = PatternExpression -> Identifier;

            if (strcmp(Name, "None") == 0) {
                BasePattern = HIRPatternNone();

                break;
            }
            
            if (isupper((unsigned char) Name[0])) {
                BasePattern = HIRPatternEnum(Name);
            } else {
                BasePattern = HIRPatternVariable(Name);
            }

            break;
        }

        case EXPR_ARRAY: {
            ASTArrayExpression *Array = &PatternExpression -> Array;
            HIRPattern **Elements = calloc(Array -> Count, sizeof(HIRPattern *));

            for (size_t I = 0; I < Array -> Count; I++) {
                Elements[I] = LLowerPattern(Context, Array -> Elements[I], NULL);
            }

            BasePattern = HIRPatternTuple(Elements, Array -> Count);

            free(Elements);

            break;
        }

        default:
            BasePattern = HIRPatternWildcard();

            break;
    }

    if (GuardExpression) {
        HIRValue *GuardValue = LowerExpression(Context, GuardExpression);
        if (GuardValue) {
            BasePattern = HIRPatternGuard(BasePattern, GuardValue);
        }
    }

    return BasePattern;
}

static void LLowerMatch(LowerContext *Context, ASTStatement *Statement) {
    HIRValue *Scrutinee = LowerExpression(Context, Statement -> Match.Target);
    if (!Scrutinee)
        return;

    size_t ArmCount  = Statement -> Match.ArmCount;
    ASTMatchArm *Arms = Statement -> Match.Arms;

    const char *EndLabel = LGenLabel(Context, "match_end");

    LEmit(Context, HIRInstMatchBegin(Scrutinee));

    const char **TestLabels = calloc(ArmCount + 1, sizeof(const char *));
    const char **BodyLabels = calloc(ArmCount, sizeof(const char *));
    for (size_t I = 0; I < ArmCount; I++) {
        TestLabels[I] = LGenLabel(Context, "mat_tst");
        BodyLabels[I] = LGenLabel(Context, "mat_arm");
    }

    TestLabels[ArmCount] = EndLabel;

    HIRBlock *PreviousBlock  = Context -> CurrentBlock;
    int HasCatchAll = 0;

    for (size_t I = 0; I < ArmCount; I++) {
        ASTMatchArm *Arm = &Arms[I];
        HIRPattern *Pattern = LLowerPattern(Context, Arm -> Pattern, Arm -> Guard);

        if (Pattern -> Kind == HIR_PAT_VARIABLE || Pattern -> Kind == HIR_PAT_WILDCARD || Pattern -> Kind == HIR_PAT_NONE) {
            HasCatchAll = 1;
        }

        HIRInstruction *Test = HIRInstMatchArmTest(Pattern, BodyLabels[I], TestLabels[I + 1]);

        LEmit(Context, Test);

        HIRBlock *BodyBlock = LNewBlock(Context, BodyLabels[I]);

        BodyBlock -> IsMatchArm = 1;

        LCFGEdge(PreviousBlock, BodyBlock);

        LowerScope *ArmScope = LPushScope(Context);

        (void) ArmScope;

        if (Pattern -> Kind == HIR_PAT_VARIABLE && Pattern -> BindName) {
            HIRValue *Bound = LMakeTemp(Context, Scrutinee -> Type);

            LEmit(Context, HIRInstMatchBind(Pattern -> BindName, Scrutinee));
            LBind(Context, Pattern -> BindName, Bound);
        }

        if (Pattern -> Kind == HIR_PAT_GUARD && Pattern -> Guard) {
            LEmit(Context, HIRInstMatchGuard(Pattern -> Guard, TestLabels[I + 1]));
        }

        if (Pattern -> Kind == HIR_PAT_TUPLE) {
            for (size_t J = 0; J < Pattern -> ElementCount; J++) {
                HIRPattern *SubprogramPattern = Pattern -> Elements[J];
                if (SubprogramPattern -> Kind == HIR_PAT_VARIABLE && SubprogramPattern -> BindName) {
                    HIRValue *IndexConstant  = HIRCreateIntConst((int64_t)J, HIRMakeIntType());
                    HIRValue *IndexType = LMakeTemp(Context, HIRMakeIntType());

                    LEmit(Context, HIRInstConst(IndexType, IndexConstant));

                    HIRType *ElementType = (Scrutinee -> Type && Scrutinee -> Type -> ElementType) ? Scrutinee -> Type -> ElementType : HIRMakeVoidType();
                    HIRValue *Element = LMakeTemp(Context, ElementType);

                    LEmit(Context, HIRInstArrayIndex(Element, Scrutinee, IndexType));
                    LEmit(Context, HIRInstMatchBind(SubprogramPattern -> BindName, Element));
                    LBind(Context, SubprogramPattern -> BindName, Element);
                }
            }
        }

        for (size_t J = 0; J < Arm -> BodyCount; J++) {
            LowerStatement(Context, Arm -> Body[J]);
        }

        LPopScope(Context);

        HIRBlock *AfterArm = Context -> CurrentBlock;
        LEmit(Context, HIRInstJmp(EndLabel));

        if (I + 1 < ArmCount) {
            HIRBlock *NextTest = LNewBlock(Context, TestLabels[I + 1]);

            LCFGEdge(AfterArm, NextTest);

            PreviousBlock = NextTest;
        }
    }

    if (!HasCatchAll) {
        HIRInstruction *Missing = calloc(1, sizeof(HIRInstruction));

        Missing -> Op = HIR_MATCH_MISSING;

        LEmit(Context, Missing);
        LWarn(Context, 0, 0, "match may not be exhaustive");
    }

    HIRBlock *EndBlock = LNewBlock(Context, EndLabel);

    LEmit(Context, HIRInstMatchEnd());

    (void) EndBlock;

    free(TestLabels);
    free(BodyLabels);
}

void LowerStatement(LowerContext *Context, ASTStatement *Statement) {
    if (!Statement) return;

    switch (Statement -> Kind) {
        case STMT_VAR_DECL: {
            const char *Name = Statement -> VariableDeclaration.Name;
            ASTType *AType = Statement -> VariableDeclaration.Type;
            ASTExpression *Initialized = Statement -> VariableDeclaration.Initializer;
            uint32_t ASTMods = Statement -> VariableDeclaration.Modifiers;

            HIRType *Type = AType ? LowerType(AType) : HIRMakeVoidType();
            uint32_t Mods = LLowerMods(ASTMods);

            HIRValue *Variable = HIRCreateVar(Name, Type, Mods);

            Variable -> ScopeID  = Context -> CurrentScope ? Context -> CurrentScope -> ID : 0;

            if (Initialized) {
                HIRValue *InitializedValue = LowerExpression(Context, Initialized);

                if (InitializedValue) {
                    HIROwnKind OK = InitializedValue -> Ownership.Kind;
                    if (OK == HIR_OWN_BORROW) {
                        LEmit(Context, HIRInstOwn(HIR_OWN_BORROW, Variable, InitializedValue));

                        Variable -> Ownership.Kind = HIR_OWN_BORROW;
                        Variable -> Ownership.Owner = InitializedValue -> Ownership.Owner;
                        Variable -> Ownership.IsBorrowed = 1;
                    } else if (OK == HIR_OWN_COPY) {
                        LEmit(Context, HIRInstCopy(Variable, InitializedValue));

                        Variable -> Ownership.Kind = HIR_OWN_COPY;
                    } else if (OK == HIR_OWN_TRAIL) {
                        LEmit(Context, HIRInstTrailLink(Variable, InitializedValue -> Ownership.Owner));

                        Variable -> Ownership.Kind  = HIR_OWN_TRAIL;
                        Variable -> Ownership.Owner = InitializedValue -> Ownership.Owner;

                        if (InitializedValue -> Ownership.Owner) {
                            LTrailAdd(Context, InitializedValue -> Ownership.Owner, Variable);
                        }
                    } else if (OK == HIR_OWN_ADDRESS) {
                        LEmit(Context, HIRInstMove(Variable, InitializedValue));

                        Variable -> Ownership.Kind = HIR_OWN_ADDRESS;
                    } else {
                        LEmit(Context, HIRInstMove(Variable, InitializedValue));

                        Variable -> Ownership.Kind = HIR_OWN_MOVE;
                    }
                }
            }

            if (Mods & HIR_MOD_GLOBAL) {
                HIRAddGlobal(Context -> Program, Variable);
            }

            if (Mods & HIR_MOD_HISTORY) {
                LEmit(Context, HIRInstHistorySnap(Variable));
            }

            LBind(Context, Name, Variable);

            if (Context -> CurrentFunction && !(Mods & HIR_MOD_GLOBAL)) {
                HIRFunction *Function = Context -> CurrentFunction;

                Function -> Locals = realloc(Function -> Locals, (Function -> LocalCount + 1) * sizeof(HIRValue *));
                Function -> Locals[Function -> LocalCount++] = Variable;
            }

            break;
        }

        case STMT_ASSIGN: {
            ASTExpression *LHSExpression = Statement -> Assign.Target;
            ASTExpression *RHSExpression = Statement -> Assign.Value;

            HIRValue *Value = LowerExpression(Context, RHSExpression);
            if (!Value)
                break;

            if (LHSExpression -> Kind == EXPR_IDENTIFIER) {
                const char *Name   = LHSExpression -> Identifier;

                HIRValue *Target = LLookup(Context, Name, 0, 0);
                if (!Target) {
                    Context -> HasError = 0;
                    Context -> DiagCount--;

                    HIRType *InferredType = Value->Type ? Value->Type : HIRMakeVoidType();

                    Target = HIRCreateVar(Name, InferredType, HIR_MOD_NONE);
                    
                    Target -> ScopeID = Context->CurrentScope ? Context->CurrentScope->ID : 0;
                    Target -> Ownership.Kind = HIR_OWN_MOVE;

                    LBind(Context, Name, Target);

                    if (Context -> CurrentFunction) {
                        HIRFunction *Function = Context -> CurrentFunction;

                        Function -> Locals = realloc(Function -> Locals, (Function -> LocalCount + 1) * sizeof(HIRValue *));
                        Function -> Locals[Function -> LocalCount++] = Target;
                    }
                }

                if (Target -> Modifiers & HIR_MOD_CONST) {
                    LError(Context, 0, 0, "cannot assign to const '%s'", Name);

                    break;
                }

                if (Target -> Ownership.IsBorrowed) {
                    LError(Context, 0, 0, "cannot assign to '%s' while it is borrowed", Name);

                    break;
                }

                LEmit(Context, HIRInstMove(Target, Value));

                if (Target -> Modifiers & HIR_MOD_HISTORY) {
                    LEmit(Context, HIRInstHistorySnap(Target));
                }

                LTrailPropagate(Context, Target);

                if (Target -> DependentCount > 0) {
                    LEmit(Context, HIRInstReactiveUpdate(Target));
                }
            } else if (LHSExpression -> Kind == EXPR_INDEX) {
                HIRValue *ArrayValue = LowerExpression(Context, LHSExpression -> Index.Target);
                HIRValue *IndexValue = LowerExpression(Context, LHSExpression -> Index.Index);
                if (!ArrayValue || !IndexValue)
                    break;

                HIRType *ElementType = (ArrayValue -> Type && ArrayValue -> Type -> ElementType) ? ArrayValue -> Type -> ElementType : HIRMakeVoidType();
                HIRValue *ElementPointer = LMakeTemp(Context, HIRMakePtrType(ElementType));

                LEmit(Context, HIRInstArrayIndex(ElementPointer, ArrayValue, IndexValue));
                LEmit(Context, HIRInstStore(ElementPointer, Value));

            } else {
                LError(Context, 0, 0, "invalid assignment target");
            }

            break;
        }

        case STMT_IF: {
            HIRValue *Condition = LowerExpression(Context, Statement -> If.Condition);
            if (!Condition)
                break;

            const char *ThenLabel = LGenLabel(Context, "then");
            const char *ElseLabel = Statement -> If.ElseCount > 0 ? LGenLabel(Context, "else") : NULL;
            const char *EndLabel  = LGenLabel(Context, "if_end");

            HIRBlock *ConditionBlock = Context -> CurrentBlock;

            LEmit(Context, HIRInstJmpIfFalse(Condition, ElseLabel ? ElseLabel : EndLabel));

            HIRBlock *ThenBlock = LNewBlock(Context, ThenLabel);

            LCFGEdge(ConditionBlock, ThenBlock);
            LPushScope(Context);

            for (size_t I = 0; I < Statement -> If.ThenCount; I++) {
                LowerStatement(Context, Statement -> If.ThenBlock[I]);
            }

            LPopScope(Context);

            HIRBlock *AfterThen = Context -> CurrentBlock;

            LEmit(Context, HIRInstJmp(EndLabel));

            HIRBlock *AfterElse = AfterThen;
            if (Statement -> If.ElseCount > 0 && ElseLabel) {
                HIRBlock *ElseBlock = LNewBlock(Context, ElseLabel);

                LCFGEdge(ConditionBlock, ElseBlock);
                LPushScope(Context);

                for (size_t I = 0; I < Statement -> If.ElseCount; I++) {
                    LowerStatement(Context, Statement -> If.ElseBlock[I]);
                }

                LPopScope(Context);

                AfterElse = Context -> CurrentBlock;

                LEmit(Context, HIRInstJmp(EndLabel));
            }

            HIRBlock *EndBlock = LNewBlock(Context, EndLabel);

            LCFGEdge(AfterThen, EndBlock);

            if (AfterElse != AfterThen)
                LCFGEdge(AfterElse, EndBlock);

            break;
        }

        case STMT_WHILE: {
            const char *HdrLabel = LGenLabel(Context, "whl_hdr");
            const char *BodyLabel = LGenLabel(Context, "whl_body");
            const char *ExitLabel = LGenLabel(Context, "whl_exit");

            HIRBlock *PreBlock = Context -> CurrentBlock;

            LEmit(Context, HIRInstJmp(HdrLabel));

            HIRBlock *HdrBlock = LNewBlock(Context, HdrLabel);

            LCFGEdge(PreBlock, HdrBlock);

            HIRValue *Condition = LowerExpression(Context, Statement -> While.Condition);
            if (!Condition)
                break;

            LEmit(Context, HIRInstJmpIfFalse(Condition, ExitLabel));

            HIRBlock *BodyBlock = LNewBlock(Context, BodyLabel);

            LCFGEdge(HdrBlock, BodyBlock);
            LPushLoop(Context, HdrLabel, ExitLabel);
            LPushScope(Context);

            for (size_t I = 0; I < Statement -> While.Count; I++) {
                LowerStatement(Context, Statement -> While.Body[I]);
            }

            LPopScope(Context);
            LPopLoop(Context);

            HIRBlock *AfterBody = Context -> CurrentBlock;

            LEmit(Context, HIRInstJmp(HdrLabel));
            LCFGEdge(AfterBody, HdrBlock);

            HIRBlock *ExitBlock = LNewBlock(Context, ExitLabel);

            LCFGEdge(HdrBlock, ExitBlock);

            break;
        }

        case STMT_FOR: {
            const char *IteratorName = Statement -> For.Iterator;

            HIRValue *StartVal = LowerExpression(Context, Statement -> For.Start);
            if (!StartVal)
                break;

            HIRType *IterType = StartVal -> Type ? StartVal -> Type : HIRMakeIntType();
            HIRValue *IteratorValue  = HIRCreateVar(IteratorName, IterType, HIR_MOD_NONE);

            LEmit(Context, HIRInstMove(IteratorValue, StartVal));
            LBind(Context, IteratorName, IteratorValue);

            if (Context -> CurrentFunction) {
                HIRFunction *Function = Context -> CurrentFunction;

                Function -> Locals = realloc(Function -> Locals, (Function -> LocalCount + 1) * sizeof(HIRValue *));
                Function -> Locals[Function -> LocalCount++] = IteratorValue;
            }

            const char *HdrLabel = LGenLabel(Context, "for_hdr");
            const char *BodyLabel = LGenLabel(Context, "for_body");
            const char *ExitLabel = LGenLabel(Context, "for_exit");

            HIRBlock *PreBlock = Context -> CurrentBlock;

            LEmit(Context, HIRInstJmp(HdrLabel));

            HIRBlock *HdrBlock = LNewBlock(Context, HdrLabel);

            LCFGEdge(PreBlock, HdrBlock);

            HIRValue *EndValue = LowerExpression(Context, Statement -> For.End);
            if (!EndValue)
                break;

            HIRValue *ConditionValue = LMakeTemp(Context, HIRMakeBoolType());

            LEmit(Context, HIRInstBinary(HIR_LT, ConditionValue, IteratorValue, EndValue));
            LEmit(Context, HIRInstJmpIfFalse(ConditionValue, ExitLabel));

            HIRBlock *BodyBlock = LNewBlock(Context, BodyLabel);

            LCFGEdge(HdrBlock, BodyBlock);
            LPushLoop(Context, HdrLabel, ExitLabel);
            LPushScope(Context);

            for (size_t I = 0; I < Statement -> For.Count; I++) {
                LowerStatement(Context, Statement -> For.Body[I]);
            }

            LPopScope(Context);
            LPopLoop(Context);

            if (Statement -> For.Step) {
                LowerExpression(Context, Statement -> For.Step);
            }

            HIRBlock *AfterBody = Context -> CurrentBlock;

            LEmit(Context, HIRInstJmp(HdrLabel));
            LCFGEdge(AfterBody, HdrBlock);

            HIRBlock *ExitBlock = LNewBlock(Context, ExitLabel);

            LCFGEdge(HdrBlock, ExitBlock);

            break;
        }

        case STMT_RETURN: {
            HIRValue *ReturnValue = NULL;

            if (Statement -> Return.Value) {
                ReturnValue = LowerExpression(Context, Statement -> Return.Value);
            }

            LDeferRunAll(Context);
            LEmit(Context, HIRInstReturn(ReturnValue));
            LNewBlock(Context, LGenLabel(Context, "unreachable"));

            break;
        }

        case STMT_BREAK: {
            if (!Context -> LoopContext) {
                LError(Context, 0, 0, "'break' outside of a loop");

                break;
            }

            LDeferRunForExit(Context, Context -> LoopContext -> ScopeDepth);
            LEmit(Context, HIRInstJmp(Context -> LoopContext -> ExitLabel));
            LNewBlock(Context, LGenLabel(Context, "after_break"));

            break;
        }

        case STMT_CONTINUE: {
            if (!Context -> LoopContext) {
                LError(Context, 0, 0, "'continue' outside of a loop");

                break;
            }

            LDeferRunForExit(Context, Context -> LoopContext -> ScopeDepth);
            LEmit(Context, HIRInstJmp(Context -> LoopContext -> HeaderLabel));
            LNewBlock(Context, LGenLabel(Context, "after_continue"));

            break;
        }

        case STMT_EXPR: {
            LowerExpression(Context, Statement -> Expression.Expression);

            break;
        }

        case STMT_BLOCK: {
            LPushScope(Context);

            for (size_t I = 0; I < Statement -> Block.Count; I++) {
                LowerStatement(Context, Statement -> Block.Statements[I]);
            }

            LPopScope(Context);

            break;
        }

        case STMT_MATCH: {
            LLowerMatch(Context, Statement);

            break;
        }

        case STMT_UNSAFE: {
            LPushSafety(Context, HIR_SAFETY_UNSAFE);
            LEmit(Context, HIRInstUnsafeEnter(HIR_SAFETY_UNSAFE));

            Context -> CurrentBlock -> Safety = HIR_SAFETY_UNSAFE;

            LPushScope(Context);

            for (size_t I = 0; I < Statement -> ScopedBlock.Count; I++) {
                LowerStatement(Context, Statement -> ScopedBlock.Body[I]);
            }

            LPopScope(Context);
            LEmit(Context, HIRInstUnsafeExit(HIR_SAFETY_UNSAFE));
            LPopSafety(Context);

            break;
        }

        case STMT_SAFE: {
            if (LCurrentSafety(Context) == HIR_SAFETY_SAFE) {
                LWarn(Context, 0, 0, "'safe' block has no effect outside of an 'unsafe' context");
            }

            LPushSafety(Context, HIR_SAFETY_SAFE);
            LEmit(Context, HIRInstUnsafeEnter(HIR_SAFETY_SAFE));

            Context -> CurrentBlock -> Safety = HIR_SAFETY_SAFE;

            LPushScope(Context);

            for (size_t I = 0; I < Statement -> ScopedBlock.Count; I++) {
                LowerStatement(Context, Statement -> ScopedBlock.Body[I]);
            }

            LPopScope(Context);

            LEmit(Context, HIRInstUnsafeExit(HIR_SAFETY_SAFE));
            LPopSafety(Context);

            break;
        }

        case STMT_TRUSTED: {
            LPushSafety(Context, HIR_SAFETY_TRUSTED);
            LEmit(Context, HIRInstUnsafeEnter(HIR_SAFETY_TRUSTED));

            Context -> CurrentBlock -> Safety = HIR_SAFETY_TRUSTED;

            LPushScope(Context);

            for (size_t I = 0; I < Statement -> ScopedBlock.Count; I++) {
                LowerStatement(Context, Statement -> ScopedBlock.Body[I]);
            }

            LPopScope(Context);

            LEmit(Context, HIRInstUnsafeExit(HIR_SAFETY_TRUSTED));

            LPopSafety(Context);

            break;
        }

        case STMT_CHECK: {
            HIRValue *Condition = LowerExpression(Context, Statement -> ConditionBlock.Condition);
            if (!Condition) 
                break;

            int IsRT = (LCurrentSafety(Context) != HIR_SAFETY_SAFE) ? 1 : 0;

            LEmit(Context, HIRInstCheck(Condition, IsRT));

            break;
        }

        case STMT_ASSUME: {
            HIRValue *Condition = LowerExpression(Context, Statement -> ConditionBlock.Condition);
            if (!Condition)
                break;

            LEmit(Context, HIRInstAssume(Condition));

            break;
        }

        case STMT_DEFER: {
            HIRBlock *SaveBlock = Context -> CurrentBlock;

            char CapLabel[256];

            snprintf(CapLabel, sizeof(CapLabel), "__defer_cap_%d", Context -> NextLabelID++);

            HIRBlock *CapBlock = HIRCreateBlock(LStrDup(CapLabel));

            CapBlock -> IsDeferred = 1;
            CapBlock -> Safety = LCurrentSafety(Context);

            HIRAddBlock(Context -> CurrentFunction, CapBlock);

            Context -> CurrentBlock = CapBlock;

            LPushScope(Context);

            for (size_t I = 0; I < Statement -> Defer.Count; I++) {
                LowerStatement(Context, Statement -> Defer.Body[I]);
            }

            LPopScope(Context);

            HIRInstruction **DeferBody  = CapBlock -> Instructions;
            size_t DeferCount = CapBlock -> InstructionCount;

            Context -> CurrentBlock = SaveBlock;

            LDeferRegister(Context, DeferBody, DeferCount);

            break;
        }

        default:
            LError(Context, 0, 0, "unrecognised statement kind (%d)", (int) Statement -> Kind);

            break;
    }
}

HIRFunction *LowerSubprogram(LowerContext *Context, ASTSubprogram *Subprogram) {
    HIRType *ReturnType = LowerType(Subprogram -> ReturnType);
    HIRFunction *Function = HIRCreateFunction(Subprogram -> Name, ReturnType);

    HIRFunction *OldFunction = Context -> CurrentFunction;
    const char *OldFunctionName = Context -> CurrentFunctionName;
    LowerLoopContext *OldLoopContext = Context -> LoopContext;
    size_t OldDeferCount  = Context -> DeferCount;

    Context -> CurrentFunction = Function;
    Context -> CurrentFunctionName = Subprogram -> Name;
    Context -> LoopContext = NULL;

    const char *EntryLabel = LGenLabel(Context, "entry");

    LNewBlock(Context, EntryLabel);
    LPushScope(Context);

    for (size_t I = 0; I < Subprogram -> ParameterCount; I++) {
        ASTParam *Paremeters = &Subprogram -> Parameters[I];
        HIRType *ParemeterType = LowerType(Paremeters -> Type);
        HIRValue *ParameterValue  = HIRCreateParam(Paremeters -> Name, ParemeterType);

        ParameterValue -> Ownership.Kind = HIR_OWN_MOVE;

        Function -> Params = realloc(Function -> Params, (Function -> ParamCount + 1) * sizeof(HIRValue *));
        Function -> Params[Function -> ParamCount++] = ParameterValue;

        LBind(Context, Paremeters -> Name, ParameterValue);
    }

    for (size_t I = 0; I < Subprogram -> RequireCount; I++) {
        ASTEnvironment *Environment  = &Subprogram -> Requires[I];
        HIREnvironment *HEnvironment = calloc(1, sizeof(HIREnvironment));

        HEnvironment -> Name = Environment -> Name;

        HIRFunctionAddRequires(Function, HEnvironment);
        LEmit(Context, HIRInstEnvRequire(Environment -> Name));
    }

    for (size_t I = 0; I < Subprogram -> ProvideCount; I++) {
        ASTEnvironment *Environment  = &Subprogram -> Provides[I];
        HIREnvironment *HEnvironment = calloc(1, sizeof(HIREnvironment));

        HEnvironment -> Name = Environment -> Name;

        HIRFunctionAddProvides(Function, HEnvironment);

        HIRValue *EnvironmentValue = NULL;

        if (Environment -> Value) {
            EnvironmentValue = LowerExpression(Context, Environment -> Value);
        }

        LEmit(Context, HIRInstEnvProvide(Environment -> Name, EnvironmentValue));
    }

    for (size_t I = 0; I < Subprogram -> BodyCount; I++) {
        LowerStatement(Context, Subprogram -> Body[I]);
    }

    {
        HIRBlock *Last = Context -> CurrentBlock;

        int HasTerminator = 0;

        if (Last -> InstructionCount > 0) {
            HIROpcode LastOp = Last -> Instructions[Last -> InstructionCount - 1] -> Op;

            HasTerminator = (LastOp == HIR_RETURN);
        }

        if (!HasTerminator) {
            LDeferRunAll(Context);
            LEmit(Context, HIRInstReturn(NULL));
        }
    }

    LPopScope(Context);

    Context -> CurrentFunction = OldFunction;
    Context -> CurrentFunctionName = OldFunctionName;
    Context -> LoopContext = OldLoopContext;
    Context -> DeferCount = OldDeferCount;

    return Function;
}

void LowerProgram(LowerContext *Context, ASTProgram *Program) {
    if (Program -> StatementCount > 0) {
        HIRType *VoidType = HIRMakeVoidType();
        HIRFunction *InitializeFunction = HIRCreateFunction("__koboi_init__", VoidType);

        HIRFunction *OldFunction = Context -> CurrentFunction;

        Context -> CurrentFunction = InitializeFunction;

        LNewBlock(Context, LGenLabel(Context, "init_entry"));
        LPushScope(Context);

        for (size_t I = 0; I < Program -> StatementCount; I++) {
            ASTStatement *Statement = Program -> Statements[I];
            if (!Statement)
                continue;

            (void) Statement;
        }

        for (size_t I = 0; I < Program -> StatementCount; I++) {
            LowerStatement(Context, Program -> Statements[I]);
        }

        LDeferRunAll(Context);
        LEmit(Context, HIRInstReturn(NULL));
        LPopScope(Context);

        Context -> CurrentFunction = OldFunction;

        HIRAddFunction(Context -> Program, InitializeFunction);
    }

    for (size_t I = 0; I < Program -> MacroCount; I++) {
        ASTMacro *Macro  = Program -> Macros[I];
        HIRType *VoidType = HIRMakeVoidType();

        HIRFunction *MacroFunction  = HIRCreateFunction(Macro -> Name, VoidType);

        MacroFunction -> IsMacroBody  = 1;

        HIRFunction *OldFunction = Context -> CurrentFunction;

        Context -> CurrentFunction = MacroFunction;

        LNewBlock(Context, LGenLabel(Context, "macro_entry"));
        LPushScope(Context);

        for (size_t Paremeters = 0; Paremeters < Macro -> ParameterCount; Paremeters++) {
            HIRValue *ParameterValue = HIRCreateParam(Macro -> Parameters[Paremeters], VoidType);

            MacroFunction -> Params  = realloc(MacroFunction -> Params, (MacroFunction -> ParamCount + 1) * sizeof(HIRValue *));
            MacroFunction -> Params[MacroFunction -> ParamCount++] = ParameterValue;

            LBind(Context, Macro -> Parameters[Paremeters], ParameterValue);
        }

        LEmit(Context, HIRInstMacroSite(Macro -> Name));

        for (size_t J = 0; J < Macro -> BodyCount; J++) {
            LowerStatement(Context, Macro -> Body[J]);
        }

        LEmit(Context, HIRInstReturn(NULL));
        LPopScope(Context);

        Context -> CurrentFunction = OldFunction;

        HIRAddFunction(Context -> Program, MacroFunction);
    }

    for (size_t I = 0; I < Program -> SubprogramCount; I++) {
        ASTSubprogram *Subprogram = Program -> Subprograms[I];
        HIRFunction *Function  = LowerSubprogram(Context, Subprogram);

        if (strcmp(Subprogram -> Name, "main") == 0) {
            Function -> IsEntryPoint = 1;
        }

        HIRAddFunction(Context -> Program, Function);
    }
}

LowerContext *LowerCreateContext(HIRProgram *Program) {
    LowerContext *Context = calloc(1, sizeof(LowerContext));

    Context -> Program = Program;

    LPushSafety(Context, HIR_SAFETY_SAFE);

    return Context;
}

void LowerDestroyContext(LowerContext *Context) {
    if (!Context)
        return;

    free(Context -> SafetyStack);
    free(Context -> DeferStack);

    for (LowerTrailEntry *Entry = Context -> TrailHead; Entry; ) {
        LowerTrailEntry *Next = Entry -> Next;

        free(Entry);

        Entry = Next;
    }

    for (size_t I = 0; I < Context -> DiagCount; I++) {
        free((char *)Context -> Diagnostics[I].Message);
    }

    free(Context -> Diagnostics);

    while (Context -> CurrentScope) {
        LowerScope *String = Context -> CurrentScope;

        Context -> CurrentScope = String -> Parent;

        free(String -> Bindings);
        free(String);
    }

    while (Context -> LoopContext) {
        LowerLoopContext *LoopContext = Context -> LoopContext;

        Context -> LoopContext = LoopContext -> Parent;

        free(LoopContext);
    }

    free(Context);
}

HIRProgram *LowerASTToHIR(ASTProgram *Program) {
    if (!Program) return NULL;

    HIRProgram *HIRProg = HIRCreateProgram();
    LowerContext *Context= LowerCreateContext(HIRProg);

    LowerProgram(Context, Program);

    if (Context -> HasError) {
        LowerPrintDiagnostics(Context);
        LowerDestroyContext(Context);
        
        return HIRProg;
    }

    LowerDestroyContext(Context);

    return HIRProg;
}

int LowerHasErrors(LowerContext *Context) {
    return Context ? Context -> HasError : 0;
}

void LowerPrintDiagnostics(LowerContext *Context) {
    if (!Context)
        return;

    for (size_t I = 0; I < Context -> DiagCount; I++) {
        LowerDiagnostic *Diagnostic = &Context -> Diagnostics[I];
        
        const char *KindString = Diagnostic -> Kind == LOWER_DIAG_ERROR   ? "ERROR"   : Diagnostic -> Kind == LOWER_DIAG_WARNING ? "WARNING" : "Note";
        
        if (Diagnostic -> Line > 0) {
            fprintf(stderr, "[%s] %u:%u: %s\n", KindString, Diagnostic -> Line, Diagnostic -> Column, Diagnostic -> Message);
        } else {
            fprintf(stderr, "[%s] %s\n", KindString, Diagnostic -> Message);
        }
    }
}
