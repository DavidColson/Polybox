// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

namespace Ast {
struct Expression;
}
struct ParsingState;

void TypeCheck(Ast::Expression* pExpr, ParsingState* pParser);