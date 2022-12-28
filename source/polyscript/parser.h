// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "virtual_machine.h"
#include "lexer.h"

template<typename T>
struct ResizableArray;

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
};

struct Binary : public Expression {
    Expression* m_pLeft;
    Token m_operator;
    Expression* m_pRight;
};

struct Unary : public Expression {
    Token m_operator;
    Expression* m_pRight;
};

struct Literal : public Expression {
    Value m_value;
};

struct Grouping : public Expression {
    Expression* m_pExpression;
};

}

Ast::Expression* ParseToAst(ResizableArray<Token>& tokens, IAllocator* pAlloc);

void DebugAst(Ast::Expression* pExpr, int indentationLevel = 0);