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
struct Compiler;

namespace Ast {

enum class NodeType {
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
    bool isConstant { false };
    Value constantValue; // TODO: Set this as you propogate up and down the tree in type checking
    // codegen will check if the node is constant, and if it is, it'll use the constant value as a literal (i.e. putting in constant table)
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
	// fn (param1, param2, ...) -> returnType
    ResizableArray<Type*> params;
    Type* pReturnType;
};

struct Block;
struct Function : public Expression {
	// identifier := func (param1, param2, ...) -> returnType {body}
    ResizableArray<Declaration*> params;
    Block* pBody;
    Type * pReturnType;
    String identifier;
};

struct Statement;
struct Structure : public Expression {
	// struct { statement1, statement2, ...}
	ResizableArray<Statement*> members;
	TypeInfo* pDescribedType;
};

struct VariableAssignment : public Expression {
	// identifier = assignment
    String identifier;
    Expression* pAssignment;
};

struct Cast : public Expression {
	// as(targetType) exprToCast
	Type* pTargetType;
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
};

struct Declaration : public Statement {
	// identifier : declaredType = initializerExpr;
    String identifier;
    Type* pDeclaredType { nullptr };
    TypeInfo* pResolvedType;
    Expression* pInitializerExpr { nullptr };
    bool isConstantDeclaration { false };

    // Used by the typechecker
    int scopeLevel{ 0 };
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

