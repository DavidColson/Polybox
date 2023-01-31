// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"
#include "lexer.h"

#include <light_string.h>
#include <resizable_array.h>

template<typename T>
struct ResizableArray;
struct String;
struct IAllocator;

namespace Ast {

enum class NodeType {
    Invalid,

    // Expressions
    Binary,
    Unary,
    Call,
    Literal,
    Function,
    Grouping,
    Variable,
    VariableAssignment,
    Type,

    // Statements
    ExpressionStmt,
    If,
    While,
    Print,
    Return,
    Block,
    Declaration,
};

// Base AST Node
struct Node {
    NodeType m_nodeKind;

    char* m_pLocation { nullptr };
    char* m_pLineStart { nullptr };
    uint32_t m_line { 0 };
};

// Expression type nodes
struct Expression : public Node {
    TypeInfo* m_pType;
};

struct Binary : public Expression {
    Expression* m_pLeft;
    Operator::Enum m_operator;
    Expression* m_pRight;
};

struct Unary : public Expression {
    Operator::Enum m_operator;
    Expression* m_pRight;
};

struct Literal : public Expression {
    Value m_value;
};

struct Grouping : public Expression {
    Expression* m_pExpression;
};

struct Variable : public Expression {
    String m_identifier;
};

struct Type : public Expression {
    String m_identifier;
    TypeInfo* m_pResolvedType;
};

struct Block;
struct Declaration;
struct Function : public Expression {
    ResizableArray<Declaration*> m_params;
    Block* m_pBody;
    Type * m_pReturnType;
    String m_identifier;
};

struct VariableAssignment : public Expression {
    String m_identifier;
    Expression* m_pAssignment;
};

struct Call : public Expression {
    Expression* m_pCallee;
    ResizableArray<Expression*> m_args;
};


// Statement type nodes
struct Statement : public Node {
};

struct ExpressionStmt : public Statement {
    Expression* m_pExpr;
};

struct If : public Statement {
    Expression* m_pCondition;
    Statement* m_pThenStmt;
    Statement* m_pElseStmt;
};

struct While: public Statement {
    Expression* m_pCondition;
    Statement* m_pBody;
};

struct Print : public Statement {
    Expression* m_pExpr;
};

struct Return : public Statement {
    Expression* m_pExpr;
};

struct Block : public Statement {
    ResizableArray<Statement*> m_declarations;
    Token m_startToken;
    Token m_endToken;
};

struct Declaration : public Statement {
    String m_identifier;
    Type* m_pDeclaredType { nullptr };
    TypeInfo* m_pResolvedType;
    int m_scopeLevel { 0 };
    Expression* m_pInitializerExpr { nullptr };
};

}

struct Error {
    String m_message;
    char* m_pLocation { nullptr };
    char* m_pLineStart { nullptr };
    size_t m_line { 0 };
};

struct ErrorState {
    ResizableArray<Error> m_errors;
    IAllocator* m_pAlloc { nullptr };

    void Init(IAllocator* pAlloc);

    void PushError(Ast::Node* pNode, const char* formatMessage, ...);
    void PushError(char* pLocation, char* pLineStart, size_t line, const char* formatMessage, ...);
    void PushError(char* pLocation, char* pLineStart, size_t line, const char* formatMessage, va_list args);

    bool ReportCompilationResult();
};

struct Token;
namespace TokenType {
enum Enum : uint32_t;
}

struct ParsingState {
    Token* m_pTokensStart { nullptr };
    Token* m_pTokensEnd { nullptr };
    Token* m_pCurrent { nullptr };
    IAllocator* pAllocator { nullptr };
    ErrorState* m_pErrorState { nullptr };
    bool m_panicMode { false };

    ResizableArray<Ast::Statement*> InitAndParse(ResizableArray<Token>& tokens, ErrorState* pErrors, IAllocator* pAlloc);

    Token Previous();

    Token Advance();

    bool IsAtEnd();

    Token Peek();

    bool Check(TokenType::Enum type);

    Token Consume(TokenType::Enum type, String message);

    bool Match(int numTokens, ...);
    
    void PushError(const char* formatMessage, ...);

    void Synchronize();

    Ast::Expression* ParseType();

    Ast::Expression* ParsePrimary();

    Ast::Expression* ParseCall();

    Ast::Expression* ParseUnary();

    Ast::Expression* ParseMulDiv();

    Ast::Expression* ParseAddSub();

    Ast::Expression* ParseComparison();

    Ast::Expression* ParseEquality();

    Ast::Expression* ParseLogicAnd();

    Ast::Expression* ParseLogicOr();

    Ast::Expression* ParseVarAssignment();

    Ast::Expression* ParseExpression();

    Ast::Statement* ParseStatement();

    Ast::Statement* ParseExpressionStmt();
    
    Ast::Statement* ParseIf();

    Ast::Statement* ParseWhile();

    Ast::Statement* ParsePrint();

    Ast::Statement* ParseReturn();

    Ast::Statement* ParseBlock();

    Ast::Statement* ParseDeclaration();
};

void DebugStatements(ResizableArray<Ast::Statement*>& statements, int indentationLevel = 0);

void DebugExpression(Ast::Expression* pExpr, int indentationLevel = 0);

