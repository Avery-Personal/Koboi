#ifndef HIR_H
#define HIR_H

    #include <stdint.h>
    #include <stddef.h>

    typedef enum {
        HIR_TYPE_INT,
        HIR_TYPE_FLOAT,
        HIR_TYPE_BOOL,
        HIR_TYPE_CHAR,
        HIR_TYPE_STRING,
        HIR_TYPE_VOID,
        HIR_TYPE_PTR,
        HIR_TYPE_ARRAY,
        HIR_TYPE_SLICE,
        HIR_TYPE_NAMED,
        HIR_TYPE_PARTIAL,
        HIR_TYPE_ENV,
        HIR_TYPE_FILE,
        HIR_TYPE_ENUM,
        HIR_TYPE_STATE,
    } HIRTypeKind;

    typedef struct HIRType {
        const char *Name;

        HIRTypeKind Kind;
        struct HIRType *ElementType;

        size_t ArraySize;

        int IsSlice;
        int IsOptional;
        int IsNoAlias;
    } HIRType;

    typedef enum {
        HIR_OWN_MOVE,
        HIR_OWN_BORROW,
        HIR_OWN_COPY,
        HIR_OWN_TRAIL,
        HIR_OWN_ADDRESS,
        HIR_OWN_FREE,
        HIR_OWN_MUT_FREE,
        HIR_OWN_IMPLICIT,
    } HIROwnKind;

    typedef enum {
        HIR_MOD_NONE = 0,
        HIR_MOD_GLOBAL = 1 << 0,
        HIR_MOD_STATIC = 1 << 1,
        HIR_MOD_CONST = 1 << 2,
        HIR_MOD_EXPORT = 1 << 3,
        HIR_MOD_PRIVATE = 1 << 4,
        HIR_MOD_SILENT = 1 << 5,
        HIR_MOD_LINEAR = 1 << 6,
        HIR_MOD_HISTORY = 1 << 7,
        HIR_MOD_SYMBOLIC = 1 << 8,
        HIR_MOD_NOALIAS = 1 << 9,
        HIR_MOD_REACTIVE = 1 << 10,
        HIR_MOD_COMPTIME = 1 << 11,
    } HIRModifiers;

    typedef enum {
        HIR_SAFETY_SAFE,
        HIR_SAFETY_UNSAFE,
        HIR_SAFETY_TRUSTED,
    } HIRSafetyLevel;

    typedef struct {
        const char *FieldName;
        HIRType *FieldType;
    } HIREnvField;

    typedef struct {
        const char *Name;
        HIRType *DataType;

        int HasInitValue;

        union {
            int64_t IntValue;
            double FloatValue;
            const char *StringValue;
        } InitValue;

        HIREnvField *Fields;
        size_t FieldCount;
    } HIREnvironment;

    typedef enum {
        HIR_VAL_TEMP,
        HIR_VAL_VAR,
        HIR_VAL_PARAM,
        HIR_VAL_CONST,
        HIR_VAL_LABEL,
        HIR_VAL_ENV,
        HIR_VAL_HISTORY,
        HIR_VAL_PARTIAL,
        HIR_VAL_ENUM_VAR,
        HIR_VAL_STATE,
    } HIRValueKind;

    typedef struct HIRValue {
        HIRValueKind Kind;
        HIRType *Type;
        uint32_t Modifiers;
        
        union {
            const char *Name;
            const char *Label;

            int64_t IntValue;
            double FloatValue;
            char CharValue;
            int BoolValue;
            const char *StringValue;
        };

        int TempID;
        
        struct {
            int IsHistoryAccess;
            int SnapshotIndex;
            int HasBranch;
            int BranchIndex;
            int BranchPath;
        } History;

        struct {
            HIROwnKind Kind;
            struct HIRValue *Owner;

            int IsBorrowed;
            int IsMoved;
            int IsFreed;
            int IsMutFree;
        } Ownership;

        int HasBeenUsed;
        int ScopeID;

        struct HIRValue **Dependents;
        size_t DependentCount;
    } HIRValue;

    typedef enum {
        HIR_PAT_LITERAL,
        HIR_PAT_VARIABLE,
        HIR_PAT_TUPLE,
        HIR_PAT_ENUM,
        HIR_PAT_GUARD,
        HIR_PAT_WILDCARD,
        HIR_PAT_NONE,
    } HIRPatternKind;

    typedef struct HIRPattern {
        HIRPatternKind  Kind;

        HIRValue *Literal;
        const char *BindName;
        struct HIRPattern **Elements;
        size_t ElementCount;
        const char *EnumVariant;

        struct HIRPattern *Inner;
        HIRValue *Guard;
    } HIRPattern;

    typedef enum {
        HIR_NOP,
        HIR_CONST,
        HIR_MOVE,
        HIR_COPY,

        HIR_OWN_OP,
        HIR_FREE,
        HIR_DROP,
        HIR_BORROW_BEGIN,
        HIR_BORROW_END,
        HIR_TRAIL_LINK,
        HIR_TRAIL_BREAK,

        HIR_ADD,
        HIR_SUB,
        HIR_MUL,
        HIR_DIV,
        HIR_MOD,
        HIR_NEG,

        HIR_EQ,
        HIR_NE,
        HIR_LT,
        HIR_LE,
        HIR_GT,
        HIR_GE,

        HIR_AND,
        HIR_OR,
        HIR_NOT,

        HIR_LOAD,
        HIR_STORE,
        HIR_ADDR,

        HIR_ARRAY_ALLOC,
        HIR_ARRAY_INDEX,
        HIR_ARRAY_LEN,
        HIR_SLICE_ALLOC,
        HIR_SLICE_FROM,


        HIR_STRUCT_INIT,
        HIR_STRUCT_FIELD,
        HIR_STRUCT_SET,
        HIR_PARTIAL_INIT,
        HIR_PARTIAL_FIELD,
        HIR_PARTIAL_CHECK,

        HIR_LABEL,
        HIR_JMP,
        HIR_JMP_IF,
        HIR_JMP_IF_FALSE,

        HIR_MATCH_BEGIN,
        HIR_MATCH_ARM_TEST,
        HIR_MATCH_BIND,
        HIR_MATCH_GUARD,
        HIR_MATCH_END,
        HIR_MATCH_MISSING,

        HIR_CALL,
        HIR_RETURN,

        HIR_ENV_PROVIDE,
        HIR_ENV_REQUIRE,
        HIR_ENV_GET,
        HIR_ENV_SET,

        HIR_UNSAFE_ENTER,
        HIR_UNSAFE_EXIT,
        HIR_SAFE_ENTER,
        HIR_SAFE_EXIT,
        HIR_TRUSTED_ENTER,
        HIR_TRUSTED_EXIT,

        HIR_CHECK,
        HIR_ASSUME,
        HIR_DEFER_PUSH,
        HIR_DEFER_RUN,

        HIR_TRANSACTION_BEGIN,
        HIR_TRANSACTION_ROLLBACK,
        HIR_TRANSACTION_COMMIT,

        HIR_REGION_ENTER,
        HIR_REGION_EXIT,

        HIR_HISTORY_SNAP,
        HIR_HISTORY_READ,

        HIR_REACTIVE_BIND,
        HIR_REACTIVE_UPDATE,

        HIR_COMPTIME_BEGIN,
        HIR_COMPTIME_END,
        HIR_COMPTIME_RESULT,

        HIR_MACRO_SITE,
        HIR_MACRO_EXPOSE,

        HIR_CONTEXT_ENTER,
        HIR_CONTEXT_EXIT,

        HIR_WORLD_ENTER,
        HIR_WORLD_EXIT,

        HIR_SYS_QUERY,

        HIR_ENUM_VARIANT,
        HIR_STATE_TRANS,

        HIR_NAO_BLOCK,

    } HIROpcode;

    typedef struct HIRInstruction {
        HIROpcode Op;

        HIRValue *Destination;
        HIRValue *Source1;
        HIRValue *Source2;

        const char *CallTarget;
        HIRValue **Args;
        size_t ArgCount;

        HIROwnKind OwnKind;

        HIRPattern  *Pattern;
        const char  *ArmLabel;
        const char  *NextLabel;
        
        const char *EnvironmentName;
        const char *EnvironmentField;

        const char *ScopeName;

        int IsRuntime;
    
        int HistoryIndex;
        int HistoryBranchIndex;
        int HistoryBranchPath;

        struct HIRInstruction **DeferBody;
        size_t DeferBodyCount;

        int IsComptimeEvaluation;

        const char *MacroName;

        const char *NaoSource;
        size_t NaoLength;

        HIRSafetyLevel Safety;

        uint32_t Line;
        uint32_t Column;
    } HIRInstruction;

    typedef struct HIRBlock {
        const char *Label;

        HIRInstruction **Instructions;
        size_t InstructionCount;
        size_t InstructionCapacity;

        struct HIRBlock **Successors;
        size_t SuccessorCount;

        struct HIRBlock **Predecessors;
        size_t PredecessorCount;

        HIRSafetyLevel Safety;

        int IsDeferred;
        int IsComptimeBlock;
        int IsMatchArm;
        int IsTransactionBody;
        int IsRegionBody;
        int IsReactiveBody;
        int IsWorldBody;
    } HIRBlock;

    typedef struct HIRFunction {
        const char *Name;
        HIRType *ReturnType;

        HIRValue **Params;
        size_t ParamCount;
        size_t ParamCapacity;

        HIRBlock **Blocks;
        size_t BlockCount;
        size_t BlockCapacity;

        HIRValue **Locals;
        size_t LocalCount;
        size_t LocalCapacity;

        HIREnvironment **Requires;
        size_t RequireCount;

        HIREnvironment **Provides;
        size_t ProvideCount;

        uint32_t Modifiers;
        HIRSafetyLevel Safety;

        int IsComptimeFn;
        int IsMacroBody;
        int IsEntryPoint;
        int IsWorldEntry;

        const char *WorldName;
        const char *ContextName;
    } HIRFunction;

    typedef struct HIRWorld {
        const char *Name;
        const char *ExtendsName;

        HIRFunction **Functions;
        size_t FunctionCount;

        HIRValue **Globals;
        size_t GlobalCount;

        struct HIRWorld *Parent;
    } HIRWorld;

    typedef struct HIRContext {
        const char *Name;

        HIRFunction **Functions;
        size_t FunctionCount;

        HIRValue **Variables;
        size_t VariableCount;
    } HIRContext;

    typedef struct HIRModule {
        const char *Name;

        HIRFunction **Functions;
        size_t FunctionCount;

        HIRValue **Exports;
        size_t ExportCount;

        HIRValue **Statics;
        size_t StaticCount;
    } HIRModule;

    typedef struct {
        HIRFunction **Functions;
        size_t FunctionCount;
        size_t FunctionCapacity;

        HIRValue **Globals;
        size_t GlobalCount;
        size_t GlobalCapacity;

        HIREnvironment **Environments;
        size_t EnvCount;

        HIRWorld **Worlds;
        size_t WorldCount;

        HIRContext **Contexts;
        size_t ContextCount;

        HIRModule **Modules;
        size_t ModuleCount;

        const char *ActiveContext;

        int ForcedSafeMode;
    } HIRProgram;

    HIRProgram *HIRCreateProgram(void);
    void HIRDestroyProgram(HIRProgram *Program);

    HIRFunction *HIRCreateFunction(const char *Name, HIRType *ReturnType);
    HIRBlock *HIRCreateBlock(const char *Label);
    void HIRAddBlock(HIRFunction *Function, HIRBlock *Block);
    void HIRAddFunction(HIRProgram *Program, HIRFunction *Function);
    void HIRAddGlobal(HIRProgram *Program, HIRValue *G);
    void HIRAddWorld(HIRProgram *Program, HIRWorld *W);
    void HIRAddContext(HIRProgram *Program, HIRContext *C);
    void HIRAddModule(HIRProgram *Program, HIRModule *M);
    void HIRAddEnv(HIRProgram *Program, HIREnvironment *E);
    void HIRFunctionAddRequires(HIRFunction *Function, HIREnvironment *E);
    void HIRFunctionAddProvides(HIRFunction *Function, HIREnvironment *E);

    HIRValue *HIRCreateTemp(HIRType *Type);
    HIRValue *HIRCreateVar(const char *Name, HIRType *Type, uint32_t Mods);
    HIRValue *HIRCreateParam(const char *Name, HIRType *Type);
    HIRValue *HIRCreateIntConst(int64_t Value, HIRType *Type);
    HIRValue *HIRCreateFloatConst(double Value);
    HIRValue *HIRCreateBoolConst(int Value);
    HIRValue *HIRCreateStringConst(const char *Value);
    HIRValue *HIRCreateCharConst(char Value);
    HIRValue *HIRCreateLabelVal(const char *Label);
    HIRValue *HIRCreateEnvHandle(const char *Name);

    HIRType *HIRMakeIntType(void);
    HIRType *HIRMakeFloatType(void);
    HIRType *HIRMakeBoolType(void);
    HIRType *HIRMakeCharType(void);
    HIRType *HIRMakeStringType(void);
    HIRType *HIRMakeVoidType(void);
    HIRType *HIRMakePtrType(HIRType *Element);
    HIRType *HIRMakeArrayType(HIRType *Element, size_t Size);
    HIRType *HIRMakeSliceType(HIRType *Element);
    HIRType *HIRMakeNamedType(const char *Name);
    HIRType *HIRMakePartialType(const char *Name);
    HIRType *HIRMakeEnvType(const char *Name);

    void HIRAddInstruction(HIRBlock *Block, HIRInstruction *Instruction);

    HIRInstruction *HIRInstNop(void);
    HIRInstruction *HIRInstConst(HIRValue *Destination, HIRValue *Literal);
    HIRInstruction *HIRInstMove(HIRValue *Destination, HIRValue *Source);
    HIRInstruction *HIRInstCopy(HIRValue *Destination, HIRValue *Source);
    HIRInstruction *HIRInstOwn(HIROwnKind Kind, HIRValue *Destination, HIRValue *Source);
    HIRInstruction *HIRInstFree(HIRValue *Source);
    HIRInstruction *HIRInstDrop(HIRValue *Source);
    HIRInstruction *HIRInstBorrowBegin(HIRValue *Target);
    HIRInstruction *HIRInstBorrowEnd(HIRValue *Target);
    HIRInstruction *HIRInstTrailLink(HIRValue *Destination, HIRValue *Source);
    HIRInstruction *HIRInstTrailBreak(HIRValue *Source);
    HIRInstruction *HIRInstBinary(HIROpcode Op, HIRValue *Destination, HIRValue *L, HIRValue *R);
    HIRInstruction *HIRInstUnary(HIROpcode Op, HIRValue *Destination, HIRValue *Source);
    HIRInstruction *HIRInstLoad(HIRValue *Destination, HIRValue *Address);
    HIRInstruction *HIRInstStore(HIRValue *Address, HIRValue *Value);
    HIRInstruction *HIRInstAddr(HIRValue *Destination, HIRValue *Variable);
    HIRInstruction *HIRInstArrayAlloc(HIRValue *Destination, HIRType *Element, HIRValue *Size);
    HIRInstruction *HIRInstArrayIndex(HIRValue *Destination, HIRValue *Array, HIRValue *Index);
    HIRInstruction *HIRInstArrayLen(HIRValue *Destination, HIRValue *Array);
    HIRInstruction *HIRInstStructField(HIRValue *Destination, HIRValue *Source, const char *Field);
    HIRInstruction *HIRInstStructSet(HIRValue *Destination, const char *Field, HIRValue *Value);
    HIRInstruction *HIRInstPartialField(HIRValue *Destination, HIRValue *Source, const char *Field);
    HIRInstruction *HIRInstPartialCheck(HIRValue *Destination, HIRValue *Source, const char *Field);
    HIRInstruction *HIRInstJmp(const char *Label);
    HIRInstruction *HIRInstJmpIf(HIRValue *Condition, const char *Label);
    HIRInstruction *HIRInstJmpIfFalse(HIRValue *Condition, const char *Label);
    HIRInstruction *HIRInstMatchBegin(HIRValue *Scrutinee);
    HIRInstruction *HIRInstMatchArmTest(HIRPattern *Pattern, const char *ArmLabel, const char *NextLabel);
    HIRInstruction *HIRInstMatchBind(const char *Name, HIRValue *Source);
    HIRInstruction *HIRInstMatchGuard(HIRValue *Condition, const char *NextLabel);
    HIRInstruction *HIRInstMatchEnd(void);
    HIRInstruction *HIRInstCall(HIRValue *Destination, const char *Target, HIRValue **Args, size_t Size);
    HIRInstruction *HIRInstReturn(HIRValue *Value);
    HIRInstruction *HIRInstEnvProvide(const char *Name, HIRValue *Value);
    HIRInstruction *HIRInstEnvRequire(const char *Name);
    HIRInstruction *HIRInstEnvGet(HIRValue *Destination, const char *Environment, const char *Field);
    HIRInstruction *HIRInstEnvSet(const char *Environment, const char *Field, HIRValue *Value);
    HIRInstruction *HIRInstUnsafeEnter(HIRSafetyLevel Level);
    HIRInstruction *HIRInstUnsafeExit(HIRSafetyLevel Level);
    HIRInstruction *HIRInstCheck(HIRValue *Condition, int IsRuntime);
    HIRInstruction *HIRInstAssume(HIRValue *Condition);
    HIRInstruction *HIRInstDeferPush(HIRInstruction **Body, size_t Size);
    HIRInstruction *HIRInstDeferRun(void);
    HIRInstruction *HIRInstTransactionBegin(void);
    HIRInstruction *HIRInstTransactionRollback(void);
    HIRInstruction *HIRInstTransactionCommit(void);
    HIRInstruction *HIRInstRegionEnter(const char *Name);
    HIRInstruction *HIRInstRegionExit(const char *Name);
    HIRInstruction *HIRInstHistorySnap(HIRValue *Variable);
    HIRInstruction *HIRInstHistoryRead(HIRValue *Destination, HIRValue *Variable, int Index, int BranchIdx, int BranchPath);
    HIRInstruction *HIRInstReactiveBind(HIRValue *Destination, HIRValue *Expression);
    HIRInstruction *HIRInstReactiveUpdate(HIRValue *Changed);
    HIRInstruction *HIRInstComptimeBegin(void);
    HIRInstruction *HIRInstComptimeEnd(void);
    HIRInstruction *HIRInstComptimeResult(HIRValue *Destination, HIRValue *Literal);
    HIRInstruction *HIRInstMacroSite(const char *MacroName);
    HIRInstruction *HIRInstMacroExpose(HIRValue *Destination, HIRValue *Source);
    HIRInstruction *HIRInstContextEnter(const char *Name);
    HIRInstruction *HIRInstContextExit(const char *Name);
    HIRInstruction *HIRInstWorldEnter(const char *Name);
    HIRInstruction *HIRInstWorldExit(const char *Name);
    HIRInstruction *HIRInstSysQuery(HIRValue *Destination, const char *SysVar);
    HIRInstruction *HIRInstNaoBlock(const char *Source, size_t Len);

    HIRPattern *HIRPatternLiteral(HIRValue *Lit);
    HIRPattern *HIRPatternVariable(const char *Name);
    HIRPattern *HIRPatternTuple(HIRPattern **Elements, size_t Size);
    HIRPattern *HIRPatternEnum(const char *Variant);
    HIRPattern *HIRPatternGuard(HIRPattern *Inner, HIRValue *Guard);
    HIRPattern *HIRPatternWildcard(void);
    HIRPattern *HIRPatternNone(void);

    void HIRPrint(const HIRProgram *Program);
    void HIRPrintFunction(const HIRFunction *Function);
    void HIRPrintBlock(const HIRBlock *Block);
    void HIRPrintInstruction(const HIRInstruction *Instruction);
    void HIRPrintValue(const HIRValue *Value);
    void HIRPrintType(const HIRType *Type);

    const char *HIROpcodeString(HIROpcode Op);
    const char *HIROwnKindString(HIROwnKind Kind);
    const char *HIRSafetyString(HIRSafetyLevel Level);
    const char *HIRModifiersString(uint32_t Mods);

#endif
