// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <stdint.h>

template<typename T>
struct ResizableArray;
struct String;
struct IAllocator;

namespace TokenType {
enum Enum : uint32_t {
    Invalid,
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
    Percent,
    Caret,
    Greater,
    Less,
    // Two character tokens
    BangEqual,
    EqualEqual,
    LessEqual,
    GreaterEqual,
    And,
    Or,
    FuncSigReturn,
    // Literals
    LiteralString,
    LiteralInteger,
    LiteralFloat,
    LiteralBool,
    // Keywords
    If,
    Else,
    While,
    Struct,
    Return,
    // Other
    Identifier,
    EndOfFile,
    Count
};
}

struct Token {
    TokenType::Enum m_type { TokenType::Invalid };

    char* m_pLocation { nullptr };
    size_t m_length { 0 };

    char* m_pLineStart { nullptr };
    uint32_t m_line { 0 };
};

ResizableArray<Token> Tokenize(IAllocator* pAllocator, String sourceText);