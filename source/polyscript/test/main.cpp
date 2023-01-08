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
// [ ] Globals assignment
// [ ] Typed variable declaration
// [ ] Variable declaration without initializer
// [ ] Move error state to it's own file
// [ ] Instead of storing pLocation and a length in tokens, store a String type, so we can more easily compare it and do useful things with it (replace strncmp with it)
// [ ] Consider removing the grouping AST node, serves no purpose and the ast can enforce the structure

int main() {
    InitValueTables();

    String actualCode;
    actualCode = "\
        lucy:= 5+5*2;\n\
        print( (5 - (lucy+5.0)) * 12 / 3 );";

    //actualCode = "(2<3 && 12.4 >= 14 || 9<15);";

    LinearAllocator compilerMemory;

    ErrorState errorState;
    errorState.Init(&compilerMemory);

    // Tokenize
    ResizableArray<Token> tokens = Tokenize(&compilerMemory, actualCode);
    defer(tokens.Free());

    // Parse
    ParsingState parser;
    ResizableArray<Ast::Statement*> program = parser.InitAndParse(tokens, &errorState, &compilerMemory);

    // Type check
    TypeCheckProgram(program, &errorState);

    // Error report
    bool success = errorState.ReportCompilationResult();

    Log::Debug("---- AST -----");
    DebugAst(program);

    if (success) {
        // Compile to bytecode
        CodeChunk chunk;
        defer(chunk.code.Free());
        defer(chunk.constants.Free());

        CodeGen(program, &chunk, &errorState);

        // For debugging
        Disassemble(chunk);
    
        Log::Info("---- Program Running -----");
        Run(&chunk);
    }

    //astMemory.Finished();

    __debugbreak();
    return 0;
}