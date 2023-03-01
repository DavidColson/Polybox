// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <resizable_array.h>

namespace Packing {
struct Rect {
    int x { 0 };
    int y { 0 };

    int w { 0 };
    int h { 0 };

    bool wasPacked { false };

    int ordering { -1 };
};

void RowPackRects(ResizableArray<Rect>& rects, int width, int height);
void SkylinePackRects(ResizableArray<Rect>& rects, int width, int height);
}
