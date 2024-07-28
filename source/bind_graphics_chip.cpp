// Copyright 2020-2022 David Colson. All rights reserved.

#include "bind_graphics_chip.h"

#include "graphics_chip_sokol.h"
#include "lua_common.h"
#include "image.h"
#include "font.h"
#include "shapes.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#include <light_string.h>
#include <string.h>

namespace Bind {
// ***********************************************************************

int LuaBeginObject2D(lua_State* pLua) {
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
    BeginObject2D(type);
    return 0;
}

// ***********************************************************************

int LuaEndObject2D(lua_State* pLua) {
    EndObject2D();
    return 0;
}

// ***********************************************************************

int LuaVertex(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);

    if (lua_isnoneornil(pLua, 3)) {
        Vertex(Vec2f(x, y));
        return 0;
    }

    f32 z = (f32)luaL_checknumber(pLua, 3);
    Vertex(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int LuaBeginObject3D(lua_State* pLua) {
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
    BeginObject3D(type);
    return 0;
}

// ***********************************************************************

int LuaEndObject3D(lua_State* pLua) {
    EndObject3D();
    return 0;
}

// ***********************************************************************

int LuaColor(lua_State* pLua) {
    f32 r = (f32)luaL_checknumber(pLua, 1);
    f32 g = (f32)luaL_checknumber(pLua, 2);
    f32 b = (f32)luaL_checknumber(pLua, 3);
    f32 a = (f32)luaL_checknumber(pLua, 4);
    Color(Vec4f(r, g, b, a));
    return 0;
}

// ***********************************************************************

int LuaTexCoord(lua_State* pLua) {
    f32 u = (f32)luaL_checknumber(pLua, 1);
    f32 v = (f32)luaL_checknumber(pLua, 2);
    TexCoord(Vec2f(u, v));
    return 0;
}

// ***********************************************************************

int LuaNormal(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);
    f32 z = (f32)luaL_checknumber(pLua, 3);
    Normal(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int LuaSetClearColor(lua_State* pLua) {
    f32 r = (f32)luaL_checknumber(pLua, 1);
    f32 g = (f32)luaL_checknumber(pLua, 2);
    f32 b = (f32)luaL_checknumber(pLua, 3);
    f32 a = (f32)luaL_checknumber(pLua, 4);
    SetClearColor(Vec4f(r, g, b, a));
    return 0;
}

// ***********************************************************************

int LuaMatrixMode(lua_State* pLua) {
    const char* matrixMode = luaL_checkstring(pLua, 1);

    EMatrixMode mode;
    if (strcmp(matrixMode, "Model") == 0)
        mode = EMatrixMode::Model;
    else if (strcmp(matrixMode, "View") == 0)
        mode = EMatrixMode::View;
    else if (strcmp(matrixMode, "Projection") == 0)
        mode = EMatrixMode::Projection;
    MatrixMode(mode);
    return 0;
}

// ***********************************************************************

int LuaPerspective(lua_State* pLua) {
    f32 screenWidth = (f32)luaL_checknumber(pLua, 1);
    f32 screenHeight = (f32)luaL_checknumber(pLua, 2);
    f32 nearPlane = (f32)luaL_checknumber(pLua, 3);
    f32 farPlane = (f32)luaL_checknumber(pLua, 4);
    f32 fov = (f32)luaL_checknumber(pLua, 5);
    Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
    return 0;
}

// ***********************************************************************

int LuaTranslate(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);
    f32 z = (f32)luaL_checknumber(pLua, 3);
    Translate(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int LuaRotate(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);
    f32 z = (f32)luaL_checknumber(pLua, 3);
    Rotate(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int LuaScale(lua_State* pLua) {
    f32 x = (f32)luaL_checknumber(pLua, 1);
    f32 y = (f32)luaL_checknumber(pLua, 2);
    f32 z = (f32)luaL_checknumber(pLua, 3);
    Scale(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int LuaIdentity(lua_State* pLua) {
    Identity();
    return 0;
}

// ***********************************************************************

int LuaBindTexture(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    BindTexture(pImage);
    return 0;
}

// ***********************************************************************

int LuaUnbindTexture(lua_State* pLua) {
    UnbindTexture();
    return 0;
}

// ***********************************************************************

int LuaNormalsMode(lua_State* pLua) {
    const char* normalsMode = luaL_checkstring(pLua, 1);
    ENormalsMode mode;
    if (strcmp(normalsMode, "Custom") == 0)
        mode = ENormalsMode::Custom;
    else if (strcmp(normalsMode, "Flat") == 0)
        mode = ENormalsMode::Flat;
    else if (strcmp(normalsMode, "Smooth") == 0)
        mode = ENormalsMode::Smooth;
    NormalsMode(mode);
    return 0;
}

// ***********************************************************************

int LuaEnableLighting(lua_State* pLua) {
    bool enabled = luax_checkboolean(pLua, 1);
    EnableLighting(enabled);
    return 0;
}

// ***********************************************************************

int LuaLight(lua_State* pLua) {
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

    Light(id, direction, color);
    return 0;
}

// ***********************************************************************

int LuaAmbient(lua_State* pLua) {
    Vec3f color;
    color.x = (f32)luaL_checknumber(pLua, 1);
    color.y = (f32)luaL_checknumber(pLua, 2);
    color.z = (f32)luaL_checknumber(pLua, 3);

    Ambient(color);
    return 0;
}

// ***********************************************************************

int LuaEnableFog(lua_State* pLua) {
    bool enabled = luax_checkboolean(pLua, 1);
    EnableFog(enabled);
    return 0;
}

// ***********************************************************************

int LuaSetFogStart(lua_State* pLua) {
    f32 start = (f32)luaL_checknumber(pLua, 1);
    SetFogStart(start);
    return 0;
}

// ***********************************************************************

int LuaSetFogEnd(lua_State* pLua) {
    f32 end = (f32)luaL_checknumber(pLua, 1);
    SetFogEnd(end);
    return 0;
}

// ***********************************************************************

int LuaSetFogColor(lua_State* pLua) {
    Vec3f color;
    color.x = (f32)luaL_checknumber(pLua, 1);
    color.y = (f32)luaL_checknumber(pLua, 2);
    color.z = (f32)luaL_checknumber(pLua, 3);
    SetFogColor(color);
    return 0;
}

// ***********************************************************************

int LuaDrawSprite(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 2);
    position.y = (f32)luaL_checknumber(pLua, 3);
    DrawSprite(pImage, position);
    return 0;
}

// ***********************************************************************

int LuaDrawSpriteRect(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    Vec4f rect;
    rect.x = (f32)luaL_checknumber(pLua, 2);
    rect.y = (f32)luaL_checknumber(pLua, 3);
    rect.z = (f32)luaL_checknumber(pLua, 4);
    rect.w = (f32)luaL_checknumber(pLua, 5);
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 6);
    position.y = (f32)luaL_checknumber(pLua, 7);
    DrawSpriteRect(pImage, rect, position);
    return 0;
}

// ***********************************************************************

int LuaDrawText(lua_State* pLua) {
    usize len;
    const char* str = luaL_checklstring(pLua, 1, &len);
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 2);
    position.y = (f32)luaL_checknumber(pLua, 3);
    f32 size = (f32)luaL_checknumber(pLua, 4);

    DrawText(str, position, size);
    return 0;
}

// ***********************************************************************

int LuaDrawTextEx(lua_State* pLua) {
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

    DrawTextEx(str, position, color, pFont, size);
    return 0;
}

int LuaDrawPixel(lua_State* pLua) {
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 1);
    position.y = (f32)luaL_checknumber(pLua, 2);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 3);
    color.y = (f32)luaL_checknumber(pLua, 4);
    color.z = (f32)luaL_checknumber(pLua, 5);
    color.w = (f32)luaL_checknumber(pLua, 6);

    DrawPixel(position, color);
    return 0;
}

int LuaDrawLine(lua_State* pLua) {
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

    DrawLine(start, end, color);
    return 0;
}

int LuaDrawCircle(lua_State* pLua) {
    Vec2f center;
    center.x = (f32)luaL_checknumber(pLua, 1);
    center.y = (f32)luaL_checknumber(pLua, 2);
    f32 radius = (f32)luaL_checknumber(pLua, 3);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 4);
    color.y = (f32)luaL_checknumber(pLua, 5);
    color.z = (f32)luaL_checknumber(pLua, 6);
    color.w = (f32)luaL_checknumber(pLua, 7);

    DrawCircle(center, radius, color);
    return 0;
}

int LuaDrawCircleOutline(lua_State* pLua) {
    Vec2f center;
    center.x = (f32)luaL_checknumber(pLua, 1);
    center.y = (f32)luaL_checknumber(pLua, 2);
    f32 radius = (f32)luaL_checknumber(pLua, 3);
    Vec4f color;
    color.x = (f32)luaL_checknumber(pLua, 4);
    color.y = (f32)luaL_checknumber(pLua, 5);
    color.z = (f32)luaL_checknumber(pLua, 6);
    color.w = (f32)luaL_checknumber(pLua, 7);

    DrawCircleOutline(center, radius, color);
    return 0;
}

int LuaDrawRectangle(lua_State* pLua) {
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

    DrawRectangle(bottomLeft, topRight, color);
    return 0;
}

int LuaDrawRectangleOutline(lua_State* pLua) {
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

    DrawRectangleOutline(bottomLeft, topRight, color);
    return 0;
}

int LuaDrawBox(lua_State* pLua) {
	
	f32 x = (f32)luaL_checknumber(pLua, 1);
	f32 y = (f32)luaL_checknumber(pLua, 2);
	f32 z = (f32)luaL_checknumber(pLua, 3);
	f32 width = (f32)luaL_checknumber(pLua, 4);
	f32 height = (f32)luaL_checknumber(pLua, 5);
	f32 depth = (f32)luaL_checknumber(pLua, 6);

    DrawBox(x, y, z, width, height, depth);
    return 0;
}

int LuaDrawIcosahedron(lua_State* pLua) {
	
	f32 maxDepth = (f32)luaL_checknumber(pLua, 1);

	DrawIcosahedron(maxDepth);
    return 0;
}

// ***********************************************************************

int LuaNewImage(lua_State* pLua) {
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

int LuaImage_GetWidth(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    lua_pushinteger(pLua, pImage->width);
    return 1;
}

// ***********************************************************************

int LuaImage_GetHeight(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    lua_pushinteger(pLua, pImage->height);
    return 1;
}

// ***********************************************************************

int LuaNewFont(lua_State* pLua) {
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

int BindGraphicsChip(lua_State* pLua) {

    // Global functions
    ///////////////////

    const luaL_Reg graphicsFuncs[] = {
        { "BeginObject2D", LuaBeginObject2D },
        { "EndObject2D", LuaEndObject2D },
        { "Vertex", LuaVertex },
        { "BeginObject3D", LuaBeginObject3D },
        { "EndObject3D", LuaEndObject3D },
        { "Color", LuaColor },
        { "TexCoord", LuaTexCoord },
        { "Normal", LuaNormal },
        { "SetClearColor", LuaSetClearColor },
        { "MatrixMode", LuaMatrixMode },
        { "Perspective", LuaPerspective },
        { "Translate", LuaTranslate },
        { "Rotate", LuaRotate },
        { "Scale", LuaScale },
        { "Identity", LuaIdentity },
        { "BindTexture", LuaBindTexture },
        { "UnbindTexture", LuaUnbindTexture },
        { "NormalsMode", LuaNormalsMode },
        { "EnableLighting", LuaEnableLighting },
        { "Light", LuaLight },
        { "Ambient", LuaAmbient },
        { "EnableFog", LuaEnableFog },
        { "SetFogStart", LuaSetFogStart },
        { "SetFogEnd", LuaSetFogEnd },
        { "SetFogColor", LuaSetFogColor },
        { "DrawSprite", LuaDrawSprite },
        { "DrawSpriteRect", LuaDrawSpriteRect },
        { "DrawText", LuaDrawText },
        { "DrawTextEx", LuaDrawTextEx },
        { "DrawPixel", LuaDrawPixel },
        { "DrawLine", LuaDrawLine },
        { "DrawCircle", LuaDrawCircle },
        { "DrawCircleOutline", LuaDrawCircleOutline },
        { "DrawRectangle", LuaDrawRectangle },
        { "DrawRectangleOutline", LuaDrawRectangleOutline },
        { "DrawBox", LuaDrawBox },
        { "DrawIcosahedron", LuaDrawIcosahedron },
        { "NewImage", LuaNewImage },
        { "NewFont", LuaNewFont },
        { NULL, NULL }
    };

    lua_pushglobaltable(pLua);
    luaL_setfuncs(pLua, graphicsFuncs, 0);

    // Types
    ////////

    const luaL_Reg imageMethods[] = {
        { "GetWidth", LuaImage_GetWidth },
        { "GetHeight", LuaImage_GetHeight },
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
