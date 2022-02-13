// Copyright 2020-2022 David Colson. All rights reserved.

#include "Scanning.h"

#include <vector>

// Scanning utilities
///////////////////////

// @Improvement, use string_view for more performance and less memory thrashing

// ***********************************************************************

bool Scan::Match(ScanningState& scan, char expected)
{
	if (*(scan.current) == expected)
	{
		Advance(scan);
		return true;
	}
	return false;
}

// ***********************************************************************

char Scan::PeekNext(ScanningState& scan)
{
	return *(scan.current++);
}

// ***********************************************************************

bool Scan::IsWhitespace(char c)
{
	if(c == ' ' || c == '\r' || c == '\t' || c == '\n')
		return true;
	return false;
}

// ***********************************************************************

void Scan::AdvanceOverWhitespace(ScanningState& scan)
{
	char c = *(scan.current);
	while (IsWhitespace(c))
	{
		Advance(scan);
		c = Peek(scan);
		if (c == '\n')
		{
			scan.line++;
			scan.currentLineStart = scan.current + 1;
		}
	}
}

// ***********************************************************************

void Scan::AdvanceOverWhitespaceNoNewline(ScanningState& scan)
{
	char c = *(scan.current);
	while (IsWhitespace(c))
	{
		if (c == '\n')
			break;
		Advance(scan);
		c = Peek(scan);
	}
}

// ***********************************************************************

bool Scan::IsPartOfNumber(char c)
{
	return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.';
}

// ***********************************************************************

bool Scan::IsDigit(char c)
{
	return (c >= '0' && c <= '9');
}

// ***********************************************************************

bool Scan::IsHexDigit(char c)
{
	return isxdigit(c);
}

// ***********************************************************************

bool Scan::IsAlpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// ***********************************************************************

bool Scan::IsAlphaNumeric(char c)
{
	return IsAlpha(c) || IsDigit(c);
}
