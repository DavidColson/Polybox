#include "compiler.h"

#include "lexer.h"
#include "type_checker.h"
#include "code_gen.h"
#include "virtual_machine.h"
#include "parser.h"

#include <resizable_array.inl>
#include <log.h>

void CompileCode(Compiler& compilerState) {
    compilerState.errorState.Init(&compilerState.compilerMemory);
    compilerState.pGlobalScope = nullptr;
    compilerState.pProgram = nullptr;

    // Tokenize
    Tokenize(compilerState);

    // Parse
    Parse(compilerState);

    // Type check
    TypeCheckProgram(compilerState);

    if (compilerState.bPrintAst) {
        Log::Debug("---- AST -----");
        DebugStatements(compilerState.syntaxTree);
    }

    // Compile to bytecode
    if (compilerState.errorState.errors.count == 0) {
        CodeGenProgram(compilerState);
        if (compilerState.bPrintByteCode) {
            Log::Debug("---- Disassembly -----");
            DisassembleProgram(compilerState);
        }
    }
}