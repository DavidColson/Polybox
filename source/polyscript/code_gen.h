// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

namespace Ast {
struct Statement;
}
struct Function;
struct ErrorState;

template<typename T>
struct ResizableArray;

Function* CodeGen(ResizableArray<Ast::Statement*>& program, ErrorState* pErrorState);