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
#include <stdio.h>

#include "parser.h"
#include "virtual_machine.h"
#include "lexer.h"
#include "code_gen.h"
#include "type_checker.h"

// TODO: 
// [ ] Move error state to it's own file
// [ ] Instead of storing pLocation and a length in tokens, store a String type, so we can more easily compare it and do useful things with it (replace strncmp with it)
// [ ] Consider removing the grouping AST node, serves no purpose and the ast can enforce the structure

int main() {
    InitTypeTable();
	InitTokenToOperatorMap();
    LinearAllocator compilerMemory;

    FILE* pFile;
    fopen_s(&pFile, "test.ps", "r");
    if (pFile == NULL) {
        return 0;
    }

    String actualCode;
    {
        uint32_t size;
        fseek(pFile, 0, SEEK_END);
        size = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);

        actualCode = AllocString(size, &compilerMemory);
        fread(actualCode.pData, size, 1, pFile);
        fclose(pFile);
    }

    ErrorState errorState;
    errorState.Init(&compilerMemory);

    // Tokenize
    ResizableArray<Token> tokens = Tokenize(&compilerMemory, actualCode);
    defer(tokens.Free());

    // Parse
    ResizableArray<Ast::Statement*> program = InitAndParse(tokens, &errorState, &compilerMemory);

    // Type check
    if (errorState.errors.count == 0) {
        TypeCheckProgram(program, &errorState, &compilerMemory);
    }

    // Error report
    bool success = errorState.ReportCompilationResult();

    //Log::Debug("---- AST -----");
    //DebugStatements(program);

    if (success) {
        // Compile to bytecode
        ResizableArray<Ast::Declaration*> emptyParams;
        Function* pFunc = CodeGen(program, emptyParams, "<script>", &errorState);
        defer(FreeFunction(pFunc));
    
        Log::Debug("---- Disassembly -----");
        Disassemble(pFunc, actualCode);
        
        Log::Info("---- Program Running -----");
        Run(pFunc);
    }

    //astMemory.Finished();
    __debugbreak();
    return 0;
}