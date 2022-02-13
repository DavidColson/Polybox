// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#define ASSERT(condition, text) ((condition) ? (void)0 : Assertion(text, __FILE__, __LINE__))

namespace An
{
    int ShowAssertDialog(const char* errorMsg, const char* file, int line);

    void Assertion(const char* errorMsg, const char* file, int line);
}