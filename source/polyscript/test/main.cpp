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
// [ ] Instead of storing pLocation and a length in tokens, store a String type, so we can more easily compare it and do useful things with it (replace strncmp with it)

int main() {
    InitValueTables();

    String actualCode;
    actualCode = "\
        myVar:= 5+5*2;\n\
        print( (5 - (12+5.0)) * 12 / 3 );";

    //actualCode = "(2<3 && 12.4 >= 14 || 9<15);";

    LinearAllocator compilerMemory;

    // Tokenize
    ResizableArray<Token> tokens = Tokenize(&compilerMemory, actualCode);
    defer(tokens.Free());

    // Parse
    ParsingState parser;
    ResizableArray<Ast::Statement*> program = parser.InitAndParse(tokens, &compilerMemory);

    // Type check
    TypeCheckProgram(program, &parser);

    // Error report
    bool success = ReportCompilationResult(parser);

    Log::Debug("---- AST -----");
    DebugAst(program);

    if (false) {
        // Compile to bytecode
        CodeChunk chunk;
        defer(chunk.code.Free());
        defer(chunk.constants.Free());

        CodeGen(program, &chunk);

        // For debugging
        Disassemble(chunk);
    
        Log::Info("---- Program Running -----");
        Run(&chunk);
    }

    //astMemory.Finished();

    __debugbreak();
    return 0;
}