// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"

#include <resizable_array.h>

struct String;

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

    SetLocal,
    GetLocal,
    JmpIfFalse,
    JmpIfTrue,
    Jmp,
    Loop,

    // Misc
    Print,
    Return,
    Pop
};
}

uint8_t DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction);

void Disassemble(CodeChunk& chunk, String codeText);

void Run(Function* pFuncToRun);