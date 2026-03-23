#ifndef MIR_H
#define MIR_H

    #include <stdint.h>
    #include <stddef.h>

    typedef enum {
        MIR_TYPE_INT,
        MIR_TYPE_FLOAT,
        MIR_TYPE_BOOL,
        MIR_TYPE_CHAR,
        MIR_TYPE_STRING,
        MIR_TYPE_VOID,
        MIR_TYPE_PTR,
        MIR_TYPE_ARRAY,
        MIR_TYPE_SLICE,
        MIR_TYPE_STRUCT,
        MIR_TYPE_PARTIAL,
        MIR_TYPE_ENV_HANDLE,
        MIR_TYPE_HISTORY_BUF,
        MIR_TYPE_ENUM_TAG,
        MIR_TYPE_STATE_TAG,
    } MIRTypeKind;

    typedef struct MIRType {
        const char *Name;
        MIRTypeKind Kind;

        struct MIRType *ElementType;

        size_t ArraySize;
        size_t StructFieldCount;

        int IsRestrict;
    } MIRType;

    typedef enum {
        MIR_VAL_TEMP,
        MIR_VAL_GLOBAL,
        MIR_VAL_STATIC,
        MIR_VAL_CONST,
        MIR_VAL_PARAM,
        MIR_VAL_ENV_PARAM,
        MIR_VAL_HISTORY_BUF,
        MIR_VAL_SHADOW,
        MIR_VAL_LABEL,
    } MIRValueKind;

    typedef enum {
        MIR_OWN_OWNED,
        MIR_OWN_BORROWED,
        MIR_OWN_COPIED,
        MIR_OWN_TRAILED,
        MIR_OWN_RAW_PTR,
        MIR_OWN_NONE,
    } MIROwnership;

    typedef struct MIRValue {
        MIRValueKind Kind;
        MIRType *Type;

        int DefID;

        union {
            const char *Name;
            const char *Label;

            int64_t IntConst;
            double FloatConst;
            char CharConst;
            int BoolConst;
            const char *StringConst;
        };

        MIROwnership Ownership;

        int IsRestrict;

        int IsEnvParam;
        const char *EnvName;

        int IsHistoryBuffer;
        int HistoryCapacity;
    } MIRValue;

    typedef struct {
        MIRValue *Destination;

        MIRValue **Incoming;
        int *FromBlock;

        size_t Count;
    } MIRPhi;

    typedef enum {
        MIR_NOP,
        MIR_CONST,
        MIR_MOVE,
        MIR_COPY,
        MIR_PHI,

        MIR_ALLOC,
        MIR_ALLOC_STACK,
        MIR_ALLOC_REGION,
        MIR_DROP,
        MIR_BORROW_BEGIN,
        MIR_BORROW_END,
        MIR_TRAIL_WRITE,

        MIR_ADD,
        MIR_SUB,
        MIR_MUL,
        MIR_DIV,
        MIR_MOD,
        MIR_NEG,

        MIR_EQ,
        MIR_NE,
        MIR_LT,
        MIR_LE,
        MIR_GT,
        MIR_GE,

        MIR_AND,
        MIR_OR,
        MIR_NOT,

        MIR_LOAD,
        MIR_STORE,
        MIR_ADDR,
        MIR_RAW_LOAD,
        MIR_RAW_STORE,

        MIR_ARRAY_ALLOC,
        MIR_ARRAY_INDEX,
        MIR_ARRAY_INDEX_UNCHECKED,
        MIR_ARRAY_LEN,
        MIR_SLICE_MAKE,

        MIR_STRUCT_ALLOC,
        MIR_STRUCT_LOAD,     
        MIR_STRUCT_STORE,    
        MIR_PARTIAL_ALLOC,
        MIR_PARTIAL_LOAD,
        MIR_PARTIAL_STORE,
        MIR_PARTIAL_VALID_CHECK,

        MIR_LABEL,
        MIR_JMP,
        MIR_JMP_IF,
        MIR_JMP_IF_FALSE,
        MIR_JMP_TABLE,

        MIR_CALL,
        MIR_CALL_INDIRECT,   
        MIR_RETURN,

        MIR_ENV_ARG_IN,
        MIR_ENV_ARG_OUT,
        MIR_ENV_FIELD_LOAD,  
        MIR_ENV_FIELD_STORE,
        MIR_ENV_CHECK,

        MIR_SAFETY_ENTER,
        MIR_SAFETY_EXIT,

        MIR_ASSERT_RT,
        MIR_ASSERT_CT,
        MIR_ASSUME_HINT,
        MIR_RESTRICT_HINT,

        MIR_CLEANUP_BLOCK,
        MIR_CLEANUP_CALL,

        MIR_SAVEPOINT,
        MIR_RESTORE,         
        MIR_COMMIT,
        MIR_SHADOW_COPY,
        MIR_SHADOW_APPLY,

        MIR_SCOPE_ENTER,
        MIR_SCOPE_EXIT,

        MIR_HISTORY_PUSH,
        MIR_HISTORY_LOAD,

        MIR_REACTIVE_DEP,
        MIR_REACTIVE_PROPAGATE,

        MIR_INLINE_ASM,      

        MIR_SIZEOF,
        MIR_OFFSETOF,
        MIR_CAST,
        MIR_BITCAST,
        MIR_ZERO_INIT,
    } MIROpcode;

    typedef enum {
        MIR_SAFETY_SAFE,
        MIR_SAFETY_UNSAFE,
        MIR_SAFETY_TRUSTED,
    } MIRSafetyLevel;

    typedef struct MIRInstruction {
        MIROpcode Op;

        MIRValue *Destination;
        MIRValue *Source1;
        MIRValue *Source2;

        const char *CallTarget;
        MIRValue **Arguments;
        size_t ArgumentCount;

        const char **JumpTargets;
        MIRValue **JumpKeys;
        size_t JumpCount;

        const char *DefaultTarget;

        MIRPhi *Phi;

        int FieldIndex;

        const char *EnvName;
        const char *EnvField;

        MIRValue *ShadowOf;

        MIRValue *HistoryBuffer;
        MIRValue *HistoryIndex;

        MIRValue *DepSource;
        
        const char *AsmSource;
        size_t AsmLength;

        MIRType *CastType;

        MIRSafetyLevel Safety;

        uint32_t Line;
        uint32_t Column;
    } MIRInstruction;

    typedef struct MIRBlock {
        int ID;
        const char  *Label;

        MIRPhi **Phis;
        size_t  PhiCount;
        size_t  PhiCapacity;

        MIRInstruction **Instructions;
        size_t InstructionCount;
        size_t InstructionCapacity;

        struct MIRBlock **Successors;
        size_t SuccessorCount;

        struct MIRBlock **Predecessors;
        size_t PredecessorCount;

        struct MIRBlock *IDom;
        struct MIRBlock **DomChildren;
        size_t DomChildCount;

        struct MIRBlock **CleanupBlocks;
        size_t CleanupCount;

        const char *RegionName;
        MIRValue **RegionAllocs;
        size_t RegionAllocCount;

        MIRSafetyLevel Safety;

        int IsCleanup;
        int IsLandingPad;
    } MIRBlock;

    typedef struct {
        const char *Name;
        MIRType *ReturnType;

        MIRValue **Params;
        size_t ParamCount;

        MIRValue **EnvParams;
        size_t EnvParamCount;

        MIRBlock **Blocks;
        size_t BlockCount;
        size_t BlockCapacity;

        MIRBlock *EntryBlock;
        MIRBlock *ExitBlock;

        MIRValue **Values;
        size_t ValueCount;
        size_t ValueCapacity;

        int  NextDefID;
        int  NextBlockID;

        MIRValue **Statics;
        size_t StaticCount;

        MIRSafetyLevel Safety;

        int IsComptimeFn;
        int IsEntryPoint;
        int IsWorldEntry;

        const char *WorldName;
        const char *ContextName;
    } MIRFunction;

    typedef struct MIRWorld {
        const char *Name;
        const char *ParentName;

        MIRFunction **Functions;
        size_t FunctionCount;

        MIRValue **Globals;
        size_t GlobalCount;

        const char *SymbolPrefix;
    } MIRWorld;

    typedef struct {
        MIRFunction **Functions;
        size_t FunctionCount;
        size_t FunctionCapacity;

        MIRValue **Globals;
        size_t GlobalCount;
        size_t GlobalCapacity;

        MIRFunction **StaticInitFunctions;
        size_t StaticInitCount;

        MIRWorld **Worlds;
        size_t WorldCount;

        struct MIRJumpTable {
            const char *Name;

            MIRValue **Keys;
            const char **Targets;

            size_t Count;
        } **JumpTables;
        size_t JumpTableCount;

        struct MIRStringLiteral {
            const char *Value;
            const char *Label;
        } **StringPool;

        size_t StringPoolCount;
    } MIRProgram;

    MIRProgram *MIRCreateProgram(void);
    void MIRDestroyProgram(MIRProgram *Program);

    MIRFunction *MIRCreateFunction(const char *Name, MIRType *ReturnType);
    MIRBlock *MIRCreateBlock(MIRFunction *Function, const char *Label);
    void MIRAddFunction(MIRProgram *Program, MIRFunction *Function);
    void MIRAddGlobal(MIRProgram *Program, MIRValue *G);
    void MIRAddWorld(MIRProgram *Program, MIRWorld *W);

    MIRValue *MIRCreateTemp(MIRFunction *Function, MIRType *Type);
    MIRValue *MIRCreateParam(MIRFunction *Function, const char *Name, MIRType *Type);
    MIRValue *MIRCreateEnvParam(MIRFunction *Function, const char *EnvName);
    MIRValue *MIRCreateIntConst(int64_t Value, MIRType *Type);
    MIRValue *MIRCreateFloatConst(double Value);
    MIRValue *MIRCreateBoolConst(int Value);
    MIRValue *MIRCreateStringConst(MIRProgram *Program, const char *Value);
    MIRValue *MIRCreateLabelVal(const char *Label);
    MIRValue *MIRCreateHistoryBuf(MIRFunction *Function, MIRType *ElemType, int Capacity);

    MIRType *MIRMakeIntType(void);
    MIRType *MIRMakeFloatType(void);
    MIRType *MIRMakeBoolType(void);
    MIRType *MIRMakeCharType(void);
    MIRType *MIRMakeStringType(void);
    MIRType *MIRMakeVoidType(void);
    MIRType *MIRMakePtrType(MIRType *Elem);
    MIRType *MIRMakeArrayType(MIRType *Elem, size_t Size);
    MIRType *MIRMakeSliceType(MIRType *Elem);
    MIRType *MIRMakeStructType(const char *Name, size_t FieldCount);
    MIRType *MIRMakePartialType(const char *Name, size_t FieldCount);

    void MIRAddInstruction(MIRBlock *Block, MIRInstruction *Instruction);
    void MIRAddPhi(MIRBlock *Block, MIRPhi *Phi);

    MIRInstruction *MIRInstNop(void);
    MIRInstruction *MIRInstConst(MIRValue *Destination, MIRValue *Lit);
    MIRInstruction *MIRInstMove(MIRValue *Destination, MIRValue *Source);
    MIRInstruction *MIRInstCopy(MIRValue *Destination, MIRValue *Source);
    MIRInstruction *MIRInstBinary(MIROpcode Op, MIRValue *Destination, MIRValue *Left, MIRValue *Right);
    MIRInstruction *MIRInstUnary(MIROpcode Op, MIRValue *Destination, MIRValue *Source);
    MIRInstruction *MIRInstAlloc(MIRValue *Destination, MIRType *Type);
    MIRInstruction *MIRInstAllocStack(MIRValue *Destination, MIRType *Type);
    MIRInstruction *MIRInstAllocRegion(MIRValue *Destination, MIRType *Type, const char *Region);
    MIRInstruction *MIRInstDrop(MIRValue *Source);
    MIRInstruction *MIRInstBorrowBegin(MIRValue *Target);
    MIRInstruction *MIRInstBorrowEnd(MIRValue *Target);
    MIRInstruction *MIRInstTrailWrite(MIRValue *Source, MIRValue *Valuie);
    MIRInstruction *MIRInstLoad(MIRValue *Destination, MIRValue *Address);
    MIRInstruction *MIRInstStore(MIRValue *Address, MIRValue *Value);
    MIRInstruction *MIRInstAddr(MIRValue *Destination, MIRValue *Var);
    MIRInstruction *MIRInstRawLoad(MIRValue *Destination, MIRValue *Address);
    MIRInstruction *MIRInstRawStore(MIRValue *Address, MIRValue *Value);
    MIRInstruction *MIRInstArrayAlloc(MIRValue *Destination, MIRType *Elem, MIRValue *Size);
    MIRInstruction *MIRInstArrayIndex(MIRValue *Destination, MIRValue *Arr, MIRValue *Index);
    MIRInstruction *MIRInstArrayIndexUnchecked(MIRValue *Destination, MIRValue *Arr, MIRValue *Index);
    MIRInstruction *MIRInstArrayLen(MIRValue *Destination, MIRValue *Arr);
    MIRInstruction *MIRInstStructLoad(MIRValue *Destination, MIRValue *Source, int FieldIdx);
    MIRInstruction *MIRInstStructStore(MIRValue *Destination, int FieldIdx, MIRValue *Value);
    MIRInstruction *MIRInstPartialLoad(MIRValue *Destination, MIRValue *Source, int FieldIdx);
    MIRInstruction *MIRInstPartialStore(MIRValue *Destination, int FieldIdx, MIRValue *Value);
    MIRInstruction *MIRInstPartialValidCheck(MIRValue *Destination, MIRValue *Source, int FieldIdx);
    MIRInstruction *MIRInstJmp(const char *Label);
    MIRInstruction *MIRInstJmpIf(MIRValue *Condition, const char *Label);
    MIRInstruction *MIRInstJmpIfFalse(MIRValue *Condition, const char *Label);
    MIRInstruction *MIRInstJmpTable(MIRValue *Key, const char **Targets, MIRValue **Keys, size_t Size, const char *Default);
    MIRInstruction *MIRInstCall(MIRValue *Destination, const char *Target, MIRValue **Arguments, size_t Size);
    MIRInstruction *MIRInstReturn(MIRValue *Value);
    MIRInstruction *MIRInstEnvArgIn(MIRValue *Destination, const char *EnvName);
    MIRInstruction *MIRInstEnvArgOut(const char *EnvName, MIRValue *Token);
    MIRInstruction *MIRInstEnvFieldLoad(MIRValue *Destination, MIRValue *EnvParam, const char *Field);
    MIRInstruction *MIRInstEnvFieldStore(MIRValue *EnvParam, const char *Field, MIRValue *Value);
    MIRInstruction *MIRInstEnvCheck(MIRValue *EnvParam);
    MIRInstruction *MIRInstSafetyEnter(MIRSafetyLevel Level);
    MIRInstruction *MIRInstSafetyExit(MIRSafetyLevel Level);
    MIRInstruction *MIRInstAssertRT(MIRValue *Condition, const char *Message);
    MIRInstruction *MIRInstAssertCT(MIRValue *Condition, const char *Message);
    MIRInstruction *MIRInstAssumeHint(MIRValue *Condition);
    MIRInstruction *MIRInstRestrictHint(MIRValue *Pointer);
    MIRInstruction *MIRInstCleanupBlock(const char *Label);
    MIRInstruction *MIRInstCleanupCall(const char *Label);
    MIRInstruction *MIRInstSavepoint(const char *Name);
    MIRInstruction *MIRInstRestore(const char *Name);
    MIRInstruction *MIRInstCommit(const char *Name);
    MIRInstruction *MIRInstShadowCopy(MIRValue *Destination, MIRValue *Original);
    MIRInstruction *MIRInstShadowApply(MIRValue *Destination, MIRValue *Shadow);
    MIRInstruction *MIRInstScopeEnter(const char *RegionName);
    MIRInstruction *MIRInstScopeExit(const char *RegionName);
    MIRInstruction *MIRInstHistoryPush(MIRValue *Buffer, MIRValue *Value);
    MIRInstruction *MIRInstHistoryLoad(MIRValue *Destination, MIRValue *Buffer, MIRValue *Index);
    MIRInstruction *MIRInstReactiveDep(MIRValue *Destination, MIRValue *Source);
    MIRInstruction *MIRInstReactivePropagate(MIRValue *Source);
    MIRInstruction *MIRInstInlineAsm(const char *Source, size_t Len);
    MIRInstruction *MIRInstSizeof(MIRValue *Destination, MIRType *Type);
    MIRInstruction *MIRInstOffsetof(MIRValue *Destination, MIRType *Type, int FieldIdx);
    MIRInstruction *MIRInstCast(MIRValue *Destination, MIRValue *Source, MIRType *TargetType);
    MIRInstruction *MIRInstBitcast(MIRValue *Destination, MIRValue *Source, MIRType *TargetType);
    MIRInstruction *MIRInstZeroInit(MIRValue *Destination);

    MIRPhi *MIRCreatePhi(MIRValue *Destination, size_t PredCount);
    void MIRPhiAddIncoming(MIRPhi *Phi, MIRValue *Value, int BlockID);

    void MIRComputeDominators(MIRFunction *Function);
    void MIRInsertPhis(MIRFunction *Function);
    int  MIRVerify(const MIRProgram *Program);
    void MIRRemoveDeadBlocks(MIRFunction *Function);
    void MIRConstantFold(MIRFunction *Function);

    void MIRPrint(const MIRProgram *Program);
    void MIRPrintFunction(const MIRFunction *Function);
    void MIRPrintBlock(const MIRBlock *Block);
    void MIRPrintInstruction(const MIRInstruction *Instruction);
    void MIRPrintValue(const MIRValue *Value);
    const char *MIROpcodeStr(MIROpcode Op);
    const char *MIROwnershipStr(MIROwnership O);
    const char *MIRSafetyStr(MIRSafetyLevel Left);

#endif
