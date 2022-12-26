
#include <log.h>
#include <resizable_array.inl>
#include <defer.h>
#include <stdint.h>
#include <string_builder.h>
#include <light_string.h>
#include <scanning.h>

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

enum class TokenType {
    // Simple one character tokens
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Colon,
    Semicolon,
    Comma,
    Dot,
    Minus,
    Plus,
    Star,
    Slash,
    Equal,
    Bang,
    Bar,
    Percent,
    Caret,
    Greater,
    Less,

    // Two character tokens
    BangEqual,
    EqualEqual,
    LessEqual,
    GreaterEqual,

    // Literals
    LiteralString,
    LiteralInteger,
    LiteralFloat,
    LiteralBool,

    // Keywords
    If,
    Else,
    For,
    Struct,
    Return,

    // Other
    Identifier,
    EndOfFile
};

struct Token {
    TokenType m_type;
    
    char* m_pLocation { nullptr };
    size_t m_length { 0 };
    
    char* m_pLineStart { nullptr };
    size_t m_line{ 0 };
};

Token MakeToken(Scan::ScanningState& scanner, TokenType type) {
    Token token;
    token.m_type = type;
    token.m_pLocation = scanner.m_pTokenStart;
    token.m_length = size_t(scanner.m_pCurrent - scanner.m_pTokenStart);
    token.m_pLineStart = scanner.m_pCurrentLineStart;
    token.m_line = scanner.m_line;
    return token;
}

Token ParseString(IAllocator* pAllocator, Scan::ScanningState& scan) {
    char* start = scan.m_pCurrent;
    while (*(scan.m_pCurrent) != '"' && !Scan::IsAtEnd(scan)) {
        if (*(scan.m_pCurrent++) == '\n') {
            scan.m_line++;
            scan.m_pCurrentLineStart = scan.m_pCurrent;
        }
    }
    scan.m_pCurrent++; // advance over closing quote
    return MakeToken(scan, TokenType::LiteralString);
}

Token ParseNumber(Scan::ScanningState& scan) {
    scan.m_pCurrent -= 1;  // Go back to get the first digit or symbol
    char* start = scan.m_pCurrent;

    // Normal number
    while (Scan::IsDigit(Scan::Peek(scan))) {
        Scan::Advance(scan);
    }

    if (Scan::Peek(scan) == '.') {
        Scan::Advance(scan);
        while (Scan::IsDigit(Scan::Peek(scan))) {
            Scan::Advance(scan);
        }
        return MakeToken(scan, TokenType::LiteralFloat);
    } else {
        return MakeToken(scan, TokenType::LiteralInteger);
    }
}

ResizableArray<Token> Tokenize(IAllocator* pAllocator, String jsonText) {
    Scan::ScanningState scan;
    scan.m_pTextStart = jsonText.m_pData;
    scan.m_pTextEnd = jsonText.m_pData + jsonText.m_length;
    scan.m_pCurrent = (char*)scan.m_pTextStart;
    scan.m_line = 1;

    ResizableArray<Token> tokens(pAllocator);

    while (!Scan::IsAtEnd(scan)) {
        scan.m_pTokenStart = scan.m_pCurrent;
        char c = Scan::Advance(scan);
        switch (c) {
            // Single character tokens
            case '(': tokens.PushBack(MakeToken(scan, TokenType::LeftParen)); break;
            case ')': tokens.PushBack(MakeToken(scan, TokenType::RightParen)); break;
            case '[': tokens.PushBack(MakeToken(scan, TokenType::LeftBracket)); break;
            case ']': tokens.PushBack(MakeToken(scan, TokenType::RightBracket)); break;
            case '{': tokens.PushBack(MakeToken(scan, TokenType::LeftBrace)); break;
            case '}': tokens.PushBack(MakeToken(scan, TokenType::RightBrace)); break;
            case ':': tokens.PushBack(MakeToken(scan, TokenType::Colon)); break;
            case ';': tokens.PushBack(MakeToken(scan, TokenType::Semicolon)); break;
            case ',': tokens.PushBack(MakeToken(scan, TokenType::Comma)); break;
            case '.': tokens.PushBack(MakeToken(scan, TokenType::Dot)); break;
            case '-': tokens.PushBack(MakeToken(scan, TokenType::Minus)); break;
            case '+': tokens.PushBack(MakeToken(scan, TokenType::Plus)); break;
            case '*': tokens.PushBack(MakeToken(scan, TokenType::Star)); break;
            case '|': tokens.PushBack(MakeToken(scan, TokenType::Bar)); break;
            case '%': tokens.PushBack(MakeToken(scan, TokenType::Percent)); break;
            case '^': tokens.PushBack(MakeToken(scan, TokenType::Caret)); break;
            case '>':
                if (Scan::Match(scan, '=')) {
                    tokens.PushBack(MakeToken(scan, TokenType::GreaterEqual));
                } else {
                    tokens.PushBack(MakeToken(scan, TokenType::Greater));
                }
                break;
            case '<':
                if (Scan::Match(scan, '=')) {
                    tokens.PushBack(MakeToken(scan, TokenType::LessEqual));
                } else {
                    tokens.PushBack(MakeToken(scan, TokenType::Less));
                }
                break;
            case '=': 
                if (Scan::Match(scan, '=')) {
                    tokens.PushBack(MakeToken(scan, TokenType::EqualEqual));
                } else {
                    tokens.PushBack(MakeToken(scan, TokenType::Equal)); 
                }
                break;
            case '!': 
                if (Scan::Match(scan, '=')) {
                    tokens.PushBack(MakeToken(scan, TokenType::BangEqual));
                } else {
                    tokens.PushBack(MakeToken(scan, TokenType::Bang));
                }
                break;

            // Comments and slash
            case '/':
                if (Scan::Match(scan, '/')) {
                    while (Scan::Peek(scan) != '\n')
                        Scan::Advance(scan);
                } else if (Scan::Match(scan, '*')) {
                    while (!(Scan::Peek(scan) == '*' && Scan::PeekNext(scan) == '/')) {
                        char commentChar = Scan::Advance(scan);
                        if (commentChar == '\n') {
                            scan.m_line++;
                            scan.m_pCurrentLineStart = scan.m_pCurrent;
                        }
                    }
                    Scan::Advance(scan);  // *
                    Scan::Advance(scan);  // /
                } else {
                    tokens.PushBack(MakeToken(scan, TokenType::Slash));
                }
                break;

            // Whitespace
            case ' ':
            case '\r':
            case '\t': break;
            case '\n':
                scan.m_line++;
                scan.m_pCurrentLineStart = scan.m_pCurrent;
                break;

            // String literals
            case '"':
                tokens.PushBack(ParseString(pAllocator, scan));
                break;

            default:
                // Numbers
                if (Scan::IsDigit(c)) {
                    tokens.PushBack(ParseNumber(scan));
                    break;
                }

                // Identifiers and keywords
                if (Scan::IsAlpha(c)) {
                    while (Scan::IsAlphaNumeric(Scan::Peek(scan)))
                        Scan::Advance(scan);

                    String identifier;
                    identifier.m_pData = scan.m_pTokenStart;
                    identifier.m_length = scan.m_pCurrent - scan.m_pTokenStart;

                    // Check for keywords
                    if (identifier == "if")
                        tokens.PushBack(MakeToken(scan, TokenType::If));
                    else if (identifier == "else")
                        tokens.PushBack(MakeToken(scan, TokenType::Else));
                    else if (identifier == "for")
                        tokens.PushBack(MakeToken(scan, TokenType::For));
                    else if (identifier == "struct")
                        tokens.PushBack(MakeToken(scan, TokenType::Struct));
                    else if (identifier == "return")
                        tokens.PushBack(MakeToken(scan, TokenType::Return));
                    else if (identifier == "true")
                        tokens.PushBack(MakeToken(scan, TokenType::LiteralBool));
                    else if (identifier == "false")
                        tokens.PushBack(MakeToken(scan, TokenType::LiteralBool));
                    else {
                        tokens.PushBack(MakeToken(scan, TokenType::Identifier));
                    }
                }
                break;
        }
    }

    scan.m_pTokenStart = scan.m_pCurrent;
    tokens.PushBack(MakeToken(scan, TokenType::EndOfFile));
    return tokens;
}


// TODO for cleanup of my prototype code

// [ ] Implement a nicer stack structure with custom indexing
// [ ] Move VM to it's own file where it can store it's context and run it's code in peace
// [ ] Move Disassembly code to it's own file for debugging code chunks
// [ ] Code chunks and opcodes should be in some universal header (probably the one for the VM)
// [ ] Move tokenizer to a new file where it can live

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