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
#include <testing.h>

#include "parser.h"
#include "virtual_machine.h"
#include "lexer.h"
#include "code_gen.h"
#include "type_checker.h"

// TODO: 
// [ ] Move error state to it's own file
// [ ] Instead of storing pLocation and a length in tokens, store a String type, so we can more easily compare it and do useful things with it (replace strncmp with it)
// [ ] Consider removing the grouping AST node, serves no purpose and the ast can enforce the structure

StringBuilder* pLogCollectorBuilder { nullptr };
void LogCollectorFunc(Log::LogLevel level, String message) {
	pLogCollectorBuilder->Append(message);
}

int RunCompilerOnTestCase(const char* testCode, const char* outputExpectation, ResizableArray<String> errorExpectations) {
	int errorCount = 0;
	StringBuilder logCollector;

	// Setup log to hide output and collect it for ourselves
	Log::LogConfig config;
	config.silencePrefixes = true;
	config.winOutput = false;
	config.consoleOutput = false;
	config.fileOutput = false;
	config.customHandler1 = LogCollectorFunc;
	pLogCollectorBuilder = &logCollector;
	Log::SetConfig(config);

	// Setup state
	ErrorState errorState;
	LinearAllocator compilerMemory;
	errorState.Init(&compilerMemory);
	
	// Run test
	// TODO: This should probably be wrapped up in a nice "compile and run" function
	{
		// Tokenize
		ResizableArray<Token> tokens = Tokenize(&compilerMemory, testCode);
		defer(tokens.Free());

		// Parse
		ResizableArray<Ast::Statement*> program = InitAndParse(tokens, &errorState, &compilerMemory);

		// Type check
		if (errorState.errors.count == 0) {
			TypeCheckProgram(program, &errorState, &compilerMemory);
		}

		// Error report
		bool success = errorState.errors.count == 0;

		//Log::Debug("---- AST -----");
		//DebugStatements(program);

		if (success) {
			// Compile to bytecode
			ResizableArray<Ast::Declaration*> emptyParams;
			Function* pFunc = CodeGen(program, emptyParams, "<script>", &errorState, &compilerMemory);
			defer(FreeFunction(pFunc));
    
			//Log::Debug("---- Disassembly -----");
			//Disassemble(pFunc, actualCode);
        
			Run(pFunc);
		}
	}

	// Reset Log
	Log::SetConfig(Log::LogConfig());
	pLogCollectorBuilder = nullptr;
		
	// Verify output
	String output = logCollector.CreateString();
	if (output != outputExpectation) {
		Log::Info("FAIL");
		Log::Info("Expected output was:\n%s\nWe got:\n%s", outputExpectation, output.pData);
		errorCount++;
	}
	FreeString(output);

	// Verify errors
	for (String expectation : errorExpectations) {
		bool found = false;
		for (Error error : errorState.errors) {
			if (error.message == expectation) {
				found = true;
				break;
			}
		}

		if (!found) {
			Log::Info("FAIL: Expected the following error, but it did not occur\n'%s'", expectation.pData);
			errorCount++;
		}
	}

	// Yeet memory
	compilerMemory.Finished();
	return errorCount;
}

void BasicExpressions() {
	StartTest("Basic Expressions");
	int errorCount = 0;
	{
		const char* subtraction = 
			"print(5-1);\n"
			"print(10-5);\n"
			"print(4-7);";

		const char* expectation =
			"4\n"
			"5\n"
			"-3\n";
		
		errorCount += RunCompilerOnTestCase(subtraction, expectation, ResizableArray<String>());
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

int main() {
	// TODO: Move to program structure
    InitTypeTable();
	InitTokenToOperatorMap();

	BasicExpressions();

    __debugbreak();
    return 0;
}