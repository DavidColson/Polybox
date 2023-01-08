// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

namespace Ast {
struct Statement;
}
struct ErrorState;

template<typename T>
struct ResizableArray;

void TypeCheckProgram(ResizableArray<Ast::Statement*>& program, ErrorState* pErrors);