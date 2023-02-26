// Copyright 2020-2022 David Colson. All rights reserved.

#include "lexer.h"

#include <scanning.h>
#include <resizable_array.inl>
#include <light_string.h>

// ***********************************************************************

Token MakeToken(Scan::ScanningState& scanner, TokenType::Enum type) {
    Token token;
    token.m_type = type;
    token.m_pLocation = scanner.m_pTokenStart;
    token.m_length = size_t(scanner.m_pCurrent - scanner.m_pTokenStart);
    token.m_pLineStart = scanner.m_pCurrentLineStart;
    token.m_line = scanner.m_line;
    return token;
}

// ***********************************************************************

Token ParseString(IAllocator* pAllocator, Scan::ScanningState& scan) {
    char* start = scan.m_pCurrent;
    while (*(scan.m_pCurrent) != '"' && !Scan::IsAtEnd(scan)) {
        if (*(scan.m_pCurrent++) == '\n') {
            scan.m_line++;
            scan.m_pCurrentLineStart = scan.m_pCurrent;
        }
    }
    scan.m_pCurrent++;  // advance over closing quote
    return MakeToken(scan, TokenType::LiteralString);
}

// ***********************************************************************

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

// ***********************************************************************

ResizableArray<Token> Tokenize(IAllocator* pAllocator, String sourceText) {
    Scan::ScanningState scan;
    scan.m_pTextStart = sourceText.m_pData;
    scan.m_pTextEnd = sourceText.m_pData + sourceText.m_length;
    scan.m_pCurrent = (char*)scan.m_pTextStart;
    scan.m_pCurrentLineStart = (char*)scan.m_pTextStart;
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
            case '+': tokens.PushBack(MakeToken(scan, TokenType::Plus)); break;
            case '*': tokens.PushBack(MakeToken(scan, TokenType::Star)); break;
            case '%': tokens.PushBack(MakeToken(scan, TokenType::Percent)); break;
            case '^': tokens.PushBack(MakeToken(scan, TokenType::Caret)); break;
            case '-':
                if (Scan::Match(scan, '>')) {
                    tokens.PushBack(MakeToken(scan, TokenType::FuncSigReturn));
                } else {
                    tokens.PushBack(MakeToken(scan, TokenType::Minus));
                }
                break;
            case '&':
                if (Scan::Match(scan, '&')) {
                    tokens.PushBack(MakeToken(scan, TokenType::And));
                }
            case '|':
                if (Scan::Match(scan, '|')) {
                    tokens.PushBack(MakeToken(scan, TokenType::Or));
                }
                break;
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
                    while (Scan::Peek(scan) != '\n' && !Scan::IsAtEnd(scan))
                        Scan::Advance(scan);
                } else if (Scan::Match(scan, '*')) {
                    while (!(Scan::Peek(scan) == '*' && Scan::PeekNext(scan) == '/') && !Scan::IsAtEnd(scan)) {
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
                    if (identifier == "fn")
                        tokens.PushBack(MakeToken(scan, TokenType::Fn));
                    else if (identifier == "if")
                        tokens.PushBack(MakeToken(scan, TokenType::If));
                    else if (identifier == "else")
                        tokens.PushBack(MakeToken(scan, TokenType::Else));
                    else if (identifier == "while")
                        tokens.PushBack(MakeToken(scan, TokenType::While));
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