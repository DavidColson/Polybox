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

    // Misc
    Print,
    Return,
    Pop
};


struct CodeChunk {
    ResizableArray<Value> constants;  // This should be a variant or something at some stage?
    ResizableArray<uint8_t> code;
};

uint8_t* DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction);

void Disassemble(CodeChunk& chunk);

void Run(CodeChunk* pChunkToRun);