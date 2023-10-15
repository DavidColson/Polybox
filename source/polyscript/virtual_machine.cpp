// Copyright 2020-2022 David Colson. All rights reserved.

#include "virtual_machine.h"

#include "compiler.h"
#include "code_gen.h"

#include <resizable_array.inl>
#include <light_string.h>
#include <string_builder.h>
#include <stack.inl>
#include <defer.h>

//#define DEBUG_TRACE

#define GetOperand1Byte(ptr) *(ptr++)

#define GetOperand2Byte(ptr) ((ptr[0] << 8) |\
	ptr[1]);\
	ptr += 2;

#define GetOperand4Byte(ptr) ((ptr[0] << 24) |\
	(ptr[1] << 16) |\
	(ptr[2] << 8) |\
	ptr[3]);\
	ptr += 4;

#define GetOperand8Byte(ptr) ((ptr[0] << 56) |\
	(ptr[1] << 48) |\
	(ptr[2] << 40) |\
	(ptr[3] << 32) |\
	(ptr[4] << 24) |\
	(ptr[5] << 16) |\
	(ptr[6] << 8) |\
	ptr[7]);\
	ptr += 8;

struct CallFrame {
    Function* pFunc { nullptr };
    uint8_t* pInstructionPointer { nullptr };
    int32_t stackBaseIndex{ 0 };
};

struct VirtualMachine {
	Stack<Value> stack;
	Stack<CallFrame> callStack;
};

// ***********************************************************************

String DisassembleInstruction(Program* pProgram, Instruction* pInstruction) {
    StringBuilder builder;
    switch (pInstruction->opcode) {
        case OpCode::PushConstant: {
            builder.Append("PushConstant ");

			Value& v = pInstruction->constant;
			// TODO: No longer know how to track constant types? Will just wing it for now
			// The debug list can still work, probably need to do a hashmap, from instruction index to type, so there's one type for every pushconstant
			//
			// TypeInfo* pType = pProgram->dbgConstantsTypes[constIndex];
            // if (pType->tag == TypeInfo::TypeTag::Void)
            //     builder.AppendFormat("%i (void)", constIndex);
            // else if (pType->tag == TypeInfo::TypeTag::F32)
            //     builder.AppendFormat("%i (%f)", constIndex, v.f32Value);
            // else if (pType->tag == TypeInfo::TypeTag::Bool)
            //     builder.AppendFormat("%i (%s)", constIndex, v.boolValue ? "true" : "false");
            // else if (pType->tag == TypeInfo::TypeTag::I32)
            //     builder.AppendFormat("%i (%i)", constIndex, v.i32Value);
            // else if (pType->tag == TypeInfo::TypeTag::Type) {
            //     if (v.pTypeInfo)
            //         builder.AppendFormat("%i (%s)", constIndex, v.pTypeInfo->name.pData);
            //     else
            //         builder.AppendFormat("%i (none)", constIndex);
            // }
            // else if (pType->tag == TypeInfo::TypeTag::Function) {
            //     if (v.pFunction)
            //         builder.AppendFormat("%i (<%s>)", constIndex, v.pFunction->name.pData ? v.pFunction->name.pData : "");
            //     else
            //         builder.AppendFormat("%i (none)", constIndex);
            // }
			builder.AppendFormat("%i", v.i32Value);
            break;
        }
        case OpCode::SetLocal: {
            builder.Append("SetLocal ");
            builder.AppendFormat("%i", pInstruction->stackIndex);
            break;
        }
        case OpCode::PushLocal: {
            builder.Append("PushLocal ");
            builder.AppendFormat("%i", pInstruction->stackIndex);
            break;
        }
		case OpCode::SetField: {
			builder.Append("SetField ");
			builder.AppendFormat("off: %i s: %i", pInstruction->fieldOffset, pInstruction->fieldSize);
			break;
		}
		case OpCode::SetFieldPtr: {
			builder.Append("SetFieldPtr ");
			builder.AppendFormat("off: %i s: %i", pInstruction->fieldOffset, pInstruction->fieldSize);
			break;
		}
		case OpCode::PushField: {
			builder.Append("GetField ");
			builder.AppendFormat("off: %i s: %i", pInstruction->fieldOffset, pInstruction->fieldSize);
			break;
		}
		case OpCode::PushFieldPtr: {
			builder.Append("GetFieldPtr ");
			builder.AppendFormat("off: %i s: %i", pInstruction->fieldOffset, pInstruction->fieldSize);
			break;
		}
        case OpCode::Negate: {
			builder.AppendFormat("Negate %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Not: {
			builder.AppendFormat("Not %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Add: {
			builder.AppendFormat("Add %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Subtract: {
			builder.AppendFormat("Subtract %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Multiply: {
			builder.AppendFormat("Multiply %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Divide: {
			builder.AppendFormat("Divide %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Greater: {
			builder.AppendFormat("Greater %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Less: {
			builder.AppendFormat("Less %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::GreaterEqual: {
			builder.AppendFormat("GreaterEqual %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::LessEqual: {
			builder.AppendFormat("LessEqual %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Equal: {
			builder.AppendFormat("Equal %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::NotEqual: {
			builder.AppendFormat("NotEqual %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Print: {
			builder.AppendFormat("Print %s", TypeInfo::TagToString(pInstruction->type));
            break;
        }
        case OpCode::Return: {
            builder.Append("Return");
            break;
        }
        case OpCode::Pop: {
            builder.Append("Pop");
            break;
        }
        case OpCode::JmpIfFalse: {
            builder.Append("JmpIfFalse");
            builder.AppendFormat(" %i", pInstruction->ipOffset);
            break;
        }
        case OpCode::JmpIfTrue: {
            builder.Append("JmpIfTrue");
            builder.AppendFormat(" %i", pInstruction->ipOffset);
            break;
        }
        case OpCode::Jmp: {
            builder.Append("Jmp");
            builder.AppendFormat(" %i", pInstruction->ipOffset);
            break;
        }
        case OpCode::Loop: {
            builder.Append("Loop");
            builder.AppendFormat(" %i", pInstruction->ipOffset);
            break;
        }
		case OpCode::PushStruct: {
			builder.Append("PushStruct");
			builder.AppendFormat(" %i", pInstruction->ipOffset);
			break;
		}
		case OpCode::Cast: {
			builder.Append("Cast ");
			builder.AppendFormat("%s %s", TypeInfo::TagToString(pInstruction->fromType), TypeInfo::TagToString(pInstruction->toType));
			break;
		}
        case OpCode::Call: {
            builder.Append("Call ");
            builder.AppendFormat("%i", pInstruction->nArgs);
            break;
        }
        default:
            builder.Append("OpUnknown");
            break;
    }

    return builder.CreateString();
}

// ***********************************************************************

void DisassembleFunction(Compiler& compilerState, Function* pFunc) {
	
	Log::Debug("----------------");
    Log::Debug("-- Function (%s)", pFunc->name.pData);

	// Split the program into lines
	// TODO: A few places do this now, maybe do this in the compiler file and store for reuse
    ResizableArray<String> lines;
    char* pCurrent = compilerState.code.pData;
    char* pLineStart = compilerState.code.pData;
    while (pCurrent < (compilerState.code.pData + compilerState.code.length)) {
        if (*pCurrent == '\n') {
            String line = CopyCStringRange(pLineStart, pCurrent);
            lines.PushBack(line);
            pLineStart = pCurrent + 1;
        }
        pCurrent++;
    }
    String line = CopyCStringRange(pLineStart, pCurrent);
    lines.PushBack(line);

    uint32_t lineCounter = 0;
    uint32_t currentLine = -1;


	// Run through the code printing instructions
	Instruction* pInstructionPointer = pFunc->code2.pData;
    while (pInstructionPointer < pFunc->code2.end()) {
        if (currentLine != pFunc->dbgLineInfo[lineCounter]) {
            currentLine = pFunc->dbgLineInfo[lineCounter];
            Log::Debug("");
            Log::Debug("  %i:%s", currentLine, lines[currentLine - 1].pData);
        }

		String output = DisassembleInstruction(compilerState.pProgram, pInstructionPointer);
		Log::Debug("%s", output.pData);
    	FreeString(output);

        pInstructionPointer += 1;
        lineCounter += 1;
    }

    lines.Free([](String& str) {
        FreeString(str);
    });
}

// ***********************************************************************

void DisassembleProgram(Compiler& compilerState) {
	// Disassemble main module function
	DisassembleFunction(compilerState, compilerState.pProgram->pMainModuleFunction);

	// Search through the constant table looking for functions to disassemble
	for (uint32_t i = 0; i < compilerState.pProgram->constantTable.count; i++) {
		TypeInfo* pType = compilerState.pProgram->dbgConstantsTypes[i];
        if (pType->tag == TypeInfo::TypeTag::Function) {
            if (compilerState.pProgram->constantTable[i].pFunction)
			    DisassembleFunction(compilerState, compilerState.pProgram->constantTable[i].pFunction);
        }
    }
}

// ***********************************************************************

void DebugStack(VirtualMachine& vm) {
	StringBuilder builder;

	builder.Append("\n");

	// TODO: When we have locals debugging, draw a stack slot that is a known local as the local rather than a standard stack slot
	for (uint32_t i = 1; i < vm.stack.array.count; i++) {
		Value& v = vm.stack[vm.stack.array.count - i];

		builder.AppendFormat("[%i: %#X|%g|%i]\n", vm.stack.array.count - i, (uint64_t)v.pFunction, v.f32Value, v.i32Value);
	}
	String s = builder.CreateString();
	Log::Debug("->%s", s.pData);
	FreeString(s);
}

// ***********************************************************************


// Proposal
// WHat if there was two run functions for the VM
// What if one maintained the debug stack, and the other one didn't?

// Maintaining a debug stack of types is by far the simplest option, and although we will have branches in more instructions,
// it might actually be okay
 
// The other idea is to have special instructions which maintain the debug stack that we insert with the codegenerator
// But this actually is quite difficult, since we lack the context of exactly what the instructions are doing with the types


void Run(Program* pProgramToRun) {
	if (pProgramToRun == nullptr)
		return;

    VirtualMachine vm;
    
	vm.stack.Reserve(1000);
	vm.callStack.Reserve(100);
	defer(vm.callStack.Free());
	defer(vm.stack.Free());

    Value fv;
    fv.pFunction = pProgramToRun->pMainModuleFunction;
	vm.stack.Push(fv);

    CallFrame frame;
    frame.pFunc = pProgramToRun->pMainModuleFunction;
    frame.pInstructionPointer = pProgramToRun->pMainModuleFunction->code.pData;
    frame.stackBaseIndex = 0;
    vm.callStack.Push(frame);

	bool debug = true;

    // VM run
    CallFrame* pFrame = &vm.callStack.Top();
	uint8_t* pEndInstruction = pFrame->pFunc->code.end();
    while (pFrame->pInstructionPointer < pEndInstruction) {
#ifdef DEBUG_TRACE
        uint8_t offset;
		String output = DisassembleInstruction(pProgramToRun, pFrame->pInstructionPointer, offset);
		Log::Debug("%s", output.pData);
    	FreeString(output);
#endif
		switch (*pFrame->pInstructionPointer++) {
			case OpCode::PushConstant: {
				uint8_t index = GetOperand1Byte(pFrame->pInstructionPointer);
				Value constant = pProgramToRun->constantTable[index];
				vm.stack.Push(constant);
				break;
			}
			case OpCode::Negate: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value v = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(-v.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(-v.i32Value));
				}
				break;
			}
			case OpCode::Not: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value v = vm.stack.Pop();

				if (typeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue(!v.boolValue));
				}
				break;
			}
			case OpCode::Add: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value + b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value + b.i32Value));
				}
				break;
			}
			case OpCode::Subtract: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value - b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value - b.i32Value));
				}
				break;
			}
			case OpCode::Multiply: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value * b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value * b.i32Value));
				}
				break;
			}
			case OpCode::Divide: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value / b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value / b.i32Value));
				}
				break;
			}
			case OpCode::Greater: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value > b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value > b.i32Value));
				}
				break;
			}
			case OpCode::Less: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value < b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value < b.i32Value));
				}
				break;
			}
			case OpCode::GreaterEqual: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value >= b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value >= b.i32Value));
				}
				break;
			}
			case OpCode::LessEqual: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value <= b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value <= b.i32Value));
				}
				break;
			}
			case OpCode::Equal: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value == b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value == b.i32Value));
				} else if (typeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue(a.boolValue == b.boolValue));
				}
				break;
			}
			case OpCode::NotEqual: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value b = vm.stack.Pop();
				Value a = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(a.f32Value != b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(a.i32Value != b.i32Value));
				} else if (typeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue(a.boolValue != b.boolValue));
				}
				break;
			}
			case OpCode::Print: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				Value v = vm.stack.Pop();
				if (typeId == TypeInfo::TypeTag::Type)
					Log::Info("%s", v.pTypeInfo->name.pData);
				if (typeId == TypeInfo::TypeTag::F32)
					Log::Info("%g", v.f32Value);
				else if (typeId == TypeInfo::TypeTag::I32)
					Log::Info("%i", v.i32Value);
				else if (typeId == TypeInfo::TypeTag::Bool)
					Log::Info("%s", v.boolValue ? "true" : "false");
				else if (typeId == TypeInfo::TypeTag::Function)
					Log::Info("<%s>", v.pFunction->name.pData ? v.pFunction->name.pData : "");  // TODO Should probably show the type sig?
				break;
			}
			case OpCode::Pop:
				vm.stack.Pop();
				break;

			case OpCode::SetLocal: {
				uint8_t opIndex = GetOperand1Byte(pFrame->pInstructionPointer);
				vm.stack[pFrame->stackBaseIndex + opIndex] = vm.stack.Top();
				break;
			}
			case OpCode::PushLocal: {
				uint8_t opIndex = GetOperand1Byte(pFrame->pInstructionPointer);
				Value v = vm.stack[pFrame->stackBaseIndex + opIndex];
				vm.stack.Push(v);
				break;
			}
			case OpCode::SetField: {
				uint32_t offset = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				uint32_t size = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				
				Value v = vm.stack.Pop();
				Value structValue = vm.stack.Pop();

				memcpy((uint8_t*)structValue.pPtr + offset, &v.pPtr, size);
				vm.stack.Push(v);
				break;
			}
			case OpCode::SetFieldPtr: {
				uint32_t offset = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				uint32_t size = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				
				Value v = vm.stack.Pop();
				Value structValue = vm.stack.Pop();

				memcpy((uint8_t*)structValue.pPtr + offset, v.pPtr, size);
				vm.stack.Push(v);
				break;
			}
			case OpCode::PushField: {
				uint32_t offset = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				uint32_t size = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				
				Value structValue = vm.stack.Pop();
				Value v;

				memcpy(&v.pPtr, (uint8_t*)structValue.pPtr + offset, size);
				vm.stack.Push(v);
				break;
			}
			case OpCode::PushFieldPtr: {
				uint32_t offset = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				uint32_t size = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				
				Value structValue = vm.stack.Pop();
				Value v;

				v.pPtr = (uint8_t*)structValue.pPtr + offset;
				vm.stack.Push(v);
				break;
			}
            case OpCode::JmpIfFalse: {
				uint16_t jmp = (uint16_t)GetOperand2Byte(pFrame->pInstructionPointer);
                if (!vm.stack.Top().boolValue)
                    pFrame->pInstructionPointer += jmp;
                break;
            }
            case OpCode::JmpIfTrue: {
				uint16_t jmp = (uint16_t)GetOperand2Byte(pFrame->pInstructionPointer);
                if (vm.stack.Top().boolValue)
                    pFrame->pInstructionPointer += jmp;
                break;
            }
            case OpCode::Jmp: {
				uint16_t jmp = (uint16_t)GetOperand2Byte(pFrame->pInstructionPointer);
                pFrame->pInstructionPointer += jmp;
                break;
            }
            case OpCode::Loop: {
				uint16_t jmp = (uint16_t)GetOperand2Byte(pFrame->pInstructionPointer);
                pFrame->pInstructionPointer -= jmp;
                break;
            }
            case OpCode::Call: {
				uint8_t argCount = GetOperand1Byte(pFrame->pInstructionPointer);
                
                Value funcValue = vm.stack[-1 - argCount];

                if (funcValue.pFunction == nullptr) {
                    Log::Info("Runtime Exception: Function pointer is not set to a function");
                    return;
                }

                CallFrame frame;
                frame.pFunc = funcValue.pFunction;
                frame.pInstructionPointer = funcValue.pFunction->code.pData;
                frame.stackBaseIndex = vm.stack.array.count - argCount - 1;
                vm.callStack.Push(frame);

                pFrame = &vm.callStack.Top();
				pEndInstruction = pFrame->pFunc->code.end();
                break;
            }
			case OpCode::PushStruct: {
				uint32_t size = (uint32_t)GetOperand4Byte(pFrame->pInstructionPointer);
				
				Value v;
				v.pPtr = g_Allocator.Allocate(size_t(size)); // TODO: Stack memory, not heap
				// TODO: This actually is a leak that we need to fix, but we'll do so by introducing stack memory
				MarkNotALeak(v.pPtr);

				// All memory will be zero allocated by default
				memset(v.pPtr, 0, size);
				vm.stack.Push(v);
				break;
			}
			case OpCode::Cast: {
				TypeInfo::TypeTag fromTypeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
				TypeInfo::TypeTag toTypeId = (TypeInfo::TypeTag)GetOperand1Byte(pFrame->pInstructionPointer);
	
				Value copy = vm.stack.Pop();
				if (toTypeId == TypeInfo::I32 && fromTypeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue((int32_t)copy.f32Value));
				} else if (toTypeId == TypeInfo::I32 && fromTypeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue((int32_t)copy.boolValue));
				} else if (toTypeId == TypeInfo::F32 && fromTypeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue((float)copy.i32Value));
				} else if (toTypeId == TypeInfo::F32 && fromTypeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue((float)copy.boolValue));
				} else if (toTypeId == TypeInfo::Bool && fromTypeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue((bool)copy.i32Value));
				} else if (toTypeId == TypeInfo::Bool && fromTypeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue((bool)copy.f32Value));
				}
				break;
			}
            case (uint8_t)OpCode::Return: {
                Value returnVal = vm.stack.Pop();

				vm.callStack.Pop();
                vm.stack.Resize(pFrame->stackBaseIndex);

                vm.stack.Push(returnVal);

                if (vm.callStack.array.count == 0) {
                    return;
                } else {
                    pFrame = &vm.callStack.Top();
					pEndInstruction = pFrame->pFunc->code.end();
				}
                break;
            }
            default:
                break;
        }
#ifdef DEBUG_TRACE
        DebugStack(vm);
#endif
    }
}