// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"
#include "virtual_machine.h"

#include <light_string.h>
#include <resizable_array.h>

namespace Ast {
struct Statement;
struct Declaration;
}
struct Function;
struct ErrorState;
template<typename T>
struct ResizableArray;
struct Scope;
struct Compiler;
struct IAllocator;

struct Program {
    ResizableArray<Instruction> code;
	ResizableArray<uint32_t> dbgLineInfo;
	
	// TODO: Update
	ResizableArray<TypeInfo*> dbgConstantsTypes;
	
	// TODO: Some way to track function names if available
	
    
    IAllocator* pMemory;
};

void CodeGenProgram(Compiler& compilerState);

Function* CodeGen(ResizableArray<Ast::Statement*>& program, ResizableArray<Ast::Declaration*>& params, Scope* pCurrentScope, String name, ErrorState* pErrorState, IAllocator* pAlloc);