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

String DisassembleInstruction(Program* pProgram, u16* pInstruction, u16& outOffset) {
	u16* pInstructionStart = pInstruction;
    StringBuilder builder;

	InstructionHeader* pHeader = (InstructionHeader*)pInstruction;
	switch (pHeader->opcode) {
        case OpCode::Const: {
            builder.Append("Const ");

			Value v;
			v.i32Value = GetOperand32bit(pInstruction);
			switch (pHeader->type) {
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
				case TypeInfo::TypeTag::Type: {
					TypeInfo* pTypeInfo = FindTypeByValue(v);
					if (pTypeInfo)
						builder.AppendFormat("%s", pTypeInfo->name.pData);
					else
						builder.AppendFormat("invalidType");
					break;
				}
				case TypeInfo::TypeTag::Function: {
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
            break;
        }
        case OpCode::Store: {
            builder.Append("Store ");
			u16 offset = GetOperand16bit(pInstruction); 
            builder.AppendFormat("%i",offset);
            break;
        }
		case OpCode::Load: {
			builder.Append("Load ");
			u16 offset = GetOperand16bit(pInstruction); 
			builder.AppendFormat("%i", offset);
			break;
		}
		case OpCode::Copy: {
			builder.Append("Copy ");
			u16 destOffset = GetOperand16bit(pInstruction); 
			u16 srcOffset = GetOperand16bit(pInstruction); 
			builder.AppendFormat("%i %i", destOffset, srcOffset);
			break;
		}
        case OpCode::Negate: {
			builder.AppendFormat("Negate %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Not: {
			builder.AppendFormat("Not %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Add: {
			builder.AppendFormat("Add %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Subtract: {
			builder.AppendFormat("Subtract %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Multiply: {
			builder.AppendFormat("Multiply %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Divide: {
			builder.AppendFormat("Divide %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Greater: {
			builder.AppendFormat("Greater %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Less: {
			builder.AppendFormat("Less %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::GreaterEqual: {
			builder.AppendFormat("GreaterEqual %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::LessEqual: {
			builder.AppendFormat("LessEqual %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Equal: {
			builder.AppendFormat("Equal %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::NotEqual: {
			builder.AppendFormat("NotEqual %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Print: {
			builder.AppendFormat("Print %s", TypeInfo::TagToString(pHeader->type));
            break;
        }
        case OpCode::Return: {
            builder.Append("Return");
            break;
        }
        case OpCode::Drop: {
            builder.Append("Drop");
            break;
        }
        case OpCode::JmpIfFalse: {
            builder.Append("JmpIfFalse");
			u16 ipOffset = GetOperand16bit(pInstruction); 
            builder.AppendFormat(" %i", ipOffset);
            break;
        }
        case OpCode::JmpIfTrue: {
            builder.Append("JmpIfTrue");
			u16 ipOffset = GetOperand16bit(pInstruction); 
            builder.AppendFormat(" %i", ipOffset);
            break;
        }
        case OpCode::Jmp: {
            builder.Append("Jmp");
			u16 ipOffset = GetOperand16bit(pInstruction); 
            builder.AppendFormat(" %i", ipOffset);
            break;
        }
		case OpCode::Cast: {
			builder.Append("Cast ");
			TypeInfo::TypeTag fromType = (TypeInfo::TypeTag)(GetOperand16bit(pInstruction)); 
			builder.AppendFormat("%s %s", TypeInfo::TagToString(fromType), TypeInfo::TagToString(pHeader->type));
			break;
		}
        case OpCode::Call: {
            builder.Append("Call ");
			u16 nArgs = GetOperand16bit(pInstruction); 
            builder.AppendFormat("%i", nArgs);
            break;
        }
        default:
            builder.Append("OpUnknown");
            break;
    }
	// walk over initial instruction
    pInstruction++;
	outOffset = (uint8_t)(pInstruction - pInstructionStart);
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
	u16* pInstructionPointer = compilerState.pProgram->code.pData;
    while (pInstructionPointer < compilerState.pProgram->code.end()) {
        if (currentLine != compilerState.pProgram->dbgLineInfo[lineCounter]) {
            currentLine = compilerState.pProgram->dbgLineInfo[lineCounter];
            Log::Debug("");
            Log::Debug("  %i:%s", currentLine, lines[currentLine - 1].pData);
        }

		u16 offset;
		String output = DisassembleInstruction(compilerState.pProgram, pInstructionPointer, offset);
		Log::Debug("%s", output.pData);
    	FreeString(output);

        pInstructionPointer += offset;
        lineCounter += offset;
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

		builder.AppendFormat("[%i: %#X|%g|%i]\n", vm.stack.array.count - i, v.i32Value, v.f32Value, v.i32Value);
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

#if 0
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
				// TODO: Need to refactor so that next 16 param is the ipoffset value
                if (!vm.stack.Top().boolValue)
                    pFrame->pInstructionPointer = pProgramToRun->code.pData + (pFrame->pInstructionPointer->ipOffset);
                break;
            }
            case OpCode::JmpIfTrue: {
				// TODO: Need to refactor so that next 16 param is the ipoffset value
                if (vm.stack.Top().boolValue)
                    pFrame->pInstructionPointer = pProgramToRun->code.pData + (pFrame->pInstructionPointer->ipOffset);
                break;
            }
            case OpCode::Jmp: {
				// TODO: Need to refactor so that next 16 param is the ipoffset value
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
#else
void Run(Program* pProgramToRun) {
}
#endif
