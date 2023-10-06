// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"
#include "lexer.h"

#include <light_string.h>
#include <resizable_array.h>
#include <hashmap.h>

template<typename T>
struct ResizableArray;
struct String;
struct IAllocator;
struct Compiler;
struct Scope;

namespace Ast {

enum class NodeKind {
    Invalid,

    // Expressions
    Binary,
    Unary,
	Call,
	GetField,
	SetField,
    Literal,
	Function,
	Structure,
    Grouping,
    Identifier,
	VariableAssignment,
	Cast,

    // Types
    Type,
    FunctionType,

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
    NodeKind nodeKind;

    char* pLocation { nullptr };
    char* pLineStart { nullptr };
    uint32_t line { 0 };
};

// Expression type nodes
struct Expression : public Node {
    TypeInfo* pType;
    bool isConstant { false };
    Value constantValue;
};

struct Binary : public Expression {
	// left op right
    Expression* pLeft;
    Operator::Enum op;
    Expression* pRight;
};

struct Unary : public Expression {
	// op right
    Operator::Enum op;
    Expression* pRight;
};

struct Literal : public Expression {
};

struct Grouping : public Expression {
    Expression* pExpression;
};

struct Identifier : public Expression {
    String identifier;
};

struct Block;
struct FunctionType;
struct Declaration;
struct Function : public Expression {
	// func (param1:type1, param2:type1, ...) -> returnType body
    FunctionType* pFuncType;
    Block* pBody;

    Scope* pScope;
    Declaration* pDeclaration;
};

struct VariableAssignment : public Expression {
	// identifier = assignment
    Identifier* pIdentifier;
    Expression* pAssignment;
};

struct Cast : public Expression {
	// as(typeExpr) exprToCast
	Expression* pTypeExpr;
	Expression* pExprToCast;
};

struct Call : public Expression {
	// callee(arg1, arg2, ..)
    Expression* pCallee;
    ResizableArray<Expression*> args;
};

struct GetField : public Expression {
	// target.fieldName
	Expression *pTarget;
	String fieldName;
};

struct SetField : public Expression {
	// target.fieldName = assignment
	Expression *pTarget;
	String fieldName;
	Expression* pAssignment;
};

// Type type nodes
struct Type : public Expression {
};

struct FunctionType : public Type {
	// func (param1, param2, ...) -> returnType
    Scope* pScope;
    ResizableArray<Node*> params;
    Expression* pReturnType;
};

struct Statement;
struct Structure : public Type {
	// struct { statement1, statement2, ...}
	ResizableArray<Statement*> members;

    Token startToken;
    Token endToken;
    
    Scope* pScope;
    Declaration* pDeclaration; // TODO: Temp, remove when we have named types
};


// Statement type nodes
struct Statement : public Node {
};

struct ExpressionStmt : public Statement {
    Expression* pExpr;
};

struct If : public Statement {
	// if condition thenStmt else elseStmt
    Expression* pCondition;
    Statement* pThenStmt;
    Statement* pElseStmt;
};

struct While: public Statement {
	// while condition body
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
    Scope* pScope;
};

struct Declaration : public Statement {
	// identifier : typeAnnotation = initializerExpr;
    String identifier;
    Expression* pTypeAnnotation { nullptr };
    TypeInfo* pType;
    Expression* pInitializerExpr { nullptr };
    bool isConstantDeclaration { false };
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

void Parse(Compiler& compilerState);

void DebugStatements(ResizableArray<Ast::Statement*>& statements, int indentationLevel = 0);

void DebugExpression(Ast::Expression* pExpr, int indentationLevel = 0);

