// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

namespace Ast {
struct Statement;
}
struct CodeChunk;
struct ErrorState;

template<typename T>
struct ResizableArray;

CodeChunk CodeGen(ResizableArray<Ast::Statement*>& program, ErrorState* pErrorState);