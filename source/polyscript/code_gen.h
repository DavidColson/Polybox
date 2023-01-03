// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

namespace Ast {
struct Statement;
}
struct CodeChunk;

template<typename T>
struct ResizableArray;

void CodeGen(ResizableArray<Ast::Statement*>& program, CodeChunk* pChunk);