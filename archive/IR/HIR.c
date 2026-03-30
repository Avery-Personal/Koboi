#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "HIR.h"

static const char *HIRStrDup(const char *Source) {
    if (!Source)
        return NULL;

    size_t Length = strlen(Source) + 1;
    char *Copy = malloc(Length);

    memcpy(Copy, Source, Length);

    return Copy;
}

static HIRInstruction *HIRAllocInst(HIROpcode Op) {
    HIRInstruction *Instruction = calloc(1, sizeof(HIRInstruction));

    Instruction -> Op = Op;
    Instruction -> Safety = HIR_SAFETY_SAFE;

    return Instruction;
}

HIRProgram *HIRCreateProgram(void) {
    HIRProgram *Program = calloc(1, sizeof(HIRProgram));

    return Program;
}

void HIRDestroyProgram(HIRProgram *Program) {
    if (!Program)
        return;
    
    free(Program -> Functions);
    free(Program -> Globals);
    free(Program -> Environments);
    free(Program -> Worlds);
    free(Program -> Contexts);
    free(Program -> Modules);

    free(Program);
}

void HIRAddFunction(HIRProgram *Program, HIRFunction *Function) {
    assert(Program && Function);

    if (Program -> FunctionCount == Program -> FunctionCapacity) {
        Program -> FunctionCapacity = Program -> FunctionCapacity ? Program -> FunctionCapacity * 2 : 8;
        Program -> Functions = realloc(Program -> Functions, Program -> FunctionCapacity * sizeof(HIRFunction *));
    }

    Program -> Functions[Program -> FunctionCount++] = Function;
}

void HIRAddGlobal(HIRProgram *Program, HIRValue *Global) {
    assert(Program && Global);

    if (Program -> GlobalCount == Program -> GlobalCapacity) {
        Program -> GlobalCapacity = Program -> GlobalCapacity ? Program -> GlobalCapacity * 2 : 8;
        Program -> Globals = realloc(Program -> Globals, Program -> GlobalCapacity * sizeof(HIRValue *));
    }

    Program -> Globals[Program -> GlobalCount++] = Global;
}

void HIRAddWorld(HIRProgram *Program, HIRWorld *World) {
    assert(Program && World);

    Program -> Worlds = realloc(Program -> Worlds, (Program -> WorldCount + 1) * sizeof(HIRWorld *));
    Program -> Worlds[Program -> WorldCount++] = World;
}

void HIRAddContext(HIRProgram *Program, HIRContext *Context) {
    assert(Program && Context);

    Program -> Contexts = realloc(Program -> Contexts, (Program -> ContextCount + 1) * sizeof(HIRContext *));
    Program -> Contexts[Program -> ContextCount++] = Context;
}

void HIRAddModule(HIRProgram *Program, HIRModule *Module) {
    assert(Program && Module);

    Program -> Modules = realloc(Program -> Modules, (Program -> ModuleCount + 1) * sizeof(HIRModule *));
    Program -> Modules[Program -> ModuleCount++] = Module;
}

void HIRAddEnv(HIRProgram *Program, HIREnvironment *Environment) {
    assert(Program && Environment);

    Program -> Environments = realloc(Program -> Environments, (Program -> EnvCount + 1) * sizeof(HIREnvironment *));
    Program -> Environments[Program -> EnvCount++] = Environment;
}

HIRFunction *HIRCreateFunction(const char *Name, HIRType *ReturnType) {
    HIRFunction *Function = calloc(1, sizeof(HIRFunction));

    Function -> Name = HIRStrDup(Name);
    Function -> ReturnType = ReturnType;
    Function -> Safety = HIR_SAFETY_SAFE;

    return Function;
}

void HIRAddBlock(HIRFunction *Function, HIRBlock *Block) {
    assert(Function && Block);

    if (Function -> BlockCount == Function -> BlockCapacity) {
        Function -> BlockCapacity = Function -> BlockCapacity ? Function -> BlockCapacity * 2 : 8;
        Function -> Blocks = realloc(Function -> Blocks, Function -> BlockCapacity * sizeof(HIRBlock *));
    }

    Function -> Blocks[Function -> BlockCount++] = Block;
}

void HIRFunctionAddRequires(HIRFunction *Function, HIREnvironment *Environment) {
    assert(Function && Environment);
    Function -> Requires = realloc(Function -> Requires, (Function -> RequireCount + 1) * sizeof(HIREnvironment *));
    Function -> Requires[Function -> RequireCount++] = Environment;
}

void HIRFunctionAddProvides(HIRFunction *Function, HIREnvironment *Environment) {
    assert(Function && Environment);

    Function -> Provides = realloc(Function -> Provides, (Function -> ProvideCount + 1) * sizeof(HIREnvironment *));
    Function -> Provides[Function -> ProvideCount++] = Environment;
}

HIRBlock *HIRCreateBlock(const char *Label) {
    HIRBlock *Block = calloc(1, sizeof(HIRBlock));

    Block -> Label = HIRStrDup(Label);
    Block -> Safety = HIR_SAFETY_SAFE;

    return Block;
}

void HIRAddInstruction(HIRBlock *Block, HIRInstruction *Instruction) {
    assert(Block && Instruction);

    if (Block -> InstructionCount == Block -> InstructionCapacity) {
        Block -> InstructionCapacity = Block -> InstructionCapacity ? Block -> InstructionCapacity * 2 : 16;
        Block -> Instructions = realloc(Block -> Instructions, Block -> InstructionCapacity * sizeof(HIRInstruction *));
    }

    Block -> Instructions[Block -> InstructionCount++] = Instruction;
}

static HIRType *HIRAllocType(HIRTypeKind Kind) {
    HIRType *Type = calloc(1, sizeof(HIRType));
    
    Type -> Kind = Kind;
    
    return Type;
}

HIRType *HIRMakeIntType(void) {
    return HIRAllocType(HIR_TYPE_INT);    
}
HIRType *HIRMakeFloatType(void) {
    return HIRAllocType(HIR_TYPE_FLOAT);  
}
HIRType *HIRMakeBoolType(void) {
    return HIRAllocType(HIR_TYPE_BOOL);   
}
HIRType *HIRMakeCharType(void) {
    return HIRAllocType(HIR_TYPE_CHAR);   
}
HIRType *HIRMakeStringType(void) {
    return HIRAllocType(HIR_TYPE_STRING); 
}
HIRType *HIRMakeVoidType(void) {
    return HIRAllocType(HIR_TYPE_VOID);   
}

HIRType *HIRMakePtrType(HIRType *Element) {
    HIRType *Type = HIRAllocType(HIR_TYPE_PTR);

    Type -> ElementType = Element;

    return Type;
}

HIRType *HIRMakeArrayType(HIRType *Element, size_t Size) {
    HIRType *Type = HIRAllocType(HIR_TYPE_ARRAY);

    Type -> ElementType = Element;
    Type -> ArraySize = Size;

    return Type;
}

HIRType *HIRMakeSliceType(HIRType *Element) {
    HIRType *Type = HIRAllocType(HIR_TYPE_SLICE);

    Type -> ElementType = Element;
    Type -> IsSlice = 1;

    return Type;
}

HIRType *HIRMakeNamedType(const char *Name) {
    HIRType *Type = HIRAllocType(HIR_TYPE_NAMED);

    Type -> Name = HIRStrDup(Name);

    return Type;
}

HIRType *HIRMakePartialType(const char *Name) {
    HIRType *Type = HIRAllocType(HIR_TYPE_PARTIAL);

    Type -> Name = HIRStrDup(Name);

    return Type;
}

HIRType *HIRMakeEnvType(const char *Name) {
    HIRType *Type = HIRAllocType(HIR_TYPE_ENV);

    Type -> Name = HIRStrDup(Name);

    return Type;
}

static HIRValue *HIRAllocValue(HIRValueKind Kind, HIRType *Type) {
    HIRValue *Value = calloc(1, sizeof(HIRValue));
    
    Value -> Kind = Kind;
    Value -> Type = Type;
    Value -> TempID = -1;
    Value -> Ownership.Kind = HIR_OWN_IMPLICIT;
    
    return Value;
}

HIRValue *HIRCreateTemp(HIRType *Type) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_TEMP, Type);

    return Value;
}

HIRValue *HIRCreateVar(const char *Name, HIRType *Type, uint32_t Modifiers) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_VAR, Type);

    Value -> Name = HIRStrDup(Name);
    Value -> Modifiers = Modifiers;

    return Value;
}

HIRValue *HIRCreateParam(const char *Name, HIRType *Type) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_PARAM, Type);

    Value -> Name = HIRStrDup(Name);

    return Value;
}

HIRValue *HIRCreateIntConst(int64_t V, HIRType *Type) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_CONST, Type ? Type : HIRMakeIntType());

    Value -> IntValue = V;

    return Value;
}

HIRValue *HIRCreateFloatConst(double V) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_CONST, HIRMakeFloatType());

    Value -> FloatValue = V;

    return Value;
}

HIRValue *HIRCreateBoolConst(int V) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_CONST, HIRMakeBoolType());

    Value -> BoolValue = V;

    return Value;
}

HIRValue *HIRCreateStringConst(const char *V) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_CONST, HIRMakeStringType());

    Value -> StringValue = HIRStrDup(V);

    return Value;
}

HIRValue *HIRCreateCharConst(char V) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_CONST, HIRMakeCharType());

    Value -> CharValue = V;

    return Value;
}

HIRValue *HIRCreateLabelVal(const char *Label) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_LABEL, HIRMakeVoidType());

    Value -> Label = HIRStrDup(Label);

    return Value;
}

HIRValue *HIRCreateEnvHandle(const char *Name) {
    HIRValue *Value = HIRAllocValue(HIR_VAL_ENV, HIRMakeEnvType(Name));

    Value -> Name = HIRStrDup(Name);

    return Value;
}

HIRInstruction *HIRInstNop(void) {
    return HIRAllocInst(HIR_NOP);
}

HIRInstruction *HIRInstConst(HIRValue *Destination, HIRValue *Literal) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_CONST);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Literal;

    return Instruction;
}

HIRInstruction *HIRInstMove(HIRValue *Destination, HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_MOVE);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstCopy(HIRValue *Destination, HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_COPY);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstOwn(HIROwnKind Kind, HIRValue *Destination, HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_OWN_OP);

    Instruction -> OwnKind = Kind;
    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstFree(HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_FREE);

    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstDrop(HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_DROP);

    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstBorrowBegin(HIRValue *Target) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_BORROW_BEGIN);

    Instruction -> Source1 = Target;

    return Instruction;
}

HIRInstruction *HIRInstBorrowEnd(HIRValue *Target) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_BORROW_END);

    Instruction -> Source1 = Target;

    return Instruction;
}

HIRInstruction *HIRInstTrailLink(HIRValue *Destination, HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_TRAIL_LINK);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstTrailBreak(HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_TRAIL_BREAK);

    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstBinary(HIROpcode Op, HIRValue *Destination, HIRValue *Left, HIRValue *Right) {
    HIRInstruction *Instruction = HIRAllocInst(Op);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Left;
    Instruction -> Source2 = Right;

    return Instruction;
}

HIRInstruction *HIRInstUnary(HIROpcode Op, HIRValue *Destination, HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(Op);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstLoad(HIRValue *Destination, HIRValue *Address) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_LOAD);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Address;

    return Instruction;
}

HIRInstruction *HIRInstStore(HIRValue *Address, HIRValue *Value) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_STORE);

    Instruction -> Destination = Address;
    Instruction -> Source1 = Value;

    return Instruction;
}

HIRInstruction *HIRInstAddr(HIRValue *Destination, HIRValue *Variable) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ADDR);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Variable;

    return Instruction;
}

HIRInstruction *HIRInstArrayAlloc(HIRValue *Destination, HIRType *Element, HIRValue *Size) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ARRAY_ALLOC);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Size;

    (void) Element;

    return Instruction;
}

HIRInstruction *HIRInstArrayIndex(HIRValue *Destination, HIRValue *Array, HIRValue *Index) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ARRAY_INDEX);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Array;
    Instruction -> Source2 = Index;

    return Instruction;
}

HIRInstruction *HIRInstArrayLen(HIRValue *Destination, HIRValue *Array) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ARRAY_LEN);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Array;

    return Instruction;
}

HIRInstruction *HIRInstStructField(HIRValue *Destination, HIRValue *Source, const char *Field) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_STRUCT_FIELD);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;
    Instruction -> EnvironmentField = HIRStrDup(Field);

    return Instruction;
}

HIRInstruction *HIRInstStructSet(HIRValue *Destination, const char *Field, HIRValue *Value) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_STRUCT_SET);

    Instruction -> Destination = Destination;
    Instruction -> EnvironmentField = HIRStrDup(Field);
    Instruction -> Source1 = Value;

    return Instruction;
}

HIRInstruction *HIRInstPartialField(HIRValue *Destination, HIRValue *Source, const char *Field) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_PARTIAL_FIELD);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;
    Instruction -> EnvironmentField = HIRStrDup(Field);

    return Instruction;
}

HIRInstruction *HIRInstPartialCheck(HIRValue *Destination, HIRValue *Source, const char *Field) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_PARTIAL_CHECK);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;
    Instruction -> EnvironmentField = HIRStrDup(Field);

    return Instruction;
}

HIRInstruction *HIRInstJmp(const char *Label) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_JMP);

    Instruction -> ScopeName = HIRStrDup(Label);

    return Instruction;
}

HIRInstruction *HIRInstJmpIf(HIRValue *Condition, const char *Label) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_JMP_IF);
    Instruction -> Source1 = Condition;
    Instruction -> ScopeName = HIRStrDup(Label);
    return Instruction;
}

HIRInstruction *HIRInstJmpIfFalse(HIRValue *Condition, const char *Label) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_JMP_IF_FALSE);

    Instruction -> Source1 = Condition;
    Instruction -> ScopeName = HIRStrDup(Label);

    return Instruction;
}

HIRInstruction *HIRInstMatchBegin(HIRValue *Scrutinee) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_MATCH_BEGIN);

    Instruction -> Source1 = Scrutinee;

    return Instruction;
}

HIRInstruction *HIRInstMatchArmTest(HIRPattern *Pattern, const char *ArmLabel, const char *NextLabel) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_MATCH_ARM_TEST);

    Instruction -> Pattern = Pattern;
    Instruction -> ArmLabel = HIRStrDup(ArmLabel);
    Instruction -> NextLabel = HIRStrDup(NextLabel);

    return Instruction;
}

HIRInstruction *HIRInstMatchBind(const char *Name, HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_MATCH_BIND);

    Instruction -> EnvironmentName = HIRStrDup(Name);
    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstMatchGuard(HIRValue *Condition, const char *NextLabel) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_MATCH_GUARD);

    Instruction -> Source1 = Condition;
    Instruction -> NextLabel = HIRStrDup(NextLabel);

    return Instruction;
}

HIRInstruction *HIRInstMatchEnd(void) {
    return HIRAllocInst(HIR_MATCH_END);
}

HIRInstruction *HIRInstCall(HIRValue *Destination, const char *Target, HIRValue **Args, size_t Count) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_CALL);

    Instruction -> Destination = Destination;
    Instruction -> CallTarget = HIRStrDup(Target);
    Instruction -> ArgCount = Count;

    if (Count > 0) {
        Instruction -> Args = malloc(Count * sizeof(HIRValue *));

        memcpy(Instruction -> Args, Args, Count * sizeof(HIRValue *));
    }

    return Instruction;
}

HIRInstruction *HIRInstReturn(HIRValue *Value) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_RETURN);

    Instruction -> Source1 = Value;

    return Instruction;
}

HIRInstruction *HIRInstEnvProvide(const char *Name, HIRValue *Value) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ENV_PROVIDE);

    Instruction -> EnvironmentName = HIRStrDup(Name);
    Instruction -> Source1 = Value;

    return Instruction;
}

HIRInstruction *HIRInstEnvRequire(const char *Name) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ENV_REQUIRE);

    Instruction -> EnvironmentName = HIRStrDup(Name);

    return Instruction;
}

HIRInstruction *HIRInstEnvGet(HIRValue *Destination, const char *Environment, const char *Field) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ENV_GET);

    Instruction -> Destination = Destination;
    Instruction -> EnvironmentName = HIRStrDup(Environment);
    Instruction -> EnvironmentField = HIRStrDup(Field);

    return Instruction;
}

HIRInstruction *HIRInstEnvSet(const char *Environment, const char *Field, HIRValue *Value) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ENV_SET);

    Instruction -> EnvironmentName = HIRStrDup(Environment);
    Instruction -> EnvironmentField = HIRStrDup(Field);
    Instruction -> Source1 = Value;

    return Instruction;
}

HIRInstruction *HIRInstUnsafeEnter(HIRSafetyLevel Level) {
    HIROpcode Op = (Level == HIR_SAFETY_TRUSTED) ? HIR_TRUSTED_ENTER : (Level == HIR_SAFETY_UNSAFE)  ? HIR_UNSAFE_ENTER :                                 HIR_SAFE_ENTER;
    HIRInstruction *Instruction = HIRAllocInst(Op);

    Instruction -> Safety = Level;

    return Instruction;
}

HIRInstruction *HIRInstUnsafeExit(HIRSafetyLevel Level) {
    HIROpcode Op = (Level == HIR_SAFETY_TRUSTED) ? HIR_TRUSTED_EXIT : (Level == HIR_SAFETY_UNSAFE)  ? HIR_UNSAFE_EXIT :                                 HIR_SAFE_EXIT;
    HIRInstruction *Instruction = HIRAllocInst(Op);

    Instruction -> Safety = Level;

    return Instruction;
}

HIRInstruction *HIRInstCheck(HIRValue *Condition, int IsRuntime) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_CHECK);

    Instruction -> Source1 = Condition;
    Instruction -> IsRuntime = IsRuntime;

    return Instruction;
}

HIRInstruction *HIRInstAssume(HIRValue *Condition) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_ASSUME);

    Instruction -> Source1 = Condition;

    return Instruction;
}

HIRInstruction *HIRInstDeferPush(HIRInstruction **Body, size_t Count) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_DEFER_PUSH);

    Instruction -> DeferBody = Body;
    Instruction -> DeferBodyCount = Count;

    return Instruction;
}

HIRInstruction *HIRInstDeferRun(void) {
    return HIRAllocInst(HIR_DEFER_RUN);
}

HIRInstruction *HIRInstTransactionBegin(void) {
    return HIRAllocInst(HIR_TRANSACTION_BEGIN);
}

HIRInstruction *HIRInstTransactionRollback(void) {
    return HIRAllocInst(HIR_TRANSACTION_ROLLBACK);
}

HIRInstruction *HIRInstTransactionCommit(void) {
    return HIRAllocInst(HIR_TRANSACTION_COMMIT);
}

HIRInstruction *HIRInstRegionEnter(const char *Name) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_REGION_ENTER);

    Instruction -> ScopeName = HIRStrDup(Name);

    return Instruction;
}

HIRInstruction *HIRInstRegionExit(const char *Name) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_REGION_EXIT);

    Instruction -> ScopeName = HIRStrDup(Name);

    return Instruction;
}

HIRInstruction *HIRInstHistorySnap(HIRValue *Variable) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_HISTORY_SNAP);

    Instruction -> Source1 = Variable;

    return Instruction;
}

HIRInstruction *HIRInstHistoryRead(HIRValue *Destination, HIRValue *Variable, int Index, int BranchIdx, int BranchPath) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_HISTORY_READ);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Variable;
    Instruction -> HistoryIndex = Index;
    Instruction -> HistoryBranchIndex = BranchIdx;
    Instruction -> HistoryBranchPath = BranchPath;

    return Instruction;
}

HIRInstruction *HIRInstReactiveBind(HIRValue *Destination, HIRValue *Expression) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_REACTIVE_BIND);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Expression;

    return Instruction;
}

HIRInstruction *HIRInstReactiveUpdate(HIRValue *Changed) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_REACTIVE_UPDATE);

    Instruction -> Source1 = Changed;

    return Instruction;
}

HIRInstruction *HIRInstComptimeBegin(void) {
    return HIRAllocInst(HIR_COMPTIME_BEGIN);
}

HIRInstruction *HIRInstComptimeEnd(void) {
    return HIRAllocInst(HIR_COMPTIME_END);
}

HIRInstruction *HIRInstComptimeResult(HIRValue *Destination, HIRValue *Literal) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_COMPTIME_RESULT);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Literal;

    return Instruction;
}

HIRInstruction *HIRInstMacroSite(const char *MacroName) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_MACRO_SITE);

    Instruction -> MacroName = HIRStrDup(MacroName);

    return Instruction;
}

HIRInstruction *HIRInstMacroExpose(HIRValue *Destination, HIRValue *Source) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_MACRO_EXPOSE);

    Instruction -> Destination = Destination;
    Instruction -> Source1 = Source;

    return Instruction;
}

HIRInstruction *HIRInstContextEnter(const char *Name) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_CONTEXT_ENTER);

    Instruction -> ScopeName = HIRStrDup(Name);

    return Instruction;
}

HIRInstruction *HIRInstContextExit(const char *Name) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_CONTEXT_EXIT);

    Instruction -> ScopeName = HIRStrDup(Name);

    return Instruction;
}

HIRInstruction *HIRInstWorldEnter(const char *Name) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_WORLD_ENTER);

    Instruction -> ScopeName = HIRStrDup(Name);

    return Instruction;
}

HIRInstruction *HIRInstWorldExit(const char *Name) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_WORLD_EXIT);

    Instruction -> ScopeName = HIRStrDup(Name);

    return Instruction;
}

HIRInstruction *HIRInstSysQuery(HIRValue *Destination, const char *SysVar) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_SYS_QUERY);

    Instruction -> Destination = Destination;
    Instruction -> EnvironmentName = HIRStrDup(SysVar);

    return Instruction;
}

HIRInstruction *HIRInstNaoBlock(const char *Source, size_t Length) {
    HIRInstruction *Instruction = HIRAllocInst(HIR_NAO_BLOCK);

    Instruction -> NaoSource = HIRStrDup(Source);
    Instruction -> NaoLength = Length;

    return Instruction;
}

static HIRPattern *HIRAllocPattern(HIRPatternKind Kind) {
    HIRPattern *Pattern = calloc(1, sizeof(HIRPattern));

    Pattern -> Kind = Kind;

    return Pattern;
}

HIRPattern *HIRPatternLiteral(HIRValue *Literal) {
    HIRPattern *Pattern = HIRAllocPattern(HIR_PAT_LITERAL);

    Pattern -> Literal = Literal;

    return Pattern;
}

HIRPattern *HIRPatternVariable(const char *Name) {
    HIRPattern *Pattern = HIRAllocPattern(HIR_PAT_VARIABLE);

    Pattern -> BindName = HIRStrDup(Name);

    return Pattern;
}

HIRPattern *HIRPatternTuple(HIRPattern **Elements, size_t Count) {
    HIRPattern *Pattern = HIRAllocPattern(HIR_PAT_TUPLE);

    Pattern -> ElementCount = Count;

    if (Count > 0) {
        Pattern -> Elements = malloc(Count * sizeof(HIRPattern *));

        memcpy(Pattern -> Elements, Elements, Count * sizeof(HIRPattern *));
    }

    return Pattern;
}

HIRPattern *HIRPatternEnum(const char *Variant) {
    HIRPattern *Pattern = HIRAllocPattern(HIR_PAT_ENUM);

    Pattern -> EnumVariant = HIRStrDup(Variant);

    return Pattern;
}

HIRPattern *HIRPatternGuard(HIRPattern *Inner, HIRValue *Guard) {
    HIRPattern *Pattern = HIRAllocPattern(HIR_PAT_GUARD);

    Pattern -> Inner = Inner;
    Pattern -> Guard = Guard;

    return Pattern;
}

HIRPattern *HIRPatternWildcard(void) {
    return HIRAllocPattern(HIR_PAT_WILDCARD);
}

HIRPattern *HIRPatternNone(void) {
    return HIRAllocPattern(HIR_PAT_NONE);
}

const char *HIROpcodeString(HIROpcode Op) {
    switch (Op) {
        case HIR_NOP: return "NOP";
        case HIR_CONST: return "CONST";
        case HIR_MOVE: return "MOVE";
        case HIR_COPY: return "COPY";
        case HIR_OWN_OP: return "OWN";
        case HIR_FREE: return "FREE";
        case HIR_DROP: return "DROP";
        case HIR_BORROW_BEGIN: return "BORROW_BEGIN";
        case HIR_BORROW_END: return "BORROW_END";
        case HIR_TRAIL_LINK: return "TRAIL_LINK";
        case HIR_TRAIL_BREAK: return "TRAIL_BREAK";
        case HIR_ADD: return "ADD";
        case HIR_SUB: return "SUB";
        case HIR_MUL: return "MUL";
        case HIR_DIV: return "DIV";
        case HIR_MOD: return "MOD";
        case HIR_NEG: return "NEG";
        case HIR_EQ: return "EQ";
        case HIR_NE: return "NE";
        case HIR_LT: return "LT";
        case HIR_LE: return "LE";
        case HIR_GT: return "GT";
        case HIR_GE: return "GE";
        case HIR_AND: return "AND";
        case HIR_OR: return "OR";
        case HIR_NOT: return "NOT";
        case HIR_LOAD: return "LOAD";
        case HIR_STORE: return "STORE";
        case HIR_ADDR: return "ADDR";
        case HIR_ARRAY_ALLOC: return "ARRAY_ALLOC";
        case HIR_ARRAY_INDEX: return "ARRAY_INDEX";
        case HIR_ARRAY_LEN: return "ARRAY_LEN";
        case HIR_SLICE_ALLOC: return "SLICE_ALLOC";
        case HIR_SLICE_FROM: return "SLICE_FROM";
        case HIR_STRUCT_INIT: return "STRUCT_INIT";
        case HIR_STRUCT_FIELD: return "STRUCT_FIELD";
        case HIR_STRUCT_SET: return "STRUCT_SET";
        case HIR_PARTIAL_INIT: return "PARTIAL_INIT";
        case HIR_PARTIAL_FIELD: return "PARTIAL_FIELD";
        case HIR_PARTIAL_CHECK: return "PARTIAL_CHECK";
        case HIR_LABEL: return "LABEL";
        case HIR_JMP: return "JMP";
        case HIR_JMP_IF: return "JMP_IF";
        case HIR_JMP_IF_FALSE: return "JMP_IF_FALSE";
        case HIR_MATCH_BEGIN: return "MATCH_BEGIN";
        case HIR_MATCH_ARM_TEST: return "MATCH_ARM_TEST";
        case HIR_MATCH_BIND: return "MATCH_BIND";
        case HIR_MATCH_GUARD: return "MATCH_GUARD";
        case HIR_MATCH_END: return "MATCH_END";
        case HIR_MATCH_MISSING: return "MATCH_MISSING";
        case HIR_CALL: return "CALL";
        case HIR_RETURN: return "RETURN";
        case HIR_ENV_PROVIDE: return "ENV_PROVIDE";
        case HIR_ENV_REQUIRE: return "ENV_REQUIRE";
        case HIR_ENV_GET: return "ENV_GET";
        case HIR_ENV_SET: return "ENV_SET";
        case HIR_UNSAFE_ENTER: return "UNSAFE_ENTER";
        case HIR_UNSAFE_EXIT: return "UNSAFE_EXIT";
        case HIR_SAFE_ENTER: return "SAFE_ENTER";
        case HIR_SAFE_EXIT: return "SAFE_EXIT";
        case HIR_TRUSTED_ENTER: return "TRUSTED_ENTER";
        case HIR_TRUSTED_EXIT: return "TRUSTED_EXIT";
        case HIR_CHECK: return "CHECK";
        case HIR_ASSUME: return "ASSUME";
        case HIR_DEFER_PUSH: return "DEFER_PUSH";
        case HIR_DEFER_RUN: return "DEFER_RUN";
        case HIR_TRANSACTION_BEGIN: return "TRANSACTION_BEGIN";
        case HIR_TRANSACTION_ROLLBACK: return "TRANSACTION_ROLLBACK";
        case HIR_TRANSACTION_COMMIT: return "TRANSACTION_COMMIT";
        case HIR_REGION_ENTER: return "REGION_ENTER";
        case HIR_REGION_EXIT: return "REGION_EXIT";
        case HIR_HISTORY_SNAP: return "HISTORY_SNAP";
        case HIR_HISTORY_READ: return "HISTORY_READ";
        case HIR_REACTIVE_BIND: return "REACTIVE_BIND";
        case HIR_REACTIVE_UPDATE: return "REACTIVE_UPDATE";
        case HIR_COMPTIME_BEGIN: return "COMPTIME_BEGIN";
        case HIR_COMPTIME_END: return "COMPTIME_END";
        case HIR_COMPTIME_RESULT: return "COMPTIME_RESULT";
        case HIR_MACRO_SITE: return "MACRO_SITE";
        case HIR_MACRO_EXPOSE: return "MACRO_EXPOSE";
        case HIR_CONTEXT_ENTER: return "CONTEXT_ENTER";
        case HIR_CONTEXT_EXIT: return "CONTEXT_EXIT";
        case HIR_WORLD_ENTER: return "WORLD_ENTER";
        case HIR_WORLD_EXIT: return "WORLD_EXIT";
        case HIR_SYS_QUERY: return "SYS_QUERY";
        case HIR_ENUM_VARIANT: return "ENUM_VARIANT";
        case HIR_STATE_TRANS: return "STATE_TRANS";
        case HIR_NAO_BLOCK: return "NAO_BLOCK";

        default: return "<unknown>";
    }
}

const char *HIROwnKindString(HIROwnKind Kind) {
    switch (Kind) {
        case HIR_OWN_MOVE: return "move";
        case HIR_OWN_BORROW: return "borrow(&)";
        case HIR_OWN_COPY: return "copy(@)";
        case HIR_OWN_TRAIL: return "trail(..)";
        case HIR_OWN_ADDRESS: return "address(#)";
        case HIR_OWN_FREE: return "free($)";
        case HIR_OWN_MUT_FREE: return "mut_free(!$)";
        case HIR_OWN_IMPLICIT: return "implicit";

        default: return "<unknown>";
    }
}

const char *HIRSafetyString(HIRSafetyLevel Level) {
    switch (Level) {
        case HIR_SAFETY_SAFE: return "safe";
        case HIR_SAFETY_UNSAFE: return "unsafe";
        case HIR_SAFETY_TRUSTED: return "trusted";

        default: return "<unknown>";
    }
}

const char *HIRModifiersString(uint32_t Modifiers) {
    static char Buffer[256];

    Buffer[0] = '\0';

    if (Modifiers & HIR_MOD_GLOBAL) strcat(Buffer, "global ");
    if (Modifiers & HIR_MOD_STATIC) strcat(Buffer, "static ");
    if (Modifiers & HIR_MOD_CONST) strcat(Buffer, "const ");
    if (Modifiers & HIR_MOD_EXPORT) strcat(Buffer, "export ");
    if (Modifiers & HIR_MOD_PRIVATE) strcat(Buffer, "private ");
    if (Modifiers & HIR_MOD_SILENT) strcat(Buffer, "silent ");
    if (Modifiers & HIR_MOD_LINEAR) strcat(Buffer, "linear ");
    if (Modifiers & HIR_MOD_HISTORY) strcat(Buffer, "history ");
    if (Modifiers & HIR_MOD_SYMBOLIC) strcat(Buffer, "symbolic ");
    if (Modifiers & HIR_MOD_NOALIAS) strcat(Buffer, "noalias ");
    if (Modifiers & HIR_MOD_REACTIVE) strcat(Buffer, "reactive ");
    if (Modifiers & HIR_MOD_COMPTIME) strcat(Buffer, "comptime ");

    if (Buffer[0] == '\0') 
        strcat(Buffer, "none");

    return Buffer;
}

static void PrintIndent(int Depth) {
    for (int i = 0; i < Depth * 2; i++) fputc(' ', stdout);
}

void HIRPrintType(const HIRType *Type) {
    if (!Type) {
        printf("(null)");

        return;
    }

    switch (Type -> Kind) {
        case HIR_TYPE_INT: printf("int"); break;
        case HIR_TYPE_FLOAT: printf("float"); break;
        case HIR_TYPE_BOOL: printf("bool"); break;
        case HIR_TYPE_CHAR: printf("char"); break;
        case HIR_TYPE_STRING: printf("string"); break;
        case HIR_TYPE_VOID: printf("void"); break;
        case HIR_TYPE_PTR:
            printf("*");
            HIRPrintType(Type -> ElementType);

            break;
            
        case HIR_TYPE_ARRAY:
            printf("[");
            HIRPrintType(Type -> ElementType);
            printf(", %zu]", Type -> ArraySize);

            break;
            
        case HIR_TYPE_SLICE:
            printf("[");
            HIRPrintType(Type -> ElementType);
            printf("]");

            break;

        case HIR_TYPE_NAMED:
        case HIR_TYPE_PARTIAL:
        case HIR_TYPE_ENV:
        case HIR_TYPE_FILE:
        case HIR_TYPE_ENUM:
        case HIR_TYPE_STATE:
            printf("%s", Type -> Name ? Type -> Name : "<anonymous>");

            break;

        default:
            printf("<type?>");

            break;
    }
}

void HIRPrintValue(const HIRValue *Value) {
    if (!Value) {
        printf("(null)");
        
        return;
    }

    switch (Value -> Kind) {
        case HIR_VAL_TEMP:
            printf("%%t%d", Value -> TempID);

            break;
        case HIR_VAL_VAR:
        case HIR_VAL_PARAM:
            printf("%%%s", Value -> Name ? Value -> Name : "?");

            break;
        case HIR_VAL_CONST:
            if (Value -> Type) {
                switch (Value -> Type -> Kind) {
                    case HIR_TYPE_INT: printf("%lld", (long long) Value -> IntValue); break;
                    case HIR_TYPE_FLOAT: printf("%g", Value -> FloatValue); break;
                    case HIR_TYPE_BOOL: printf("%s", Value -> BoolValue ? "true" : "false"); break;
                    case HIR_TYPE_CHAR: printf("'%c'", Value -> CharValue); break;
                    case HIR_TYPE_STRING: printf("\"%s\"", Value -> StringValue ? Value -> StringValue : ""); break;

                    default:              printf("<const>");                              break;
                }
            } else {
                printf("<const:%lld>", (long long)Value -> IntValue);
            }

            break;
        case HIR_VAL_LABEL:
            printf("@%s", Value -> Label ? Value -> Label : "?");

            break;
        case HIR_VAL_ENV:
            printf("env(%s)", Value -> Name ? Value -> Name : "?");

            break;
        default:
            printf("<value?>");

            break;
    }

    if (Value -> Type) {
        printf(":");
        HIRPrintType(Value -> Type);
    }
}

void HIRPrintInstruction(const HIRInstruction *Instruction) {
    if (!Instruction)
        return;

    PrintIndent(2);
    printf("%-20s", HIROpcodeString(Instruction -> Op));

    if (Instruction -> Destination) {
        printf("  ");
        HIRPrintValue(Instruction -> Destination);
        printf(" =");
    }

    if (Instruction -> Source1) {
        printf("  ");
        HIRPrintValue(Instruction -> Source1);
    }

    if (Instruction -> Source2) {
        printf(",  ");
        HIRPrintValue(Instruction -> Source2);
    }

    if (Instruction -> CallTarget) {
        printf("  @%s(", Instruction -> CallTarget);

        for (size_t i = 0; i < Instruction -> ArgCount; i++) {
            if (i > 0)
                printf(", ");

            HIRPrintValue(Instruction -> Args[i]);
        }

        printf(")");
    }

    if (Instruction -> ScopeName && (Instruction -> Op == HIR_JMP     || Instruction -> Op == HIR_JMP_IF  || Instruction -> Op == HIR_JMP_IF_FALSE || Instruction -> Op == HIR_REGION_ENTER || Instruction -> Op == HIR_REGION_EXIT  || Instruction -> Op == HIR_CONTEXT_ENTER|| Instruction -> Op == HIR_CONTEXT_EXIT || Instruction -> Op == HIR_WORLD_ENTER  || Instruction -> Op == HIR_WORLD_EXIT)) {
        printf("  -> %s", Instruction -> ScopeName);
    }

    if (Instruction -> EnvironmentName) {
        printf("  env=%s", Instruction -> EnvironmentName);

        if (Instruction -> EnvironmentField) {
            printf(".%s", Instruction -> EnvironmentField);
        }
    }

    if (Instruction -> MacroName) {
        printf("  macro=%s", Instruction -> MacroName);
    }

    if (Instruction -> Op == HIR_MATCH_ARM_TEST && Instruction -> ArmLabel) {
        printf("  arm=%s  next=%s", Instruction -> ArmLabel, Instruction -> NextLabel ? Instruction -> NextLabel : "?");
    }

    if (Instruction -> Op == HIR_HISTORY_READ) {
        printf("  @%d", Instruction -> HistoryIndex);

        if (Instruction -> HistoryBranchIndex >= 0) {
            printf("b%d", Instruction -> HistoryBranchPath);
        }
    }

    if (Instruction -> Op == HIR_CHECK || Instruction -> Op == HIR_ASSUME) {
        printf("  [%s]", Instruction -> IsRuntime ? "runtime" : "comptime");
    }

    if (Instruction -> Safety != HIR_SAFETY_SAFE) {
        printf("  {%s}", HIRSafetyString(Instruction -> Safety));
    }

    printf("\n");
}

void HIRPrintBlock(const HIRBlock *Block) {
    if (!Block)
        return;

    PrintIndent(1);
    printf("%s:  [safety=%s]", Block -> Label ? Block -> Label : "<?>", HIRSafetyString(Block -> Safety));

    if (Block -> IsDeferred) printf(" [deferred]");
    if (Block -> IsMatchArm) printf(" [match-arm]");
    if (Block -> IsTransactionBody) printf(" [transaction]");
    if (Block -> IsRegionBody) printf(" [region]");
    if (Block -> IsReactiveBody) printf(" [reactive]");
    if (Block -> IsComptimeBlock) printf(" [comptime]");
    if (Block -> IsWorldBody) printf(" [world]");

    printf("\n");

    for (size_t i = 0; i < Block -> InstructionCount; i++) {
        HIRPrintInstruction(Block -> Instructions[i]);
    }

    if (Block -> SuccessorCount > 0) {
        PrintIndent(2);
        printf("(successors:");

        for (size_t i = 0; i < Block -> SuccessorCount; i++) {
            printf(" %s", Block -> Successors[i]->Label ? Block -> Successors[i]->Label : "?");
        }

        printf(")\n");
    }
}

void HIRPrintFunction(const HIRFunction *Function) {
    if (!Function)
        return;

    printf("fn %s(", Function -> Name ? Function -> Name : "?");

    for (size_t i = 0; i < Function -> ParamCount; i++) {
        if (i > 0)
            printf(", ");

        HIRPrintValue(Function -> Params[i]);
    }

    printf(") -> ");
    HIRPrintType(Function -> ReturnType);

    if (Function -> Safety != HIR_SAFETY_SAFE) {
        printf("  [%s]", HIRSafetyString(Function -> Safety));
    }

    if (Function -> IsEntryPoint) printf("  [entry]");
    if (Function -> IsWorldEntry) printf("  [world-entry]");
    if (Function -> IsComptimeFn) printf("  [comptime]");
    if (Function -> IsMacroBody) printf("  [macro]");
    if (Function -> WorldName)  printf("  [world=%s]", Function -> WorldName);
    if (Function -> ContextName)  printf("  [context=%s]", Function -> ContextName);

    if (Function -> RequireCount > 0) {
        printf("  requires(");

        for (size_t i = 0; i < Function -> RequireCount; i++) {
            if (i > 0) printf(", ");

            printf("%s", Function -> Requires[i]->Name ? Function -> Requires[i]->Name : "?");
        }

        printf(")");
    }

    if (Function -> ProvideCount > 0) {
        printf("  provides(");

        for (size_t i = 0; i < Function -> ProvideCount; i++) {
            if (i > 0)
                printf(", ");

            printf("%s", Function -> Provides[i]->Name ? Function -> Provides[i]->Name : "?");
        }

        printf(")");
    }

    printf(" {\n");

    for (size_t i = 0; i < Function -> BlockCount; i++) {
        HIRPrintBlock(Function -> Blocks[i]);
    }

    printf("}\n\n");
}

void HIRPrint(const HIRProgram *Program) {
    if (!Program)
        return;

    printf("Globals: %zu  |  Functions: %zu  |  Worlds: %zu  |  Contexts: %zu\n\n", Program -> GlobalCount, Program -> FunctionCount, Program -> WorldCount, Program -> ContextCount);

    if (Program -> GlobalCount > 0) {
        printf("--- Globals ---\n");

        for (size_t i = 0; i < Program -> GlobalCount; i++) {
            printf("  global ");

            HIRPrintValue(Program -> Globals[i]);

            printf("\n");
        }

        printf("\n");
    }

    for (size_t i = 0; i < Program -> FunctionCount; i++) {
        HIRPrintFunction(Program -> Functions[i]);
    }
}
