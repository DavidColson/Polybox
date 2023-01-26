// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

namespace Ast {
struct Statement;
struct Declaration;
}
struct Function;
struct ErrorState;

template<typename T>
struct ResizableArray;

struct String;

Function* CodeGen(ResizableArray<Ast::Statement*>& program, ResizableArray<Ast::Declaration*>& params, String name, ErrorState* pErrorState);