// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"

#include <resizable_array.h>

enum class OpCode : uint8_t {
    LoadConstant,
    
    // Arithmetic
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,
    
    // Logical
    Not,
    And,
    Or,

    // Comparison
    Greater,
    Less,
    Equal,
    NotEqual,
    GreaterEqual,
    LessEqual,

    SetLocal,
    GetLocal,

    // Misc
    Print,
    Return,
    Pop
};


struct CodeChunk {
    ResizableArray<Value> constants;
    ResizableArray<uint8_t> code;
    uint8_t m_globalsCount;
};

uint8_t* DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction);

void Disassemble(CodeChunk& chunk);

void Run(CodeChunk* pChunkToRun);