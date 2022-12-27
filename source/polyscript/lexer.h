#pragma once

template<typename T>
struct ResizableArray;
struct String;
struct IAllocator;

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
    size_t m_line { 0 };
};

ResizableArray<Token> Tokenize(IAllocator* pAllocator, String sourceText);