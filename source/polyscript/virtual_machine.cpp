// Copyright 2020-2022 David Colson. All rights reserved.

#include "virtual_machine.h"

#include "compiler.h"
#include "code_gen.h"

#include <hashmap.inl>
#include <resizable_array.inl>
#include <light_string.h>
#include <string_builder.h>
#include <stack.inl>
#include <defer.h>

// #define DEBUG_TRACE

struct CallFrame {
    Instruction* pInstructionPointer { nullptr };
    size stackBaseIndex{ 0 };
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
			usize index = pProgram->code.IndexFromPointer(pInstruction);
			TypeInfo::TypeTag* tag = pProgram->dbgConstantsTypes.Get(index);
			if (tag != nullptr) {
				switch (*tag) {
					case TypeInfo::TypeTag::Void:
						builder.AppendFormat("(void)", v.i32Value);
						break;
					case TypeInfo::TypeTag::F32:
						builder.AppendFormat("%f", v.f32Value);
						break;
					case TypeInfo::TypeTag::Bool:
						builder.AppendFormat("%s", v.boolValue ? "true" : "false");
						break;
					case TypeInfo::TypeTag::I32:
						builder.AppendFormat("%i", v.i32Value);
						break;
					case TypeInfo::TypeTag::Type:
						if (v.pTypeInfo)
							builder.AppendFormat("%s", v.pTypeInfo->name.pData);
						else
							builder.AppendFormat("invalidType");
					break;
					case TypeInfo::TypeTag::Function:
					{
						FunctionDbgInfo* pDbgInfo = pProgram->dbgFunctionInfo.Get(v.functionPointer);
						if (pDbgInfo != nullptr) {
							builder.AppendFormat("%s <%s>", pDbgInfo->name.pData, pDbgInfo->pType->name.pData);
						}
						break;
					}
					default:
						builder.AppendFormat("%i", v.i32Value);
						break;
				}
			}
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

void DisassembleProgram(Compiler& compilerState) {
    // TODO: Some way to tell what instructions are from what functions
    // Log::Debug("-- Function (%s)", pFunc->name.pData);

	// Split the program i32o lines
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

    size lineCounter = 0;
    size currentLine = -1;


	// Run through the code printing instructions
	Instruction* pInstructionPointer = compilerState.pProgram->code.pData;
    while (pInstructionPointer < compilerState.pProgram->code.end()) {
        if (currentLine != compilerState.pProgram->dbgLineInfo[lineCounter]) {
            currentLine = compilerState.pProgram->dbgLineInfo[lineCounter];
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

void DebugStack(VirtualMachine& vm) {
	StringBuilder builder;

	builder.Append("\n");

	// TODO: When we have locals debugging, draw a stack slot that is a known local as the local rather than a standard stack slot
	for (u32 i = 1; i < vm.stack.array.count; i++) {
		Value& v = vm.stack[vm.stack.array.count - i];

		builder.AppendFormat("[%i: %#X|%g|%i]\n", vm.stack.array.count - i, (u64)v.pPtr, v.f32Value, v.i32Value);
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
    fv.functionPointer = 0; // start IP of main level function
	vm.stack.Push(fv);

    CallFrame frame;
    frame.pInstructionPointer = pProgramToRun->code.pData;
    frame.stackBaseIndex = 0;
    vm.callStack.Push(frame);
    
	bool debug = true;

    // VM run
    CallFrame* pFrame = &vm.callStack.Top();
	Instruction* pEndInstruction = pProgramToRun->code.end();
    while (pFrame->pInstructionPointer < pEndInstruction) {
#ifdef DEBUG_TRACE
		String output = DisassembleInstruction(pProgramToRun, pFrame->pInstructionPointer);
		Log::Debug("%s", output.pData);
    	FreeString(output);
#endif
		switch (pFrame->pInstructionPointer->opcode) {
			case OpCode::PushConstant: {
				vm.stack.Push(pFrame->pInstructionPointer->constant);
				break;
			}
			case OpCode::Negate: {
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
				Value v = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(-v.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(-v.i32Value));
				}
				break;
			}
			case OpCode::Not: {
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
				Value v = vm.stack.Pop();

				if (typeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue(!v.boolValue));
				}
				break;
			}
			case OpCode::Add: {
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
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
				TypeInfo::TypeTag typeId = pFrame->pInstructionPointer->type;
				Value v = vm.stack.Pop();
				if (typeId == TypeInfo::TypeTag::Type)
					Log::Info("%s", v.pTypeInfo->name.pData);
				if (typeId == TypeInfo::TypeTag::F32)
					Log::Info("%g", v.f32Value);
				else if (typeId == TypeInfo::TypeTag::I32)
					Log::Info("%i", v.i32Value);
				else if (typeId == TypeInfo::TypeTag::Bool)
					Log::Info("%s", v.boolValue ? "true" : "false");
				else if (typeId == TypeInfo::TypeTag::Function) {
					FunctionDbgInfo* pDbgInfo = pProgramToRun->dbgFunctionInfo.Get(v.functionPointer);
					if (pDbgInfo != nullptr) {
						Log::Info("%s <%s>", pDbgInfo->name.pData, pDbgInfo->pType->name.pData);
					}
				}
				break;
			}
			case OpCode::Pop:
				vm.stack.Pop();
				break;

			case OpCode::SetLocal: {
				vm.stack[pFrame->stackBaseIndex + pFrame->pInstructionPointer->stackIndex] = vm.stack.Top();
				break;
			}
			case OpCode::PushLocal: {
				Value v = vm.stack[pFrame->stackBaseIndex + pFrame->pInstructionPointer->stackIndex];
				vm.stack.Push(v);
				break;
			}
			case OpCode::SetField: {
				u32 offset = pFrame->pInstructionPointer->fieldOffset;
				u32 size = pFrame->pInstructionPointer->fieldSize;
				
				Value v = vm.stack.Pop();
				Value structValue = vm.stack.Pop();

				memcpy((u8*)structValue.pPtr + offset, &v.pPtr, size);
				vm.stack.Push(v);
				break;
			}
			case OpCode::SetFieldPtr: {
				u32 offset = pFrame->pInstructionPointer->fieldOffset;
				u32 size = pFrame->pInstructionPointer->fieldSize;
				
				Value v = vm.stack.Pop();
				Value structValue = vm.stack.Pop();

				memcpy((u8*)structValue.pPtr + offset, v.pPtr, size);
				vm.stack.Push(v);
				break;
			}
			case OpCode::PushField: {
				u32 offset = pFrame->pInstructionPointer->fieldOffset;
				u32 size = pFrame->pInstructionPointer->fieldSize;
				
				Value structValue = vm.stack.Pop();
				Value v;

				memcpy(&v.pPtr, (u8*)structValue.pPtr + offset, size);
				vm.stack.Push(v);
				break;
			}
			case OpCode::PushFieldPtr: {
				u32 offset = pFrame->pInstructionPointer->fieldOffset;
				u32 size = pFrame->pInstructionPointer->fieldSize;
				
				Value structValue = vm.stack.Pop();
				Value v;

				v.pPtr = (u8*)structValue.pPtr + offset;
				vm.stack.Push(v);
				break;
			}
            case OpCode::JmpIfFalse: {
                if (!vm.stack.Top().boolValue)
                    pFrame->pInstructionPointer = pProgramToRun->code.pData + (pFrame->pInstructionPointer->ipOffset);
                break;
            }
            case OpCode::JmpIfTrue: {
                if (vm.stack.Top().boolValue)
                    pFrame->pInstructionPointer = pProgramToRun->code.pData + (pFrame->pInstructionPointer->ipOffset);
                break;
            }
            case OpCode::Jmp: {
                pFrame->pInstructionPointer = pProgramToRun->code.pData + (pFrame->pInstructionPointer->ipOffset);
                break;
            }
            case OpCode::Call: {
				u64 argCount = pFrame->pInstructionPointer->nArgs;
                
                Value funcValue = vm.stack[-1 - argCount];

                CallFrame frame;
                frame.pInstructionPointer =  pProgramToRun->code.pData + (funcValue.functionPointer - 1);
                frame.stackBaseIndex = vm.stack.array.count - argCount - 1;
                vm.callStack.Push(frame);

                pFrame = &vm.callStack.Top();
                break;
            }
			case OpCode::PushStruct: {
				u64 size = pFrame->pInstructionPointer->size;
				
				Value v;
				v.pPtr = g_Allocator.Allocate(usize(size)); // TODO: Stack memory, not heap
				// TODO: This actually is a leak that we need to fix, but we'll do so by i32roducing stack memory
				MarkNotALeak(v.pPtr);

				// All memory will be zero allocated by default
				memset(v.pPtr, 0, size);
				vm.stack.Push(v);
				break;
			}
			case OpCode::Cast: {
				TypeInfo::TypeTag fromTypeId = pFrame->pInstructionPointer->fromType;
				TypeInfo::TypeTag toTypeId = pFrame->pInstructionPointer->toType;
	
				Value copy = vm.stack.Pop();
				if (toTypeId == TypeInfo::I32 && fromTypeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue((i32)copy.f32Value));
				} else if (toTypeId == TypeInfo::I32 && fromTypeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue((i32)copy.boolValue));
				} else if (toTypeId == TypeInfo::F32 && fromTypeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue((f32)copy.i32Value));
				} else if (toTypeId == TypeInfo::F32 && fromTypeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue((f32)copy.boolValue));
				} else if (toTypeId == TypeInfo::Bool && fromTypeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue((bool)copy.i32Value));
				} else if (toTypeId == TypeInfo::Bool && fromTypeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue((bool)copy.f32Value));
				}
				break;
			}
            case OpCode::Return: {
                Value returnVal = vm.stack.Pop();

				vm.callStack.Pop();
                vm.stack.Resize(pFrame->stackBaseIndex);

                vm.stack.Push(returnVal);

                if (vm.callStack.array.count == 0) {
                    return;
                } else {
                    pFrame = &vm.callStack.Top();
				}
                break;
            }
            default:
                break;
        }
        pFrame->pInstructionPointer += 1;
#ifdef DEBUG_TRACE
        DebugStack(vm);
#endif
    }
}
