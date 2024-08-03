// Copyright 2020-2022 David Colson. All rights reserved.

#include "shapes.h"

#include "graphics.h"
#include "vec3.inl"

// ***********************************************************************

void DrawBox(f32 x, f32 y, f32 z, f32 width, f32 height, f32 depth) {
    BeginObject3D(EPrimitiveType::Triangles);

    // Color(Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y, z));
    TexCoord(Vec2f(1.0f, 0.0f));
    Vertex(Vec3f(x + width, y, z));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y, z));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z));
    TexCoord(Vec2f(0.0f, 1.0f));
    Vertex(Vec3f(x, y + height, z));

    // Color(Vec4f(1.0f, 0.0f, 1.0f, 1.0f));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y, z + depth));
    TexCoord(Vec2f(0.0f, 1.0f));
    Vertex(Vec3f(x, y + height, z + depth));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z + depth));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y, z + depth));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z + depth));
    TexCoord(Vec2f(1.0f, 0.0f));
    Vertex(Vec3f(x + width, y, z + depth));

    // Color(Vec4f(1.0f, 1.0f, 0.0f, 1.0f));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y, z));
    TexCoord(Vec2f(0.0f, 1.0f));
    Vertex(Vec3f(x, y + height, z));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x, y + height, z + depth));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y, z));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x, y + height, z + depth));
    TexCoord(Vec2f(1.0f, 0.0f));
    Vertex(Vec3f(x, y, z + depth));

    // Color(Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x + width, y, z));
    TexCoord(Vec2f(1.0f, 0.0f));
    Vertex(Vec3f(x + width, y, z + depth));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z + depth));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x + width, y, z));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z + depth));
    TexCoord(Vec2f(0.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z));

    // Color(Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y, z));
    TexCoord(Vec2f(0.0f, 1.0f));
    Vertex(Vec3f(x, y, z + depth));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y, z + depth));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y, z));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y, z + depth));
    TexCoord(Vec2f(1.0f, 0.0f));
    Vertex(Vec3f(x + width, y, z));

    // Color(Vec4f(0.0f, 1.0f, 1.0f, 1.0f));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y + height, z));
    TexCoord(Vec2f(1.0f, 0.0f));
    Vertex(Vec3f(x + width, y + height, z));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z + depth));
    TexCoord(Vec2f(0.0f, 0.0f));
    Vertex(Vec3f(x, y + height, z));
    TexCoord(Vec2f(1.0f, 1.0f));
    Vertex(Vec3f(x + width, y + height, z + depth));
    TexCoord(Vec2f(0.0f, 1.0f));
    Vertex(Vec3f(x, y + height, z + depth));

    EndObject3D();
}


#define NFACE 20
#define NVERTEX 12

#define X .525731112119133606f
#define Z .850650808352039932f

static Vec3f vdata[NVERTEX] = {
    { -X, 0.0, Z }, { X, 0.0, Z }, { -X, 0.0, -Z }, { X, 0.0, -Z }, { 0.0, Z, X }, { 0.0, Z, -X }, { 0.0, -Z, X }, { 0.0, -Z, -X }, { Z, X, 0.0 }, { -Z, X, 0.0 }, { Z, -X, 0.0 }, { -Z, -X, 0.0 }
};

// These are the 20 faces.  Each of the three entries for each
// vertex gives the 3 vertices that make the face.
static i32 tindices[NFACE][3] = {
    { 0, 4, 1 }, { 0, 9, 4 }, { 9, 5, 4 }, { 4, 5, 8 }, { 4, 8, 1 }, { 8, 10, 1 }, { 8, 3, 10 }, { 5, 3, 8 }, { 5, 2, 3 }, { 2, 7, 3 }, { 7, 10, 3 }, { 7, 6, 10 }, { 7, 11, 6 }, { 11, 0, 6 }, { 0, 1, 6 }, { 6, 1, 10 }, { 9, 0, 11 }, { 9, 11, 2 }, { 9, 2, 5 }, { 7, 2, 11 }
};

// ***********************************************************************

void DrawTriangle(Vec3f v1, Vec3f v2, Vec3f v3) {
    Vertex(v1);
    Vertex(v2);
    Vertex(v3);
}

// ***********************************************************************

void SubDivide(Vec3f v1, Vec3f v2, Vec3f v3, i32 depth) {
    if (depth == 0) {
        DrawTriangle( v1, v2, v3);
        return;
    }
    // midpoint of each edge
    Vec3f v12;
    Vec3f v23;
    Vec3f v31;
    for (int i = 0; i < 3; i++) {
        v12[i] = v1[i] + v2[i];
        v23[i] = v2[i] + v3[i];
        v31[i] = v3[i] + v1[i];
    }
    v12 = v12.GetNormalized();
    v23 = v23.GetNormalized();
    v31 = v31.GetNormalized();

    SubDivide( v1, v12, v31, depth - 1);
    SubDivide( v2, v23, v12, depth - 1);
    SubDivide( v3, v31, v23, depth - 1);
    SubDivide( v12, v23, v31, depth - 1);
}

// ***********************************************************************

void DrawIcosahedron(i32 maxDepth) {
    BeginObject3D(EPrimitiveType::Triangles);
    for (int i = 0; i < NFACE; i++) {
        SubDivide( vdata[tindices[i][0]], vdata[tindices[i][1]], vdata[tindices[i][2]], maxDepth);
    }
    EndObject3D();
}
