// Copyright 2020-2022 David Colson. All rights reserved.

#include "virtual_machine.h"

#include <resizable_array.inl>
#include <light_string.h>
#include <string_builder.h>
#include <stack.inl>

//#define DEBUG_TRACE

struct VirtualMachine {
    CodeChunk* pCurrentChunk;
    uint8_t* pInstructionPointer;
    Stack<Value> stack;
};

// ***********************************************************************

uint8_t* DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction) {
    StringBuilder builder;
    uint8_t* pReturnInstruction = pInstruction;
    switch (*pInstruction) {
        case (uint8_t)OpCode::LoadConstant: {
            builder.Append("OpLoadConstant ");
            uint8_t constIndex = *(pInstruction + 1);
            Value& v = chunk.constants[constIndex];
            if (v.m_type == ValueType::F32)
                builder.AppendFormat("%f", v.m_f32Value);
            else if (v.m_type == ValueType::Bool)
                builder.AppendFormat("%s", v.m_boolValue ? "true" : "false");
            else if (v.m_type == ValueType::I32)
                builder.AppendFormat("%i", v.m_i32Value);
            pReturnInstruction += 2;
            break;
        }
        case (uint8_t)OpCode::Negate: {
            builder.Append("OpNegate ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Not: {
            builder.Append("OpNot ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Add: {
            builder.Append("OpAdd ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Subtract: {
            builder.Append("OpSubtract ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Multiply: {
            builder.Append("OpMultiply ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Divide: {
            builder.Append("OpDivide ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Greater: {
            builder.Append("OpGreater ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Less: {
            builder.Append("OpLess ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Equal: {
            builder.Append("OpEqual ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::And: {
            builder.Append("OpAnd ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Or: {
            builder.Append("OpOr ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Print: {
            builder.Append("OpPrint ");
            pReturnInstruction += 1;
            break;
        }
        case (uint8_t)OpCode::Return:
            builder.Append("OpReturn");
            pReturnInstruction += 1;
            break;
        default:
            break;
    }

    String output = builder.CreateString();
    Log::Debug("%s", output.m_pData);
    FreeString(output);
    return pReturnInstruction;
}

// ***********************************************************************

void Disassemble(CodeChunk& chunk) {
    Log::Debug("--------- Disassembly ---------");
    uint8_t* pInstructionPointer = chunk.code.m_pData;
    while (pInstructionPointer < chunk.code.end()) {
        pInstructionPointer = DisassembleInstruction(chunk, pInstructionPointer);
    }
}

// ***********************************************************************

void Run(CodeChunk* pChunkToRun) {
    VirtualMachine vm;
    vm.pCurrentChunk = pChunkToRun;
    vm.pInstructionPointer = vm.pCurrentChunk->code.m_pData;

    // VM run
    while (vm.pInstructionPointer < vm.pCurrentChunk->code.end()) {
#ifdef DEBUG_TRACE
        DisassembleInstruction(*vm.pCurrentChunk, vm.pInstructionPointer);
#endif
        switch (*vm.pInstructionPointer++) {
            case (uint8_t)OpCode::LoadConstant: {
                Value constant = vm.pCurrentChunk->constants[*vm.pInstructionPointer++];
                vm.stack.Push(constant);
                break;
            }
            case (uint8_t)OpCode::Negate: {
                Value v = vm.stack.Pop();
                v.m_f32Value = -v.m_f32Value;
                vm.stack.Push(v);
                break;
            }
            case (uint8_t)OpCode::Not: {
                Value v = vm.stack.Pop();
                v.m_boolValue = !v.m_boolValue;
                vm.stack.Push(v);
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
            case (uint8_t)OpCode::Equal: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Equal, a, b));
                break;
            }
            case (uint8_t)OpCode::And: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::And, a, b));
                break;
            }
            case (uint8_t)OpCode::Or: {
                Value b = vm.stack.Pop();
                Value a = vm.stack.Pop();
                vm.stack.Push(EvaluateOperator(Operator::Or, a, b));
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
                break;
            }
            case (uint8_t)OpCode::Return:

                break;
            default:
                break;
        }
    }
}