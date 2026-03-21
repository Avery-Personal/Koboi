#ifndef PARSER_H
#define PARSER_H

    #include "../Lexer/Lexer.h"
    #include "AST.h"

    #define PARSER_TRACE 0

    typedef struct {
        TokenStream *Tokens;
 
        Token *Current;
        Token *Previous;
 
        int HasError;
        int PanicMode;
 
        int ScopeDepth;
        int FunctionDepth;
 
        int InUnsafe;
        int InMatch;
        int InLoop;
        int InMacro;
 
        int Precedence;
 
        const char *CurrentFunction;
    } Parser;

    int IsSyncToken(TokenType Type);
    int GetOperatorPrecedence(TokenType Type);
    ASTOperator TokenToOperator(TokenType Type);
 
    Parser CreateParser(TokenStream *Tokens);
    ASTProgram *MergePrograms(ASTProgram *Prelude, ASTProgram *User);
 
    ASTProgram *ParseProgram(Parser *_Parser);
    ASTSubprogram *ParseSubprogram(Parser *_Parser);
 
    ASTStatement *ParseStatement(Parser *_Parser);
    ASTStatement *ParseBlock(Parser *_Parser);
    ASTStatement *ParseControlBlock(Parser *_Parser);
    ASTStatement *ParseRepeatBody(Parser *_Parser);
    ASTStatement *ParseRepeatStatement(Parser *_Parser);
    ASTStatement *ParseVariableDeclaration(Parser *_Parser, int Modifiers);
 
    ASTStatement *ParseIfStatement(Parser *_Parser);
    ASTStatement *ParseWhileStatement(Parser *_Parser);
    ASTStatement *ParseForStatement(Parser *_Parser);
    ASTStatement *ParseReturnStatement(Parser *_Parser);
 
    ASTExpression *ParseExpression(Parser *_Parser);
    ASTExpression *ParseBinary(Parser *_Parser, int MinimumPrecedence);
    ASTExpression *ParseBinaryFromLeft(Parser *_Parser, ASTExpression *Left, int MinimumPrecedence);
    ASTExpression *ParseUnary(Parser *_Parser);
    ASTExpression *ParsePrimary(Parser *_Parser);
    ASTExpression *ParsePostfix(Parser *_Parser);
    ASTExpression *ParseArrayLiteral(Parser *_Parser);
 
    ASTType *ParseType(Parser *_Parser);
    Token *ParserPeek(Parser *_Parser);
    Token *ParserPrevious(Parser *_Parser);
    Token *ParserAdvance(Parser *_Parser);
 
    int ParserMatch(Parser *_Parser, TokenType Type);
    int ParserCheck(Parser *_Parser, TokenType Type);
    int ParserCheckNext(Parser *_Parser, TokenType Type);

#endif
