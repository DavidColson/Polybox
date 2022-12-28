// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

namespace Ast {
struct Expression;
}
struct CodeChunk;

void CodeGen(Ast::Expression* pExpr, CodeChunk* pChunk);