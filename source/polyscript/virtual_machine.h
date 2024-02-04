// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"

struct String;
struct Compiler;
struct Program;

namespace OpCode {
enum Enum : u8 {

    // Opcode          |   Instructions
    // ---------------------------------
    // Arithmetic
    Negate,             // type
    Add,                // type
    Subtract,           // type
    Multiply,           // type
    Divide,             // type

    // Logical
    Not,                // type

    // Comparison
    Greater,            // type
    Less,               // type
    Equal,              // type
    NotEqual,           // type
    GreaterEqual,       // type
    LessEqual,          // type

    // Flow control
    JmpIfFalse,         // ipOffset
    JmpIfTrue,          // ipOffset
    Jmp,                // ipOffset
    Loop,               // ipOffset
    Return,             // ---
    Call,               // nArgs (number of stack values to pass as arguments)

    // Stack manipulation
    PushConstant,       // value (to push)
	PushStruct,         // size
    SetLocal,           // stackIndex
    PushLocal,          // stackIndex
	SetField,           // fieldSize, fieldOffset
	SetFieldPtr,        // fieldSize, fieldOffset
	PushField,          // fieldSize, fieldOffset
	PushFieldPtr,       // fieldSize, fieldOffset
    Pop,                // ---

    // Misc
	Cast,               // toType, fromType
    Print               // type
};
}

// instruction is 9 bytes, will end up as 16 due to alignment, so there is some wasted
// space, decided that this probably isn't worth worrying about for now. 
// Could reduce it with pragma pack, worth investigating if that's better or not at some point
// But for now this is simple and easily understood.
struct Instruction {
    OpCode::Enum opcode;
    union {
        // All possible arguments to the above instructions are packed in here
        // Mostly for convenience and readability of the VM code.
        Value constant;
        TypeInfo::TypeTag type;
        u64 ipOffset;
        u64 nArgs;
        u64 stackIndex;
        u64 size;
        struct {
            u32 fieldSize;
            u32 fieldOffset;
        };
        struct {
            TypeInfo::TypeTag toType;
            TypeInfo::TypeTag fromType;
        };
    };
};

String DisassembleInstruction(Program* pProgram, Instruction* pInstruction);

void DisassembleProgram(Compiler& compilerState);

void Run(Program* pProgramToRun);
