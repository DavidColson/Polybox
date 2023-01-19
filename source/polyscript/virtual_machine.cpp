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

struct VirtualMachine {
    Stack<Value> stack;
    Stack<CallFrame> callStack;
};

// ***********************************************************************

uint8_t DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction) {
    StringBuilder builder;
    uint8_t returnIPOffset = 0;
    switch (*pInstruction) {
        case (uint8_t)OpCode::LoadConstant: {
            builder.Append("LoadConstant ");
            uint8_t constIndex = *(pInstruction + 1);
            Value& v = chunk.constants[constIndex];
            if (v.m_type == ValueType::F32)
                builder.AppendFormat("%i (%f)", constIndex, v.m_f32Value);
            else if (v.m_type == ValueType::Bool)
                builder.AppendFormat("%i (%s)", constIndex, v.m_boolValue ? "true" : "false");
            else if (v.m_type == ValueType::I32)
                builder.AppendFormat("%i (%i)", constIndex, v.m_i32Value);
            else if (v.m_type == ValueType::Function)
                builder.AppendFormat("%i (<fn %s>)", constIndex, v.m_pFunction->m_name.m_pData ? v.m_pFunction->m_name.m_pData : "~anon~");  // TODO Should probably show the type sig?
            returnIPOffset = 2;
            break;
        }
        case (uint8_t)OpCode::SetLocal: {
            builder.Append("SetLocal ");
            uint8_t index = *(pInstruction + 1);
            builder.AppendFormat("%i", index);
            returnIPOffset = 2;
            break;
        }
        case (uint8_t)OpCode::GetLocal: {
            builder.Append("GetLocal ");
            uint8_t index = *(pInstruction + 1);
            builder.AppendFormat("%i", index);
            returnIPOffset = 2;
            break;
        }
        case (uint8_t)OpCode::Negate: {
            builder.Append("Negate ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Not: {
            builder.Append("Not ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Add: {
            builder.Append("Add ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Subtract: {
            builder.Append("Subtract ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Multiply: {
            builder.Append("Multiply ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Divide: {
            builder.Append("Divide ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Greater: {
            builder.Append("Greater ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Less: {
            builder.Append("Less ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::GreaterEqual: {
            builder.Append("GreaterEqual ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::LessEqual: {
            builder.Append("LessEqual ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Equal: {
            builder.Append("Equal ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::NotEqual: {
            builder.Append("NotEqual ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Print: {
            builder.Append("Print ");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Return: {
            builder.Append("Return");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::Pop: {
            builder.Append("Pop");
            returnIPOffset = 1;
            break;
        }
        case (uint8_t)OpCode::JmpIfFalse: {
            builder.Append("JmpIfFalse");
            uint16_t jmp = (uint16_t)((pInstruction[1] << 8) | pInstruction[2]);
            builder.AppendFormat(" %i", jmp);
            returnIPOffset = 3;
            break;
        }
        case (uint8_t)OpCode::JmpIfTrue: {
            builder.Append("JmpIfTrue");
            uint16_t jmp = (uint16_t)((pInstruction[1] << 8) | pInstruction[2]);
            builder.AppendFormat(" %i", jmp);
            returnIPOffset = 3;
            break;
        }
        case (uint8_t)OpCode::Jmp: {
            builder.Append("Jmp");
            uint16_t jmp = (uint16_t)((pInstruction[1] << 8) | pInstruction[2]);
            builder.AppendFormat(" %i", jmp);
            returnIPOffset = 3;
            break;
        }
        case (uint8_t)OpCode::Loop: {
            builder.Append("Loop");
            uint16_t jmp = (uint16_t)((pInstruction[1] << 8) | pInstruction[2]);
            builder.AppendFormat(" %i", -jmp);
            returnIPOffset = 3;
            break;
        }
        default:
            builder.Append("OpUnknown");
            returnIPOffset = 1;
            break;
    }

    String output = builder.CreateString();
    Log::Debug("%s", output.m_pData);
    FreeString(output);
    return returnIPOffset;
}

// ***********************************************************************

void Disassemble(Function* pFunc, String codeText) {
    CodeChunk& chunk = pFunc->m_chunk;

    uint8_t* pInstructionPointer = chunk.code.m_pData;

    for (Value& v : chunk.constants) {
        if (v.m_type == ValueType::Function) {
            Disassemble(v.m_pFunction, codeText);
        }
    }

    Log::Debug("----------------");
    Log::Debug("-- Function (%s)", pFunc->m_name.m_pData ? pFunc->m_name.m_pData : "~anon~");

    ResizableArray<String> m_lines;
    char* pCurrent = codeText.m_pData;
    char* pLineStart = codeText.m_pData;
    while (pCurrent < (codeText.m_pData + codeText.m_length)) {
        if (*pCurrent == '\n') {
            String line = CopyCStringRange(pLineStart, pCurrent);
            m_lines.PushBack(line);
            pLineStart = pCurrent + 1;
        }
        pCurrent++;
    }
    String line = CopyCStringRange(pLineStart, pCurrent);
    m_lines.PushBack(line);

    uint32_t lineCounter = 0;
    uint32_t currentLine = -1;

    while (pInstructionPointer < chunk.code.end()) {
        if (currentLine != chunk.m_lineInfo[lineCounter]) {
            currentLine = chunk.m_lineInfo[lineCounter];
            Log::Debug("");
            Log::Debug("  %i:%s", currentLine, m_lines[currentLine - 1].m_pData);
        }

        uint8_t offset = DisassembleInstruction(chunk, pInstructionPointer);
        pInstructionPointer += offset;
        lineCounter += offset;
    }

    m_lines.Free([](String& str) {
        FreeString(str);
    });
}

// ***********************************************************************

void DebugStack(VirtualMachine& vm) {
    StringBuilder builder;

    for (uint32_t i = 0; i < vm.stack.m_array.m_count; i++) {
        Value& v = vm.stack[i];

        switch (v.m_type) {
            case ValueType::Bool:
                builder.AppendFormat("[%i: %s]", i, v.m_boolValue ? "true" : "false");
                break;
            case ValueType::F32:
                builder.AppendFormat("[%i: %f]", i, v.m_f32Value);
                break;
            case ValueType::I32:
                builder.AppendFormat("[%i: %i]", i, v.m_i32Value);
                break;
            case ValueType::Function:
                builder.AppendFormat("[%i: <fn %s>]", i, v.m_pFunction->m_name.m_pData ? v.m_pFunction->m_name.m_pData : "~anon~");
                break;
            default:
                break;
        }
    }
    String s = builder.CreateString();
    Log::Debug("->%s", s.m_pData);
    FreeString(s);
}

// ***********************************************************************

void Run(Function* pFuncToRun) {
    VirtualMachine vm;
    
    CallFrame frame;
    frame.pFunc = pFuncToRun;
    frame.pInstructionPointer = pFuncToRun->m_chunk.code.m_pData;
    frame.stackBaseIndex = 0;
    vm.callStack.Push(frame);

    // VM run
    CallFrame* pFrame = &vm.callStack.Top();
    while (pFrame->pInstructionPointer < pFrame->pFunc->m_chunk.code.end()) {
#ifdef DEBUG_TRACE
        DisassembleInstruction(pFrame->pFunc->m_chunk, pFrame->pInstructionPointer);
#endif
        switch (*pFrame->pInstructionPointer++) {
            case (uint8_t)OpCode::LoadConstant: {
                Value constant = pFrame->pFunc->m_chunk.constants[*pFrame->pInstructionPointer++];
                vm.stack.Push(constant);
                break;
            }
            case (uint8_t)OpCode::Negate: {
                Value v = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::UnaryMinus, v, Value()));
                break;
            }
            case (uint8_t)OpCode::Not: {
                Value v = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Not, v, Value()));
                break;
            }
            case (uint8_t)OpCode::Add: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Add, a, b));
                break;
            }
            case (uint8_t)OpCode::Subtract: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Subtract, a, b));
                break;
            }
            case (uint8_t)OpCode::Multiply: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Multiply, a, b));
                break;
            }
            case (uint8_t)OpCode::Divide: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Divide, a, b));
                break;
            }
            case (uint8_t)OpCode::Greater: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Greater, a, b));
                break;
            }
            case (uint8_t)OpCode::Less: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Less, a, b));
                break;
            }
            case (uint8_t)OpCode::GreaterEqual: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::GreaterEqual, a, b));
                break;
            }
            case (uint8_t)OpCode::LessEqual: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::LessEqual, a, b));
                break;
            }
            case (uint8_t)OpCode::Equal: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Equal, a, b));
                break;
            }
            case (uint8_t)OpCode::NotEqual: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::NotEqual, a, b));
                break;
            }
            case (uint8_t)OpCode::Print: {
                Value v = vm.stack.Pop();
                if (v.m_type == ValueType::F32)
                    Log::Info("%f", v.m_f32Value);
                else if (v.m_type == ValueType::I32)
                    Log::Info("%i", v.m_i32Value);
                else if (v.m_type == ValueType::Bool)
                    Log::Info("%s", v.m_boolValue ? "true" : "false");
                else if (v.m_type == ValueType::Function)
                    Log::Info("<fn %s>", v.m_pFunction->m_name.m_pData ? v.m_pFunction->m_name.m_pData : "~anon~");  // TODO Should probably show the type sig?
                break;
            }
            case (uint8_t)OpCode::Pop:
                vm.stack.Pop();
                break;

            case (uint8_t)OpCode::SetLocal: {
                uint8_t opIndex = *pFrame->pInstructionPointer++;
                vm.stack[pFrame->stackBaseIndex + opIndex] = vm.stack.Top();
                break;
            }
            case (uint8_t)OpCode::GetLocal: {
                uint8_t opIndex = *pFrame->pInstructionPointer++;
                vm.stack.Push(vm.stack[pFrame->stackBaseIndex + opIndex]);
                break;
            }
            case (uint8_t)OpCode::JmpIfFalse: {
                uint16_t jmp = (uint16_t)((pFrame->pInstructionPointer[0] << 8) | pFrame->pInstructionPointer[1]);
                pFrame->pInstructionPointer += 2;
                if (!vm.stack.Top().m_boolValue)
                    pFrame->pInstructionPointer += jmp;
                break;
            }
            case (uint8_t)OpCode::JmpIfTrue: {
                uint16_t jmp = (uint16_t)((pFrame->pInstructionPointer[0] << 8) | pFrame->pInstructionPointer[1]);
                pFrame->pInstructionPointer += 2;
                if (vm.stack.Top().m_boolValue)
                    pFrame->pInstructionPointer += jmp;
                break;
            }
            case (uint8_t)OpCode::Jmp: {
                uint16_t jmp = (uint16_t)((pFrame->pInstructionPointer[0] << 8) | pFrame->pInstructionPointer[1]);
                pFrame->pInstructionPointer += 2;
                pFrame->pInstructionPointer += jmp;
                break;
            }
            case (uint8_t)OpCode::Loop: {
                uint16_t jmp = (uint16_t)((pFrame->pInstructionPointer[0] << 8) | pFrame->pInstructionPointer[1]);
                pFrame->pInstructionPointer += 2;
                pFrame->pInstructionPointer -= jmp;
                break;
            }
            case (uint8_t)OpCode::Return:
                break;
            default:
                break;
        }
#ifdef DEBUG_TRACE
        DebugStack(vm);
#endif
    }
}