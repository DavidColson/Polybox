
#include <log.h>
#include <resizable_array.inl>
#include <defer.h>
#include <stdint.h>
#include <string_builder.h>
#include <light_string.h>
#include <scanning.h>

#include "virtual_machine.h"
#include "lexer.h"

#define DEBUG_TRACE

// TODO for cleanup of my prototype code

// [ ] Implement a nicer stack structure with custom indexing

int main() {
    // Scanning?
    // Want to reuse scanner from commonLib don't we

    String actualCode;
    actualCode = "myvar := ( 5 - 2 ) * 12; true if for 22.42";

    ResizableArray<Token> tokens = Tokenize(&g_Allocator, actualCode);

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

    Run(&chunk);

    //Disassemble(chunk);

    __debugbreak();
    return 0;
}