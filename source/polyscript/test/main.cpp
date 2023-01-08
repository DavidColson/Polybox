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
// [ ] Move error state to it's own file
// [ ] Instead of storing pLocation and a length in tokens, store a String type, so we can more easily compare it and do useful things with it (replace strncmp with it)

int main() {
    InitValueTables();

    String actualCode;
    actualCode = "\
        myVar:= 5+5*2;\n\
        print( (5 - (12+5.0)) * 12 / 3 );";

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

    // Bind variables?
    
    // Typecheck
    // Figure out what the type of the variable is and store it in it's AST node
    // We won't support out of order definitions for now, so error if it's not present in the table.

    // Codegen phase
    // As we encounter a var decl, put the var in a hash table
    // Decide what it's index will be in the output globals array
    // As we encounter a var get or set, lookup the var in the table to see if it's defined
    // If it is, get the address in the globals array and push that as your constant operand to the bytecode

    // Runtime
    // var decls are removed from the output code
    // var get and set can just use their operands to lookup in the globals array

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