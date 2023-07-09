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
		if (errorExpectations.count == 0) errorState.ReportCompilationResult();
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
		Log::Info("The following test failed:\n%s", testCode);
		Log::Info("Expected output was:\n%s\nWe got:\n%s", outputExpectation, output.pData);
		errorCount++;
	}
	FreeString(output);

	// Verify errors
	bool failed = false;
	for (String expectation : errorExpectations) {
		bool found = false;
		for (Error error : errorState.errors) {
			if (error.message == expectation) {
				found = true;
				break;
			}
		}

		if (!found) {
			Log::Info("The following test failed:\n%s", testCode);
			Log::Info("Expected the following error, but it did not occur\n'%s'\n", expectation.pData);
			errorCount++;
			failed = true;
		}
	}
	if (failed) {
		Log::Info("Instead we got the following errors:");
		errorState.ReportCompilationResult();
	}

	// Yeet memory
	compilerMemory.Finished();
	return errorCount;
}

void Values() {
	StartTest("Values");
	int errorCount = 0;
	{
		const char* basicLiteralValues = 
			"print(7);"
			"print(true);"
			"print(false);"
			"print(5.231);";

		const char* expectation =
			"7\n"
			"true\n"
			"false\n"
			"5.231\n";
		
		errorCount += RunCompilerOnTestCase(basicLiteralValues, expectation, ResizableArray<String>());

		const char* typeLiterals = 
			"print(Type);\n"
			"print(i32);\n"
			"print(f32);\n"
			"print(bool);\n"
			"print(fn () -> void);\n"
			"print(fn (i32) -> void);\n"
			"print(fn () -> f32);\n"
			"print(fn (i32, f32, bool) -> i32);";

		expectation =
			"Type\n"
			"i32\n"
			"f32\n"
			"bool\n"
			"fn () -> void\n"
			"fn (i32) -> void\n"
			"fn () -> f32\n"
			"fn (i32, f32, bool) -> i32\n";
		
		errorCount += RunCompilerOnTestCase(typeLiterals, expectation, ResizableArray<String>());
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void ArithmeticOperators() {
	StartTest("Arithmetic Operators");
	int errorCount = 0;
	{
		const char* addition = 
			"print(5+2);\n"
			"print(5.0+2);\n"
			"print(5+2.0);\n";
		const char* expectation =
			"7\n"
			"7\n"
			"7\n";
		errorCount += RunCompilerOnTestCase(addition, expectation, ResizableArray<String>());

		const char* subtraction = 
			"print(5-2);\n"
			"print(5.0-2);\n"
			"print(5-2.0);\n";
		expectation =
			"3\n"
			"3\n"
			"3\n";
		errorCount += RunCompilerOnTestCase(subtraction, expectation, ResizableArray<String>());

		const char* multiplication = 
			"print(5*2);\n"
			"print(5.0*2);\n"
			"print(5*2.0);\n";
		expectation =
			"10\n"
			"10\n"
			"10\n";
		errorCount += RunCompilerOnTestCase(multiplication, expectation, ResizableArray<String>());

		const char* division = 
			"print(5/2);\n"
			"print(5.0/2);\n"
			"print(5/2.0);\n";
		expectation =
			"2\n"
			"2.5\n"
			"2.5\n";
		errorCount += RunCompilerOnTestCase(division, expectation, ResizableArray<String>());

		// Test bad combinations
		{
			const char* invalidTypes =
				"print(5 + bool);\n";
			ResizableArray<String> expectedErrors;
			defer(expectedErrors.Free());
			expectedErrors.PushBack("Invalid types (i32, Type) used with op \"+\"");
			errorCount += RunCompilerOnTestCase(invalidTypes, "", expectedErrors);
		}

		{
			const char* invalidTypes2 =
				"print(true * 2.0);\n";
			ResizableArray<String> expectedErrors;
			defer(expectedErrors.Free());
			expectedErrors.PushBack("Invalid types (bool, f32) used with op \"*\"");
			errorCount += RunCompilerOnTestCase(invalidTypes2, "", expectedErrors);
		}
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

int main() {
	// TODO: Move to program structure
    InitTypeTable();
	InitTokenToOperatorMap();

	Values();
	ArithmeticOperators();

    __debugbreak();
    return 0;
}