#ifndef SSSS_H
#define SSSS_H

    #include "../SyntaxTapeS/SS.h"

    typedef enum {
        SSSS_LIFECYCLE_UNINITIALIZED,
        SSSS_LIFECYCLE_OWNED,
        SSSS_LIFECYCLE_BORROWED_IMM,
        SSSS_LIFECYCLE_BORROWED_MUT,
        SSSS_LIFECYCLE_MOVED,
        SSSS_LIFECYCLE_FREED
    } SSSSLifecycleState;

    typedef enum {
        SSSS_TRAIT_NORMAL = 0,
        SSSS_TRAIT_CONST = 1 << 0,
        SSSS_TRAIT_LINEAR = 1 << 1,
        SSSS_TRAIT_STATIC = 1 << 2,
        SSSS_TRAIT_GLOBAL = 1 << 3
    } SSSSVariableTraits;

    typedef struct {
        const char *Name;

        SSSSLifecycleState State;
        uint32_t Traits;
        
        uint32_t ScopeDepth;
        uint32_t DeclarationLine;
        uint32_t DeclarationColumn;
        
        uint32_t UseCount;
    } SSSSSymbol;

    typedef struct {
        SSSSSymbol *Symbols;
        size_t SymbolCount;
        size_t SymbolCapacity;

        uint32_t CurrentScopeDepth;
        uint32_t ErrorCount;
        uint32_t WarningCount;  

        int EnforceNamingConventions;
        int StrictLinearChecking;
    } SSSSContext;

    SSSSContext *CreateContext();
    void DestroyContext(SSSSContext *Context);

    int AnalyzeProgram(SSSSContext *Context, SSProgram *Program);

    void ReportError(SSSSContext *Context, uint32_t Line, uint32_t Column, const char *Message);
    void ReportWarning(SSSSContext *Context, uint32_t Line, uint32_t Column, const char *Message);

#endif
