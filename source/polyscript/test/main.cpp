// Copyright 2020-2022 David Colson. All rights reserved.

#include <log.h>
#include <resizable_array.inl>
#include <defer.h>
#include <stdint.h>
#include <string_builder.h>
#include <light_string.h>
#include <scanning.h>
#include <linear_allocator.h>
#include <maths.h>

#include "parser.h"
#include "virtual_machine.h"
#include "lexer.h"
#include "code_gen.h"
#include "type_checker.h"

// TODO: 


int main() {
    InitValueTables();

    String actualCode;
    actualCode = "( 5 - (12+5.0)) * 12 / 3";
    //actualCode = "false<3 && 12.4 >= 14 || 9<15";

    LinearAllocator compilerMemory;

    // Tokenize
    ResizableArray<Token> tokens = Tokenize(&compilerMemory, actualCode);
    defer(tokens.Free());

    // Parse
    ParsingState parser;
    Ast::Expression* pAst = parser.InitAndParse(tokens, &compilerMemory);

    // Type check
    TypeCheck(pAst, &parser);

    // Error report
    bool success = ReportCompilationResult(parser);

    Log::Debug("---- AST -----");
    DebugAst(pAst);

    if (success) {
        // Compile to bytecode
        CodeChunk chunk;
        defer(chunk.code.Free());
        defer(chunk.constants.Free());

        CodeGen(pAst, &chunk);
        chunk.code.PushBack((uint8_t)OpCode::Print);
        chunk.code.PushBack((uint8_t)OpCode::Return);

        // For debugging
        Disassemble(chunk);
    
        Log::Info("---- Program Running -----");
        Run(&chunk);
    }

    //astMemory.Finished();

    __debugbreak();
    return 0;
}