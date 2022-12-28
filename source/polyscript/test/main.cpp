// Copyright 2020-2022 David Colson. All rights reserved.

#include <log.h>
#include <resizable_array.inl>
#include <defer.h>
#include <stdint.h>
#include <string_builder.h>
#include <light_string.h>
#include <scanning.h>
#include <linear_allocator.h>

#include "parser.h"
#include "virtual_machine.h"
#include "lexer.h"
#include "code_gen.h"


 // Type checking?
// Firstly, climb up the tree, taking the types from each leaf node, and pulling them up to the parent nodes
// The operators in expressions need to be checked if they take the two types, and what the return type is
// the the binary node stores it's result type and we continue up the tree

// Need some operation like
// OperatorCheck(Operator, TypeA, TypeB)
// Operators will probably be defined per type will they not?

int main() {
    InitValueTables();

    String actualCode;
    //actualCode = "( 5 - (12+3) ) * 12 / 3";
    actualCode = "5<3 && 12 >= 14 || 9<15";

    ResizableArray<Token> tokens = Tokenize(&g_Allocator, actualCode);
    defer(tokens.Free());

    LinearAllocator astMemory;
    Ast::Expression* pAst = ParseToAst(tokens, &astMemory);

    Log::Debug("---- AST -----");
    DebugAst(pAst);

    CodeChunk chunk;
    defer(chunk.code.Free());
    defer(chunk.constants.Free());

    CodeGen(pAst, &chunk);
    chunk.code.PushBack((uint8_t)OpCode::Print);
    chunk.code.PushBack((uint8_t)OpCode::Return);

    Disassemble(chunk);
    
    Log::Info("---- Program Running -----");

    Run(&chunk);

    //astMemory.Finished();

    __debugbreak();
    return 0;
}