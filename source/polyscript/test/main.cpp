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
	defer(FreeString(output));
	if (output != outputExpectation) {
		Log::Info("The following test failed:\n%s", testCode);
		Log::Info("Expected output was:\n%s\nWe got:\n%s", outputExpectation, output.pData);
		errorCount++;
	}

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
			Log::Info("Expected the following error, but it did not occur\n'%s'\n", expectation.pData);
			errorCount++;
			failed = true;
		}
	}
	if (errorExpectations.count != errorState.errors.count) {
		Log::Info("Expected %d errors, but got %d", errorExpectations.count, errorState.errors.count);
		failed = true;
	}

	if (failed) {
		Log::Info("In test:\n%s", testCode);
		Log::Info("We got the following output:\n%s ", output.pData);
		Log::Info("And the following Errors: ");
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
			"print(5.0+2.0);\n";
		const char* expectation =
			"7\n"
			"7\n";
		errorCount += RunCompilerOnTestCase(addition, expectation, ResizableArray<String>());

		const char* subtraction = 
			"print(5-2);\n"
			"print(5.0-2.0);\n";
		expectation =
			"3\n"
			"3\n";
		errorCount += RunCompilerOnTestCase(subtraction, expectation, ResizableArray<String>());

		const char* multiplication = 
			"print(5*2);\n"
			"print(5.0*2.0);\n";
		expectation =
			"10\n"
			"10\n";
		errorCount += RunCompilerOnTestCase(multiplication, expectation, ResizableArray<String>());

		const char* division = 
			"print(5/2);\n"
			"print(5.0/2.0);\n";
		expectation =
			"2\n"
			"2.5\n";
		errorCount += RunCompilerOnTestCase(division, expectation, ResizableArray<String>());

		const char* unary = 
			"print(-5);\n"
			"print(--5);\n"
			"print(1--5);\n"
			"print(---5);\n";
		expectation = 
			"-5\n"
			"5\n"
			"6\n"
			"-5\n";
		errorCount += RunCompilerOnTestCase(unary, expectation, ResizableArray<String>());

		// Test bad combinations
		{
			const char* invalidTypes =
				"print(5 + bool);\n"
				"print(true * 2.0);\n"
				"print(-true);";
			ResizableArray<String> expectedErrors;
			defer(expectedErrors.Free());
			expectedErrors.PushBack("Invalid types (i32, Type) used with op \"+\"");
			expectedErrors.PushBack("Invalid types (bool, f32) used with op \"*\"");
			expectedErrors.PushBack("Invalid type (bool) used with op \"-\"");
			errorCount += RunCompilerOnTestCase(invalidTypes, "", expectedErrors);
		}
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void LogicalOperators() {
	StartTest("Logical Operators");
	int errorCount = 0;
	{
		const char* lessThan = 
			"print(2 < 5);\n"
			"print(5 < 2);\n"
			"print(5 < 5);\n"
			"print(5.0 < 2.0);\n";
		const char* expectation =
			"true\n"
			"false\n"
			"false\n"
			"false\n";
		errorCount += RunCompilerOnTestCase(lessThan, expectation, ResizableArray<String>());

		const char* greaterThan = 
			"print(2 > 5);\n"
			"print(5 > 2);\n"
			"print(5.0 > 2.0);\n";
		expectation =
			"false\n"
			"true\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(greaterThan, expectation, ResizableArray<String>());

		const char* lessThanEqual = 
			"print(2 <= 5);\n"
			"print(5 <= 5);\n"
			"print(5 <= 2);\n"
			"print(2.0 <= 2.0);\n";
		expectation =
			"true\n"
			"true\n"
			"false\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(lessThanEqual, expectation, ResizableArray<String>());

		const char* greaterThanEqual = 
			"print(2 <= 5);\n"
			"print(5 <= 5);\n"
			"print(5 <= 2);\n"
			"print(2.0 <= 2.0);\n";
		expectation =
			"true\n"
			"true\n"
			"false\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(lessThanEqual, expectation, ResizableArray<String>());

		const char* equal = 
			"print(2 == 5);\n"
			"print(5 == 5);\n"
			"print(2.0 == 2.0);\n";
		expectation =
			"false\n"
			"true\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(equal, expectation, ResizableArray<String>());

		const char* notEqual = 
			"print(2 != 5);\n"
			"print(5 != 5);\n"
			"print(2.0 != 2.0);\n";
		expectation =
			"true\n"
			"false\n"
			"false\n";
		errorCount += RunCompilerOnTestCase(notEqual, expectation, ResizableArray<String>());

		const char* andOp = 
			"print(true && false);\n"
			"print(true && true);\n"
			"print(false && false);\n";
		expectation =
			"false\n"
			"true\n"
			"false\n";
		errorCount += RunCompilerOnTestCase(andOp, expectation, ResizableArray<String>());

		const char* orOp = 
			"print(true || false);\n"
			"print(true || true);\n"
			"print(false || false);\n";
		expectation =
			"true\n"
			"true\n"
			"false\n";
		errorCount += RunCompilerOnTestCase(orOp, expectation, ResizableArray<String>());

		const char* notOp = 
			"print(!false);\n"
			"print(!true);\n";
		expectation =
			"true\n"
			"false\n";
		errorCount += RunCompilerOnTestCase(notOp, expectation, ResizableArray<String>());

		// Test bad combinations
		const char* invalidTypes =
			"print(true < 5);\n"
			"print(5.0 && 5.0);\n"
			"print(0 || 3);\n"
			"print(true < false);\n"
			"print(!3.2);\n";
			ResizableArray<String> expectedErrors;
		defer(expectedErrors.Free());
		expectedErrors.PushBack("Invalid types (bool, i32) used with op \"<\"");
		expectedErrors.PushBack("Invalid types (f32, f32) used with op \"&&\"");
		expectedErrors.PushBack("Invalid types (i32, i32) used with op \"||\"");
		expectedErrors.PushBack("Invalid types (bool, bool) used with op \"<\"");
		expectedErrors.PushBack("Invalid type (f32) used with op \"!\"");
		errorCount += RunCompilerOnTestCase(invalidTypes, "", expectedErrors);
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void Expressions() {
	StartTest("Expressions");
	int errorCount = 0;
	{
		// Grouping
		// Operator precedence

		const char* grouping = 
			"print((10 - 20) / (2 - 4));\n"
			"print(((1 + (5 - (8 / 2))) * 2) + 2);\n";
		const char* expectation =
			"5\n"
			"6\n";
		errorCount += RunCompilerOnTestCase(grouping, expectation, ResizableArray<String>());

		// Operator precedence
		const char* precedence = 
			"print(2 * 2 + 4 / 2 - 1);\n"
			"print(5 * -5);\n"
			"print(5 + 1 < 7 * 2 == -5 > (2 * 10));\n";
		expectation =
			"5\n"
			"-25\n"
			"false\n";
		errorCount += RunCompilerOnTestCase(precedence, expectation, ResizableArray<String>());

		// test logical operators
		const char* logic = 
			"print(true && false);\n"
			"print(true || false);\n"
			"print(true && false || true);\n"
			"print(true && (false || true));\n";
		expectation = 
			"false\n"
			"true\n"
			"true\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(logic, expectation, ResizableArray<String>());

		// test invalid grouping expressions
		const char* invalidGrouping = 
			"print(5 + (2 * 2);\n"
			"print(5 + ((2 * 2) + 1);\n"
			"print(5 + 2 * 2));\n"
			"print(5 + 2+)1 * 2);\n";
		ResizableArray<String> expectedErrors;
		defer(expectedErrors.Free());
		expectedErrors.PushBack("Expected \")\" to close print expression");
		expectedErrors.PushBack("Expected \";\" at the end of this statement");
		expectedErrors.PushBack("Expected \";\" at the end of this statement");
		errorCount += RunCompilerOnTestCase(invalidGrouping, "", expectedErrors);

		// test mismatched types in and/or expressions
		const char* invalidLogic = 
			"print(5 && true);\n"
			"print(true || 5);\n";
		expectedErrors.Resize(0);
		expectedErrors.PushBack("Invalid types (i32, bool) used with op \"&&\"");
		expectedErrors.PushBack("Invalid types (bool, i32) used with op \"||\"");
		errorCount += RunCompilerOnTestCase(invalidLogic, "", expectedErrors);
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void ControlFlow() {
	StartTest("Control Flow");
	int errorCount = 0;
	{
		// test all the possible if cases
		const char* ifStatements = 
			"if true { print(1); }\n"
			"if false { print(2); }\n"
			"if true { print(3); } else { print(4); }\n"
			"if false { print(5); } else { print(6); }\n"
			"if true { print(7); } else if false { print(8); } else { print(9); }\n"
			"if false { print(10); } else if true { print(11); } else { print(12); }\n"
			"if false { print(13); } else if false { print(14); } else { print(15); }\n";
		const char* expectation =	
			"1\n"
			"3\n"
			"6\n"
			"7\n"
			"11\n"
			"15\n";
		errorCount += RunCompilerOnTestCase(ifStatements, expectation, ResizableArray<String>());

		// test while loops
		const char* whileLoops = 
			"i := 0;\n"
			"while i < 5 { print(i); i = i + 1; }\n";
		expectation =
			"0\n"
			"1\n"
			"2\n"
			"3\n"
			"4\n";
		errorCount += RunCompilerOnTestCase(whileLoops, expectation, ResizableArray<String>());
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
	LogicalOperators();
	Expressions();
	ControlFlow();

    __debugbreak();
    return 0;
}