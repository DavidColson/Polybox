#include "virtual_machine.h"

#include <resizable_array.inl>
#include <light_string.h>
#include <string_builder.h>

// Consider, we make a resizable array type structure, but with a special indexing mechanism, similar to lua's stack operations
template<typename T>
struct Stack {
    ResizableArray<T> m_internalArray;

    T& GetTop() {
        return m_internalArray[m_internalArray.m_count - 1];
    }

    void Push(T value) {
        m_internalArray.PushBack(value);
    }

    T Pop() {
        T v = m_internalArray[m_internalArray.m_count - 1];
        m_internalArray.Erase(m_internalArray.m_count - 1);
        return v;
    }
};

struct VirtualMachine {
    CodeChunk* pCurrentChunk;
    uint8_t* pInstructionPointer;
    Stack<double> stack;
};

uint8_t* DisassembleInstruction(CodeChunk& chunk, uint8_t* pInstruction) {
    StringBuilder builder;
    uint8_t* pReturnInstruction = pInstruction;
    switch (*pInstruction) {
        case (uint8_t)OpCode::LoadConstant: {
            builder.Append("OpLoadConstant ");
            uint8_t constIndex = *(pInstruction + 1);
            builder.AppendFormat("%f", chunk.constants[constIndex]);
            pReturnInstruction += 2;
            break;
        }
        case (uint8_t)OpCode::Negate: {
            builder.Append("OpNegate ");
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

void Disassemble(CodeChunk& chunk) {
    Log::Debug("--------- Disassembly ---------");
    uint8_t* pInstructionPointer = chunk.code.m_pData;
    while (pInstructionPointer < chunk.code.end()) {
        pInstructionPointer = DisassembleInstruction(chunk, pInstructionPointer);
    }
}

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
                double constant = vm.pCurrentChunk->constants[*vm.pInstructionPointer++];
                vm.stack.Push(constant);
                break;
            }
            case (uint8_t)OpCode::Negate: {
                vm.stack.Push(-vm.stack.Pop());
                break;
            }
            case (uint8_t)OpCode::Add: {
                double b = vm.stack.Pop();
                double a = vm.stack.Pop();
                vm.stack.Push(a + b);
                break;
            }
            case (uint8_t)OpCode::Subtract: {
                double b = vm.stack.Pop();
                double a = vm.stack.Pop();
                vm.stack.Push(a - b);
                break;
            }
            case (uint8_t)OpCode::Multiply: {
                double b = vm.stack.Pop();
                double a = vm.stack.Pop();
                vm.stack.Push(a * b);
                break;
            }
            case (uint8_t)OpCode::Divide: {
                double b = vm.stack.Pop();
                double a = vm.stack.Pop();
                vm.stack.Push(a / b);
                break;
            }
            case (uint8_t)OpCode::Print: {
                double value = vm.stack.Pop();
                Log::Info("%f", value);
                break;
            }
            case (uint8_t)OpCode::Return:

                break;
            default:
                break;
        }
    }
}