#include <light_string.h>
#include <resizable_array.h>
#include <linear_allocator.h>

#include "parser.h"

struct Program;
struct Function;
struct Scope;

struct Compiler {
    // Input
    String code;
    bool bPrintAst = false;
    bool bPrintByteCode = false;

    // Compilation Byproducts
    ResizableArray<Token> tokens;
    ResizableArray<Ast::Statement*> syntaxTree;
    Scope* pGlobalScope;
    
    // Output
    Program* pProgram;
    ErrorState errorState;

    // State
    LinearAllocator compilerMemory; // working memory for the compiler
    LinearAllocator outputMemory; // memory for the output bytecode and program
};

void CompileCode(Compiler& compilerState);