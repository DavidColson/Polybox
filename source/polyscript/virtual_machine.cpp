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
    u16* pInstructionPointer { nullptr };
    i32 stackBaseAddress{ 0 };
};

struct VirtualMachine {
	// At some point it would be cool to refactor this callstack structure to actually be in the 
	// literal stack.
	Stack<CallFrame> callStack;

	u8* pMemory; // This is the VM's RAM
	u32 stackBaseAddress; // The stack is at the end of the RAM block with a predetermined size 
	u32 stackHeadAddress;
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
        case OpCode::LocalAddr: {
            builder.Append("LocalAddr ");
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
		case OpCode::StackChange: {
			builder.Append("StackChange");
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

	// TODO: we can no longer debug the stack like this. Need a new method to do so
	// for (u32 i = 1; i < vm.stack.array.count; i++) {
	// 	Value& v = vm.stack[vm.stack.array.count - i];
	// 	builder.AppendFormat("[%i: %#X|%g|%i]\n", vm.stack.array.count - i, v.i32Value, v.f32Value, v.i32Value);
	// }
	String s = builder.CreateString();
	Log::Debug("->%s", s.pData);
	FreeString(s);
}

// ***********************************************************************

inline void PushStack(VirtualMachine& vm, Value value) {
	u32* targetStackSlot = (u32*)(vm.pMemory + vm.stackHeadAddress);
	*targetStackSlot = value.i32Value;
	vm.stackHeadAddress += 4;
}

// ***********************************************************************

inline Value PopStack(VirtualMachine& vm) {
	vm.stackHeadAddress -= 4;
	Value* targetStackSlot = (Value*)(vm.pMemory + vm.stackHeadAddress);
	return *targetStackSlot;
}

// ***********************************************************************

inline Value ReadStack(VirtualMachine& vm) {
	Value* targetStackSlot = (Value*)(vm.pMemory + vm.stackHeadAddress - 4);
	return *targetStackSlot;
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
	i32 memorySize = 2 * 1024 * 1024;
    vm.pMemory = (u8*)g_Allocator.Allocate(memorySize);
	defer(g_Allocator.Free(vm.pMemory));
	vm.stackBaseAddress = (u32)memorySize - 1024 * 4; // Stack has 1024, 4 byte slots (must be kept to 4 byte alignment)
	vm.stackHeadAddress = vm.stackBaseAddress; 

	// Maybe at some point this could be in the actual stack?
	vm.callStack.Reserve(100);
	defer(vm.callStack.Free());

    Value fv;
    fv.functionPointer = 0; // start IP of main level function
	PushStack(vm, fv);

    CallFrame frame;
    frame.pInstructionPointer = pProgramToRun->code.pData;
    frame.stackBaseAddress = vm.stackBaseAddress;
    vm.callStack.Push(frame);
    
	bool debug = true;

    // VM run
    CallFrame* pFrame = &vm.callStack.Top();
	u16* pEndInstruction = pProgramToRun->code.end();
    while (pFrame->pInstructionPointer < pEndInstruction) {
#ifdef DEBUG_TRACE
		String output = DisassembleInstruction(pProgramToRun, pFrame->pInstructionPointer);
		Log::Debug("%s", output.pData);
    	FreeString(output);
#endif
		InstructionHeader* pHeader = (InstructionHeader*)pFrame->pInstructionPointer;
		switch (pHeader->opcode) {
			case OpCode::Const: {
				// Push immediate value ontop of stack
				i32 mem = GetOperand32bit(pFrame->pInstructionPointer);
				PushStack(vm, *(Value*)&mem);
				break;
			}
			case OpCode::Load: {
				u16 offset = GetOperand16bit(pFrame->pInstructionPointer);
				i32 sourceAddress = PopStack(vm).i32Value;
				Value* pSource = (Value*)(vm.pMemory + sourceAddress + offset);
				PushStack(vm, *pSource);
				break;
			}
			case OpCode::Store: {
				// Instruction arg is a memory offset
				u16 offset = GetOperand16bit(pFrame->pInstructionPointer);

				// Pop the target memory address off the stack
				i32 destAddress = PopStack(vm).i32Value;
				Value* pDest = (Value*)(vm.pMemory + destAddress + offset);

				// Read the value to store off top of stack 
				Value value = ReadStack(vm);
				*pDest = value;
				break;
			}
			case OpCode::LocalAddr: {
				u16 offset = GetOperand16bit(pFrame->pInstructionPointer);

				// Calculate an address that offsets from the current frame stack base address
				i32 address = pFrame->stackBaseAddress + offset;
				PushStack(vm, MakeValue(address));
				break;
			}
			case OpCode::Copy: {
				u16 desOffset = GetOperand16bit(pFrame->pInstructionPointer);
				u16 srcOffset = GetOperand16bit(pFrame->pInstructionPointer);
				
				// Pop the size off the stack
				i32 size = PopStack(vm).i32Value;

				// Pop the destination address off the stack
				i32 destAddress = PopStack(vm).i32Value;
				u16* pDest = (u16*)(vm.pMemory + destAddress + desOffset);

				// Read the source address off the stack
				i32 srcAddress = ReadStack(vm).i32Value;
				u16* pSrc = (u16*)(vm.pMemory + srcAddress + srcOffset);

				memcpy(pDest, pSrc, size);
				break;
			}
			case OpCode::Drop: {
				vm.stackHeadAddress -= 4;
				break;
			}
			case OpCode::StackChange: {
				Value offset = PopStack(vm);
				vm.stackHeadAddress += offset.i32Value * 4;
				break;
			}
			case OpCode::Negate: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value v = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(-v.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(-v.i32Value));
				}
				break;
			}
			case OpCode::Not: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value v = PopStack(vm);

				if (typeId == TypeInfo::Bool) {
					PushStack(vm, MakeValue(!v.boolValue));
				}
				break;
			}
			case OpCode::Add: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value + b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value + b.i32Value));
				}
				break;
			}
			case OpCode::Subtract: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value - b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value - b.i32Value));
				}
				break;
			}
			case OpCode::Multiply: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value * b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value * b.i32Value));
				}
				break;
			}
			case OpCode::Divide: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value / b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value / b.i32Value));
				}
				break;
			}
			case OpCode::Greater: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value > b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value > b.i32Value));
				}
				break;
			}
			case OpCode::Less: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value < b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value < b.i32Value));
				}
				break;
			}
			case OpCode::GreaterEqual: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value >= b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value >= b.i32Value));
				}
				break;
			}
			case OpCode::LessEqual: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value <= b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value <= b.i32Value));
				}
				break;
			}
			case OpCode::Equal: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value == b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value == b.i32Value));
				} else if (typeId == TypeInfo::Bool) {
					PushStack(vm, MakeValue(a.boolValue == b.boolValue));
				}
				break;
			}
			case OpCode::NotEqual: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value b = PopStack(vm);
				Value a = PopStack(vm);

				if (typeId == TypeInfo::F32) {
					PushStack(vm, MakeValue(a.f32Value != b.f32Value));
				} else if (typeId == TypeInfo::I32) {
					PushStack(vm, MakeValue(a.i32Value != b.i32Value));
				} else if (typeId == TypeInfo::Bool) {
					PushStack(vm, MakeValue(a.boolValue != b.boolValue));
				}
				break;
			}
			case OpCode::Print: {
				TypeInfo::TypeTag typeId = pHeader->type;
				Value v = PopStack(vm);
				if (typeId == TypeInfo::TypeTag::Type)
					Log::Info("%s", FindTypeByValue(v)->name.pData);
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
            case OpCode::JmpIfFalse: {
				u16 ipOffset = GetOperand16bit(pFrame->pInstructionPointer);
                if (!ReadStack(vm).boolValue) {
                    pFrame->pInstructionPointer = pProgramToRun->code.pData + ipOffset;
				}
                break;
            }
            case OpCode::JmpIfTrue: {
				u16 ipOffset = GetOperand16bit(pFrame->pInstructionPointer);
                if (ReadStack(vm).boolValue) {
                    pFrame->pInstructionPointer = pProgramToRun->code.pData + ipOffset;
				}
                break;
            }
            case OpCode::Jmp: {
				u16 ipOffset = GetOperand16bit(pFrame->pInstructionPointer);
                pFrame->pInstructionPointer = pProgramToRun->code.pData + ipOffset;
                break;
            }
            case OpCode::Call: {
				u16 argCount = GetOperand16bit(pFrame->pInstructionPointer);
                
				// Need to go back in the stack past the args, to retrieve the function pointer
				i32 funcIndex = 1 + argCount;
				i32 stackFrameBaseAddress = vm.stackHeadAddress - 4 * funcIndex;
				Value funcValue = *(Value*)(vm.pMemory + stackFrameBaseAddress);

                CallFrame frame;
                frame.pInstructionPointer =  pProgramToRun->code.pData + (funcValue.functionPointer - 1);
                frame.stackBaseAddress = stackFrameBaseAddress;
                vm.callStack.Push(frame);

                pFrame = &vm.callStack.Top();
                break;
            }
			case OpCode::Cast: {
				TypeInfo::TypeTag fromTypeId = (TypeInfo::TypeTag)GetOperand16bit(pFrame->pInstructionPointer);
				TypeInfo::TypeTag toTypeId = pHeader->type;
	
				Value copy = PopStack(vm);
				if (toTypeId == TypeInfo::I32 && fromTypeId == TypeInfo::F32) {
					PushStack(vm, MakeValue((i32)copy.f32Value));
				} else if (toTypeId == TypeInfo::I32 && fromTypeId == TypeInfo::Bool) {
					PushStack(vm, MakeValue((i32)copy.boolValue));
				} else if (toTypeId == TypeInfo::F32 && fromTypeId == TypeInfo::I32) {
					PushStack(vm, MakeValue((f32)copy.i32Value));
				} else if (toTypeId == TypeInfo::F32 && fromTypeId == TypeInfo::Bool) {
					PushStack(vm, MakeValue((f32)copy.boolValue));
				} else if (toTypeId == TypeInfo::Bool && fromTypeId == TypeInfo::I32) {
					PushStack(vm, MakeValue((bool)copy.i32Value));
				} else if (toTypeId == TypeInfo::Bool && fromTypeId == TypeInfo::F32) {
					PushStack(vm, MakeValue((bool)copy.f32Value));
				}
				break;
			}
            case OpCode::Return: {
                Value returnVal = PopStack(vm);

				vm.callStack.Pop();
				vm.stackHeadAddress = pFrame->stackBaseAddress;

                PushStack(vm, returnVal);

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
