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
    BadExpression,
    Binary,
    Unary,
	Call,
	GetField,
	SetField,
	StructLiteral,
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
    BadStatement,
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
    u32 line { 0 };
};

// Expression type nodes
struct Expression : public Node {
    TypeInfo* pType;
    bool isConstant { false };
    Value constantValue;
};

struct BadExpression : public Expression {
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

struct StructLiteral : public Expression {
	// StructName{ var = 1, var2 = 3};

	// I think it sensible that we have two hard types of struct literal
	// A named struct literal and a non-named struct literal
	// We may be able to have two different ast nodes for each type if they are created _after_ the 
	// contents are parsed.

	// Additionally, an interesting feature from jai is that named assignments can be _any_ scoped variable assignment
	// So you can do StructName{ arrayInside[2] = 4, mem = 3}. This is interesting and poses the idea that for the named
	// Struct literal, you _literally_ have assignment nodes for each comma separated section, and somehow they all know
	// That they are in the scope of the parent struct. How can we do this?

	// Jai implements struct literals as a type of the "literal" node that has just a list of code nodes as arguments
	// So we must figure out how to process that.


	// It definitely closely matches assignments in terms of syntax. So I guess it could be assignments or just 
	// literal expressions. If it's an expression it's easy, we'll just match the number of expressions to the number of
	// struct members and generate set field code for each.

	// If it's named though. Then what? We basically want all the expressions in the list to be either assignments, setfields,
	// or in future, setarray nodes.





	// ASIDE
	// Looking at jai's code I suspect actually that there is no setfield, variable assignment nodes in the language at all
	// I think that '=' and '.' are binary operators, that are just represented with a binary node.

	// Consider the dot operator. What is it really doing? It's basically just adding some offset to the address that preceeded it?
	// If you look at our implementation of VariableAssignment, you'll see that it's actually identical to the pairing of 
	// load identifier that is struct, and then store/copy. And in fact in thinking about this I realise that Variable Assignment
	// doesn't handle the case of copying a struct i.e. structInstance = anotherStructInstance. If it did support that, it would 
	// Literally be identical to setField.

	// Plan for dave:
	// You should be able to implement '.' and '=' as pure operators that do not understand what the left hand side is.
	// Dot will just add an offset to the previous address, equal will store/copy to that offset, that's it.
	// You can then in fact implement pratt parsing and drastically simplify the parser.

	// How this relates to struct literals?
	
	// Process for codegen a struct literal:
	// First you will push onto stack space for the struct with some const 0's

	// Two cases, each expression is either an assignment node, or it's not.
	// If it's not an assignment node, then codegen will loop through each one, generating the equivalent code as
	// a dot operator, but using the member info in the typeinfo struct.
	// That is gen expression, push struct, store with offset, repeat

	// Case two, the node is an assignment node, now it's more interesting
	// Now you want to do codegen rhs of assignment node, then push struct, then do what a dot operator would do, 
	// which is resolve lhs identifier, then add the offset of the rhs identifier to it.
	


	// Alright dave, here's the hard truth you need to hear. You're bordering on overanalyzing this problem and getting yourself
	// into knots. Why don't we just try and implement this with the code we have, brute forcing it with explicitly written
	// Instructions, no refactors. Save these comments as notes to return to later, and we'll talk about the following refactors after:

	// 1. Define "." and "=" as binary operators
	// 2. Merge variable assignment and setfield nodes into one
	// 3. Make identifier nodes generate an address or a value using semantic analysis
	// 4. Implement pratt precedence parsing.




	// The statements are guaranteed to either be assignments, or general expressions
	// If it's an assignment we will extract the identifier and handle it separately, but they must all be assignments
	// if it's an expression we'll handle that separately.
	String structName;
	ResizableArray<Expression*> members;
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
	Expression* pTarget;
	String fieldName;
};

struct SetField : public Expression {
	// target.fieldName = assignment
	Expression* pTarget;
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

struct BadStatement : public Statement {
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
    usize line { 0 };
};

struct ErrorState {
    ResizableArray<Error> errors;
    IAllocator* pAlloc { nullptr };

    void Init(IAllocator* pAlloc);

    void PushError(Ast::Node* pNode, const char* formatMessage, ...);
    void PushError(char* pLocation, char* pLineStart, usize line, const char* formatMessage, ...);
    void PushError(char* pLocation, char* pLineStart, usize line, const char* formatMessage, va_list args);

    bool ReportCompilationResult();
};

struct Token;

void Parse(Compiler& compilerState);

void DebugStatements(ResizableArray<Ast::Statement*>& statements, i32 indentationLevel = 0);

void DebugExpression(Ast::Expression* pExpr, i32 indentationLevel = 0);

