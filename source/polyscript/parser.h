// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "virtual_machine.h"
#include "lexer.h"
#include "value.h"
#include "light_string.h"

template<typename T>
struct ResizableArray;
struct String;
struct IAllocator;

namespace Ast {

enum class NodeType {
    Binary,
    Unary,
    Literal,
    Grouping
};

struct Expression {
    NodeType m_type;
    ValueType::Enum m_valueType;
    
    char* m_pLocation { nullptr };
    char* m_pLineStart { nullptr };
    size_t m_line { 0 };
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

}

struct Error {
    String m_message;
    char* m_pLocation { nullptr };
    char* m_pLineStart { nullptr };
    size_t m_line { 0 };
};

struct ParsingState {
    Token* m_pTokensStart { nullptr };
    Token* m_pTokensEnd { nullptr };
    Token* m_pCurrent { nullptr };
    IAllocator* pAllocator { nullptr };
    bool m_panicMode;
    ResizableArray<Error> m_errors;

    Ast::Expression* InitAndParse(ResizableArray<Token>& tokens, IAllocator* pAlloc);

    Token Previous();

    Token Advance();

    bool IsAtEnd();

    Token Peek();

    bool Check(TokenType::Enum type);

    Token Consume(TokenType::Enum type, String message);

    bool Match(int numTokens, ...);

    void PushError(Ast::Expression* pNode, const char* formatMessage, ...);

    Ast::Expression* ParsePrimary();

    Ast::Expression* ParseUnary();

    Ast::Expression* ParseMulDiv();

    Ast::Expression* ParseAddSub();

    Ast::Expression* ParseComparison();

    Ast::Expression* ParseEquality();

    Ast::Expression* ParseLogicAnd();

    Ast::Expression* ParseLogicOr();

    Ast::Expression* ParseExpression();
};

void DebugAst(Ast::Expression* pExpr, int indentationLevel = 0);