// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <stdint.h>

template<typename T>
struct ResizableArray;
struct String;
struct IAllocator;
struct Compiler;

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
	Func,
    Fn,
	As,
    // Other
    Identifier,
    EndOfFile,
    Count
};
}

struct Token {
    TokenType::Enum type { TokenType::Invalid };

    char* pLocation { nullptr };
    size_t length { 0 };

    char* pLineStart { nullptr };
    uint32_t line { 0 };
};

void Tokenize(Compiler& compilerState);