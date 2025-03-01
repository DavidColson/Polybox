// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

namespace Packing {
struct Rect {
    i32 x { 0 };
    i32 y { 0 };

    i32 w { 0 };
    i32 h { 0 };

    bool wasPacked { false };

    i32 ordering { -1 };
};

void RowPackRects(ResizableArray<Rect>& rects, i32 width, i32 height);
void SkylinePackRects(Arena* pScratchMem, ResizableArray<Rect>& rects, i32 width, i32 height);
}
