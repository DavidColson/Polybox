// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <string>

namespace Scan
{
    // Scanning utilities
    ///////////////////////

    struct ScanningState
    {
        const char* textStart;
        const char* textEnd;
        char* current{ nullptr };
        char* currentLineStart{ nullptr };
        int line{ 1 };
        bool encounteredError{ false };
    };

    inline char Advance(ScanningState& scan)
    {
		return *(scan.current++);
	}

    inline char Peek(ScanningState& scan)
    {
		return *(scan.current);
	}

    inline bool IsAtEnd(ScanningState& scan)
    {
		return scan.current >= scan.textEnd;
	}

    bool Match(ScanningState& scan, char expected);

    char PeekNext(ScanningState& scan);

    bool IsWhitespace(char c);

    void AdvanceOverWhitespace(ScanningState& scan);

    void AdvanceOverWhitespaceNoNewline(ScanningState& scan);

    bool IsPartOfNumber(char c);

    bool IsDigit(char c);

    bool IsHexDigit(char c);

    bool IsAlpha(char c);
    
    bool IsAlphaNumeric(char c);

    std::string ParseToString(ScanningState& scan, char bound);
}