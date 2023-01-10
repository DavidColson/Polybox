// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"

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
    Literal,
    Grouping,
    Variable,
    VariableAssignment,

    // Statements
    ExpressionStmt,
    PrintStmt,

    // Declarations
    VarDecl
};

// Base AST Node
struct Node {
    NodeType m_type;

    char* m_pLocation { nullptr };
    char* m_pLineStart { nullptr };
    size_t m_line { 0 };
};

// Expression type nodes
struct Expression : public Node {
    ValueType::Enum m_valueType;
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

struct VariableAssignment : public Expression {
    String m_identifier;
    Expression* m_pAssignment;
};


// Statement type nodes
struct Statement : public Node {
};

struct ExpressionStatement : public Statement {
    Expression* m_pExpr;
};

struct PrintStatement : public Statement {
    Expression* m_pExpr;
};

struct VariableDeclaration : public Statement {
    String m_identifier;
    Expression* m_pInitializerExpr;
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

    void PushError(Ast::Expression* pNode, const char* formatMessage, ...);
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

    Ast::Expression* ParsePrimary();

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

    Ast::Statement* ParseExpressionStatement();

    Ast::Statement* ParsePrintStatement();

    Ast::Statement* ParseDeclaration();

    Ast::Statement* ParseVarDeclaration();
};

void DebugAst(ResizableArray<Ast::Statement*>& program);

void DebugExpression(Ast::Expression* pExpr, int indentationLevel = 0);

