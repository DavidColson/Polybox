// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "types.h"

class GraphicsChip;

void DrawBox(GraphicsChip& gpu, f32 x, f32 y, f32 z, f32 width, f32 height, f32 depth);

void DrawIcosahedron(GraphicsChip& gpu, i32 maxDepth);
