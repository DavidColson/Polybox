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
	Structure,
    Grouping,
    Identifier,
	VariableAssignment,
	Cast,

    // Types
    Type,
    FnType,

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
    NodeType nodeKind;

    char* pLocation { nullptr };
    char* pLineStart { nullptr };
    uint32_t line { 0 };
};

// Expression type nodes
struct Expression : public Node {
    TypeInfo* pType;
};

struct Binary : public Expression {
    Expression* pLeft;
    Operator::Enum op;
    Expression* pRight;
};

struct Unary : public Expression {
    Operator::Enum op;
    Expression* pRight;
};

struct Literal : public Expression {
    Value value;
};

struct Grouping : public Expression {
    Expression* pExpression;
};

struct Identifier : public Expression {
    String identifier;
};

struct Type : public Expression {
    String identifier;
    TypeInfo* pResolvedType;
};

struct Declaration;
struct FnType : public Type {
    ResizableArray<Type*> params;
    Type* pReturnType;
};

struct Block;
struct Function : public Expression {
    ResizableArray<Declaration*> params;
    Block* pBody;
    Type * pReturnType;
    String identifier;
};

struct Statement;
struct Structure : public Expression {
	ResizableArray<Statement*> members;
	TypeInfo* pDescribedType;
};

struct VariableAssignment : public Expression {
    String identifier;
    Expression* pAssignment;
};

struct Cast : public Expression {
	Type* pTargetType;
	Expression* pExprToCast;
};

struct Call : public Expression {
    Expression* pCallee;
    ResizableArray<Expression*> args;
};


// Statement type nodes
struct Statement : public Node {
};

struct ExpressionStmt : public Statement {
    Expression* pExpr;
};

struct If : public Statement {
    Expression* pCondition;
    Statement* pThenStmt;
    Statement* pElseStmt;
};

struct While: public Statement {
    Expression* pCondition;
    Statement* pBody;
};

struct Print : public Statement {
    Expression* pExpr;
};

struct Return : public Statement {
    Expression* pExpr;
};

struct Block : public Statement {
    ResizableArray<Statement*> declarations;
    Token startToken;
    Token endToken;
};

struct Declaration : public Statement {
    String identifier;
    Type* pDeclaredType { nullptr };
    TypeInfo* pResolvedType;
    Expression* pInitializerExpr { nullptr };
    
    // Used by the typechecker
    int scopeLevel{ 0 };
    bool initialized{ 0 };
};

}

struct Error {
    String message;
    char* pLocation { nullptr };
    char* pLineStart { nullptr };
    size_t line { 0 };
};

struct ErrorState {
    ResizableArray<Error> errors;
    IAllocator* pAlloc { nullptr };

    void Init(IAllocator* pAlloc);

    void PushError(Ast::Node* pNode, const char* formatMessage, ...);
    void PushError(char* pLocation, char* pLineStart, size_t line, const char* formatMessage, ...);
    void PushError(char* pLocation, char* pLineStart, size_t line, const char* formatMessage, va_list args);

    bool ReportCompilationResult();
};

struct Token;

ResizableArray<Ast::Statement*> InitAndParse(ResizableArray<Token>& tokens, ErrorState* pErrors, IAllocator* pAlloc);

void DebugStatements(ResizableArray<Ast::Statement*>& statements, int indentationLevel = 0);

void DebugExpression(Ast::Expression* pExpr, int indentationLevel = 0);

