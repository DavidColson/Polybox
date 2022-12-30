// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "virtual_machine.h"
#include "lexer.h"
#include "value.h"

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

}

Ast::Expression* ParseToAst(ResizableArray<Token>& tokens, IAllocator* pAlloc);

void DebugAst(Ast::Expression* pExpr, int indentationLevel = 0);