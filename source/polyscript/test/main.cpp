
#include <log.h>
#include <resizable_array.inl>
#include <defer.h>
#include <stdint.h>
#include <string_builder.h>
#include <light_string.h>

#define DEBUG_TRACE

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



enum class OpCode : uint8_t {
    LoadConstant,
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,
    Print,
    Return
};

struct CodeChunk {
    ResizableArray<double> constants;  // This should be a variant or something at some stage?
    ResizableArray<uint8_t> code;
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

struct VirtualMachine {
    CodeChunk* pCurrentChunk;
    uint8_t* pInstructionPointer;
    Stack<double> stack;
};

// TODO for cleanup of my prototype code

// [ ] Implement a nicer stack structure with custom indexing
// [ ] Move VM to it's own file where it can store it's context and run it's code in peace
// [ ] Move Disassembly code to it's own file for debugging code chunks
// [ ] Code chunks and opcodes should be in some universal header (probably the one for the VM)

int main() {
    CodeChunk chunk;
    defer(chunk.code.Free());
    defer(chunk.constants.Free());

    // Add constant to table and get it's index
    chunk.constants.PushBack(7);
    uint8_t constIndex7 = (uint8_t)chunk.constants.m_count - 1;

    chunk.constants.PushBack(14);
    uint8_t constIndex14 = (uint8_t)chunk.constants.m_count - 1;

    chunk.constants.PushBack(6);
    uint8_t constIndex6 = (uint8_t)chunk.constants.m_count - 1;

    // Add a load constant opcode
    chunk.code.PushBack((uint8_t)OpCode::LoadConstant);
    chunk.code.PushBack(constIndex6);

    chunk.code.PushBack((uint8_t)OpCode::LoadConstant);
    chunk.code.PushBack(constIndex7);

    chunk.code.PushBack((uint8_t)OpCode::LoadConstant);
    chunk.code.PushBack(constIndex14);

    // return opcode
    chunk.code.PushBack((uint8_t)OpCode::Subtract);
    //chunk.code.PushBack((uint8_t)OpCode::Add);
    chunk.code.PushBack((uint8_t)OpCode::Print);
    chunk.code.PushBack((uint8_t)OpCode::Return);

    

    VirtualMachine vm;
    vm.pCurrentChunk = &chunk;
    vm.pInstructionPointer = vm.pCurrentChunk->code.m_pData;

    // VM run 
    {
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

    //Disassemble(chunk);

    __debugbreak();
    return 0;
}