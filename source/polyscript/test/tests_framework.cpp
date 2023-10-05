#include <resizable_array.inl>
#include <string_builder.h>
#include <light_string.h>
#include <linear_allocator.h>
#include <defer.h>

#include "parser.h"
#include "code_gen.h"
#include "type_checker.h"
#include "virtual_machine.h"
#include "compiler.h"

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

	// Run test
	Compiler compiler;
	compiler.code = testCode;
	CompileCode(compiler);

	if (errorExpectations.count == 0) compiler.errorState.ReportCompilationResult();

	if (compiler.errorState.errors.count == 0) {
		Run(compiler.pProgram);
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
		for (Error error : compiler.errorState.errors) {
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
	if (errorExpectations.count != compiler.errorState.errors.count) {
		Log::Info("Expected %d errors, but got %d", errorExpectations.count, compiler.errorState.errors.count);
		failed = true;
	}

	if (failed) {
		Log::Info("In test:\n%s", testCode);
		Log::Info("We got the following output:\n%s ", output.pData);
		Log::Info("And the following Errors: ");
		compiler.errorState.ReportCompilationResult();
	}

	// Yeet memory
	compiler.compilerMemory.Finished();
	return errorCount;
}