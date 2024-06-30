// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "graphics_chip_sokol.h"
#include "lua_common.h"

#include <light_string.h>
#include <resizable_array.h>

struct Primitive : public LuaObject {
    ResizableArray<VertexData> vertices;

    u32 baseColorTexture { UINT32_MAX };

    i32 GetNumVertices();
    Vec3f GetVertexPosition(int index);
    Vec4f GetVertexColor(int index);
    Vec2f GetVertexTexCoord(int index);
    Vec3f GetVertexNormal(int index);

    i32 GetMaterialTextureId();

    // TODO: API for modifying vertex data (i.e. allow creating meshes on the fly)
};

struct Mesh : public LuaObject {
    virtual ~Mesh();

    String name;
    ResizableArray<Primitive> primitives;

    i32 GetNumPrimitives();
    Primitive* GetPrimitive(int index);

    static ResizableArray<Mesh*> LoadMeshes(const char* filePath);
    static ResizableArray<Image*> LoadTextures(const char* filePath);
};
