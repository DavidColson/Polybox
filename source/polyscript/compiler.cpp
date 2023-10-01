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

    // Tokenize
    Tokenize(compilerState);

    // Parse
    Parse(compilerState);

    // Type check
    TypeCheckProgram(compilerState);

    if (compilerState.bPrintAst) {
        Log::Debug("---- AST -----");
        DebugStatements(compilerState.program);
    }

    // Compile to bytecode
    if (compilerState.errorState.errors.count == 0) {
        ResizableArray<Ast::Declaration*> emptyParams;
        compilerState.pTopLevelFunction = CodeGen(compilerState.program, emptyParams, "<script>", &compilerState.errorState, &compilerState.compilerMemory);

        if (compilerState.bPrintByteCode) {
            Log::Debug("---- Disassembly -----");
            Disassemble(compilerState.pTopLevelFunction, compilerState.code);
        }
    }
}