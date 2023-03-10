// Copyright 2020-2022 David Colson. All rights reserved.

#include "virtual_machine.h"

#include <resizable_array.inl>
#include <light_string.h>
#include <string_builder.h>
#include <stack.inl>

//#define DEBUG_TRACE

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

uint8_t DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction) {
    StringBuilder builder;
    uint8_t returnIPOffset = 0;
    switch (*pInstruction) {
        case OpCode::LoadConstant: {
            builder.Append("LoadConstant ");
            uint8_t constIndex = *(pInstruction + 1);

            Value& v = chunk.constants[constIndex];
			TypeInfo* pType = chunk.dbgConstantsTypes[constIndex];
            if (pType->tag == TypeInfo::TypeTag::Void)
                builder.AppendFormat("%i (void)", constIndex);
            else if (pType->tag == TypeInfo::TypeTag::F32)
                builder.AppendFormat("%i (%f)", constIndex, v.f32Value);
            else if (pType->tag == TypeInfo::TypeTag::Bool)
                builder.AppendFormat("%i (%s)", constIndex, v.boolValue ? "true" : "false");
            else if (pType->tag == TypeInfo::TypeTag::I32)
                builder.AppendFormat("%i (%i)", constIndex, v.i32Value);
            else if (pType->tag == TypeInfo::TypeTag::Type) {
                if (v.pTypeInfo)
                    builder.AppendFormat("%i (%s)", constIndex, v.pTypeInfo->name.pData);
                else
                    builder.AppendFormat("%i (none)", constIndex);
            }
            else if (pType->tag == TypeInfo::TypeTag::Function) {
                if (v.pFunction)
                    builder.AppendFormat("%i (<fn %s>)", constIndex, v.pFunction->name.pData ? v.pFunction->name.pData : "");
                else
                    builder.AppendFormat("%i (none)", constIndex);
            }
            returnIPOffset = 2;
            break;
        }
        case OpCode::SetLocal: {
            builder.Append("SetLocal ");
            uint8_t index = *(pInstruction + 1);
            builder.AppendFormat("%i", index);
            returnIPOffset = 2;
            break;
        }
        case OpCode::GetLocal: {
            builder.Append("GetLocal ");
            uint8_t index = *(pInstruction + 1);
            builder.AppendFormat("%i", index);
            returnIPOffset = 2;
            break;
        }
        case OpCode::Negate: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Negate %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Not: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Not %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Add: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Add %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Subtract: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Subtract %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Multiply: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Multiply %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Divide: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Divide %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Greater: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Greater %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Less: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Less %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::GreaterEqual: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("GreaterEqual %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::LessEqual: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("LessEqual %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Equal: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Equal %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::NotEqual: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("NotEqual %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Print: {
			TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			builder.AppendFormat("Print %s", TypeInfo::TagToString(typeId));
            returnIPOffset = 2;
            break;
        }
        case OpCode::Return: {
            builder.Append("Return");
            returnIPOffset = 1;
            break;
        }
        case OpCode::Pop: {
            builder.Append("Pop");
            returnIPOffset = 1;
            break;
        }
        case OpCode::JmpIfFalse: {
            builder.Append("JmpIfFalse");
            uint16_t jmp = (uint16_t)((pInstruction[1] << 8) | pInstruction[2]);
            builder.AppendFormat(" %i", jmp);
            returnIPOffset = 3;
            break;
        }
        case OpCode::JmpIfTrue: {
            builder.Append("JmpIfTrue");
            uint16_t jmp = (uint16_t)((pInstruction[1] << 8) | pInstruction[2]);
            builder.AppendFormat(" %i", jmp);
            returnIPOffset = 3;
            break;
        }
        case OpCode::Jmp: {
            builder.Append("Jmp");
            uint16_t jmp = (uint16_t)((pInstruction[1] << 8) | pInstruction[2]);
            builder.AppendFormat(" %i", jmp);
            returnIPOffset = 3;
            break;
        }
        case OpCode::Loop: {
            builder.Append("Loop");
            uint16_t jmp = (uint16_t)((pInstruction[1] << 8) | pInstruction[2]);
            builder.AppendFormat(" %i", -jmp);
            returnIPOffset = 3;
            break;
        }
		case OpCode::Cast: {
			builder.Append("Cast ");
			TypeInfo::TypeTag fromTypeId = (TypeInfo::TypeTag)*(pInstruction + 1);
			TypeInfo::TypeTag toTypeId = (TypeInfo::TypeTag)*(pInstruction + 2);
			builder.AppendFormat("%s %s", TypeInfo::TagToString(fromTypeId), TypeInfo::TagToString(toTypeId));
			returnIPOffset = 3;
			break;
		}
        case OpCode::Call: {
            builder.Append("Call ");
            uint8_t argCount = *(pInstruction + 1);
            builder.AppendFormat("%i", argCount);
            returnIPOffset = 2;
            break;
        }
        default:
            builder.Append("OpUnknown");
            returnIPOffset = 1;
            break;
    }

    String output = builder.CreateString();
    Log::Debug("%s", output.pData);
    FreeString(output);
    return returnIPOffset;
}

// ***********************************************************************

void Disassemble(Function* pFunc, String codeText) {
    CodeChunk& chunk = pFunc->chunk;

    uint8_t* pInstructionPointer = chunk.code.pData;

	for (uint32_t i = 0; i < chunk.constants.count; i++) {
		TypeInfo* pType = chunk.dbgConstantsTypes[i];
        if (pType->tag == TypeInfo::TypeTag::Function) {
            if (chunk.constants[i].pFunction)
			    Disassemble(chunk.constants[i].pFunction, codeText);
        }
    }

    Log::Debug("----------------");
    Log::Debug("-- Function (%s)", pFunc->name.pData);

    ResizableArray<String> lines;
    char* pCurrent = codeText.pData;
    char* pLineStart = codeText.pData;
    while (pCurrent < (codeText.pData + codeText.length)) {
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

    while (pInstructionPointer < chunk.code.end()) {
        if (currentLine != chunk.dbgLineInfo[lineCounter]) {
            currentLine = chunk.dbgLineInfo[lineCounter];
            Log::Debug("");
            Log::Debug("  %i:%s", currentLine, lines[currentLine - 1].pData);
        }

        uint8_t offset = DisassembleInstruction(chunk, pInstructionPointer);
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


void Run(Function* pFuncToRun) {
    VirtualMachine vm;
    
	vm.stack.Reserve(1000);
	vm.callStack.Reserve(100);

    Value fv;
    fv.pFunction = pFuncToRun;
	vm.stack.Push(fv);

    CallFrame frame;
    frame.pFunc = pFuncToRun;
    frame.pInstructionPointer = pFuncToRun->chunk.code.pData;
    frame.stackBaseIndex = 0;
    vm.callStack.Push(frame);

	bool debug = true;

    // VM run
    CallFrame* pFrame = &vm.callStack.Top();
	uint8_t* pEndInstruction = pFrame->pFunc->chunk.code.end();
    while (pFrame->pInstructionPointer < pEndInstruction) {
#ifdef DEBUG_TRACE
        DisassembleInstruction(pFrame->pFunc->chunk, pFrame->pInstructionPointer);
#endif
        switch (*pFrame->pInstructionPointer++) {
            case OpCode::LoadConstant: {
				uint8_t index = *pFrame->pInstructionPointer++;
				Value constant = pFrame->pFunc->chunk.constants[index];
				vm.stack.Push(constant);
                break;
            }
            case OpCode::Negate: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
				Value v = vm.stack.Pop();

				if (typeId == TypeInfo::F32) {
					vm.stack.Push(MakeValue(-v.f32Value));
				} else if (typeId == TypeInfo::I32) {
					vm.stack.Push(MakeValue(-v.i32Value));
				}
                break;
            }
            case OpCode::Not: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
				Value v = vm.stack.Pop();

				if (typeId == TypeInfo::Bool) {
					vm.stack.Push(MakeValue(!v.boolValue));
				}
                break;
            }
            case OpCode::Add: {
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
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
				TypeInfo::TypeTag typeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
                Value v = vm.stack.Pop();
				if (typeId == TypeInfo::TypeTag::Type)
                    Log::Info("%s", v.pTypeInfo->name.pData);
				if (typeId == TypeInfo::TypeTag::F32)
                    Log::Info("%f", v.f32Value);
				else if (typeId == TypeInfo::TypeTag::I32)
                    Log::Info("%i", v.i32Value);
				else if (typeId == TypeInfo::TypeTag::Bool)
                    Log::Info("%s", v.boolValue ? "true" : "false");
				else if (typeId == TypeInfo::TypeTag::Function)
                    Log::Info("<fn %s>", v.pFunction->name.pData ? v.pFunction->name.pData : "");  // TODO Should probably show the type sig?
                break;
            }
            case OpCode::Pop:
                vm.stack.Pop();
                break;

            case OpCode::SetLocal: {
                uint8_t opIndex = *pFrame->pInstructionPointer++;
                vm.stack[pFrame->stackBaseIndex + opIndex] = vm.stack.Top();
                break;
            }
            case OpCode::GetLocal: {
                uint8_t opIndex = *pFrame->pInstructionPointer++;
				Value v = vm.stack[pFrame->stackBaseIndex + opIndex];
				vm.stack.Push(v);
				break;
            }
            case OpCode::JmpIfFalse: {
                uint16_t jmp = (uint16_t)((pFrame->pInstructionPointer[0] << 8) | pFrame->pInstructionPointer[1]);
                pFrame->pInstructionPointer += 2;
                if (!vm.stack.Top().boolValue)
                    pFrame->pInstructionPointer += jmp;
                break;
            }
            case OpCode::JmpIfTrue: {
                uint16_t jmp = (uint16_t)((pFrame->pInstructionPointer[0] << 8) | pFrame->pInstructionPointer[1]);
                pFrame->pInstructionPointer += 2;
                if (vm.stack.Top().boolValue)
                    pFrame->pInstructionPointer += jmp;
                break;
            }
            case OpCode::Jmp: {
                uint16_t jmp = (uint16_t)((pFrame->pInstructionPointer[0] << 8) | pFrame->pInstructionPointer[1]);
                pFrame->pInstructionPointer += 2;
                pFrame->pInstructionPointer += jmp;
                break;
            }
            case OpCode::Loop: {
                uint16_t jmp = (uint16_t)((pFrame->pInstructionPointer[0] << 8) | pFrame->pInstructionPointer[1]);
                pFrame->pInstructionPointer += 2;
                pFrame->pInstructionPointer -= jmp;
                break;
            }
            case OpCode::Call: {
                uint8_t argCount = *pFrame->pInstructionPointer++;
                
                Value funcValue = vm.stack[-1 - argCount];

                if (funcValue.pFunction == nullptr) {
                    Log::Info("Runtime Exception: Function pointer is not set to a function");
                    return;
                }

                CallFrame frame;
                frame.pFunc = funcValue.pFunction;
                frame.pInstructionPointer = funcValue.pFunction->chunk.code.pData;
                frame.stackBaseIndex = vm.stack.array.count - argCount - 1;
                vm.callStack.Push(frame);

                pFrame = &vm.callStack.Top();
				pEndInstruction = pFrame->pFunc->chunk.code.end();
                break;
            }
			case OpCode::Cast: {
				TypeInfo::TypeTag fromTypeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
				TypeInfo::TypeTag toTypeId = (TypeInfo::TypeTag)*pFrame->pInstructionPointer++;
	
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
					pEndInstruction = pFrame->pFunc->chunk.code.end();
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