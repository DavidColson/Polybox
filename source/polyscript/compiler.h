#include <light_string.h>
#include <resizable_array.h>
#include <linear_allocator.h>

#include "parser.h"

struct Function;
struct Scope;

struct Compiler {
    // Input
    String code;
    bool bPrintAst = false;
    bool bPrintByteCode = false;

    // Output
    ResizableArray<Token> tokens;
    ResizableArray<Ast::Statement*> program;
    Scope* pGlobalScope;
    Function* pTopLevelFunction;
    ErrorState errorState;

    // State
    LinearAllocator compilerMemory;
};

void CompileCode(Compiler& compilerState);