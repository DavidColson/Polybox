// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"

struct String;
struct Compiler;
struct Program;

namespace OpCode {
enum Enum : u8 {

    // Opcode      | Uses Type  | Operands     		| Stack
    // ----------------------------------------------------------------------------------
    // Arithmetic
    Negate,       // Yes		|					| [value] -> [value]
    Add,          // Yes		|					| [value1][value2] -> [value]
    Subtract,     // Yes		|					| [value1][value2] -> [value]
    Multiply,     // Yes		|					| [value1][value2] -> [value]
    Divide,       // Yes		|					| [value1][value2] -> [value]

    // Logical
    Not,          // Yes		|					| [boolValue] -> [boolValue]

    // Comparison
    Greater,      // Yes		|					| [value1][value2] -> [boolValue]
    Less,         // Yes		|					| [value1][value2] -> [boolValue]
    Equal,        // Yes		|					| [value1][value2] -> [boolValue]
    NotEqual,     // Yes		|					| [value1][value2] -> [boolValue]
    GreaterEqual, // Yes		|					| [value1][value2] -> [boolValue]
    LessEqual,    // Yes		|					| [value1][value2] -> [boolValue]

    // Flow control
    JmpIfFalse,   // Yes		| 16b ipOffset		| [boolValue] -> [boolValue]
    JmpIfTrue,    // Yes		| 16b ipOffset		| [boolValue] -> [boolValue]
    Jmp,          // Yes		| 16b ipOffset		| 
    Return,       // No			|					| [retValue] -> [retValue]
    Call,         // No			| 16b argCount		| [argCount number of Params] -> []

    // Stack manipulation
    Const,        // Yes		| 32b value			| [] -> [value]
	Load,         // No			| 16b offset		| [address] -> [value]
    Store,        // No			| 16b offset		| [address][value] -> [value]
	LocalAddr,	  // No			| 16b offset		| [] -> [address]
	Drop,		  // No			|					| [value] -> []
	Copy,		  // No			| destOff, srcOff	| [srcAddr][dstAddr][size] -> [srcAddr]
	StackChange,  // No			| 					| [offset] -> []

    // Misc
	Cast,         // Yes		| 16b fromTypeId	| [value] -> [castedValue]
    Print         // Yes		|					| [value] -> []
};
}

// Instructions are minimum 2 bytes, just a header. They can optionally have arguments that are some multiple
// of 2 bytes, as per the documentation in the opcode table above
struct InstructionHeader {
	OpCode::Enum opcode;
	TypeInfo::TypeTag type;
};

String DisassembleInstruction(Program* pProgram, u16* pInstruction, u16& outOffset);
void DisassembleProgram(Compiler& compilerState);

void Run(Program* pProgramToRun);
