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
    Literal,
    Function,
    Grouping,
    Variable,
    VariableAssignment,

    // Statements
    ExpressionStmt,
    If,
    While,
    Print,
    Block,
    Declaration,
};

// Base AST Node
struct Node {
    NodeType m_type;

    char* m_pLocation { nullptr };
    char* m_pLineStart { nullptr };
    uint32_t m_line { 0 };
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

struct Block;
struct Function : public Expression {
    Block* m_pBody;
};

struct VariableAssignment : public Expression {
    String m_identifier;
    Expression* m_pAssignment;
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

struct Block : public Statement {
    ResizableArray<Statement*> m_declarations;
    Token m_startToken;
    Token m_endToken;
};

struct Declaration : public Statement {
    String m_identifier;
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

    Ast::Statement* ParseExpressionStmt();
    
    Ast::Statement* ParseIf();

    Ast::Statement* ParseWhile();

    Ast::Statement* ParsePrint();

    Ast::Statement* ParseBlock();

    Ast::Statement* ParseDeclaration();

    Ast::Statement* ParseVarDeclaration(String identifierToBind);

    Ast::Statement* ParseFuncDeclaration(String identifierToBind);
};

void DebugStatements(ResizableArray<Ast::Statement*>& statements, int indentationLevel = 0);

void DebugExpression(Ast::Expression* pExpr, int indentationLevel = 0);

