// Copyright 2020-2022 David Colson. All rights reserved.

#include "lexer.h"

#include <scanning.h>
#include <resizable_array.inl>
#include <light_string.h>

#include "compiler.h"

// ***********************************************************************

Token MakeToken(Scan::ScanningState& scanner, TokenType::Enum type) {
    Token token;
    token.type = type;
    token.pLocation = scanner.pTokenStart;
    token.length = usize(scanner.pCurrent - scanner.pTokenStart);
    token.pLineStart = scanner.pCurrentLineStart;
    token.line = scanner.line;
    return token;
}

// ***********************************************************************

Token ParseString(IAllocator* pAllocator, Scan::ScanningState& scan) {
    char* start = scan.pCurrent;
    while (*(scan.pCurrent) != '"' && !Scan::IsAtEnd(scan)) {
        if (*(scan.pCurrent++) == '\n') {
            scan.line++;
            scan.pCurrentLineStart = scan.pCurrent;
        }
    }
    scan.pCurrent++;  // advance over closing quote
    return MakeToken(scan, TokenType::LiteralString);
}

// ***********************************************************************

Token ParseNumber(Scan::ScanningState& scan) {
    scan.pCurrent -= 1;  // Go back to get the first digit or symbol
    char* start = scan.pCurrent;

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

void Tokenize(Compiler& compilerState) {
    IAllocator* pAllocator = &compilerState.compilerMemory;
    String sourceText = compilerState.code;

    Scan::ScanningState scan;
    scan.pTextStart = sourceText.pData;
    scan.pTextEnd = sourceText.pData + sourceText.length;
    scan.pCurrent = (char*)scan.pTextStart;
    scan.pCurrentLineStart = (char*)scan.pTextStart;
    scan.line = 1;

    compilerState.tokens = ResizableArray<Token>(pAllocator);
    ResizableArray<Token>& tokens = compilerState.tokens;

    while (!Scan::IsAtEnd(scan)) {
        scan.pTokenStart = scan.pCurrent;
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
            case '+': tokens.PushBack(MakeToken(scan, TokenType::Plus)); break;
            case '*': tokens.PushBack(MakeToken(scan, TokenType::Star)); break;
            case '%': tokens.PushBack(MakeToken(scan, TokenType::Percent)); break;
			case '@': tokens.PushBack(MakeToken(scan, TokenType::Address)); break;
            case '^': tokens.PushBack(MakeToken(scan, TokenType::Caret)); break;
			case '.': 
                if (Scan::Match(scan, '{')) {
                    tokens.PushBack(MakeToken(scan, TokenType::StructLiteralOp));
                } else if (Scan::Match(scan, '[')) {
                    tokens.PushBack(MakeToken(scan, TokenType::ArrayLiteralOp));
				} else {
                    tokens.PushBack(MakeToken(scan, TokenType::Dot));
                }
                break;
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
                            scan.line++;
                            scan.pCurrentLineStart = scan.pCurrent;
                        }
                    }
					if (!Scan::IsAtEnd(scan)) Scan::Advance(scan);  // *
					if (!Scan::IsAtEnd(scan)) Scan::Advance(scan);  // /
                } else {
                    tokens.PushBack(MakeToken(scan, TokenType::Slash));
                }
                break;

            // Whitespace
            case ' ':
            case '\r':
            case '\t': break;
            case '\n':
                scan.line++;
                scan.pCurrentLineStart = scan.pCurrent;
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
                    identifier.pData = scan.pTokenStart;
                    identifier.length = scan.pCurrent - scan.pTokenStart;

                    // Check for keywords
                    if (identifier == "fn")
                        tokens.PushBack(MakeToken(scan, TokenType::Fn));
					else if (identifier == "as")
						tokens.PushBack(MakeToken(scan, TokenType::As));
					else if (identifier == "len")
						tokens.PushBack(MakeToken(scan, TokenType::Len));
					else if (identifier == "func")
						tokens.PushBack(MakeToken(scan, TokenType::Func));
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

    scan.pTokenStart = scan.pCurrent;
    tokens.PushBack(MakeToken(scan, TokenType::EndOfFile));
}
