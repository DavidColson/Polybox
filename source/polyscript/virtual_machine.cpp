// Copyright 2020-2022 David Colson. All rights reserved.

#include "virtual_machine.h"

#include <resizable_array.inl>
#include <light_string.h>
#include <string_builder.h>
#include <stack.inl>

#define DEBUG_TRACE

struct CallFrame {
    Function* pFunc { nullptr };
    uint8_t* pInstructionPointer { nullptr };
    int32_t stackBaseIndex{ 0 };
};

struct DebugInfo {
	Stack<TypeInfo*> stackTypes;
};

struct VirtualMachine {
	Stack<Value> stack;
	Stack<CallFrame> callStack;
	DebugInfo* pDebugInfo { nullptr };
};


// ***********************************************************************

uint8_t DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction) {
    StringBuilder builder;
    uint8_t returnIPOffset = 0;
    switch (*pInstruction) {
        case OpCode::LoadConstant: {
            builder.Append("LoadConstant ");
            uint8_t constIndex = *(pInstruction + 1);
			// TODO: Debug information needs to store constant table types
            Value& v = chunk.constants[constIndex];
            if (v.pType->tag == TypeInfo::TypeTag::Void)
                builder.AppendFormat("%i (void)", constIndex);
            if (v.pType->tag == TypeInfo::TypeTag::Type)
                builder.AppendFormat("%i (%s)", constIndex, v.pTypeInfo->name.pData);
            if (v.pType->tag == TypeInfo::TypeTag::F32)
                builder.AppendFormat("%i (%f)", constIndex, v.f32Value);
            else if (v.pType->tag == TypeInfo::TypeTag::Bool)
                builder.AppendFormat("%i (%s)", constIndex, v.boolValue ? "true" : "false");
            else if (v.pType->tag == TypeInfo::TypeTag::I32)
                builder.AppendFormat("%i (%i)", constIndex, v.i32Value);
            else if (v.pType->tag == TypeInfo::TypeTag::Function)
                builder.AppendFormat("%i (<fn %s>)", constIndex, v.pFunction->name.pData ? v.pFunction->name.pData : "");  // TODO Should probably show the type sig?
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
            builder.Append("Print ");
            returnIPOffset = 1;
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

    for (Value& v : chunk.constants) {
        if (v.pType->tag == TypeInfo::TypeTag::Function) {
            Disassemble(v.pFunction, codeText);
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
        if (currentLine != chunk.lineInfo[lineCounter]) {
            currentLine = chunk.lineInfo[lineCounter];
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

    for (uint32_t i = 1; i < vm.stack.array.count; i++) {
        Value& v = vm.stack[i];

        // TODO: This can now handle more complex types
        // So upgrade it

		// TODO: No idea what the types of these values are?
		// Might need to maintain a debug version of the stack with extra information such as types
        switch (v.pType->tag) {
            case TypeInfo::TypeTag::Void:
                builder.AppendFormat("[%i: void]", i);
                break;
            case TypeInfo::TypeTag::Type:
                builder.AppendFormat("[%i: %s]", i, v.pTypeInfo->name.pData);
                break;
            case TypeInfo::TypeTag::Bool:
                builder.AppendFormat("[%i: %s]", i, v.boolValue ? "true" : "false");
                break;
            case TypeInfo::TypeTag::F32:
                builder.AppendFormat("[%i: %f]", i, v.f32Value);
                break;
            case TypeInfo::TypeTag::I32:
                builder.AppendFormat("[%i: %i]", i, v.i32Value);
                break;
            case TypeInfo::TypeTag::Function:
                builder.AppendFormat("[%i: <fn %s>]", i, v.pFunction->name.pData ? v.pFunction->name.pData : "");
                break;
            default:
                break;
        }
    }
    String s = builder.CreateString();
    Log::Debug("->%s", s.pData);
    FreeString(s);
}

// ***********************************************************************

void Run(Function* pFuncToRun) {
    VirtualMachine vm;
    
    Value fv;
    fv.pFunction = pFuncToRun;
    fv.pType = GetEmptyFuncType(); //TODO: Don't need this
    vm.stack.Push(fv);

    CallFrame frame;
    frame.pFunc = pFuncToRun;
    frame.pInstructionPointer = pFuncToRun->chunk.code.pData;
    frame.stackBaseIndex = 0;
    vm.callStack.Push(frame);

    // VM run
    CallFrame* pFrame = &vm.callStack.Top();
    while (pFrame->pInstructionPointer < pFrame->pFunc->chunk.code.end()) {
#ifdef DEBUG_TRACE
        DisassembleInstruction(pFrame->pFunc->chunk, pFrame->pInstructionPointer);
#endif
        switch (*pFrame->pInstructionPointer++) {
            case OpCode::LoadConstant: {
                Value constant = pFrame->pFunc->chunk.constants[*pFrame->pInstructionPointer++];
                vm.stack.Push(constant); // knows from constant type table
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
                Value v = vm.stack.Pop();
				// TODO: print for the time being needs to encode the type in it's instruction like binops
                // TODO: Runtime error if you try print a void type value
                // TODO: As before this can be upgrade to be more robust at printing values of different types now that types are more complex
                if (v.pType->tag == TypeInfo::TypeTag::Type)
                    Log::Info("%s", v.pTypeInfo->name.pData);
                if (v.pType->tag == TypeInfo::TypeTag::F32)
                    Log::Info("%f", v.f32Value);
                else if (v.pType->tag == TypeInfo::TypeTag::I32)
                    Log::Info("%i", v.i32Value);
                else if (v.pType->tag == TypeInfo::TypeTag::Bool)
                    Log::Info("%s", v.boolValue ? "true" : "false");
                else if (v.pType->tag == TypeInfo::TypeTag::Function)
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
                // Must make a copy in case stack reallocates during push
                Value copy = vm.stack[pFrame->stackBaseIndex + opIndex];
                vm.stack.Push(copy);
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
                funcValue.pFunction;

                CallFrame frame;
                frame.pFunc = funcValue.pFunction;
                frame.pInstructionPointer = funcValue.pFunction->chunk.code.pData;
                frame.stackBaseIndex = vm.stack.array.count - argCount - 1;
                vm.callStack.Push(frame);

                pFrame = &vm.callStack.Top();
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