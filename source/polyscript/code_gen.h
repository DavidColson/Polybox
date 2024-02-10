// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"
#include "virtual_machine.h"

#include <light_string.h>
#include <resizable_array.h>
#include <hashmap.h>

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

struct FunctionDbgInfo {
	String name;
	TypeInfo* pType;
};

struct Program {
    ResizableArray<u16> code;
	ResizableArray<size> dbgLineInfo;
	
	// This maps the instruction index of the function's entry point to the debug info
	// Such that the function pointer constant values can be used to look up the debug info
	HashMap<size, FunctionDbgInfo> dbgFunctionInfo;

	// This maps PushConstant instruction indices to constant type information
	HashMap<size, TypeInfo::TypeTag> dbgConstantsTypes;
    IAllocator* pMemory;
};

#define GetOperand16bit(ptr) *(++ptr)
#define GetOperand32bit(ptr) ((ptr[1] << 16) | ptr[2]); ptr += 2;

void CodeGenProgram(Compiler& compilerState);

Function* CodeGen(ResizableArray<Ast::Statement*>& program, ResizableArray<Ast::Declaration*>& params, Scope* pCurrentScope, String name, ErrorState* pErrorState, IAllocator* pAlloc);
