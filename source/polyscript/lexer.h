// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "types.h"

template<typename T>
struct ResizableArray;
struct String;
struct IAllocator;
struct Compiler;

namespace TokenType {
enum Enum : u32 {
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
    usize length { 0 };

    char* pLineStart { nullptr };
    u32 line { 0 };
};

void Tokenize(Compiler& compilerState);
