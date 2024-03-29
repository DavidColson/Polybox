// Copyright 2020-2022 David Colson. All rights reserved.

#include "bind_graphics_chip.h"

#include "graphics_chip.h"
#include "lua_common.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#include <light_string.h>
#include <string.h>

namespace {
GraphicsChip* pGpu { nullptr };
}

namespace Bind {
// ***********************************************************************

int BeginObject2D(lua_State* pLua) {
    const char* primitiveType = luaL_checkstring(pLua, 1);

    EPrimitiveType type;
    if (strcmp(primitiveType, "Points") == 0)
        type = EPrimitiveType::Points;
    else if (strcmp(primitiveType, "Triangles") == 0)
        type = EPrimitiveType::Triangles;
    else if (strcmp(primitiveType, "Lines") == 0)
        type = EPrimitiveType::Lines;
    else if (strcmp(primitiveType, "LineStrip") == 0)
        type = EPrimitiveType::LineStrip;
    pGpu->BeginObject2D(type);
    return 0;
}

// ***********************************************************************

int EndObject2D(lua_State* pLua) {
    pGpu->EndObject2D();
    return 0;
}

// ***********************************************************************

int Vertex(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);

    if (lua_isnoneornil(pLua, 3)) {
        pGpu->Vertex(Vec2f(x, y));
        return 0;
    }

    f32 z = (f32)luaL_checknumber(pLua, 3);
    pGpu->Vertex(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int BeginObject3D(lua_State* pLua) {
    const char* primitiveType = luaL_checkstring(pLua, 1);

    EPrimitiveType type;
    if (strcmp(primitiveType, "Points") == 0)
        type = EPrimitiveType::Points;
    else if (strcmp(primitiveType, "Triangles") == 0)
        type = EPrimitiveType::Triangles;
    else if (strcmp(primitiveType, "Lines") == 0)
        type = EPrimitiveType::Lines;
    else if (strcmp(primitiveType, "LineStrip") == 0)
        type = EPrimitiveType::LineStrip;
    pGpu->BeginObject3D(type);
    return 0;
}

// ***********************************************************************

int EndObject3D(lua_State* pLua) {
    pGpu->EndObject3D();
    return 0;
}

// ***********************************************************************

int Color(lua_State* pLua) {
    f32 r = (f32)luaL_checknumber(pLua, 1);
    f32 g = (f32)luaL_checknumber(pLua, 2);
    f32 b = (f32)luaL_checknumber(pLua, 3);
    f32 a = (f32)luaL_checknumber(pLua, 4);
    pGpu->Color(Vec4f(r, g, b, a));
    return 0;
}

// ***********************************************************************

int TexCoord(lua_State* pLua) {
    f32 u = (f32)luaL_checknumber(pLua, 1);
    f32 v = (f32)luaL_checknumber(pLua, 2);
    pGpu->TexCoord(Vec2f(u, v));
    return 0;
}

// ***********************************************************************

int Normal(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);
    f32 z = (f32)luaL_checknumber(pLua, 3);
    pGpu->Normal(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int SetClearColor(lua_State* pLua) {
    f32 r = (f32)luaL_checknumber(pLua, 1);
    f32 g = (f32)luaL_checknumber(pLua, 2);
    f32 b = (f32)luaL_checknumber(pLua, 3);
    f32 a = (f32)luaL_checknumber(pLua, 4);
    pGpu->SetClearColor(Vec4f(r, g, b, a));
    return 0;
}

// ***********************************************************************

int MatrixMode(lua_State* pLua) {
    const char* matrixMode = luaL_checkstring(pLua, 1);

    EMatrixMode mode;
    if (strcmp(matrixMode, "Model") == 0)
        mode = EMatrixMode::Model;
    else if (strcmp(matrixMode, "View") == 0)
        mode = EMatrixMode::View;
    else if (strcmp(matrixMode, "Projection") == 0)
        mode = EMatrixMode::Projection;
    pGpu->MatrixMode(mode);
    return 0;
}

// ***********************************************************************

int Perspective(lua_State* pLua) {
    f32 screenWidth = (f32)luaL_checknumber(pLua, 1);
    f32 screenHeight = (f32)luaL_checknumber(pLua, 2);
    f32 nearPlane = (f32)luaL_checknumber(pLua, 3);
    f32 farPlane = (f32)luaL_checknumber(pLua, 4);
    f32 fov = (f32)luaL_checknumber(pLua, 5);
    pGpu->Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
    return 0;
}

// ***********************************************************************

int Translate(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);
    f32 z = (f32)luaL_checknumber(pLua, 3);
    pGpu->Translate(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int Rotate(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);
    f32 z = (f32)luaL_checknumber(pLua, 3);
    pGpu->Rotate(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int Scale(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);
    f32 z = (f32)luaL_checknumber(pLua, 3);
    pGpu->Scale(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int Identity(lua_State* pLua) {
    pGpu->Identity();
    return 0;
}

// ***********************************************************************

int BindTexture(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    pGpu->BindTexture(pImage);
    return 0;
}

// ***********************************************************************

int UnbindTexture(lua_State* pLua) {
    pGpu->UnbindTexture();
    return 0;
}

// ***********************************************************************

int NormalsMode(lua_State* pLua) {
    const char* normalsMode = luaL_checkstring(pLua, 1);
    ENormalsMode mode;
    if (strcmp(normalsMode, "Custom") == 0)
        mode = ENormalsMode::Custom;
    else if (strcmp(normalsMode, "Flat") == 0)
        mode = ENormalsMode::Flat;
    else if (strcmp(normalsMode, "Smooth") == 0)
        mode = ENormalsMode::Smooth;
    pGpu->NormalsMode(mode);
    return 0;
}

// ***********************************************************************

int EnableLighting(lua_State* pLua) {
    bool enabled = luax_checkboolean(pLua, 1);
    pGpu->EnableLighting(enabled);
    return 0;
}

// ***********************************************************************

int Light(lua_State* pLua) {
    i32 id = (i32)luaL_checkinteger(pLua, 1);

    Vec3f direction;
    direction.x = (f32)luaL_checknumber(pLua, 2);
    direction.y = (f32)luaL_checknumber(pLua, 3);
    direction.z = (f32)luaL_checknumber(pLua, 4);

    // consider making colors a type that can be more easily passed in?
    Vec3f color;
    color.x = (f32)luaL_checknumber(pLua, 5);
    color.y = (f32)luaL_checknumber(pLua, 6);
    color.z = (f32)luaL_checknumber(pLua, 7);

    pGpu->Light(id, direction, color);
    return 0;
}

// ***********************************************************************

int Ambient(lua_State* pLua) {
    Vec3f color;
    color.x = (f32)luaL_checknumber(pLua, 1);
    color.y = (f32)luaL_checknumber(pLua, 2);
    color.z = (f32)luaL_checknumber(pLua, 3);

    pGpu->Ambient(color);
    return 0;
}

// ***********************************************************************

int EnableFog(lua_State* pLua) {
    bool enabled = luax_checkboolean(pLua, 1);
    pGpu->EnableFog(enabled);
    return 0;
}

// ***********************************************************************

int SetFogStart(lua_State* pLua) {
    f32 start = (f32)luaL_checknumber(pLua, 1);
    pGpu->SetFogStart(start);
    return 0;
}

// ***********************************************************************

int SetFogEnd(lua_State* pLua) {
    f32 end = (f32)luaL_checknumber(pLua, 1);
    pGpu->SetFogEnd(end);
    return 0;
}

// ***********************************************************************

int SetFogColor(lua_State* pLua) {
    Vec3f color;
    color.x = (f32)luaL_checknumber(pLua, 1);
    color.y = (f32)luaL_checknumber(pLua, 2);
    color.z = (f32)luaL_checknumber(pLua, 3);
    pGpu->SetFogColor(color);
    return 0;
}

// ***********************************************************************

int DrawSprite(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 2);
    position.y = (f32)luaL_checknumber(pLua, 3);
    pGpu->DrawSprite(pImage, position);
    return 0;
}

// ***********************************************************************

int DrawSpriteRect(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    Vec4f rect;
    rect.x = (f32)luaL_checknumber(pLua, 2);
    rect.y = (f32)luaL_checknumber(pLua, 3);
    rect.z = (f32)luaL_checknumber(pLua, 4);
    rect.w = (f32)luaL_checknumber(pLua, 5);
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 6);
    position.y = (f32)luaL_checknumber(pLua, 7);
    pGpu->DrawSpriteRect(pImage, rect, position);
    return 0;
}

// ***********************************************************************

int DrawText(lua_State* pLua) {
    usize len;
    const char* str = luaL_checklstring(pLua, 1, &len);
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 2);
    position.y = (f32)luaL_checknumber(pLua, 3);
    f32 size = (f32)luaL_checknumber(pLua, 4);

    pGpu->DrawText(str, position, size);
    return 0;
}

// ***********************************************************************

int DrawTextEx(lua_State* pLua) {
    usize len;
    const char* str = luaL_checklstring(pLua, 1, &len);
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 2);
    position.y = (f32)luaL_checknumber(pLua, 3);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 4);
    color.y = (f32)luaL_checknumber(pLua, 5);
    color.z = (f32)luaL_checknumber(pLua, 6);
    color.w = (f32)luaL_checknumber(pLua, 7);
    Font* pFont = *(Font**)luaL_checkudata(pLua, 8, "Font");
    f32 size = (f32)luaL_checknumber(pLua, 9);

    pGpu->DrawTextEx(str, position, color, pFont, size);
    return 0;
}

int DrawPixel(lua_State* pLua) {
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 1);
    position.y = (f32)luaL_checknumber(pLua, 2);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 3);
    color.y = (f32)luaL_checknumber(pLua, 4);
    color.z = (f32)luaL_checknumber(pLua, 5);
    color.w = (f32)luaL_checknumber(pLua, 6);

    pGpu->DrawPixel(position, color);
    return 0;
}

int DrawLine(lua_State* pLua) {
    Vec2f start;
    start.x = (f32)luaL_checknumber(pLua, 1);
    start.y = (f32)luaL_checknumber(pLua, 2);
    Vec2f end;
    end.x = (f32)luaL_checknumber(pLua, 3);
    end.y = (f32)luaL_checknumber(pLua, 4);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 5);
    color.y = (f32)luaL_checknumber(pLua, 6);
    color.z = (f32)luaL_checknumber(pLua, 7);
    color.w = (f32)luaL_checknumber(pLua, 8);

    pGpu->DrawLine(start, end, color);
    return 0;
}

int DrawCircle(lua_State* pLua) {
    Vec2f center;
    center.x = (f32)luaL_checknumber(pLua, 1);
    center.y = (f32)luaL_checknumber(pLua, 2);
    f32 radius = (f32)luaL_checknumber(pLua, 3);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 4);
    color.y = (f32)luaL_checknumber(pLua, 5);
    color.z = (f32)luaL_checknumber(pLua, 6);
    color.w = (f32)luaL_checknumber(pLua, 7);

    pGpu->DrawCircle(center, radius, color);
    return 0;
}

int DrawCircleOutline(lua_State* pLua) {
    Vec2f center;
    center.x = (f32)luaL_checknumber(pLua, 1);
    center.y = (f32)luaL_checknumber(pLua, 2);
    f32 radius = (f32)luaL_checknumber(pLua, 3);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 4);
    color.y = (f32)luaL_checknumber(pLua, 5);
    color.z = (f32)luaL_checknumber(pLua, 6);
    color.w = (f32)luaL_checknumber(pLua, 7);

    pGpu->DrawCircleOutline(center, radius, color);
    return 0;
}

int DrawRectangle(lua_State* pLua) {
    Vec2f bottomLeft;
    bottomLeft.x = (f32)luaL_checknumber(pLua, 1);
    bottomLeft.y = (f32)luaL_checknumber(pLua, 2);
    Vec2f topRight;
    topRight.x = (f32)luaL_checknumber(pLua, 3);
    topRight.y = (f32)luaL_checknumber(pLua, 4);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 5);
    color.y = (f32)luaL_checknumber(pLua, 6);
    color.z = (f32)luaL_checknumber(pLua, 7);
    color.w = (f32)luaL_checknumber(pLua, 8);

    pGpu->DrawRectangle(bottomLeft, topRight, color);
    return 0;
}

int DrawRectangleOutline(lua_State* pLua) {
    Vec2f bottomLeft;
    bottomLeft.x = (f32)luaL_checknumber(pLua, 1);
    bottomLeft.y = (f32)luaL_checknumber(pLua, 2);
    Vec2f topRight;
    topRight.x = (f32)luaL_checknumber(pLua, 3);
    topRight.y = (f32)luaL_checknumber(pLua, 4);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 5);
    color.y = (f32)luaL_checknumber(pLua, 6);
    color.z = (f32)luaL_checknumber(pLua, 7);
    color.w = (f32)luaL_checknumber(pLua, 8);

    pGpu->DrawRectangleOutline(bottomLeft, topRight, color);
    return 0;
}

// ***********************************************************************

int NewImage(lua_State* pLua) {
    usize len;
    const char* str = luaL_checklstring(pLua, 1, &len);
    String path(str);

    // Create a new userdata for our object
    Image** ppImage = (Image**)lua_newuserdata(pLua, sizeof(Image*));
    *ppImage = new Image(path);

    // Sets the metatable of this new userdata to the type's table
    luaL_getmetatable(pLua, "Image");
    lua_setmetatable(pLua, -2);

    return 1;
}

// ***********************************************************************

int Image_GetWidth(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    lua_pushinteger(pLua, pImage->width);
    return 1;
}

// ***********************************************************************

int Image_GetHeight(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    lua_pushinteger(pLua, pImage->height);
    return 1;
}

// ***********************************************************************

int NewFont(lua_State* pLua) {
    usize len;
    const char* path = luaL_checklstring(pLua, 1, &len);
    bool antialiasing = luax_checkboolean(pLua, 2);
    f32 weight = (f32)luaL_checknumber(pLua, 3);

    // Create a new userdata for our object
    Font** ppFont = (Font**)lua_newuserdata(pLua, sizeof(Font*));
    *ppFont = new Font(path, antialiasing, weight);

    // Sets the metatable of this new userdata to the type's table
    luaL_getmetatable(pLua, "Font");
    lua_setmetatable(pLua, -2);

    return 1;
}

// ***********************************************************************

int BindGraphicsChip(lua_State* pLua, GraphicsChip* _pGraphics) {
    pGpu = _pGraphics;

    // Global functions
    ///////////////////

    const luaL_Reg graphicsFuncs[] = {
        { "BeginObject2D", BeginObject2D },
        { "EndObject2D", EndObject2D },
        { "Vertex", Vertex },
        { "BeginObject3D", BeginObject3D },
        { "EndObject3D", EndObject3D },
        { "Color", Color },
        { "TexCoord", TexCoord },
        { "Normal", Normal },
        { "SetClearColor", SetClearColor },
        { "MatrixMode", MatrixMode },
        { "Perspective", Perspective },
        { "Translate", Translate },
        { "Rotate", Rotate },
        { "Scale", Scale },
        { "Identity", Identity },
        { "BindTexture", BindTexture },
        { "UnbindTexture", UnbindTexture },
        { "NormalsMode", NormalsMode },
        { "EnableLighting", EnableLighting },
        { "Light", Light },
        { "Ambient", Ambient },
        { "EnableFog", EnableFog },
        { "SetFogStart", SetFogStart },
        { "SetFogEnd", SetFogEnd },
        { "SetFogColor", SetFogColor },
        { "DrawSprite", DrawSprite },
        { "DrawSpriteRect", DrawSpriteRect },
        { "DrawText", DrawText },
        { "DrawTextEx", DrawTextEx },
        { "DrawPixel", DrawPixel },
        { "DrawLine", DrawLine },
        { "DrawCircle", DrawCircle },
        { "DrawCircleOutline", DrawCircleOutline },
        { "DrawRectangle", DrawRectangle },
        { "DrawRectangleOutline", DrawRectangleOutline },
        { "NewImage", NewImage },
        { "NewFont", NewFont },
        { NULL, NULL }
    };

    lua_pushglobaltable(pLua);
    luaL_setfuncs(pLua, graphicsFuncs, 0);

    // Types
    ////////

    const luaL_Reg imageMethods[] = {
        { "GetWidth", Image_GetWidth },
        { "GetHeight", Image_GetHeight },
        { NULL, NULL }
    };
    luax_registertype(pLua, "Image", imageMethods);

    luax_registertype(pLua, "Font", nullptr);

    // // push enum tables to global
    // lua_newtable(pLua);
    // {
    //     lua_pushinteger(pLua, 0);
    //     lua_setfield(pLua, -2, "Points");
    //     lua_pushinteger(pLua, 1);
    //     lua_setfield(pLua, -2, "Triangles");
    //     lua_pushinteger(pLua, 2);
    //     lua_setfield(pLua, -2, "Lines");
    //     lua_pushinteger(pLua, 3);
    //     lua_setfield(pLua, -2, "LineStrip");
    // }
    // lua_setglobal(pLua, "PrimitiveType");

    return 0;
}
}
