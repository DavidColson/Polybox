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

enum class ValueType {
    Float,
    Bool
};

struct Value {
    ValueType m_type;
    union {
        bool m_boolValue;
        double m_floatValue;
    };
};

inline Value MakeValue(bool value) {
    Value v;
    v.m_type = ValueType::Bool;
    v.m_boolValue = value;
    return v;
}

inline Value MakeValue(float value) {
    Value v;
    v.m_type = ValueType::Float;
    v.m_floatValue = value;
    return v;
}

struct CodeChunk {
    ResizableArray<Value> constants;  // This should be a variant or something at some stage?
    ResizableArray<uint8_t> code;
};

uint8_t* DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction);

void Disassemble(CodeChunk& chunk);

void Run(CodeChunk* pChunkToRun);