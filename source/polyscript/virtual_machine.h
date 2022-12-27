#pragma once

#include <resizable_array.h>
#include <stdint.h>

enum class OpCode : uint8_t {
    LoadConstant,
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,
    Print,
    Return
};

struct CodeChunk {
    ResizableArray<double> constants;  // This should be a variant or something at some stage?
    ResizableArray<uint8_t> code;
};

uint8_t* DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction);

void Disassemble(CodeChunk& chunk);

void Run(CodeChunk* pChunkToRun);