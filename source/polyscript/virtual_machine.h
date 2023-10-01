// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"

struct String;
struct Compiler;

namespace OpCode {
enum Enum : uint8_t {
    LoadConstant,

    // Arithmetic
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,

    // Logical
    Not,

    // Comparison
    Greater,
    Less,
    Equal,
    NotEqual,
    GreaterEqual,
    LessEqual,

    // Flow control
    JmpIfFalse,
    JmpIfTrue,
    Jmp,
    Loop,

    // Misc
	StructAlloc,
    SetLocal,
    GetLocal,
	SetField,
	SetFieldStruct,
	GetField,
	GetFieldStruct,
    Call,
	Cast,
    Print,
    Return,
    Pop
};
}

uint8_t DisassembleInstruction(Function& function, uint8_t* pInstruction);

void Disassemble(Function* pFunc, String codeText);

void Run(Function* pFuncToRun);