// Copyright 2020-2022 David Colson. All rights reserved.

#include <resizable_array.inl>
#include <defer.h>
#include <light_string.h>
#include <linear_allocator.h>
#include <testing.h>

#include "parser.h"
#include "virtual_machine.h"
#include "lexer.h"
#include "code_gen.h"
#include "type_checker.h"
#include "compiler_explorer.h"
#include "tests_framework.h"

// TODO: 
// [ ] Move error state to it's own file
// [ ] Instead of storing pLocation and a length in tokens, store a String type, so we can more easily compare it and do useful things with it (replace strncmp with it)
// [ ] Consider removing the grouping AST node, serves no purpose and the ast can enforce the structure

void RunTestPlayground() {
	LinearAllocator compilerMemory;

    FILE* pFile;
    fopen_s(&pFile, "test.ps", "r");
    if (pFile == NULL) {
        return;
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

    Log::Debug("---- AST -----");
    DebugStatements(program);

    if (success) {
        // Compile to bytecode
        ResizableArray<Ast::Declaration*> emptyParams;
        Function* pFunc = CodeGen(program, emptyParams, "<script>", &errorState, &compilerMemory);
        defer(FreeFunction(pFunc));
    
        Log::Debug("---- Disassembly -----");
        Disassemble(pFunc, actualCode);
        
        Log::Info("---- Program Running -----");
        Run(pFunc);
    }
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

void Declarations() {
	StartTest("Declarations");
	int errorCount = 0;
	{
		// At some point expand this to print the type of the declarations so we know they are infered correctly
		// Right now there is no "type()" function that'll tell us the type
		const char* basicDeclaration = 
			"i := 5;\n"
			"print(i);\n"
			"a : bool;\n"
			"a = false;\n"
			"print(a);\n"
			"b:f32 = 2.5;\n"
			"print(b);\n"
			"t:Type = i32;\n"
			"print(t);\n";
		const char* expectation =
			"5\n"
			"false\n"
			"2.5\n"
			"i32\n";
		errorCount += RunCompilerOnTestCase(basicDeclaration, expectation, ResizableArray<String>());

		// test for invalid declarations and type mismatches
		const char* invalidDeclarations = 
			"i : 5;\n";
		ResizableArray<String> expectedErrors;
		defer(expectedErrors.Free());
		expectedErrors.PushBack("Expected a type here, potentially missing an equal sign before an initializer?");
		errorCount += RunCompilerOnTestCase(invalidDeclarations, "", expectedErrors);

		const char* invalidDeclarations2 = 
			"j := 22.0\n"
			"k:i32 = 10;\n";
		expectedErrors.Resize(0);
		expectedErrors.PushBack("Expected \";\" to end a previous declaration");
		errorCount += RunCompilerOnTestCase(invalidDeclarations2, "", expectedErrors);

		const char* invalidDeclarations3 = 
			"k:i32 = true;\n";
		expectedErrors.Resize(0);
		expectedErrors.PushBack("Type mismatch in declaration, declared as i32 and initialized as bool");
		errorCount += RunCompilerOnTestCase(invalidDeclarations3, "", expectedErrors);
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void VariableAssignment() {
	StartTest("Variable Assignment");
	int errorCount = 0;
	{
		const char* assignment = 
			"i := 5;\n"
			"i = 10;\n"
			"print(i);\n"
			"i = i + 5 * 10;\n"
			"print(i);\n"
			"b := true;\n"
			"b = 5 * 5 < 10 || true;\n"
			"print(b);\n";
		const char* expectation =
			"10\n"
			"60\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(assignment, expectation, ResizableArray<String>());

		const char* invalidAssignment = 
			"i := 5;\n"
			"i = true;\n"
			"j = 10;\n";
		ResizableArray<String> expectedErrors;
		defer(expectedErrors.Free());
		expectedErrors.PushBack("Type mismatch on assignment, 'i' has type 'i32', but is being assigned a value with type 'bool'");
		expectedErrors.PushBack("Assigning to undeclared variable \'j\', missing a declaration somewhere before?");
		errorCount += RunCompilerOnTestCase(invalidAssignment, "", expectedErrors);
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void Scopes() {
	StartTest("Scopes");
	int errorCount = 0;
	{
		// test that scopes work correctly
		const char* scopes = 
			"i := 5;\n"
			"{} // testing empty scope\n"
			"{\n"
			"	j := 10;\n"
			"	print(j);\n"
			"}\n"
			"print(i);\n";
		const char* expectation =
			"10\n"
			"5\n";
		errorCount += RunCompilerOnTestCase(scopes, expectation, ResizableArray<String>());

		// test for variables being out of scope
		const char* invalidScopes = 
			"i := 5;\n"
			"{\n"
			"   i := 2;\n"
			"	j = 10;\n"
			"}\n"
			"print(j);\n";
		ResizableArray<String> expectedErrors;
		defer(expectedErrors.Free());
		expectedErrors.PushBack("Redefinition of variable 'i'");
		expectedErrors.PushBack("Assigning to undeclared variable 'j', missing a declaration somewhere before?");
		expectedErrors.PushBack("Undeclared variable 'j', missing a declaration somewhere before?");
		errorCount += RunCompilerOnTestCase(invalidScopes, "", expectedErrors);
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void Casting() {
	StartTest("Casting");
	int errorCount = 0;
	{
		// test implicit typecasting on left and right of binary operator
		// TODO: Should print type of these expressions when we can do that
		const char* implicitCasting = 
			"i:i32 = 5;\n"
			"print(i + 5.0);\n"
			"print(5.0 + i);\n";
		const char* expectation =
			"10\n"
			"10\n";
		errorCount += RunCompilerOnTestCase(implicitCasting, expectation, ResizableArray<String>());

		// test explicit typecasting which should cover f32 to i32 and bool to f32 and i32
		const char* explicitCasting = 
			"i:i32 = 5;\n"
			"print(i + as(i32) 5.0);\n"
			"print(as(f32) 5 + i);\n"
			"print(as(i32) true);\n"
			"print(as(f32) true);\n"
			"print(as(bool) 1);\n"
			"print(as(bool) 0.0);\n";
		expectation =
			"10\n"
			"10\n"
			"1\n"
			"1\n"
			"true\n"
			"false\n";
		errorCount += RunCompilerOnTestCase(explicitCasting, expectation, ResizableArray<String>());

		// test invalid casting, when from and to are the same and the casts are not covered above
		const char* invalidCasting = 
			"i:i32 = 5;\n"
			"print(as(i32) i);\n"
			"print(as(Type) i);\n";
		ResizableArray<String> expectedErrors;
		defer(expectedErrors.Free());
		expectedErrors.PushBack("Cast from \"i32\" to \"i32\" is pointless");
		expectedErrors.PushBack("Not possible to cast from type \"i32\" to \"Type\"");
		errorCount += RunCompilerOnTestCase(invalidCasting, "", expectedErrors);
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void Functions() {
	StartTest("Functions");
	int errorCount = 0;
	{
		// Test function type literals
		const char* functionTypeLiterals = 
			"print(fn ());\n"
			"print(fn (i32));\n"
			"print(fn () -> f32);\n"
			"print(fn (i32, f32, bool) -> i32);\n";
		const char* expectation =
			"fn () -> void\n"
			"fn (i32) -> void\n"
			"fn () -> f32\n"
			"fn (i32, f32, bool) -> i32\n";
		errorCount += RunCompilerOnTestCase(functionTypeLiterals, expectation, ResizableArray<String>());

		// Test function declarations
		const char* functionDeclarations = 
			"test := func() { print(1); };\n"
			"test2 := func(i:i32) { print(i); };\n"
			"test3 := func() -> f32 { return 1.0; };\n"
			"test4 := func(i:i32, f:f32, b:bool) -> i32 { return i; };\n"
			"test5 := func(i:i32) -> i32 { return test5(i); };\n"
			"print(test);\n"
			"print(test2);\n"
			"print(test3);\n"
			"print(test4);\n"
			"print(test5);\n";
		expectation =	
			"<fn test>\n"
			"<fn test2>\n"
			"<fn test3>\n"
			"<fn test4>\n"
			"<fn test5>\n";
		errorCount += RunCompilerOnTestCase(functionDeclarations, expectation, ResizableArray<String>());

		// Test function calling
		const char* functionCalling = 
			"test := func() { print(1); };\n"
			"test2 := func(i:i32) { print(i); };\n"
			"test3 := func() -> f32 { return 1.0; };\n"
			"test4 := func(i:i32, f:f32, b:bool) -> i32 { return i; };\n"
			"test5 := func(i:i32) -> bool { return i > 5; };\n"
			"test();\n"
			"test2(5);\n"
			"print(test3());\n"
			"print(test4(5, 2.0, true));\n"
			"print(test5(10));\n";
		expectation =
			"1\n"
			"5\n"
			"1\n"
			"5\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(functionCalling, expectation, ResizableArray<String>());

		// function pointer changing
		const char* functionPointerChanging = 
			"test := func(i: i32) { print(i); };\n"
			"test2 := func(i: i32) { print(i*2); };\n"
			"test(5);\n"
			"test = test2;\n"
			"test(5);\n";
		expectation =
			"5\n"
			"10\n";
		errorCount += RunCompilerOnTestCase(functionPointerChanging, expectation, ResizableArray<String>());

		// TODO: Functions don't have any typechecking errors, but test for parse errors
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

void Structs() {
	StartTest("Structs");
	int errorCount = 0;
	{
		// Test struct declarations
		const char* structDeclarations = 
			"test := struct { i:i32 = 2; f:f32 = 2.0; b:bool = true; };\n"
			"print(test);\n"
			"test2 := struct { i:i32 = 3; f:f32 = 2.0; b:bool = false; };\n"
			"print(test2);\n";
		const char* expectation =	
			"test\n"
			"test2\n";
		errorCount += RunCompilerOnTestCase(structDeclarations, expectation, ResizableArray<String>());

		// Test struct member access
		const char* structMemberAccess = 
			"TestStruct := struct { i:i32; f:f32; b:bool; };\n"
			"instance:TestStruct;"
			"instance.i = 2;\n"
			"instance.f = 4.0;\n"
			"instance.b = true;\n"
			"print(instance.i);\n"
			"print(instance.f);\n"
			"print(instance.b);\n";
		expectation =
			"2\n"
			"4\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(structMemberAccess, expectation, ResizableArray<String>());

		// test struct member being another struct
		const char* structMemberStruct = 
			"TestStruct := struct { i:i32; f:f32; b:bool; };\n"
			"TestStruct2 := struct { s:TestStruct; };\n"
			"instance:TestStruct;\n"
			"instance.i = 2;\n"
			"instance.f = 4.0;\n"
			"instance.b = true;\n"
			"instance2:TestStruct2;\n"
			"instance2.s = instance;\n"
			"print(instance2.s.i);\n"
			"print(instance2.s.f);\n"
			"print(instance2.s.b);\n";
		expectation =
			"2\n"
			"4\n"
			"true\n";
		errorCount += RunCompilerOnTestCase(structMemberStruct, expectation, ResizableArray<String>());

		// TODO: Test struct type check and compile error messages
	}
	errorCount += ReportMemoryLeaks();
	EndTest(errorCount);
}

int main(int argc, char *argv[]) {
	// TODO: Move to program structure
    InitTypeTable();
	InitTokenToOperatorMap();

	Values();
	ArithmeticOperators();
	LogicalOperators();
	Expressions();
	ControlFlow();
	Declarations();
	VariableAssignment();
	Scopes();
	Casting();
	Functions();
	Structs();

	//RunTestPlayground();

	RunCompilerExplorer();

    __debugbreak();
    return 0;
}