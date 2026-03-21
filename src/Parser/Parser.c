#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "../Lexer/Tokens.h"
#include "../Lexer/Lexer.h"
#include "Parser.h"

static ASTStatement *ParseMatchStatement(Parser *_Parser);
static ASTStatement *ParseUnsafeBlock(Parser *_Parser);
static ASTStatement *ParseSafeBlock(Parser *_Parser);
static ASTStatement *ParseTrustedBlock(Parser *_Parser);
static ASTStatement *ParseCheckBlock(Parser *_Parser);
static ASTStatement *ParseAssumeBlock(Parser *_Parser);
static ASTStatement *ParseDeferBlock(Parser *_Parser);
static ASTStatement *ParseRegionBlock(Parser *_Parser);
static ASTStatement *ParseTransactionBlock(Parser *_Parser);
static ASTStatement *ParseReactiveBlock(Parser *_Parser);
static ASTStatement *ParseWithBlock(Parser *_Parser);
static ASTStatement *ParseComptimeBlock(Parser *_Parser);
static ASTStatement *ParseNaoBlock(Parser *_Parser);
static ASTStatement *ParseConceptBlock(Parser *_Parser);
static ASTStatement *ParseExpressionStatement(Parser *_Parser);

static ASTExpression *ParseOwnership(Parser *_Parser);
static ASTExpression *ParseCallOrAccess(Parser *_Parser, ASTExpression *Base);

static ASTType *ParseTypeAnnotation(Parser *_Parser);

static ASTSubprogram *ParseFunction(Parser *_Parser, int IsComptime);
static void ParseEnumDecl(Parser *_Parser, ASTProgram *Program);
static void ParseStateDecl(Parser *_Parser, ASTProgram *Program);
static void ParseStructDecl(Parser *_Parser, ASTProgram *Program);
static void ParsePartialDeclaration(Parser *_Parser, ASTProgram *Program);
static void ParseModuleDeclaration(Parser *_Parser, ASTProgram *Program);
static void ParseWorldDeclaration(Parser *_Parser, ASTProgram *Program);
static void ParseContextDeclaration(Parser *_Parser, ASTProgram *Program);
static void ParseMacroDeclaration(Parser *_Parser, ASTProgram *Program);
static void ParseEnvironmentDeclaration(Parser *_Parser, ASTProgram *Program);
static void ParseUsing(Parser *_Parser, ASTProgram *Program);

static void *XMalloc(size_t Bytes) {
    void *_Parser = malloc(Bytes);
    if (!_Parser) {
        fprintf(stderr, "[Parser] Out of memory\n");
        
        exit(1);
    }

    return _Parser;
}

static void *XCalloc(size_t Size, size_t Bytes) {
    void *_Parser = calloc(Size, Bytes);
    if (!_Parser) {
        fprintf(stderr, "[Parser] Out of memory\n");
        
        exit(1);
    }

    return _Parser;
}

static char *XStrndup(const char *String, size_t Len) {
    char *StringMalloc = (char *) XMalloc(Len + 1);
    
    memcpy(StringMalloc, String, Len);
    
    StringMalloc[Len] = '\0';

    return StringMalloc;
}

#if PARSER_TRACE

    static void TraceEnter(const char *FunctionName, Parser *_Parser) {
        Token *_Token = _Parser -> Current;

        fprintf(stderr, "[TRACE >>] %-30s  tok=%-20s  line=%u\n", FunctionName, _Token ? TokenTypeToString(_Token -> Type) : "NULL", _Token ? _Token -> Line : 0);
    }

    static void TraceExit(const char *FunctionName, Parser *_Parser) {
        Token *_Token = _Parser -> Current;

        fprintf(stderr, "[TRACE <<] %-30s  tok=%-20s  line=%u\n", FunctionName, _Token ? TokenTypeToString(_Token -> Type) : "NULL", _Token ? _Token -> Line : 0);
    }

    static void TraceToken(const char *Action, Parser *_Parser) {
        Token *_Token = _Parser -> Current;

        fprintf(stderr, "[TRACE --] %-20s tok=%-20s  line=%u\n", Action, _Token ? TokenTypeToString(_Token -> Type) : "NULL", _Token ? _Token -> Line : 0);
    }

#else

    static void TraceEnter(const char *FunctionName, Parser *_Parser) {}
    static void TraceExit (const char *FunctionName, Parser *_Parser) {}
    static void TraceToken(const char *Action, Parser *_Parser) {}

#endif

int IsSyncToken(TokenType Type) {
    switch (Type) {
        case TOKEN_FUNCTION:
        case TOKEN_IF:
        case TOKEN_FOR:
        case TOKEN_WHILE:
        case TOKEN_RETURN:
        case TOKEN_MATCH:
        case TOKEN_UNSAFE:
        case TOKEN_TRUSTED:
        case TOKEN_SAFE:
        case TOKEN_DEFER:
        case TOKEN_CHECK:
        case TOKEN_ASSUME:
        case TOKEN_REGION:
        case TOKEN_TRANSACTION:
        case TOKEN_COMPTIME:
        case TOKEN_MODULE:
        case TOKEN_CONTEXT:
        case TOKEN_WORLD:
        case TOKEN_ENUM:
        case TOKEN_STATE:
        case TOKEN_STRUCT:
        case TOKEN_PARTIAL:
        case TOKEN_MACRO:
        case TOKEN_ENV:
        case TOKEN_EOF: return 1;

        default: return 0;
    }
}

static void ParserError(Parser *_Parser, const char *Message) {
    if (_Parser -> PanicMode) return;

    _Parser -> HasError = 1;
    _Parser -> PanicMode = 1;

    Token *_Token = _Parser -> Current;

    fprintf(stderr, "[Parser] Error at line %u:%u — %s", _Token ? _Token -> Line : 0, _Token ? _Token -> Column : 0, Message);

    if (_Token && _Token -> Start && _Token -> Length > 0)
        fprintf(stderr, " (near '%.*s')", (int) _Token -> Length, _Token -> Start);

    fprintf(stderr, "\n");
}

static void ParsingError(Parser *_Parser, const char *Format, ...) {
    if (_Parser -> PanicMode)
        return;

    char Buffer[512];

    va_list Arguments;

    va_start(Arguments, Format);
    vsnprintf(Buffer, sizeof(Buffer), Format, Arguments);
    va_end(Arguments);

    ParserError(_Parser, Buffer);
}

static void Synchronise(Parser *_Parser) {
    _Parser -> PanicMode = 0;

    while (_Parser -> Current && _Parser -> Current -> Type != TOKEN_EOF) {
        if (IsSyncToken(_Parser -> Current -> Type))
            return;

        ParserAdvance(_Parser);
    }
}

Parser CreateParser(TokenStream *Tokens) {
    Parser _Parser = {0};

    _Parser.Tokens = Tokens;
    _Parser.Current = &Tokens -> Data[0];
    _Parser.Previous = NULL;

    return _Parser;
}

Token *ParserAdvance(Parser *_Parser) {
    _Parser -> Previous = _Parser -> Current;

    TokenStream *Tokens = _Parser -> Tokens;

    if (Tokens -> Cursor < Tokens -> Count)
        Tokens -> Cursor++;

    _Parser -> Current = (Tokens -> Cursor < Tokens -> Count) ? &Tokens -> Data[Tokens -> Cursor] : &Tokens -> Data[Tokens -> Count - 1];

    TraceToken("advance", _Parser);

    return _Parser -> Previous;
}

Token *ParserPeek(Parser *_Parser) {
    return _Parser -> Current;
}

Token *ParserPrevious(Parser *_Parser) {
    return _Parser -> Previous;
}

static Token *PeekNext(Parser *_Parser) {
    TokenStream *Tokens = _Parser -> Tokens;

    size_t Next = Tokens -> Cursor + 1;

    if (Next < Tokens -> Count)
        return &Tokens -> Data[Next];

    return &Tokens -> Data[Tokens -> Count - 1];
}

static Token *PeekNextNext(Parser *_Parser) {
    TokenStream *Tokens = _Parser -> Tokens;

    size_t Next = Tokens -> Cursor + 2;

    if (Next < Tokens -> Count)
        return &Tokens -> Data[Next];
        
    return &Tokens -> Data[Tokens -> Count - 1];
}

int ParserCheck(Parser *_Parser, TokenType Type) {
    return _Parser -> Current && _Parser -> Current -> Type == Type;
}

int ParserCheckNext(Parser *_Parser, TokenType Type) {
    Token *Size = PeekNext(_Parser);

    return Size && Size -> Type == Type;
}

int ParserMatch(Parser *_Parser, TokenType Type) {
    if (ParserCheck(_Parser, Type)) {
        ParserAdvance(_Parser);

        return 1;
    }

    return 0;
}

static Token *Expect(Parser *_Parser, TokenType Type, const char *What) {
    if (!ParserCheck(_Parser, Type)) {
        ParsingError(_Parser, "expected %s, got '%s'", What, TokenTypeToString(_Parser -> Current -> Type));

        return _Parser -> Current;
    }

    return ParserAdvance(_Parser);
}

int GetOperatorPrecedence(TokenType Type) {
    switch (Type) {
        case TOKEN_OR: return 1;
        case TOKEN_AND: return 2;
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_NE: return 3;
        case TOKEN_LT:
        case TOKEN_LE:
        case TOKEN_GT:
        case TOKEN_GE: return 4;
        case TOKEN_PLUS:
        case TOKEN_MINUS: return 5;
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT: return 6;

        default: return -1;
    }
}

ASTOperator TokenToOperator(TokenType Type) {
    switch (Type) {
        case TOKEN_PLUS: return OP_ADD;
        case TOKEN_MINUS: return OP_SUB;
        case TOKEN_STAR: return OP_MUL;
        case TOKEN_SLASH: return OP_DIV;
        case TOKEN_PERCENT: return OP_MOD;
        case TOKEN_AND: return OP_AND;
        case TOKEN_OR: return OP_OR;
        case TOKEN_EXCLAMATION: return OP_NOT;
        case TOKEN_EQUAL_EQUAL: return OP_EQ;
        case TOKEN_NE: return OP_NE;
        case TOKEN_LT: return OP_LT;
        case TOKEN_LE: return OP_LE;
        case TOKEN_GT: return OP_GT;
        case TOKEN_GE: return OP_GE;

        default: return OP_ADD;
    }
}

static ASTExpression *NewExpression(ASTExpressionKind Kind) {
    ASTExpression *Expression = (ASTExpression *) XCalloc(1, sizeof(ASTExpression));

    Expression -> Kind = Kind;

    return Expression;
}

static ASTStatement *NewStatement(ASTStmtKind Kind) {
    ASTStatement *String = (ASTStatement *) XCalloc(1, sizeof(ASTStatement));

    String -> Kind = Kind;

    return String;
}

static ASTType *NewType(ASTTypeKind Kind) {
    ASTType *_Token = (ASTType *) XCalloc(1, sizeof(ASTType));

    _Token -> Kind = Kind;

    return _Token;
}

ASTType *ParseType(Parser *_Parser) {
    TraceEnter("ParseType", _Parser);

    if (ParserMatch(_Parser, TOKEN_NOALIAS)) {
        ASTType *Inner = ParseType(_Parser);

        TraceExit("ParseType", _Parser);

        return Inner;
    }

    ASTType *_Token = NULL;

    if (ParserMatch(_Parser, TOKEN_LBRACKET)) {
        ASTType *Element = ParseType(_Parser);

        _Token = NewType(TYPE_ARRAY);
        _Token -> ElementType = Element;

        if (ParserMatch(_Parser, TOKEN_COMMA)) {
            Token *TokenSize = Expect(_Parser, TOKEN_INT_LITERAL, "array size");

            _Token -> ArraySize = (size_t) TokenSize -> Literal.Int;
            _Token -> IsSlice = 0;
        } else {
            _Token -> IsSlice = 1;
            _Token -> ArraySize = 0;
            _Token -> Kind = TYPE_SLICE;
        }

        Expect(_Parser, TOKEN_RBRACKET, "']'");
    } else if (ParserMatch(_Parser, TOKEN_INT)) {
        _Token = NewType(TYPE_INT);
    } else if (ParserMatch(_Parser, TOKEN_FLOAT)) {
        _Token = NewType(TYPE_FLOAT);
    } else if (ParserMatch(_Parser, TOKEN_BOOL)) {
        _Token = NewType(TYPE_BOOL);
    } else if (ParserMatch(_Parser, TOKEN_STRING)) {
        _Token = NewType(TYPE_STRING);
    } else if (ParserMatch(_Parser, TOKEN_VOID)) {
        _Token = NewType(TYPE_VOID);
    } else if (ParserMatch(_Parser, TOKEN_IDENTIFIER)) {
        _Token = NewType(TYPE_NAMED);

        Token *Previous = ParserPrevious(_Parser);

        _Token -> Name = XStrndup(Previous -> Start, Previous -> Length);
    } else {
        ParsingError(_Parser, "expected a type, got '%s'", TokenTypeToString(_Parser -> Current -> Type));

        _Token = NewType(TYPE_VOID);
    }

    TraceExit("ParseType", _Parser);

    return _Token;
}

ASTExpression *ParsePrimary(Parser *_Parser) {
    TraceEnter("ParsePrimary", _Parser);

    ASTExpression *Expression = NULL;

    if (ParserCheck(_Parser, TOKEN_AMPERSAND) || ParserCheck(_Parser, TOKEN_AT) || ParserCheck(_Parser, TOKEN_HASH) || ParserCheck(_Parser, TOKEN_EXCLAMATION) || ParserCheck(_Parser, TOKEN_DOLLAR)) {
        Expression = ParseOwnership(_Parser);

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_MINUS) || ParserMatch(_Parser, TOKEN_EXCLAMATION)) {
        Token *Op = ParserPrevious(_Parser);
        ASTExpression *Operand = ParseUnary(_Parser);

        Expression = NewExpression(EXPR_UNARY);
        Expression -> Unary.Op = (Op->Type == TOKEN_MINUS) ? OP_SUB : OP_NOT;
        Expression -> Unary.Operand = Operand;

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_INT_LITERAL)) {
        Token *_Token = ParserPrevious(_Parser);

        Expression = NewExpression(EXPR_LITERAL);
        Expression -> Literal.LiteralKind = TYPE_INT;
        Expression -> Literal.Int = _Token -> Literal.Int;

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_FLOAT_LITERAL)) {
        Token *_Token = ParserPrevious(_Parser);

        Expression = NewExpression(EXPR_LITERAL);
        Expression -> Literal.LiteralKind = TYPE_FLOAT;
        Expression -> Literal.Float = _Token -> Literal.Float;

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_STRING_LITERAL)) {
        Token *_Token = ParserPrevious(_Parser);

        Expression = NewExpression(EXPR_LITERAL);
        Expression -> Literal.LiteralKind = TYPE_STRING;
        Expression -> Literal.String = XStrndup(_Token -> Start, _Token -> Length);

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_CHAR_LITERAL)) {
        Token *_Token = ParserPrevious(_Parser);

        Expression = NewExpression(EXPR_LITERAL);
        Expression -> Literal.LiteralKind = TYPE_CHAR;
        Expression -> Literal.Char = (char)_Token -> Literal.Int;

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_BOOL_LITERAL)) {
        Token *_Token = ParserPrevious(_Parser);

        Expression = NewExpression(EXPR_LITERAL);
        Expression -> Literal.LiteralKind = TYPE_BOOL;
        Expression -> Literal.Bool = (_Token -> Length == 4 && memcmp(_Token -> Start, "true", 4) == 0) ? 1 : 0;

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_NONE)) {
        Expression = NewExpression(EXPR_LITERAL);
        Expression -> Literal.LiteralKind = TYPE_VOID;

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_LPAREN)) {
        Expression = ParseExpression(_Parser);

        Expect(_Parser, TOKEN_RPAREN, "')'");
        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserCheck(_Parser, TOKEN_LBRACE)) {
        Expression = ParseArrayLiteral(_Parser);

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_IDENTIFIER) || ParserMatch(_Parser, TOKEN_IDENTIFIER_SPECIAL)) {
        Token *_Token = ParserPrevious(_Parser);

        Expression = NewExpression(EXPR_IDENTIFIER);
        Expression -> Identifier = XStrndup(_Token -> Start, _Token -> Length);

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    if (ParserMatch(_Parser, TOKEN_SYS)) {
        Expression = NewExpression(EXPR_IDENTIFIER);
        Expression -> Identifier = "sys";

        TraceExit("ParsePrimary", _Parser);

        return Expression;
    }

    ParsingError(_Parser, "unexpected token '%s' in expression", TokenTypeToString(_Parser -> Current -> Type));

    Expression = NewExpression(EXPR_LITERAL);
    Expression -> Literal.LiteralKind = TYPE_VOID;

    ParserAdvance(_Parser);

    TraceExit("ParsePrimary", _Parser);

    return Expression;
}

static ASTExpression *ParseOwnership(Parser *_Parser) {
    TraceEnter("ParseOwnership", _Parser);

    ASTOwnershipKind Kind = OWN_MOVE;

    int HasFree = 0;

    if (ParserCheck(_Parser, TOKEN_EXCLAMATION) && ParserCheckNext(_Parser, TOKEN_DOLLAR)) {
        ParserAdvance(_Parser); ParserAdvance(_Parser);

        HasFree = 1;
        Kind = OWN_MOVE;
    } else if (ParserMatch(_Parser, TOKEN_DOLLAR)) {
        ASTExpression *Target = ParsePrimary(_Parser);
        ASTExpression *Expression = NewExpression(EXPR_OWNERSHIP);

        Expression -> Ownership.Kind = OWN_MOVE;
        Expression -> Ownership.Target = Target;
        Expression -> Metadata.OwnershipState = -1;

        TraceExit("ParseOwnership", _Parser);

        return Expression;
    } else if (ParserMatch(_Parser, TOKEN_AMPERSAND)) {
        Kind = OWN_BORROW;
    } else if (ParserMatch(_Parser, TOKEN_AT)) {
        Kind = OWN_COPY;
    } else if (ParserMatch(_Parser, TOKEN_HASH)) {
        Kind = OWN_ADDRESS;
    }

    ASTExpression *Target = ParsePrimary(_Parser);

    if (ParserMatch(_Parser, TOKEN_DOT_DOT)) {
        Kind = OWN_TRAIL;
    }

    ASTExpression *Expression = NewExpression(EXPR_OWNERSHIP);

    Expression -> Ownership.Kind = Kind;
    Expression -> Ownership.Target = Target;

    if (HasFree)
        Expression -> Metadata.OwnershipState = -2;

    TraceExit("ParseOwnership", _Parser);

    return Expression;
}

ASTExpression *ParsePostfix(Parser *_Parser) {
    TraceEnter("ParsePostfix", _Parser);

    ASTExpression *Expression = ParsePrimary(_Parser);

    for (;;) {
        if (ParserCheck(_Parser, TOKEN_LBRACE) &&
            Expression -> Kind == EXPR_IDENTIFIER) {

            Token *LookAhead1 = PeekNext(_Parser);
            
            TokenStream *Tokens = _Parser -> Tokens;
            size_t SavedCursor = Tokens -> Cursor;

            Token *Inside = (Tokens -> Cursor + 1 < Tokens -> Count) ? &Tokens -> Data[Tokens -> Cursor + 1] : &Tokens -> Data[Tokens -> Count - 1];
            Token *AfterInside = (Tokens -> Cursor + 2 < Tokens -> Count) ? &Tokens -> Data[Tokens -> Cursor + 2] : &Tokens -> Data[Tokens -> Count - 1];

            int IsStructInit = (Inside -> Type == TOKEN_IDENTIFIER && AfterInside -> Type == TOKEN_COLON);
            if (IsStructInit) {
                ParserAdvance(_Parser);
                
                size_t Cap = 8, Count = 0;

                ASTExpression **Fields = (ASTExpression **) XMalloc(sizeof(ASTExpression *) * Cap);

                while (!ParserCheck(_Parser, TOKEN_RBRACE) &&
                       !ParserCheck(_Parser, TOKEN_EOF)) {

                    if (ParserCheck(_Parser, TOKEN_IDENTIFIER))
                        ParserAdvance(_Parser);

                    ParserMatch(_Parser, TOKEN_COLON);

                    if (Count >= Cap) {
                        Cap *= 2;

                        Fields = (ASTExpression **) realloc(Fields, sizeof(ASTExpression *) * Cap);
                    }

                    Fields[Count++] = ParseExpression(_Parser);

                    if (!ParserMatch(_Parser, TOKEN_COMMA)) break;
                }

                Expect(_Parser, TOKEN_RBRACE, "'}'");

                ASTExpression *Call = NewExpression(EXPR_CALL);

                Call -> Call.Callee = Expression;
                Call -> Call.Arguments = Fields;
                Call -> Call.ArgumentCount = Count;

                Expression = Call;

                continue;
            }
        }

        if (ParserMatch(_Parser, TOKEN_LPAREN)) {
            Expression = ParseCallOrAccess(_Parser, Expression);
        } else if (ParserMatch(_Parser, TOKEN_LBRACKET)) {
            ASTExpression *Index = ParseExpression(_Parser);

            Expect(_Parser, TOKEN_RBRACKET, "']'");

            ASTExpression *IndexExpression = NewExpression(EXPR_INDEX);

            IndexExpression -> Index.Target = Expression;
            IndexExpression -> Index.Index  = Index;

            Expression = IndexExpression;
        } else if (ParserMatch(_Parser, TOKEN_DOT)) {
            Token *Member = Expect(_Parser, TOKEN_IDENTIFIER, "member name");
            ASTExpression *MemberExpression = NewExpression(EXPR_IDENTIFIER);

            MemberExpression -> Identifier = XStrndup(Member -> Start, Member -> Length);

            ASTExpression *BinaryExpression = NewExpression(EXPR_BINARY);

            BinaryExpression -> Binary.Left = Expression;
            BinaryExpression -> Binary.Right = MemberExpression;
            BinaryExpression -> Binary.Op = OP_ADD;
            BinaryExpression -> Binary.IsAssignment = 0;
            BinaryExpression -> Metadata.IsLValue = 1;

            Expression = BinaryExpression;
        } else if (ParserMatch(_Parser, TOKEN_AT)) {
            Token *Index = Expect(_Parser, TOKEN_INT_LITERAL, "history index");
            ASTExpression *IndexLiteral = NewExpression(EXPR_LITERAL);

            IndexLiteral -> Literal.LiteralKind = TYPE_INT;
            IndexLiteral -> Literal.Int = Index -> Literal.Int;

            if (ParserCheck(_Parser, TOKEN_IDENTIFIER)) {
                Token *Branch = _Parser -> Current;
                if (Branch -> Length >= 2 && Branch -> Start[0] == 'b')
                    ParserAdvance(_Parser);
            }

            ASTExpression *IndexExpression = NewExpression(EXPR_INDEX);

            IndexExpression -> Index.Target = Expression;
            IndexExpression -> Index.Index = IndexLiteral;
            IndexExpression -> Metadata.OwnershipState = 1;

            Expression = IndexExpression;
        } else if (ParserMatch(_Parser, TOKEN_DOT_DOT)) {
            ASTExpression *OwnershipExpression = NewExpression(EXPR_OWNERSHIP);

            OwnershipExpression -> Ownership.Kind = OWN_TRAIL;
            OwnershipExpression -> Ownership.Target = Expression;

            Expression = OwnershipExpression;
        } else {
            break;
        }
    }

    TraceExit("ParsePostfix", _Parser);

    return Expression;
}

static ASTExpression *ParseCallOrAccess(Parser *_Parser, ASTExpression *Base) {
    TraceEnter("ParseCallOrAccess", _Parser);

    ASTExpression *Expression = NewExpression(EXPR_CALL);

    Expression -> Call.Callee = Base;

    size_t Cap = 8;
    size_t Count = 0;

    ASTExpression **Arguments = (ASTExpression **) XMalloc(sizeof(ASTExpression *) * Cap);

    while (!ParserCheck(_Parser, TOKEN_RPAREN) && !ParserCheck(_Parser, TOKEN_EOF)) {
        if (Count >= Cap) {
            Cap *= 2;

            Arguments = (ASTExpression **) realloc(Arguments, sizeof(ASTExpression *) * Cap);
        }

        Arguments[Count++] = ParseExpression(_Parser);

        if (!ParserMatch(_Parser, TOKEN_COMMA))
            break;
    }

    Expect(_Parser, TOKEN_RPAREN, "')'");

    Expression -> Call.Arguments = Arguments;
    Expression -> Call.ArgumentCount = Count;

    TraceExit("ParseCallOrAccess", _Parser);

    return Expression;
}

ASTExpression *ParseUnary(Parser *_Parser) {
    TraceEnter("ParseUnary", _Parser);

    if (ParserMatch(_Parser, TOKEN_EXCLAMATION) || ParserMatch(_Parser, TOKEN_MINUS)) {
        Token *Op = ParserPrevious(_Parser);
        ASTExpression *Operand = ParseUnary(_Parser);
        ASTExpression *Expression = NewExpression(EXPR_UNARY);

        Expression -> Unary.Op = (Op -> Type == TOKEN_MINUS) ? OP_SUB : OP_NOT;
        Expression -> Unary.Operand = Operand;

        TraceExit("ParseUnary", _Parser);

        return Expression;
    }

    ASTExpression *Expression = ParsePostfix(_Parser);

    TraceExit("ParseUnary", _Parser);

    return Expression;
}

ASTExpression *ParseBinary(Parser *_Parser, int MinPrec) {
    TraceEnter("ParseBinary", _Parser);

    ASTExpression *Left = ParseUnary(_Parser);

    Left = ParseBinaryFromLeft(_Parser, Left, MinPrec);

    TraceExit("ParseBinary", _Parser);

    return Left;
}

ASTExpression *ParseBinaryFromLeft(Parser *_Parser, ASTExpression *Left, int MinimumPrecedence) {
    for (;;) {
        TokenType OpToken = _Parser -> Current -> Type;

        int Precedence = GetOperatorPrecedence(OpToken);
        if (Precedence < MinimumPrecedence)
            break;

        ParserAdvance(_Parser);

        ASTExpression *Right = ParseUnary(_Parser);

        Right = ParseBinaryFromLeft(_Parser, Right, Precedence + 1);

        ASTExpression *Expression = NewExpression(EXPR_BINARY);

        Expression -> Binary.Left = Left;
        Expression -> Binary.Right = Right;
        Expression -> Binary.Op = TokenToOperator(OpToken);

        Left = Expression;
    }

    return Left;
}

ASTExpression *ParseExpression(Parser *_Parser) {
    TraceEnter("ParseExpression", _Parser);

    ASTExpression *Expression = ParseBinary(_Parser, 0);

    TraceExit("ParseExpression", _Parser);

    return Expression;
}

ASTExpression *ParseArrayLiteral(Parser *_Parser) {
    TraceEnter("ParseArrayLiteral", _Parser);

    Expect(_Parser, TOKEN_LBRACE, "'{'");

    size_t Cap = 8;
    size_t Count = 0;

    ASTExpression **Elements = (ASTExpression **) XMalloc(sizeof(ASTExpression *) * Cap);

    while (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF)) {
        if (Count >= Cap) {
            Cap *= 2;

            Elements = (ASTExpression **) realloc(Elements, sizeof(ASTExpression *) * Cap);
        }

        Elements[Count++] = ParseExpression(_Parser);

        if (!ParserMatch(_Parser, TOKEN_COMMA))
            break;
    }

    Expect(_Parser, TOKEN_RBRACE, "'}'");

    ASTExpression *Expression = NewExpression(EXPR_ARRAY);

    Expression -> Array.Elements = Elements;
    Expression -> Array.Count = Count;

    TraceExit("ParseArrayLiteral", _Parser);

    return Expression;
}

ASTStatement *ParseVariableDeclaration(Parser *_Parser, int Modifiers) {
    TraceEnter("ParseVariableDeclaration", _Parser);

    const char *Name = NULL;
    ASTType *Type = NULL;

    Token *First = _Parser -> Current;

    int IsBuiltinType = (First -> Type == TOKEN_INT || First -> Type == TOKEN_FLOAT || First -> Type == TOKEN_BOOL || First -> Type == TOKEN_STRING || First -> Type == TOKEN_VOID);

    if (IsBuiltinType) {
        Type = ParseType(_Parser);

        Token *NameToken = Expect(_Parser, TOKEN_IDENTIFIER, "variable name");

        Name = XStrndup(NameToken -> Start, NameToken -> Length);
    } else {
        Token *NameToken = Expect(_Parser, TOKEN_IDENTIFIER, "variable name");

        Name = XStrndup(NameToken -> Start, NameToken -> Length);

        if (ParserMatch(_Parser, TOKEN_COLON)) {
            Type = ParseType(_Parser);
        }
    }

    ASTExpression *Initializer = NULL;

    if (ParserMatch(_Parser, TOKEN_EQUAL)) {
        Initializer = ParseExpression(_Parser);
    }

    ASTStatement *String = NewStatement(STMT_VAR_DECL);

    String -> VariableDeclaration.Name = Name;
    String -> VariableDeclaration.Type = Type;
    String -> VariableDeclaration.Initializer = Initializer;
    String -> VariableDeclaration.Modifiers = (uint32_t)Modifiers;

    TraceExit("ParseVariableDeclaration", _Parser);

    return String;
}

static uint32_t ParseModifiers(Parser *_Parser) {
    uint32_t Mods = MOD_NONE;

    for (;;) {
        if (ParserMatch(_Parser, TOKEN_GLOBAL)) Mods |= MOD_GLOBAL;
        else if (ParserMatch(_Parser, TOKEN_STATIC))
            Mods |= MOD_STATIC;
        else if (ParserMatch(_Parser, TOKEN_CONST))
            Mods |= MOD_CONST;
        else if (ParserMatch(_Parser, TOKEN_EXPORT))
            Mods |= MOD_EXPORT;
        else if (ParserMatch(_Parser, TOKEN_PRIVATE))
            Mods |= MOD_PRIVATE;
        else if (ParserMatch(_Parser, TOKEN_SILENT))
            Mods |= MOD_SILENT;
        else if (ParserMatch(_Parser, TOKEN_LINEAR)) {
            Mods |= MOD_LINER;
        } else if (ParserMatch(_Parser, TOKEN_HISTORY)) {
            Mods |= MOD_HISTORY;
        } else if (ParserMatch(_Parser, TOKEN_SYMBOLIC)) {
            Mods |= MOD_SYMBOLIC;
        } else break;
    }

    return Mods;
}

ASTStatement *ParseBlock(Parser *_Parser) {
    TraceEnter("ParseBlock", _Parser);

    Expect(_Parser, TOKEN_LBRACE, "'{'");

    _Parser -> ScopeDepth++;

    size_t Cap = 16;
    size_t Count = 0;

    ASTStatement **Statements = (ASTStatement **) XMalloc(sizeof(ASTStatement *) * Cap);

    while (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF)) {
        ASTStatement *String = ParseStatement(_Parser);

        if (!String) {
            continue;
        }

        if (Count >= Cap) {
            Cap *= 2;

            Statements = (ASTStatement **) realloc(Statements, sizeof(ASTStatement *) * Cap);
        }

        Statements[Count++] = String;
    }

    Expect(_Parser, TOKEN_RBRACE, "'}'");

    _Parser -> ScopeDepth--;

    ASTStatement *Block = NewStatement(STMT_BLOCK);

    Block -> Block.Statements = Statements;
    Block -> Block.Count = Count;

    TraceExit("ParseBlock", _Parser);

    return Block;
}

static ASTStatement *ParseConditionBlock(Parser *_Parser, ASTStmtKind Kind) {
    Expect(_Parser, TOKEN_LBRACE, "'{'");

    ASTExpression *Condition = ParseExpression(_Parser);

    Expect(_Parser, TOKEN_RBRACE, "'}'");

    ASTStatement *String = NewStatement(Kind);

    String -> ConditionBlock.Condition = Condition;

    return String;
}

ASTStatement *ParseIfStatement(Parser *_Parser) {
    TraceEnter("ParseIfStatement", _Parser);

    ASTExpression *Condition = ParseExpression(_Parser);

    ASTStatement **ThenStmts;
    size_t ThenCount;

    if (ParserCheck(_Parser, TOKEN_LBRACE)) {
        ASTStatement *Then = ParseBlock(_Parser);

        ThenStmts = Then -> Block.Statements;
        ThenCount  = Then -> Block.Count;
    } else {
        ASTStatement *Single = ParseStatement(_Parser);

        ThenStmts = (ASTStatement **) XMalloc(sizeof(ASTStatement *));
        ThenStmts[0] = Single;
        ThenCount = 1;
    }

    ASTStatement **ElseStatements = NULL;
    size_t ElseCount = 0;

    if (ParserMatch(_Parser, TOKEN_ELSE)) {
        if (ParserCheck(_Parser, TOKEN_IF)) {
            ParserAdvance(_Parser);

            ASTStatement *ElseIf = ParseIfStatement(_Parser);

            ElseStatements = (ASTStatement **) XMalloc(sizeof(ASTStatement *));
            ElseStatements[0] = ElseIf;
            ElseCount = 1;
        } else if (ParserCheck(_Parser, TOKEN_LBRACE)) {
            ASTStatement *ElseBlock = ParseBlock(_Parser);

            ElseStatements = ElseBlock -> Block.Statements;
            ElseCount = ElseBlock -> Block.Count;
        } else {
            ASTStatement *Single = ParseStatement(_Parser);

            ElseStatements = (ASTStatement **) XMalloc(sizeof(ASTStatement *));
            ElseStatements[0] = Single;

            ElseCount = 1;
        }
    }

    ASTStatement *String = NewStatement(STMT_IF);

    String -> If.Condition = Condition;
    String -> If.ThenBlock = ThenStmts;
    String -> If.ThenCount = ThenCount;
    String -> If.ElseBlock = ElseStatements;
    String -> If.ElseCount = ElseCount;

    TraceExit("ParseIfStatement", _Parser);

    return String;
}

ASTStatement *ParseWhileStatement(Parser *_Parser) {
    TraceEnter("ParseWhileStatement", _Parser);

    _Parser -> InLoop++;

    ASTExpression *Condition = ParseExpression(_Parser);
    ASTStatement *Body = ParseBlock(_Parser);

    _Parser -> InLoop--;

    ASTStatement *String = NewStatement(STMT_WHILE);

    String -> While.Condition = Condition;
    String -> While.Body = Body -> Block.Statements;
    String -> While.Count = Body -> Block.Count;

    TraceExit("ParseWhileStatement", _Parser);

    return String;
}

ASTStatement *ParseForStatement(Parser *_Parser) {
    TraceEnter("ParseForStatement", _Parser);

    Token *IterationToken = Expect(_Parser, TOKEN_IDENTIFIER, "iterator name");
    const char *Iterator = XStrndup(IterationToken -> Start, IterationToken -> Length);

    Expect(_Parser, TOKEN_EQUAL, "'='");

    ASTExpression *Start = ParseExpression(_Parser);

    Expect(_Parser, TOKEN_COMMA, "','");

    ASTExpression *Condition = ParseExpression(_Parser);

    Expect(_Parser, TOKEN_COMMA, "','");

    ASTExpression *StepLHS = ParseExpression(_Parser);
    ASTExpression *Step;

    if (ParserMatch(_Parser, TOKEN_PLUS_EQUAL) || ParserMatch(_Parser, TOKEN_MINUS_EQUAL)) {
        TokenType OpToken = ParserPrevious(_Parser) -> Type;
        ASTExpression *RHS = ParseExpression(_Parser);
        ASTExpression *Compound = NewExpression(EXPR_BINARY);

        Compound -> Binary.Left  = StepLHS;
        Compound -> Binary.Right = RHS;
        Compound -> Binary.Op = (OpToken == TOKEN_PLUS_EQUAL) ? OP_ADD : OP_SUB;

        Step = Compound;
    } else {
        Step = StepLHS;
    }

    _Parser -> InLoop++;

    ASTStatement *Body = ParseBlock(_Parser);

    _Parser -> InLoop--;

    ASTStatement *String = NewStatement(STMT_FOR);

    String -> For.Iterator = Iterator;
    String -> For.Start = Start;
    String -> For.End = Condition;
    String -> For.Step = Step;
    String -> For.Body = Body -> Block.Statements;
    String -> For.Count = Body -> Block.Count;

    TraceExit("ParseForStatement", _Parser);

    return String;
}

ASTStatement *ParseReturnStatement(Parser *_Parser) {
    TraceEnter("ParseReturnStatement", _Parser);

    ASTExpression *Value = NULL;

    if (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF) && !ParserCheck(_Parser, TOKEN_SEMICOLON)) {
        Value = ParseExpression(_Parser);
    }

    ParserMatch(_Parser, TOKEN_SEMICOLON);

    ASTStatement *String = NewStatement(STMT_RETURN);

    String -> Return.Value = Value;

    TraceExit("ParseReturnStatement", _Parser);

    return String;
}

static ASTStatement *ParseMatchStatement(Parser *_Parser) {
    TraceEnter("ParseMatchStatement", _Parser);

    _Parser -> InMatch++;

    ASTExpression *Target = ParseExpression(_Parser);

    Expect(_Parser, TOKEN_LBRACE, "'{'");

    size_t ArmCap = 8;
    size_t ArmCount = 0;

    ASTMatchArm *Arms = (ASTMatchArm *) XMalloc(sizeof(ASTMatchArm) * ArmCap);

    while (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF)) {
        ASTMatchArm Arm = {0};

        if (ParserCheck(_Parser, TOKEN_LPAREN)) {
            ParserAdvance(_Parser); 

            size_t ElementCap = 4, ElemCount = 0;
            ASTExpression **Elements = (ASTExpression **) XMalloc(sizeof(ASTExpression *) * ElementCap);

            while (!ParserCheck(_Parser, TOKEN_RPAREN) && !ParserCheck(_Parser, TOKEN_EOF)) {
                if (ElemCount >= ElementCap) {
                    ElementCap *= 2;

                    Elements = (ASTExpression **) realloc(Elements, sizeof(ASTExpression *) * ElementCap);
                }

                Elements[ElemCount++] = ParseExpression(_Parser);

                if (!ParserMatch(_Parser, TOKEN_COMMA))
                    break;
            }

            Expect(_Parser, TOKEN_RPAREN, "')'");

            ASTExpression *Tuple = NewExpression(EXPR_ARRAY);

            Tuple -> Array.Elements = Elements;
            Tuple -> Array.Count = ElemCount;

            Arm.Pattern = Tuple;
        } else {
            Arm.Pattern = ParseExpression(_Parser);
        }

        if (ParserMatch(_Parser, TOKEN_IF))
            Arm.Guard = ParseExpression(_Parser);

        Expect(_Parser, TOKEN_MATCH_ARROW, "'>>'");

        if (ParserCheck(_Parser, TOKEN_LBRACE)) {
            ASTStatement *Block = ParseBlock(_Parser);
            
            Arm.Body = Block -> Block.Statements;
            Arm.BodyCount = Block -> Block.Count;
        } else {
            ASTStatement *Expression = ParseExpressionStatement(_Parser);

            Arm.Body = (ASTStatement **) XMalloc(sizeof(ASTStatement *));
            Arm.Body[0] = Expression;
            Arm.BodyCount = 1;
        }

        ParserMatch(_Parser, TOKEN_COMMA);

        if (ArmCount >= ArmCap) {
            ArmCap *= 2;

            Arms = (ASTMatchArm *) realloc(Arms, sizeof(ASTMatchArm) * ArmCap);
        }

        Arms[ArmCount++] = Arm;
    }

    Expect(_Parser, TOKEN_RBRACE, "'}'");

    _Parser -> InMatch--;

    ASTStatement *String = NewStatement(STMT_MATCH);

    String -> Match.Target = Target;
    String -> Match.Arms = Arms;
    String -> Match.ArmCount = ArmCount;

    TraceExit("ParseMatchStatement", _Parser);

    return String;
}

static ASTStatement *ParseUnsafeBlock(Parser *_Parser) {
    TraceEnter("ParseUnsafeBlock", _Parser);

    _Parser -> InUnsafe++;

    ASTStatement *Body = ParseBlock(_Parser);

    _Parser -> InUnsafe--;

    ASTStatement *String = NewStatement(STMT_UNSAFE);

    String -> ScopedBlock.Body = Body -> Block.Statements;
    String -> ScopedBlock.Count = Body -> Block.Count;

    TraceExit("ParseUnsafeBlock", _Parser);

    return String;
}

static ASTStatement *ParseSafeBlock(Parser *_Parser) {
    TraceEnter("ParseSafeBlock", _Parser);

    ASTStatement *Body = ParseBlock(_Parser);

    ASTStatement *String = NewStatement(STMT_SAFE);
    String -> ScopedBlock.Body = Body -> Block.Statements;
    String -> ScopedBlock.Count = Body -> Block.Count;

    TraceExit("ParseSafeBlock", _Parser);

    return String;
}

static ASTStatement *ParseTrustedBlock(Parser *_Parser) {
    TraceEnter("ParseTrustedBlock", _Parser);

    ParserMatch(_Parser, TOKEN_UNSAFE);

    _Parser -> InUnsafe++;

    ASTStatement *Body = ParseBlock(_Parser);

    _Parser -> InUnsafe--;

    ASTStatement *String = NewStatement(STMT_TRUSTED);

    String -> ScopedBlock.Body = Body -> Block.Statements;
    String -> ScopedBlock.Count = Body -> Block.Count;

    TraceExit("ParseTrustedBlock", _Parser);

    return String;
}

static ASTStatement *ParseCheckBlock(Parser *_Parser) {
    return ParseConditionBlock(_Parser, STMT_CHECK);
}

static ASTStatement *ParseAssumeBlock(Parser *_Parser) {
    return ParseConditionBlock(_Parser, STMT_ASSUME);
}

static ASTStatement *ParseDeferBlock(Parser *_Parser) {
    TraceEnter("ParseDeferBlock", _Parser);

    ASTStatement *Body = ParseBlock(_Parser);
    ASTStatement *String = NewStatement(STMT_DEFER);

    String -> Defer.Body = Body -> Block.Statements;
    String -> Defer.Count = Body -> Block.Count;

    TraceExit("ParseDeferBlock", _Parser);

    return String;
}

static ASTStatement *ParseRegionBlock(Parser *_Parser) {
    TraceEnter("ParseRegionBlock", _Parser);

    if (ParserCheck(_Parser, TOKEN_IDENTIFIER))
        ParserAdvance(_Parser);

    ASTStatement *String = ParseBlock(_Parser);

    TraceExit("ParseRegionBlock", _Parser);

    return String;
}

static ASTStatement *ParseTransactionBlock(Parser *_Parser) {
    TraceEnter("ParseTransactionBlock", _Parser);

    ASTStatement *Body = ParseBlock(_Parser);

    TraceExit("ParseTransactionBlock", _Parser);

    return Body;
}

static ASTStatement *ParseReactiveBlock(Parser *_Parser) {
    TraceEnter("ParseReactiveBlock", _Parser);

    ASTStatement *Body = ParseBlock(_Parser);

    TraceExit("ParseReactiveBlock", _Parser);

    return Body;
}

static ASTStatement *ParseWithBlock(Parser *_Parser) {
    TraceEnter("ParseWithBlock", _Parser);

    if (ParserCheck(_Parser, TOKEN_IDENTIFIER))
        ParserAdvance(_Parser);

    ASTStatement *Body = ParseBlock(_Parser);

    TraceExit("ParseWithBlock", _Parser);

    return Body;
}

static ASTStatement *ParseComptimeBlock(Parser *_Parser) {
    TraceEnter("ParseComptimeBlock", _Parser);

    ASTStatement *Body = ParseBlock(_Parser);

    TraceExit("ParseComptimeBlock", _Parser);

    return Body;
}

static ASTStatement *ParseNaoBlock(Parser *_Parser) {
    TraceEnter("ParseNaoBlock", _Parser);

    Expect(_Parser, TOKEN_LBRACE, "'{'");

    int Depth = 1;

    while (!ParserCheck(_Parser, TOKEN_EOF)) {
        if (ParserMatch(_Parser, TOKEN_LBRACE))
            Depth++;
        else if (ParserMatch(_Parser, TOKEN_RBRACE)) {
            if (--Depth == 0)
                break;
        } else ParserAdvance(_Parser);
    }

    ASTStatement *String = NewStatement(STMT_BLOCK);

    String -> Block.Statements = NULL;
    String -> Block.Count = 0;

    TraceExit("ParseNaoBlock", _Parser);

    return String;
}

static ASTStatement *ParseConceptBlock(Parser *_Parser) {
    TraceEnter("ParseConceptBlock", _Parser);

    ASTStatement *Body = ParseBlock(_Parser);

    TraceExit("ParseConceptBlock", _Parser);

    return Body;
}

static ASTStatement *ParseExpressionStatement(Parser *_Parser) {
    TraceEnter("ParseExpressionStatement", _Parser);

    ASTExpression *Expression = ParseExpression(_Parser);

    if (ParserMatch(_Parser, TOKEN_EQUAL) || ParserMatch(_Parser, TOKEN_PLUS_EQUAL) || ParserMatch(_Parser, TOKEN_MINUS_EQUAL)) {
        TokenType OpToken = ParserPrevious(_Parser) -> Type;
        ASTExpression *RHS = ParseExpression(_Parser);

        if (OpToken != TOKEN_EQUAL) {
            ASTExpression *Compound = NewExpression(EXPR_BINARY);

            Compound -> Binary.Left = Expression;
            Compound -> Binary.Right = RHS;
            Compound -> Binary.Op = (OpToken == TOKEN_PLUS_EQUAL) ? OP_ADD : OP_SUB;

            RHS = Compound;
        }

        ASTStatement *String = NewStatement(STMT_ASSIGN);

        String -> Assign.Target = Expression;
        String -> Assign.Value = RHS;

        ParserMatch(_Parser, TOKEN_SEMICOLON);

        TraceExit("ParseExpressionStatement", _Parser);

        return String;
    }

    ParserMatch(_Parser, TOKEN_SEMICOLON);

    ASTStatement *String = NewStatement(STMT_EXPR);

    String -> Expression.Expression = Expression;

    TraceExit("ParseExpressionStatement", _Parser);

    return String;
}

ASTStatement *ParseControlBlock(Parser *_Parser) {
    TraceEnter("ParseControlBlock", _Parser);

    if (ParserMatch(_Parser, TOKEN_BREAK)) {
        TraceExit("ParseControlBlock", _Parser);
        
        return NewStatement(STMT_BREAK);
    }

    if (ParserMatch(_Parser, TOKEN_ROLLBACK)) {
        TraceExit("ParseControlBlock", _Parser);
        
        return NewStatement(STMT_BREAK);
    }

    TraceExit("ParseControlBlock", _Parser);

    return NULL;
}

ASTStatement *ParseStatement(Parser *_Parser) {
    TraceEnter("ParseStatement", _Parser);

    if (_Parser -> PanicMode) {
        Synchronise(_Parser);
        
        return NULL;
    }

    TokenType _Token = _Parser -> Current -> Type;

    if (_Token == TOKEN_GLOBAL || _Token == TOKEN_STATIC || _Token == TOKEN_CONST || _Token == TOKEN_EXPORT || _Token == TOKEN_PRIVATE || _Token == TOKEN_SILENT || _Token == TOKEN_LINEAR || _Token == TOKEN_HISTORY || _Token == TOKEN_SYMBOLIC) {
        uint32_t Mods = ParseModifiers(_Parser);
        ASTStatement *String = ParseVariableDeclaration(_Parser, (int)Mods);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (_Token == TOKEN_INT || _Token == TOKEN_FLOAT || _Token == TOKEN_BOOL || _Token == TOKEN_STRING || _Token == TOKEN_VOID) {
        ASTStatement *String = ParseVariableDeclaration(_Parser, MOD_NONE);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_IF)) {
        ASTStatement *String = ParseIfStatement(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_WHILE)) {
        ASTStatement *String = ParseWhileStatement(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_FOR)) {
        ASTStatement *String = ParseForStatement(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_RETURN)) {
        ASTStatement *String = ParseReturnStatement(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_BREAK)) {
        TraceExit("ParseStatement", _Parser);

        return NewStatement(STMT_BREAK);
    }

    if (ParserMatch(_Parser, TOKEN_MATCH)) {
        ASTStatement *String = ParseMatchStatement(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_UNSAFE)) {
        ASTStatement *String = ParseUnsafeBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_SAFE)) {
        ASTStatement *String = ParseSafeBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_TRUSTED)) {
        ASTStatement *String = ParseTrustedBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_CHECK)) {
        ASTStatement *String = ParseCheckBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_ASSUME)) {
        ASTStatement *String = ParseAssumeBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_DEFER)) {
        ASTStatement *String = ParseDeferBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_REGION)) {
        ASTStatement *String = ParseRegionBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_TRANSACTION)) {
        ASTStatement *String = ParseTransactionBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_ROLLBACK)) {
        TraceExit("ParseStatement", _Parser);

        return NewStatement(STMT_BREAK);
    }

    if (ParserMatch(_Parser, TOKEN_COMPTIME)) {
        if (ParserCheck(_Parser, TOKEN_FUNCTION)) {
            ParserAdvance(_Parser);
            
            ASTStatement *String = NewStatement(STMT_EXPR);

            String -> Expression.Expression = NewExpression(EXPR_LITERAL);

            TraceExit("ParseStatement", _Parser);

            return String;
        }

        ASTStatement *String = ParseComptimeBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserCheck(_Parser, TOKEN_IDENTIFIER) && ParserCheckNext(_Parser, TOKEN_COLON)) {
        Token *NameToken = ParserAdvance(_Parser);

        ParserAdvance(_Parser);

        ASTType *Type = ParseType(_Parser);

        ASTExpression *Initialization = NULL;
        if (ParserMatch(_Parser, TOKEN_EQUAL))
            Initialization = ParseExpression(_Parser);

        ASTStatement *Declaration = NewStatement(STMT_VAR_DECL);

        Declaration -> VariableDeclaration.Name = XStrndup(NameToken -> Start, NameToken -> Length);
        Declaration -> VariableDeclaration.Type = Type;
        Declaration -> VariableDeclaration.Initializer = Initialization;
        Declaration -> VariableDeclaration.Modifiers  = MOD_NONE;

        TraceExit("ParseStatement", _Parser);

        return Declaration;
    }

    if (ParserCheck(_Parser, TOKEN_IDENTIFIER) && ParserCheckNext(_Parser, TOKEN_IDENTIFIER)) {
        Token *Third = PeekNextNext(_Parser);

        if (Third -> Type == TOKEN_EQUAL || Third -> Type == TOKEN_SEMICOLON || Third -> Type == TOKEN_RBRACE || Third -> Type == TOKEN_EOF) {
            Token *TypeToken = ParserAdvance(_Parser);
            Token *NameToken = ParserAdvance(_Parser);

            ASTExpression *Initialization = NULL;
            if (ParserMatch(_Parser, TOKEN_EQUAL))
                Initialization = ParseExpression(_Parser);

            ASTType *Type = NewType(TYPE_NAMED);

            Type -> Name = XStrndup(TypeToken -> Start, TypeToken -> Length);

            ASTStatement *Declaration = NewStatement(STMT_VAR_DECL);

            Declaration -> VariableDeclaration.Name = XStrndup(NameToken -> Start, NameToken -> Length);
            Declaration -> VariableDeclaration.Type = Type;
            Declaration -> VariableDeclaration.Initializer = Initialization;
            Declaration -> VariableDeclaration.Modifiers = MOD_NONE;

            TraceExit("ParseStatement", _Parser);

            return Declaration;
        }
    }

    if (ParserMatch(_Parser, TOKEN_IDENTIFIER)) {
        Token *Previous = ParserPrevious(_Parser);

        if (Previous -> Length == 8 && memcmp(Previous -> Start, "reactive", 8) == 0) {
            ASTStatement *String = ParseReactiveBlock(_Parser);

            TraceExit("ParseStatement", _Parser);

            return String;
        }

        ASTExpression *Expression = NewExpression(EXPR_IDENTIFIER);

        Expression -> Identifier = XStrndup(Previous -> Start, Previous -> Length);

        for (;;) {
            if (ParserMatch(_Parser, TOKEN_LPAREN)) {
                Expression = ParseCallOrAccess(_Parser, Expression);
            } else if (ParserMatch(_Parser, TOKEN_LBRACKET)) {
                ASTExpression *Index = ParseExpression(_Parser);

                Expect(_Parser, TOKEN_RBRACKET, "']'");

                ASTExpression *IndexExpression = NewExpression(EXPR_INDEX);

                IndexExpression -> Index.Target = Expression;
                IndexExpression -> Index.Index  = Index;

                Expression = IndexExpression;
            } else if (ParserMatch(_Parser, TOKEN_DOT)) {
                Token *Member = Expect(_Parser, TOKEN_IDENTIFIER, "member name");
                ASTExpression *MemberExpression = NewExpression(EXPR_IDENTIFIER);

                MemberExpression -> Identifier = XStrndup(Member -> Start, Member -> Length);

                ASTExpression *BinaryExpression = NewExpression(EXPR_BINARY);

                BinaryExpression -> Binary.Left = Expression;
                BinaryExpression -> Binary.Right = MemberExpression;
                BinaryExpression -> Binary.Op = OP_ADD;
                BinaryExpression -> Metadata.IsLValue  = 1;

                Expression = BinaryExpression;
            } else if (ParserMatch(_Parser, TOKEN_AT)) {
                Token *Index = Expect(_Parser, TOKEN_INT_LITERAL, "history index");
                ASTExpression *IndexLiteral = NewExpression(EXPR_LITERAL);

                IndexLiteral -> Literal.LiteralKind = TYPE_INT;
                IndexLiteral -> Literal.Int = Index -> Literal.Int;

                if (ParserCheck(_Parser, TOKEN_IDENTIFIER)) {
                    Token *Br = _Parser -> Current;

                    if (Br -> Length >= 2 && Br -> Start[0] == 'b')
                        ParserAdvance(_Parser);
                }

                ASTExpression *IndexExpression = NewExpression(EXPR_INDEX);

                IndexExpression -> Index.Target = Expression;
                IndexExpression -> Index.Index = IndexLiteral;
                IndexExpression -> Metadata.OwnershipState = 1;

                Expression = IndexExpression;
            } else if (ParserMatch(_Parser, TOKEN_DOT_DOT)) {
                ASTExpression *OwnershipExpression = NewExpression(EXPR_OWNERSHIP);

                OwnershipExpression -> Ownership.Kind   = OWN_TRAIL;
                OwnershipExpression -> Ownership.Target = Expression;

                Expression = OwnershipExpression;
            } else {
                break;
            }
        }

        Expression = ParseBinaryFromLeft(_Parser, Expression, 0);

        if (ParserMatch(_Parser, TOKEN_EQUAL) || ParserMatch(_Parser, TOKEN_PLUS_EQUAL) || ParserMatch(_Parser, TOKEN_MINUS_EQUAL)) {
            TokenType OpToken = ParserPrevious(_Parser) -> Type;
            ASTExpression *RHS = ParseExpression(_Parser);

            if (OpToken != TOKEN_EQUAL) {
                ASTExpression *Compound = NewExpression(EXPR_BINARY);

                Compound -> Binary.Left  = Expression;
                Compound -> Binary.Right = RHS;
                Compound -> Binary.Op = (OpToken == TOKEN_PLUS_EQUAL) ? OP_ADD : OP_SUB;

                RHS = Compound;
            }

            ASTStatement *String = NewStatement(STMT_ASSIGN);

            String -> Assign.Target = Expression;
            String -> Assign.Value  = RHS;

            ParserMatch(_Parser, TOKEN_SEMICOLON);
            TraceExit("ParseStatement", _Parser);

            return String;
        }

        ParserMatch(_Parser, TOKEN_SEMICOLON);

        ASTStatement *String = NewStatement(STMT_EXPR);

        String -> Expression.Expression = Expression;

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    if (ParserMatch(_Parser, TOKEN_FUNCTION)) {
        ParseSubprogram(_Parser);

        ASTStatement *Stub = NewStatement(STMT_EXPR);

        Stub -> Expression.Expression = NewExpression(EXPR_LITERAL);

        TraceExit("ParseStatement", _Parser);

        return Stub;
    }

    if (ParserMatch(_Parser, TOKEN_WITH)) {
        ASTStatement *String = ParseWithBlock(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }

    {
        ASTStatement *String = ParseExpressionStatement(_Parser);

        TraceExit("ParseStatement", _Parser);

        return String;
    }
}

ASTSubprogram *ParseSubprogram(Parser *_Parser) {
    TraceEnter("ParseSubprogram", _Parser);

    ASTSubprogram *Subprogram = (ASTSubprogram *) XCalloc(1, sizeof(ASTSubprogram));

    Token *NameToken = Expect(_Parser, TOKEN_IDENTIFIER, "function name");

    Subprogram -> Name = XStrndup(NameToken -> Start, NameToken -> Length);

    _Parser -> CurrentFunction = Subprogram -> Name;
    _Parser -> FunctionDepth++;

    Expect(_Parser, TOKEN_LPAREN, "'('");

    size_t ParameterCap = 8;
    size_t ParameterCount = 0;
    
    ASTParam *Parameters = (ASTParam *) XMalloc(sizeof(ASTParam) * ParameterCap);

    while (!ParserCheck(_Parser, TOKEN_RPAREN) && !ParserCheck(_Parser, TOKEN_EOF)) {
        ASTParam Parameter = {0};
        
        Token *ParameterToken = Expect(_Parser, TOKEN_IDENTIFIER, "parameter name");

        Parameter.Name = XStrndup(ParameterToken -> Start, ParameterToken -> Length);

        if (ParserMatch(_Parser, TOKEN_COLON)) {
            Parameter.Type = ParseType(_Parser);
        } else {
            Parameter.Type = NULL;
        }

        if (ParameterCount >= ParameterCap) {
            ParameterCap *= 2;

            Parameters = (ASTParam *) realloc(Parameters, sizeof(ASTParam) * ParameterCap);
        }

        Parameters[ParameterCount++] = Parameter;

        if (!ParserMatch(_Parser, TOKEN_COMMA)) break;
    }

    Expect(_Parser, TOKEN_RPAREN, "')'");

    Subprogram -> Parameters = Parameters;
    Subprogram -> ParameterCount = ParameterCount;

    size_t RequiresCap = 4, RequiresCount = 0;
    size_t ProvidesCap = 4, ProvidesCount = 0;

    ASTEnvironment *Requires = (ASTEnvironment *) XMalloc(sizeof(ASTEnvironment) * RequiresCap);
    ASTEnvironment *Provides = (ASTEnvironment *) XMalloc(sizeof(ASTEnvironment) * ProvidesCap);

    for (;;) {
        if (ParserMatch(_Parser, TOKEN_REQUIRES)) {
            ParserMatch(_Parser, TOKEN_ENV);
            Token *EnvName = Expect(_Parser, TOKEN_IDENTIFIER, "environment name");

            ASTEnvironment Env = {0};
            Env.Name = XStrndup(EnvName -> Start, EnvName -> Length);

            if (RequiresCount >= RequiresCap) {
                RequiresCap *= 2;

                Requires = (ASTEnvironment *) realloc(Requires, sizeof(ASTEnvironment) * RequiresCap);
            }
            
            Requires[RequiresCount++] = Env;
        } else if (ParserMatch(_Parser, TOKEN_PROVIDES)) {
            ParserMatch(_Parser, TOKEN_ENV);
            Token *EnvName = Expect(_Parser, TOKEN_IDENTIFIER, "environment name");

            ASTEnvironment Env = {0};

            Env.Name = XStrndup(EnvName -> Start, EnvName -> Length);

            if (ParserMatch(_Parser, TOKEN_EQUAL))
                Env.Value = ParseExpression(_Parser);

            if (ProvidesCount >= ProvidesCap) {
                ProvidesCap *= 2;

                Provides = (ASTEnvironment *) realloc(Provides, sizeof(ASTEnvironment) * ProvidesCap);
            }
            
            Provides[ProvidesCount++] = Env;
        } else {
            break;
        }
    }

    Subprogram -> Requires = Requires;
    Subprogram -> RequireCount = RequiresCount;
    Subprogram -> Provides = Provides;
    Subprogram -> ProvideCount = ProvidesCount;

    if (ParserMatch(_Parser, TOKEN_ARROW) || ParserMatch(_Parser, TOKEN_COLON)) {
        Subprogram -> ReturnType = ParseType(_Parser);
    }

    ASTStatement *Body = ParseBlock(_Parser);

    Subprogram -> Body = Body -> Block.Statements;
    Subprogram -> BodyCount = Body -> Block.Count;

    _Parser -> FunctionDepth--;
    _Parser -> CurrentFunction = NULL;

    TraceExit("ParseSubprogram", _Parser);

    return Subprogram;
}

static void ParseEnumDecl(Parser *_Parser, ASTProgram *Program) {
    Expect(_Parser, TOKEN_IDENTIFIER, "enum name");
    Expect(_Parser, TOKEN_LBRACE, "'{'");

    while (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF)) {
        if (ParserCheck(_Parser, TOKEN_IDENTIFIER))
            ParserAdvance(_Parser);

        ParserMatch(_Parser, TOKEN_COMMA);
    }

    Expect(_Parser, TOKEN_RBRACE, "'}'");
}

static void ParseStateDecl(Parser *_Parser, ASTProgram *Program) {
    ParseEnumDecl(_Parser, Program);
}

static void ParseStructDecl(Parser *_Parser, ASTProgram *Program) {
    Expect(_Parser, TOKEN_IDENTIFIER, "struct name");
    Expect(_Parser, TOKEN_LBRACE, "'{'");

    while (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF)) {
        if (ParserCheck(_Parser, TOKEN_IDENTIFIER)) {
            ParserAdvance(_Parser);
            Expect(_Parser, TOKEN_COLON, "':'");
            ParseType(_Parser);
        }

        ParserMatch(_Parser, TOKEN_COMMA);
        ParserMatch(_Parser, TOKEN_SEMICOLON);
    }

    Expect(_Parser, TOKEN_RBRACE, "'}'");
}

static void ParsePartialDeclaration(Parser *_Parser, ASTProgram *Program) {
    ParseStructDecl(_Parser, Program);
}

static void ParseWorldDeclaration(Parser *_Parser, ASTProgram *Program) {
    Expect(_Parser, TOKEN_IDENTIFIER, "world name");

    if (ParserMatch(_Parser, TOKEN_EXTENDS)) {
        Expect(_Parser, TOKEN_IDENTIFIER, "base world name");
    }

    Expect(_Parser, TOKEN_LBRACE, "'{'");

    while (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF)) {
        if (ParserMatch(_Parser, TOKEN_FUNCTION)) {
            ASTSubprogram *Subprogram = ParseSubprogram(_Parser);

            Program -> SubprogramCount++;
            Program -> Subprograms = (ASTSubprogram **) realloc(Program -> Subprograms, sizeof(ASTSubprogram *) * Program -> SubprogramCount);
            Program -> Subprograms[Program -> SubprogramCount - 1] = Subprogram;
        } else {
            ASTStatement *String = ParseStatement(_Parser);

            if (String) {
                Program -> StatementCount++;
                Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
                Program -> Statements[Program -> StatementCount - 1] = String;
            }
        }
    }

    Expect(_Parser, TOKEN_RBRACE, "'}'");
}

static void ParseContextDeclaration(Parser *_Parser, ASTProgram *Program) {
    Expect(_Parser, TOKEN_IDENTIFIER, "context name");
    Expect(_Parser, TOKEN_LBRACE, "'{'");

    while (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF)) {
        if (ParserMatch(_Parser, TOKEN_FUNCTION)) {
            ASTSubprogram *Subprogram = ParseSubprogram(_Parser);

            Program -> SubprogramCount++;
            Program -> Subprograms = (ASTSubprogram **) realloc(Program -> Subprograms, sizeof(ASTSubprogram *) * Program -> SubprogramCount);
            Program -> Subprograms[Program -> SubprogramCount - 1] = Subprogram;
        } else {
            ASTStatement *String = ParseStatement(_Parser);

            if (String) {
                Program -> StatementCount++;
                Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
                Program -> Statements[Program -> StatementCount - 1] = String;
            }
        }
    }

    Expect(_Parser, TOKEN_RBRACE, "'}'");
}

static void ParseModuleDeclaration(Parser *_Parser, ASTProgram *Program) {
    Expect(_Parser, TOKEN_IDENTIFIER, "module name");
    Expect(_Parser, TOKEN_LBRACE, "'{'");

    while (!ParserCheck(_Parser, TOKEN_RBRACE) && !ParserCheck(_Parser, TOKEN_EOF)) {
        if (ParserMatch(_Parser, TOKEN_FUNCTION) || ParserMatch(_Parser, TOKEN_EXPORT)) {
            ParserMatch(_Parser, TOKEN_FUNCTION);

            ASTSubprogram *Subprogram = ParseSubprogram(_Parser);

            Program -> SubprogramCount++;
            Program -> Subprograms = (ASTSubprogram **) realloc(Program -> Subprograms, sizeof(ASTSubprogram *) * Program -> SubprogramCount);
            Program -> Subprograms[Program -> SubprogramCount - 1] = Subprogram;
        } else {
            ASTStatement *String = ParseStatement(_Parser);

            if (String) {
                Program -> StatementCount++;
                Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
                Program -> Statements[Program -> StatementCount - 1] = String;
            }
        }
    }

    Expect(_Parser, TOKEN_RBRACE, "'}'");
}

static void ParseMacroDeclaration(Parser *_Parser, ASTProgram *Program) {
    TraceEnter("ParseMacroDeclaration", _Parser);

    Token *NameToken = Expect(_Parser, TOKEN_IDENTIFIER, "macro name");
    ASTMacro *Macro = (ASTMacro *) XCalloc(1, sizeof(ASTMacro));

    Macro -> Name = XStrndup(NameToken -> Start, NameToken -> Length);

    _Parser -> InMacro++;

    Expect(_Parser, TOKEN_LPAREN, "'('");

    size_t Cap = 4, Count = 0;

    const char **Parameters = (const char **) XMalloc(sizeof(const char *) * Cap);

    while (!ParserCheck(_Parser, TOKEN_RPAREN) && !ParserCheck(_Parser, TOKEN_EOF)) {
        Token *Parameter = Expect(_Parser, TOKEN_IDENTIFIER, "macro parameter");

        if (Count >= Cap) {
            Cap *= 2;
            
            Parameters = (const char **) realloc(Parameters, sizeof(const char *) * Cap);
        }

        Parameters[Count++] = XStrndup(Parameter->Start, Parameter->Length);

        if (!ParserMatch(_Parser, TOKEN_COMMA)) break;
    }

    Expect(_Parser, TOKEN_RPAREN, "')'");

    Macro -> Parameters = Parameters;
    Macro -> ParameterCount = Count;

    ASTStatement *Body = ParseBlock(_Parser);

    Macro -> Body = Body -> Block.Statements;
    Macro -> BodyCount = Body -> Block.Count;

    _Parser -> InMacro--;

    Program -> MacroCount++;
    Program -> Macros = (ASTMacro **) realloc(Program -> Macros, sizeof(ASTMacro *) * Program -> MacroCount);
    Program -> Macros[Program -> MacroCount - 1] = Macro;

    TraceExit("ParseMacroDeclaration", _Parser);
}

static void ParseEnvironmentDeclaration(Parser *_Parser, ASTProgram *Program) {
    Expect(_Parser, TOKEN_IDENTIFIER, "environment name");

    if (ParserCheck(_Parser, TOKEN_LBRACE))
        ParseBlock(_Parser);
}

static void ParseUsing(Parser *_Parser, ASTProgram *Program) {
    if (ParserCheck(_Parser, TOKEN_IDENTIFIER))
        ParserAdvance(_Parser);

    while (ParserMatch(_Parser, TOKEN_DOUBLE_COLON)) {
        if (ParserCheck(_Parser, TOKEN_IDENTIFIER))
            ParserAdvance(_Parser);
    }

    ParserMatch(_Parser, TOKEN_SEMICOLON);
}

static void ParseSysDecl(Parser *_Parser, ASTProgram *Program) {
    Token *NameToken = Expect(_Parser, TOKEN_IDENTIFIER, "sys variable name");

    Expect(_Parser, TOKEN_EQUAL, "'='");

    ASTExpression *Value = ParseExpression(_Parser);
    ASTStatement *String = NewStatement(STMT_VAR_DECL);

    String -> VariableDeclaration.Name = XStrndup(NameToken -> Start, NameToken -> Length);
    String -> VariableDeclaration.Initializer = Value;
    String -> VariableDeclaration.Modifiers = MOD_STATIC | MOD_CONST;

    Program -> StatementCount++;
    Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
    Program -> Statements[Program -> StatementCount - 1] = String;
}

ASTProgram *ParseProgram(Parser *_Parser) {
    TraceEnter("ParseProgram", _Parser);

    ASTProgram *Program = (ASTProgram *) XCalloc(1, sizeof(ASTProgram));

    while (!ParserCheck(_Parser, TOKEN_EOF)) {
        if (_Parser -> PanicMode) { Synchronise(_Parser); }

        TokenType _Token = _Parser -> Current -> Type;

        if (ParserMatch(_Parser, TOKEN_FUNCTION)) {
            ASTSubprogram *Subprogram = ParseSubprogram(_Parser);

            Program -> SubprogramCount++;
            Program -> Subprograms = (ASTSubprogram **) realloc(Program -> Subprograms, sizeof(ASTSubprogram *) * Program -> SubprogramCount);
            Program -> Subprograms[Program -> SubprogramCount - 1] = Subprogram;

            continue;
        }

        if (ParserMatch(_Parser, TOKEN_COMPTIME)) {
            if (ParserMatch(_Parser, TOKEN_FUNCTION)) {
                ASTSubprogram *Subprogram = ParseSubprogram(_Parser);

                Program -> SubprogramCount++;
                Program -> Subprograms = (ASTSubprogram **) realloc(Program -> Subprograms, sizeof(ASTSubprogram *) * Program -> SubprogramCount);
                Program -> Subprograms[Program -> SubprogramCount - 1] = Subprogram;
            } else {
                ASTStatement *String = ParseComptimeBlock(_Parser);

                Program -> StatementCount++;
                Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
                Program -> Statements[Program -> StatementCount - 1] = String;
            }

            continue;
        }

        if (_Token == TOKEN_UNSAFE && ParserCheckNext(_Parser, TOKEN_FUNCTION)) {
            ParserAdvance(_Parser);
            ParserAdvance(_Parser);

            ASTSubprogram *Subprogram = ParseSubprogram(_Parser);

            Program -> SubprogramCount++;
            Program -> Subprograms = (ASTSubprogram **) realloc(Program -> Subprograms, sizeof(ASTSubprogram *) * Program -> SubprogramCount);
            Program -> Subprograms[Program -> SubprogramCount - 1] = Subprogram;

            continue;
        }

        if (ParserMatch(_Parser, TOKEN_ENUM)) {
            ParseEnumDecl(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_STATE)) {
            ParseStateDecl(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_STRUCT)) {
            ParseStructDecl(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_PARTIAL)) {
            ParsePartialDeclaration(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_MODULE)) {
            ParseModuleDeclaration(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_WORLD)) {
            ParseWorldDeclaration(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_CONTEXT)) {
            ParseContextDeclaration(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_MACRO)) {
            ParseMacroDeclaration(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_ENV)) {
            ParseEnvironmentDeclaration(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_USING)) {
            ParseUsing(_Parser, Program);
            
            continue;
        }

        if (ParserMatch(_Parser, TOKEN_SYS)) {
            ParseSysDecl(_Parser, Program);
            
            continue;
        }


        if (ParserCheck(_Parser, TOKEN_IDENTIFIER)) {
            Token *_Token = _Parser -> Current;
            if (_Token -> Length == 3 && memcmp(_Token -> Start, "nao", 3) == 0) {
                ParserAdvance(_Parser);

                ASTStatement *String = ParseNaoBlock(_Parser);

                Program -> StatementCount++;
                Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
                Program -> Statements[Program -> StatementCount - 1] = String;

                continue;
            }

            if (_Token -> Length == 7 && memcmp(_Token -> Start, "concept", 7) == 0) {
                ParserAdvance(_Parser);

                ASTStatement *String = ParseConceptBlock(_Parser);

                Program -> StatementCount++;
                Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
                Program -> Statements[Program -> StatementCount - 1] = String;

                continue;
            }

            if (_Token -> Length == 8 && memcmp(_Token -> Start, "reactive", 8) == 0) {
                ParserAdvance(_Parser);

                ASTStatement *String = ParseReactiveBlock(_Parser);

                Program -> StatementCount++;
                Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
                Program -> Statements[Program -> StatementCount - 1] = String;

                continue;
            }
        }

        if (ParserMatch(_Parser, TOKEN_WITH)) {
            ASTStatement *String = ParseWithBlock(_Parser);

            Program -> StatementCount++;
            Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
            Program -> Statements[Program -> StatementCount - 1] = String;

            continue;
        }

        {
            ASTStatement *String = ParseStatement(_Parser);

            if (String) {
                Program -> StatementCount++;
                Program -> Statements = (ASTStatement **) realloc(Program -> Statements, sizeof(ASTStatement *) * Program -> StatementCount);
                Program -> Statements[Program -> StatementCount - 1] = String;
            }
        }
    }

    TraceExit("ParseProgram", _Parser);

    return Program;
}

ASTProgram *MergePrograms(ASTProgram *Prelude, ASTProgram *User) {
    if (!Prelude) return User;
    if (!User) return Prelude;

    ASTProgram *Merged = (ASTProgram *) XCalloc(1, sizeof(ASTProgram));

    Merged -> SubprogramCount = Prelude -> SubprogramCount + User -> SubprogramCount;
    Merged -> Subprograms = (ASTSubprogram **) XMalloc(sizeof(ASTSubprogram *) * Merged -> SubprogramCount);

    memcpy(Merged -> Subprograms, Prelude -> Subprograms, sizeof(ASTSubprogram *) * Prelude -> SubprogramCount);
    memcpy(Merged -> Subprograms + Prelude -> SubprogramCount, User -> Subprograms, sizeof(ASTSubprogram *) * User -> SubprogramCount);

    Merged -> StatementCount = Prelude -> StatementCount + User -> StatementCount;
    Merged -> Statements = (ASTStatement **) XMalloc(sizeof(ASTStatement *) * Merged -> StatementCount);

    memcpy(Merged -> Statements, Prelude -> Statements, sizeof(ASTStatement *) * Prelude -> StatementCount);
    memcpy(Merged -> Statements + Prelude -> StatementCount, User -> Statements, sizeof(ASTStatement *) * User -> StatementCount);

    Merged -> MacroCount = Prelude -> MacroCount + User -> MacroCount;
    Merged -> Macros = (ASTMacro **) XMalloc(sizeof(ASTMacro *) * Merged -> MacroCount);

    memcpy(Merged -> Macros, Prelude -> Macros, sizeof(ASTMacro *) * Prelude -> MacroCount);
    memcpy(Merged -> Macros + Prelude -> MacroCount, User -> Macros, sizeof(ASTMacro *) * User -> MacroCount);

    return Merged;
}

ASTStatement *ParseRepeatBody(Parser *_Parser) {
    return ParseBlock(_Parser);
}

ASTStatement *ParseRepeatStatement(Parser *_Parser) {
    ASTStatement *String = ParseExpressionStatement(_Parser);

    return String;
}
