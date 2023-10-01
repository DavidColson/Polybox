#include <resizable_array.inl>
#include <string_builder.h>
#include <light_string.h>
#include <linear_allocator.h>
#include <defer.h>

#include "parser.h"
#include "code_gen.h"
#include "type_checker.h"
#include "virtual_machine.h"

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