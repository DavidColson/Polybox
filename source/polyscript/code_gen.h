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

struct Function {
	String name;
	ResizableArray<uint8_t> code;
    ResizableArray<Instruction> code2;
	ResizableArray<uint32_t> dbgLineInfo;
};

struct Program {
    // This is a file level function corresponding to the primary script file
    Function* pMainModuleFunction;
    
    ResizableArray<Value> constantTable;
	ResizableArray<TypeInfo*> dbgConstantsTypes;
    
    IAllocator* pMemory;
};

void CodeGenProgram(Compiler& compilerState);

Function* CodeGen(ResizableArray<Ast::Statement*>& program, ResizableArray<Ast::Declaration*>& params, Scope* pCurrentScope, String name, ErrorState* pErrorState, IAllocator* pAlloc);