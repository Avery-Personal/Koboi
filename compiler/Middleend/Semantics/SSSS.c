#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "SSSS.h"

#define SS_DEBUG 0

static int IsPascalCase(const char *String) {
    if (!String || !isupper(String[0]))
        return 0;
    
    for (int i = 1; String[i]; i++) {
        if (String[i] == '_')
            return 0;
    }
    
    return 1;
}

static int IsCamelCase(const char *String) {
    if (!String || !islower(String[0]))
        return 0;
    
    for (int i = 1; String[i]; i++) {
        if (String[i] == '_')
            return 0;
    }
    
    return 1;
}

static int IsUpperSnakeCase(const char *String) {
    if (!String)
        return 0;

    for (int i = 0; String[i]; i++) {
        if (!isupper(String[i]) && !isdigit(String[i]) && String[i] != '_')
            return 0;
    }

    return 1;
}

static void AddSymbol(SSSSContext *Context, const char *Name, uint32_t Traits, uint32_t Line, uint32_t Column) {
    if (Context -> SymbolCount >= Context -> SymbolCapacity) {
        Context -> SymbolCapacity = Context -> SymbolCapacity == 0 ? 64 : Context -> SymbolCapacity * 2;
        Context -> Symbols = realloc(Context -> Symbols, Context -> SymbolCapacity  *sizeof(SSSSSymbol));
    }

    if (Context -> EnforceNamingConventions) {
        if (Traits & SSSS_TRAIT_CONST) {
            if (!IsUpperSnakeCase(Name)) {
                ReportWarning(Context, Line, Column, "Constants should be UPPER_SNAKE_CASE.");
            }
        } else if (!IsCamelCase(Name) && !IsPascalCase(Name)) {
            ReportWarning(Context, Line, Column, "Variables should be camelCase or PascalCase.");
        }
    }

    SSSSSymbol *Symbol = &Context -> Symbols[Context -> SymbolCount++];

    Symbol -> Name = Name;
    Symbol -> State = SSSS_LIFECYCLE_UNINITIALIZED;
    Symbol -> Traits = Traits;
    Symbol -> ScopeDepth = Context -> CurrentScopeDepth;
    Symbol -> DeclarationLine = Line;
    Symbol -> DeclarationColumn = Column;
    Symbol -> UseCount = 0;
}

static SSSSSymbol *FindSymbol(SSSSContext *Context, const char *Name) {
    for (int i = Context -> SymbolCount - 1; i >= 0; i--) {
        if (strcmp(Context -> Symbols[i].Name, Name) == 0) {
            return &Context -> Symbols[i];
        }
    }

    return NULL;
}

SSSSContext *CreateContext() {
    SSSSContext *Context = malloc(sizeof(SSSSContext));

    Context -> Symbols = NULL;
    Context -> SymbolCount = 0;
    Context -> SymbolCapacity = 0;
    Context -> CurrentScopeDepth = 0;
    Context -> ErrorCount = 0;
    Context -> WarningCount = 0;
    Context -> EnforceNamingConventions = 1;
    Context -> StrictLinearChecking = 1;

    return Context;
}

void DestroyContext(SSSSContext *Context) {
    if (Context -> Symbols)
        free(Context -> Symbols);

    free(Context);
}

void ReportError(SSSSContext *Context, uint32_t Line, uint32_t Column, const char *Message) {
    fprintf(stderr, "[SSSS ERROR] Line %u:%u -> %s\n", Line, Column, Message);

    Context -> ErrorCount++;
}

void ReportWarning(SSSSContext *Context, uint32_t Line, uint32_t Column, const char *Message) {
    fprintf(stderr, "[SSSS WARN]  Line %u:%u -> %s\n", Line, Column, Message);

    Context -> WarningCount++;
}

static void CheckOperandRead(SSSSContext *Context, SSOperand Op, uint32_t Line, uint32_t Column) {
    if (Op.Type != SS_OPERAND_VARIABLE)
        return;

    SSSSSymbol *Symbol = FindSymbol(Context, Op.Variable);
    if (!Symbol) {
        ReportError(Context, Line, Column, "Undeclared variable referenced.");

        return;
    }

    Symbol -> UseCount++;

    switch (Symbol -> State) {
        case SSSS_LIFECYCLE_UNINITIALIZED:
            ReportError(Context, Line, Column, "Use of uninitialized variable.");
            
            break;
        case SSSS_LIFECYCLE_MOVED:
            ReportError(Context, Line, Column, "Use of MOVED variable. Ownership was already transferred.");
            
            break;
        case SSSS_LIFECYCLE_FREED:
            ReportError(Context, Line, Column, "Use of FREED variable. Memory is dead.");
            
            break;

        default: break;
    }
}

static void HandleScopeEnd(SSSSContext *Context, uint32_t Line, uint32_t Column) {
    for (int i = Context -> SymbolCount - 1; i >= 0; i--) {
        SSSSSymbol *Symbol = &Context -> Symbols[i];
        
        if (Symbol -> ScopeDepth == Context -> CurrentScopeDepth) {
            if (Context -> StrictLinearChecking && (Symbol -> Traits & SSSS_TRAIT_LINEAR)) {
                if (Symbol -> UseCount != 1) {
                    char buf[256];

                    snprintf(buf, sizeof(buf), "Linear variable '%s' must be used exactly once. Used %u times.", Symbol -> Name, Symbol -> UseCount);
                    ReportError(Context, Line, Column, buf);
                }
            }
            
            Context -> SymbolCount--; 
        } else {
            break;
        }
    }
    Context -> CurrentScopeDepth--;
}

int AnalyzeProgram(SSSSContext *Context, SSProgram *Program) {
    for (size_t i = 0; i < Program -> Count; i++) {
        SSInstruction *Instruction = &Program -> Instructions[i];
        
        switch (Instruction -> Op) {
            case SS_OP_SCOPE_BEGIN:
                Context -> CurrentScopeDepth++;

                break;
                
            case SS_OP_SCOPE_END:
                HandleScopeEnd(Context, Instruction -> Line, Instruction -> Column);

                break;

            case SS_OP_STORE: {
                if (Instruction -> Destination.Type == SS_OPERAND_VARIABLE) {
                    SSSSSymbol *Destination = FindSymbol(Context, Instruction -> Destination.Variable);

                    if (!Destination) {
                        AddSymbol(Context, Instruction -> Destination.Variable, SSSS_TRAIT_NORMAL, Instruction -> Line, Instruction -> Column);
                        
                        Destination = FindSymbol(Context, Instruction -> Destination.Variable);
                    } else {
                        if (Destination -> Traits & SSSS_TRAIT_CONST) {
                            ReportError(Context, Instruction -> Line, Instruction -> Column, "Cannot reassign a CONST variable.");
                        }
                    }

                    Destination -> State = SSSS_LIFECYCLE_OWNED;
                }
                CheckOperandRead(Context, Instruction -> Source1, Instruction -> Line, Instruction -> Column);

                break;
            }

            case SS_OP_MOVE: {
                CheckOperandRead(Context, Instruction -> Source1, Instruction -> Line, Instruction -> Column);
                
                if (Instruction -> Source1.Type == SS_OPERAND_VARIABLE) {
                    SSSSSymbol *Source = FindSymbol(Context, Instruction -> Source1.Variable);
                    if (Source)
                        Source -> State = SSSS_LIFECYCLE_MOVED;
                }

                if (Instruction -> Destination.Type == SS_OPERAND_VARIABLE) {
                    SSSSSymbol *Destination = FindSymbol(Context, Instruction -> Destination.Variable);
                    if (!Destination) {
                        AddSymbol(Context, Instruction -> Destination.Variable, SSSS_TRAIT_NORMAL, Instruction -> Line, Instruction -> Column);
                        
                        Destination = FindSymbol(Context, Instruction -> Destination.Variable);
                    }
                    
                    Destination -> State = SSSS_LIFECYCLE_OWNED;
                }

                break;
            }

            case SS_OP_FREE: {
                CheckOperandRead(Context, Instruction -> Source1, Instruction -> Line, Instruction -> Column);

                if (Instruction -> Source1.Type == SS_OPERAND_VARIABLE) {
                    SSSSSymbol *Source = FindSymbol(Context, Instruction -> Source1.Variable);
                    if (Source) {
                        if (Source -> State == SSSS_LIFECYCLE_FREED) {
                            ReportError(Context, Instruction -> Line, Instruction -> Column, "Double free detected.");
                        }

                        Source -> State = SSSS_LIFECYCLE_FREED;
                    }
                }

                break;
            }

            case SS_OP_ADD:
            case SS_OP_SUB:
            case SS_OP_MUL:
            case SS_OP_DIV:
                CheckOperandRead(Context, Instruction -> Source1, Instruction -> Line, Instruction -> Column);
                CheckOperandRead(Context, Instruction -> Source2, Instruction -> Line, Instruction -> Column);

                break;

            default: break;
        }
    }

    if (Context -> CurrentScopeDepth == 0 && Context -> SymbolCount > 0) {
        HandleScopeEnd(Context, 0, 0);
    }

    #if SS_DEBUG
        printf("[SSSS] Analysis Complete. Errors: %u, Warnings: %u\n", Context -> ErrorCount, Context -> WarningCount);
    #endif

    return Context -> ErrorCount == 0;
}
