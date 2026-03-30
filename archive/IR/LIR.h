#ifndef LIR_H
#define LIR_H

    #include <stdint.h>
    #include <stddef.h>

    typedef enum {
        LIR_MTYPE_I8,
        LIR_MTYPE_I16,
        LIR_MTYPE_I32,
        LIR_MTYPE_I64,
        LIR_MTYPE_F32,
        LIR_MTYPE_F64,
        LIR_MTYPE_PTR,
        LIR_MTYPE_VOID,
    } LIRMachineType;

    typedef enum {
        LIR_REG_VIRTUAL,   /* pre-allocation virtual register                   */
        LIR_REG_PHYSICAL,  /* post-allocation physical register index           */
        LIR_REG_SPILLED,   /* spilled to a stack slot                           */
        LIR_REG_FIXED,     /* ABI-fixed register (e.g. return register, sp)     */
    } LIRRegKind;

    typedef enum {
        /* Caller-save (scratch) — target-defined set; indices 0..N */
        LIR_REGCLASS_CALLER_SAVE,
        /* Callee-save (preserved) */
        LIR_REGCLASS_CALLEE_SAVE,
        /* Floating-point */
        LIR_REGCLASS_FLOAT,
        /* Special (stack pointer, frame pointer, link register, …) */
        LIR_REGCLASS_SPECIAL,
    } LIRRegClass;

    typedef struct {
        LIRRegKind    Kind;
        LIRMachineType Type;
        int           ID;           /* unique virtual ID; physical index if PHYSICAL */
        LIRRegClass   Class;
        int           IsFixed;      /* must not be re-allocated                  */
        int           FixedPhysID;  /* physical reg index if IsFixed             */
        const char   *Name;         /* human-readable name for debugging         */
    } LIRReg;

    /* =========================================================================
    * Stack Slots
    * ========================================================================= */

    typedef struct {
        int            ID;
        LIRMachineType Type;
        int            ByteSize;
        int            Alignment;
        int            FrameOffset; /* set after frame layout pass; negative =
                                    below frame pointer / above stack pointer */
        int            IsParam;     /* incoming parameter slot                   */
        int            IsSpill;     /* register spill slot                       */
        int            IsLocal;     /* explicit local variable                   */
        const char    *Name;
    } LIRStackSlot;

    /* =========================================================================
    * Operands  (union of register, immediate, stack slot, label)
    * ========================================================================= */

    typedef enum {
        LIR_OPERAND_REG,
        LIR_OPERAND_IMMI,    /* integer immediate                               */
        LIR_OPERAND_IMMF,    /* floating-point immediate                        */
        LIR_OPERAND_SLOT,    /* stack slot reference                            */
        LIR_OPERAND_LABEL,   /* branch / call target label                      */
        LIR_OPERAND_GLOBAL,  /* global symbol reference                         */
        LIR_OPERAND_NONE,
    } LIROperandKind;

    typedef struct {
        LIROperandKind Kind;
        LIRMachineType Type;

        union {
            LIRReg       *Reg;
            int64_t       ImmI;
            double        ImmF;
            LIRStackSlot *Slot;
            const char   *Label;    /* LIR_OPERAND_LABEL / LIR_OPERAND_GLOBAL   */
        };
    } LIROperand;

    #define LIR_OP_REG(r)   ((LIROperand){ .Kind = LIR_OPERAND_REG,    .Reg   = (r) })
    #define LIR_OP_IMMI(v)  ((LIROperand){ .Kind = LIR_OPERAND_IMMI,   .ImmI  = (v) })
    #define LIR_OP_IMMF(v)  ((LIROperand){ .Kind = LIR_OPERAND_IMMF,   .ImmF  = (v) })
    #define LIR_OP_SLOT(s)  ((LIROperand){ .Kind = LIR_OPERAND_SLOT,   .Slot  = (s) })
    #define LIR_OP_LABEL(l) ((LIROperand){ .Kind = LIR_OPERAND_LABEL,  .Label = (l) })
    #define LIR_OP_GLOBAL(n)((LIROperand){ .Kind = LIR_OPERAND_GLOBAL, .Label = (n) })
    #define LIR_OP_NONE     ((LIROperand){ .Kind = LIR_OPERAND_NONE              })

    /* =========================================================================
    * Instruction Opcodes
    * ========================================================================= */

    typedef enum {

        /* — Core ——————————————————————————————————————————————————————————— */
        LIR_NOP,
        LIR_MOVE,            /* Dest = Src  (register-to-register)             */
        LIR_LOAD_IMM,        /* Dest = <immediate>                              */
        LIR_PCOPY,           /* parallel copy: multiple (Dest, Src) pairs       */

        /* — Stack frame ————————————————————————————————————————————————————— */
        LIR_FRAME_ALLOC,     /* reserve Imm bytes on the stack frame            */
        LIR_FRAME_FREE,      /* release Imm bytes from the stack frame          */
        LIR_SLOT_ADDR,       /* Dest = address of StackSlot                     */

        /* — Arithmetic (integer) ——————————————————————————————————————————— */
        LIR_ADD_I, LIR_SUB_I, LIR_MUL_I,
        LIR_DIV_I, LIR_MOD_I, LIR_NEG_I,

        /* — Arithmetic (floating-point) ———————————————————————————————————— */
        LIR_ADD_F, LIR_SUB_F, LIR_MUL_F,
        LIR_DIV_F, LIR_NEG_F,

        /* — Bitwise ————————————————————————————————————————————————————————— */
        LIR_AND, LIR_OR, LIR_XOR, LIR_NOT,
        LIR_SHL, LIR_SHR_L,  /* logical shift right  */
        LIR_SHR_A,            /* arithmetic shift right */

        /* — Comparison (sets Dest to 0 or 1) ——————————————————————————————— */
        LIR_CMP_EQ, LIR_CMP_NE,
        LIR_CMP_LT, LIR_CMP_LE, LIR_CMP_GT, LIR_CMP_GE,

        /* — Memory ————————————————————————————————————————————————————————— */
        LIR_LOAD8,           /* Dest = *[8-bit] Src                             */
        LIR_LOAD16,
        LIR_LOAD32,
        LIR_LOAD64,
        LIR_LOADF32,
        LIR_LOADF64,
        LIR_STORE8,          /* *[8-bit] Dest = Src                             */
        LIR_STORE16,
        LIR_STORE32,
        LIR_STORE64,
        LIR_STOREF32,
        LIR_STOREF64,
        LIR_MEMCPY,          /* Dest, Src, Bytes — raw block copy               */
        LIR_MEMSET,          /* Dest, Value, Bytes — raw block fill             */

        /* — Address arithmetic ————————————————————————————————————————————— */
        LIR_LEA,             /* Dest = Base + Offset (pointer arithmetic)       */
        LIR_FIELD_ADDR,      /* Dest = StructPtr + ByteOffset                   */

        /* — Type conversions ——————————————————————————————————————————————— */
        LIR_ZEXT,            /* zero-extend Src → Dest                         */
        LIR_SEXT,            /* sign-extend Src → Dest                         */
        LIR_TRUNC,           /* truncate Src → Dest                             */
        LIR_INT_TO_FLOAT,
        LIR_FLOAT_TO_INT,
        LIR_FLOAT_EXTEND,    /* f32 → f64                                        */
        LIR_FLOAT_TRUNC,     /* f64 → f32                                        */
        LIR_BITCAST,         /* reinterpret bits without conversion              */
        LIR_PTR_TO_INT,
        LIR_INT_TO_PTR,

        /* — Control Flow ———————————————————————————————————————————————————— */
        LIR_LABEL,
        LIR_JMP,             /* unconditional jump                               */
        LIR_JMP_IF,          /* jump if Src != 0                                 */
        LIR_JMP_IF_FALSE,    /* jump if Src == 0                                 */
        LIR_JMP_TABLE,       /* indirect jump via address table                  */
        LIR_JMP_REG,         /* jump to address in register (Naomi: computed)    */

        /* — Calls (ABI-level) ——————————————————————————————————————————————— */
        LIR_ARG,             /* place argument in arg-position slot/register     */
        LIR_CALL,            /* direct call to named symbol                      */
        LIR_CALL_REG,        /* indirect call via function pointer register      */
        LIR_RETURN,
        LIR_RET_VOID,

        /* — Prologue / Epilogue ————————————————————————————————————————————— */
        LIR_PROLOGUE,        /* function entry: save callee-saves, set up frame  */
        LIR_EPILOGUE,        /* function exit: restore callee-saves, pop frame   */

        /* — Runtime support calls ——————————————————————————————————————————— */
        LIR_ALLOC_HEAP,      /* Dest = __koboi_alloc(SizeReg)                    */
        LIR_FREE_HEAP,       /* __koboi_free(PtrReg)                             */
        LIR_ALLOC_REGION,    /* Dest = __koboi_region_alloc(RegionHdl, SizeReg)  */
        LIR_SCOPE_ENTER,     /* __koboi_scope_enter(&scope_handle)               */
        LIR_SCOPE_EXIT,      /* __koboi_scope_exit(&scope_handle)  — drops allocs*/
        LIR_SAVEPOINT_PUSH,  /* __koboi_savepoint_push(&frame)                   */
        LIR_SAVEPOINT_POP,   /* __koboi_savepoint_pop(&frame)  — rollback        */
        LIR_SAVEPOINT_DISCARD,/* __koboi_savepoint_discard(&frame)  — commit     */

        /* — Bounds / safety checks ————————————————————————————————————————— */
        LIR_BOUNDS_CHECK,    /* if (Idx >= Len) trap LIR_TRAP_BOUNDS             */
        LIR_NULL_CHECK,      /* if (Ptr == 0) trap LIR_TRAP_NULL                 */
        LIR_TRAP,            /* unconditional trap (unreachable / error)         */
        LIR_TRAP_IF,         /* conditional trap                                 */

        /* — Ring-buffer (history variables lowered) ————————————————————————— */
        LIR_RING_PUSH,       /* ring_buf[idx++ & mask] = Src                     */
        LIR_RING_LOAD,       /* Dest = ring_buf[(head - Idx) & mask]             */

        /* — Partial-struct validity bits ———————————————————————————————————— */
        LIR_VALID_BIT_SET,   /* validity_word |= (1 << FieldBit)                */
        LIR_VALID_BIT_CLEAR, /* validity_word &= ~(1 << FieldBit)               */
        LIR_VALID_BIT_CHECK, /* Dest = (validity_word >> FieldBit) & 1          */

        /* — Annotations (zero-width; metadata only) ————————————————————————— */
        LIR_ASSUME_HINT,     /* compiler may assume condition holds; not emitted */
        LIR_RESTRICT_HINT,   /* noalias annotation on Src pointer; not emitted  */
        LIR_LIFETIME_START,  /* Src is live from here (for stack slot coalescing)*/
        LIR_LIFETIME_END,    /* Src is dead after here                          */
        LIR_SPILL,           /* Slot = Reg  (register → stack slot)             */
        LIR_FILL,            /* Reg = Slot  (stack slot → register)             */

        /* — Inline assembly (Naomi) ——————————————————————————————————————— */
        LIR_INLINE_ASM,      /* verbatim Naomi source; clobbers declared         */

        /* — Debug ——————————————————————————————————————————————————————————— */
        LIR_DEBUG_LOC,       /* update source location for debug info            */
        LIR_DEBUG_VAR,       /* associate register / slot with a source var name */

    } LIROpcode;

    /* =========================================================================
    * Trap kinds  (used by LIR_TRAP / LIR_TRAP_IF / LIR_BOUNDS_CHECK)
    * ========================================================================= */

    typedef enum {
        LIR_TRAP_BOUNDS,          /* out-of-bounds array / slice access          */
        LIR_TRAP_NULL,            /* null pointer dereference                    */
        LIR_TRAP_OVERFLOW,        /* integer overflow (in checked arithmetic)    */
        LIR_TRAP_UNREACHABLE,     /* match non-exhaustive / assert false         */
        LIR_TRAP_PARTIAL_ACCESS,  /* access to uninitialised partial field       */
        LIR_TRAP_ALLOC_FAIL,      /* allocation returned null                    */
        LIR_TRAP_USER,            /* explicit EXIT() or panic                    */
    } LIRTrapKind;

    /* =========================================================================
    * Inline assembly operand  (Naomi / nao{} blocks)
    * ========================================================================= */

    typedef struct {
        const char *Source;   /* verbatim Naomi source text                      */
        size_t      Length;

        /* Clobber list: registers that this block may modify */
        LIRReg **Clobbers;
        size_t   ClobberCount;

        /* Input / output constraints */
        LIROperand *Inputs;
        LIROperand *Outputs;
        size_t      InputCount;
        size_t      OutputCount;
    } LIRInlineAsm;

    /* =========================================================================
    * Parallel-copy  (phi-elimination result)
    *
    * Emitted at the end of a predecessor block to simultaneously assign
    * values to phi-destination registers before jumping to the join point.
    * Uses a sequentialisation algorithm to resolve copy cycles.
    * ========================================================================= */

    typedef struct {
        LIROperand Dest;
        LIROperand Src;
    } LIRCopyPair;

    /* =========================================================================
    * Instructions
    * ========================================================================= */

    typedef struct LIRInstruction {
        LIROpcode Op;
        LIRMachineType OperandType;  /* bit-width the operation works on         */

        LIROperand Dest;
        LIROperand Src1;
        LIROperand Src2;

        /* Call / arg passing */
        const char   *CallTarget;   /* direct call symbol name                   */
        LIROperand   *ArgOperands;  /* per-arg operands (LIR_ARG emits these)   */
        size_t        ArgCount;

        /* Jump-table */
        const char  **JumpTargets;
        LIROperand   *JumpKeys;
        size_t        JumpCount;
        const char   *DefaultTarget;

        /* Parallel copy (LIR_PCOPY) */
        LIRCopyPair *CopyPairs;
        size_t       CopyPairCount;

        /* Spill / fill */
        LIRStackSlot *SpillSlot;    /* LIR_SPILL / LIR_FILL target slot         */

        /* Memory operation width for LOAD* / STORE* (encoded in Op + OperandType) */
        int MemByteOffset;          /* constant byte offset added to Src address */

        /* Field address */
        int FieldByteOffset;        /* LIR_FIELD_ADDR byte offset                */

        /* Bounds / trap */
        LIRTrapKind TrapKind;
        LIROperand  BoundsLen;      /* LIR_BOUNDS_CHECK: length operand          */

        /* Ring-buffer */
        LIROperand  RingMask;       /* LIR_RING_PUSH / RING_LOAD: capacity mask */
        LIROperand  RingHead;       /* current head index register               */

        /* Validity bit */
        int ValidBitIndex;          /* LIR_VALID_BIT_*: field index in word      */

        /* Inline assembly */
        LIRInlineAsm *InlineAsm;

        /* Debug annotation */
        uint32_t    DebugLine;
        uint32_t    DebugColumn;
        const char *DebugVarName;   /* LIR_DEBUG_VAR                             */

        /* Annotation only (never emits bytes) */
        int IsAnnotationOnly;
    } LIRInstruction;

    /* =========================================================================
    * Basic Blocks  (linear — no SSA, no phis)
    * ========================================================================= */

    typedef struct LIRBlock {
        int          ID;
        const char  *Label;

        LIRInstruction **Instructions;
        size_t           InstructionCount;
        size_t           InstructionCapacity;

        /* CFG (retained for register allocation and liveness analysis) */
        struct LIRBlock **Successors;
        size_t            SuccessorCount;
        struct LIRBlock **Predecessors;
        size_t            PredecessorCount;

        /* Liveness sets (populated by liveness analysis pass) */
        uint64_t *LiveIn;    /* bitset over virtual register IDs               */
        uint64_t *LiveOut;

        /* Fall-through label (NULL = block ends with explicit jump)            */
        const char *FallThrough;

        int IsCleanup;      /* defer-lowered cleanup block                      */
        int IsLandingPad;   /* exception / trap landing pad                     */
        int IsCold;         /* unlikely path; may be placed in cold section     */
    } LIRBlock;

    /* =========================================================================
    * Frame layout
    * ========================================================================= */

    typedef struct {
        LIRStackSlot **Slots;
        size_t         SlotCount;
        size_t         SlotCapacity;

        int  TotalByteSize;    /* computed after frame layout pass               */
        int  ReturnAddressOffset;
        int  SavedFPOffset;    /* saved frame pointer                            */
        int  Alignment;        /* ABI-required alignment (typically 16)          */

        /* Callee-save registers that must be saved/restored in prologue/epilogue */
        LIRReg **CalleeSaves;
        size_t   CalleeSaveCount;
    } LIRFrame;

    /* =========================================================================
    * Functions
    * ========================================================================= */

    typedef struct {
        const char *Name;
        const char *MangledName;   /* symbol name used in output                 */
        LIRMachineType ReturnType;

        /* ABI parameter registers / slots */
        LIROperand *ParamLocations;
        size_t      ParamCount;

        LIRBlock **Blocks;
        size_t     BlockCount;
        size_t     BlockCapacity;

        LIRBlock *EntryBlock;
        LIRBlock *ExitBlock;

        /* All virtual registers in this function */
        LIRReg **Regs;
        size_t   RegCount;
        size_t   RegCapacity;
        int      NextVirtualID;

        LIRFrame Frame;

        int IsEntryPoint;
        int IsWorldEntry;
        int IsComptimeFn;   /* already evaluated; should not appear in output    */
        int IsCold;         /* hint: place in cold section                       */
        int IsLeaf;         /* makes no calls; can omit frame pointer            */

        const char *WorldName;
    } LIRFunction;

    /* =========================================================================
    * Jump table  (emitted as a read-only data section entry)
    * ========================================================================= */

    typedef struct {
        const char  *Name;
        const char **Targets;
        size_t       EntryCount;
    } LIRJumpTable;

    /* =========================================================================
    * Global variables  (flat machine-level representation)
    * ========================================================================= */

    typedef struct {
        const char    *Name;
        LIRMachineType Type;
        int            ByteSize;
        int            Alignment;

        int IsReadOnly;    /* const → placed in .rodata                         */
        int IsZeroInit;    /* zero-initialised → placed in .bss                 */
        int IsExported;    /* visible outside the compilation unit              */
        int IsThreadLocal; /* not generated by Koboi currently; reserved        */

        /* Initial value (for non-zero-init globals) */
        union {
            int64_t     IntValue;
            double      FloatValue;
            const char *StringValue;
            void       *BlobData;     /* arbitrary byte sequence for arrays      */
        } Init;
        size_t InitByteSize;
    } LIRGlobal;

    /* =========================================================================
    * String literal pool entry
    * ========================================================================= */

    typedef struct {
        const char *Value;       /* null-terminated UTF-8 string                */
        size_t      ByteLen;     /* length not counting null terminator         */
        const char *Label;       /* symbol name in output assembly / object     */
    } LIRStringLiteral;

    /* =========================================================================
    * Program
    * ========================================================================= */

    typedef struct {
        LIRFunction **Functions;
        size_t        FunctionCount;
        size_t        FunctionCapacity;

        LIRGlobal **Globals;
        size_t      GlobalCount;
        size_t      GlobalCapacity;

        LIRJumpTable **JumpTables;
        size_t         JumpTableCount;

        LIRStringLiteral **StringPool;
        size_t             StringPoolCount;

        /* Static initialiser function (generated from module-level code) */
        LIRFunction *StaticInitFn;  /* NULL if no static inits                   */

        /* Target descriptor */
        struct {
            const char *TripleName;  /* e.g. "x86_64-linux", "arm64-macos"       */
            int         PointerSize; /* 4 or 8 bytes                              */
            int         IntSize;     /* default int size in bytes                 */
            int         StackAlignment;
        } Target;
    } LIRProgram;

    /* =========================================================================
    * Constructor / Builder API
    * ========================================================================= */

    LIRProgram    *LIRCreateProgram(const char *TargetTriple);
    void           LIRDestroyProgram(LIRProgram *P);

    LIRFunction   *LIRCreateFunction(const char *Name, LIRMachineType ReturnType);
    LIRBlock      *LIRCreateBlock(LIRFunction *Fn, const char *Label);
    void           LIRAddFunction(LIRProgram *P, LIRFunction *Fn);
    void           LIRAddGlobal(LIRProgram *P, LIRGlobal *G);
    void           LIRAddJumpTable(LIRProgram *P, LIRJumpTable *T);
    const char    *LIRInternString(LIRProgram *P, const char *Str, size_t Len);

    LIRReg        *LIRCreateVirtualReg(LIRFunction *Fn, LIRMachineType Type);
    LIRReg        *LIRCreateFixedReg(LIRFunction *Fn, LIRMachineType Type,
                                    int PhysID, const char *Name);
    LIRStackSlot  *LIRCreateStackSlot(LIRFunction *Fn, LIRMachineType Type,
                                    int ByteSize, int Alignment, const char *Name);
    LIRStackSlot  *LIRCreateParamSlot(LIRFunction *Fn, int ParamIndex,
                                    LIRMachineType Type);

    void           LIRAddInstruction(LIRBlock *B, LIRInstruction *I);

    /* Instruction builders */
    LIRInstruction *LIRInstNop(void);
    LIRInstruction *LIRInstMove(LIROperand Dest, LIROperand Src, LIRMachineType T);
    LIRInstruction *LIRInstLoadImm(LIROperand Dest, int64_t Imm, LIRMachineType T);
    LIRInstruction *LIRInstLoadImmF(LIROperand Dest, double Imm);
    LIRInstruction *LIRInstPCopy(LIRCopyPair *Pairs, size_t N);
    LIRInstruction *LIRInstFrameAlloc(int Bytes);
    LIRInstruction *LIRInstFrameFree(int Bytes);
    LIRInstruction *LIRInstSlotAddr(LIROperand Dest, LIRStackSlot *Slot);
    LIRInstruction *LIRInstBinaryI(LIROpcode Op, LIROperand Dest,
                                    LIROperand L, LIROperand R, LIRMachineType T);
    LIRInstruction *LIRInstBinaryF(LIROpcode Op, LIROperand Dest,
                                    LIROperand L, LIROperand R, LIRMachineType T);
    LIRInstruction *LIRInstUnaryI(LIROpcode Op, LIROperand Dest,
                                LIROperand Src, LIRMachineType T);
    LIRInstruction *LIRInstCmp(LIROpcode Op, LIROperand Dest,
                                LIROperand L, LIROperand R, LIRMachineType T);
    LIRInstruction *LIRInstLoad(LIROpcode Op, LIROperand Dest,
                                LIROperand Addr, int ByteOffset);
    LIRInstruction *LIRInstStore(LIROpcode Op, LIROperand Addr,
                                LIROperand Src, int ByteOffset);
    LIRInstruction *LIRInstMemcpy(LIROperand Dest, LIROperand Src, LIROperand Bytes);
    LIRInstruction *LIRInstMemset(LIROperand Dest, LIROperand Val, LIROperand Bytes);
    LIRInstruction *LIRInstLEA(LIROperand Dest, LIROperand Base, LIROperand Offset);
    LIRInstruction *LIRInstFieldAddr(LIROperand Dest, LIROperand Ptr, int ByteOffset);
    LIRInstruction *LIRInstConvert(LIROpcode Op, LIROperand Dest, LIROperand Src,
                                    LIRMachineType SrcT, LIRMachineType DstT);
    LIRInstruction *LIRInstBitcast(LIROperand Dest, LIROperand Src,
                                    LIRMachineType SrcT, LIRMachineType DstT);
    LIRInstruction *LIRInstLabel(const char *Label);
    LIRInstruction *LIRInstJmp(const char *Label);
    LIRInstruction *LIRInstJmpIf(LIROperand Cond, const char *Label);
    LIRInstruction *LIRInstJmpIfFalse(LIROperand Cond, const char *Label);
    LIRInstruction *LIRInstJmpTable(LIROperand Key, LIRJumpTable *Table);
    LIRInstruction *LIRInstJmpReg(LIROperand Addr);
    LIRInstruction *LIRInstArg(LIROperand Val, int ArgIndex);
    LIRInstruction *LIRInstCall(LIROperand Dest, const char *Target,
                                LIROperand *Args, size_t N);
    LIRInstruction *LIRInstCallReg(LIROperand Dest, LIROperand FnPtr,
                                    LIROperand *Args, size_t N);
    LIRInstruction *LIRInstReturn(LIROperand Val);
    LIRInstruction *LIRInstRetVoid(void);
    LIRInstruction *LIRInstPrologue(LIRFunction *Fn);
    LIRInstruction *LIRInstEpilogue(LIRFunction *Fn);
    LIRInstruction *LIRInstAllocHeap(LIROperand Dest, LIROperand Size);
    LIRInstruction *LIRInstFreeHeap(LIROperand Ptr);
    LIRInstruction *LIRInstAllocRegion(LIROperand Dest, LIROperand RegionHdl,
                                        LIROperand Size);
    LIRInstruction *LIRInstScopeEnter(LIROperand RegionHdl);
    LIRInstruction *LIRInstScopeExit(LIROperand RegionHdl);
    LIRInstruction *LIRInstSavepointPush(LIROperand FrameHdl);
    LIRInstruction *LIRInstSavepointPop(LIROperand FrameHdl);
    LIRInstruction *LIRInstSavepointDiscard(LIROperand FrameHdl);
    LIRInstruction *LIRInstBoundsCheck(LIROperand Idx, LIROperand Len);
    LIRInstruction *LIRInstNullCheck(LIROperand Ptr);
    LIRInstruction *LIRInstTrap(LIRTrapKind Kind);
    LIRInstruction *LIRInstTrapIf(LIROperand Cond, LIRTrapKind Kind);
    LIRInstruction *LIRInstRingPush(LIROperand BufBase, LIROperand Head,
                                    LIROperand Mask, LIROperand Val);
    LIRInstruction *LIRInstRingLoad(LIROperand Dest, LIROperand BufBase,
                                    LIROperand Head, LIROperand Idx,
                                    LIROperand Mask);
    LIRInstruction *LIRInstValidBitSet(LIROperand ValidWord, int FieldBit);
    LIRInstruction *LIRInstValidBitClear(LIROperand ValidWord, int FieldBit);
    LIRInstruction *LIRInstValidBitCheck(LIROperand Dest, LIROperand ValidWord,
                                        int FieldBit);
    LIRInstruction *LIRInstAssumeHint(LIROperand Cond);
    LIRInstruction *LIRInstRestrictHint(LIROperand Ptr);
    LIRInstruction *LIRInstLifetimeStart(LIROperand Reg);
    LIRInstruction *LIRInstLifetimeEnd(LIROperand Reg);
    LIRInstruction *LIRInstSpill(LIRStackSlot *Slot, LIROperand Reg);
    LIRInstruction *LIRInstFill(LIROperand Reg, LIRStackSlot *Slot);
    LIRInstruction *LIRInstInlineAsm(LIRInlineAsm *Asm);
    LIRInstruction *LIRInstDebugLoc(uint32_t Line, uint32_t Column);
    LIRInstruction *LIRInstDebugVar(LIROperand Reg, const char *VarName);

    /* =========================================================================
    * Register Allocation API
    * ========================================================================= */

    /* Run linear-scan register allocation over Fn; fills in
    * LIRReg.Kind / FixedPhysID and inserts LIR_SPILL / LIR_FILL instructions. */
    void LIRRegAlloc(LIRFunction *Fn, int PhysRegCount);

    /* Compute live ranges for all virtual registers in Fn */
    void LIRComputeLiveRanges(LIRFunction *Fn);

    /* Compute liveness (LiveIn / LiveOut) for all blocks in Fn */
    void LIRLivenessAnalysis(LIRFunction *Fn);

    /* Resolve parallel copies after phi elimination (may insert temp regs) */
    void LIRResolvePCopies(LIRFunction *Fn);

    /* Compute final stack frame byte layout */
    void LIRLayoutFrame(LIRFunction *Fn);

    /* =========================================================================
    * Verification
    * ========================================================================= */

    int LIRVerify(const LIRProgram *P);
    int LIRVerifyFunction(const LIRFunction *Fn);

    /* =========================================================================
    * Debug / Print
    * ========================================================================= */

    void        LIRPrint(const LIRProgram *P);
    void        LIRPrintFunction(const LIRFunction *Fn);
    void        LIRPrintBlock(const LIRBlock *B);
    void        LIRPrintInstruction(const LIRInstruction *I);
    void        LIRPrintOperand(const LIROperand *Op);
    void        LIRPrintFrame(const LIRFrame *F);
    const char *LIROpcodeStr(LIROpcode Op);
    const char *LIRMachineTypeStr(LIRMachineType T);
    const char *LIRTrapKindStr(LIRTrapKind K);

#endif /* LIR_H */