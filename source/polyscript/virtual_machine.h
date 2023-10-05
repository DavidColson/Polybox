// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"

struct String;
struct Compiler;
struct Program;

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

String DisassembleInstruction(Program* pProgram, uint8_t* pInstruction, uint8_t& outOffset);

void DisassembleFunction(Compiler& compilerState, Function* pFunc);

void DisassembleProgram(Compiler& compilerState);

void Run(Program* pProgramToRun);