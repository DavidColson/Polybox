// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

class GraphicsChip;

void DrawBox(GraphicsChip& gpu, float x, float y, float z, float width, float height, float depth);

void DrawIcosahedron(GraphicsChip& gpu, int maxDepth);
