// Copyright 2020-2022 David Colson. All rights reserved.

#include "Bind_GraphicsChip.h"

#include "GraphicsChip.h"

#include <light_string.h>

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
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);

    if (lua_isnoneornil(pLua, 3)) {
        pGpu->Vertex(Vec2f(x, y));
        return 0;
    }

    float z = (float)luaL_checknumber(pLua, 3);
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
    float r = (float)luaL_checknumber(pLua, 1);
    float g = (float)luaL_checknumber(pLua, 2);
    float b = (float)luaL_checknumber(pLua, 3);
    float a = (float)luaL_checknumber(pLua, 4);
    pGpu->Color(Vec4f(r, g, b, a));
    return 0;
}

// ***********************************************************************

int TexCoord(lua_State* pLua) {
    float u = (float)luaL_checknumber(pLua, 1);
    float v = (float)luaL_checknumber(pLua, 2);
    pGpu->TexCoord(Vec2f(u, v));
    return 0;
}

// ***********************************************************************

int Normal(lua_State* pLua) {
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);
    float z = (float)luaL_checknumber(pLua, 3);
    pGpu->Normal(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int SetClearColor(lua_State* pLua) {
    float r = (float)luaL_checknumber(pLua, 1);
    float g = (float)luaL_checknumber(pLua, 2);
    float b = (float)luaL_checknumber(pLua, 3);
    float a = (float)luaL_checknumber(pLua, 4);
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
    float screenWidth = (float)luaL_checknumber(pLua, 1);
    float screenHeight = (float)luaL_checknumber(pLua, 2);
    float nearPlane = (float)luaL_checknumber(pLua, 3);
    float farPlane = (float)luaL_checknumber(pLua, 4);
    float fov = (float)luaL_checknumber(pLua, 5);
    pGpu->Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
    return 0;
}

// ***********************************************************************

int Translate(lua_State* pLua) {
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);
    float z = (float)luaL_checknumber(pLua, 3);
    pGpu->Translate(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int Rotate(lua_State* pLua) {
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);
    float z = (float)luaL_checknumber(pLua, 3);
    pGpu->Rotate(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int Scale(lua_State* pLua) {
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);
    float z = (float)luaL_checknumber(pLua, 3);
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
    int id = (int)luaL_checkinteger(pLua, 1);

    Vec3f direction;
    direction.x = (float)luaL_checknumber(pLua, 2);
    direction.y = (float)luaL_checknumber(pLua, 3);
    direction.z = (float)luaL_checknumber(pLua, 4);

    // consider making colors a type that can be more easily passed in?
    Vec3f color;
    color.x = (float)luaL_checknumber(pLua, 5);
    color.y = (float)luaL_checknumber(pLua, 6);
    color.z = (float)luaL_checknumber(pLua, 7);

    pGpu->Light(id, direction, color);
    return 0;
}

// ***********************************************************************

int Ambient(lua_State* pLua) {
    Vec3f color;
    color.x = (float)luaL_checknumber(pLua, 1);
    color.y = (float)luaL_checknumber(pLua, 2);
    color.z = (float)luaL_checknumber(pLua, 3);

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
    float start = (float)luaL_checknumber(pLua, 1);
    pGpu->SetFogStart(start);
    return 0;
}

// ***********************************************************************

int SetFogEnd(lua_State* pLua) {
    float end = (float)luaL_checknumber(pLua, 1);
    pGpu->SetFogEnd(end);
    return 0;
}

// ***********************************************************************

int SetFogColor(lua_State* pLua) {
    Vec3f color;
    color.x = (float)luaL_checknumber(pLua, 1);
    color.y = (float)luaL_checknumber(pLua, 2);
    color.z = (float)luaL_checknumber(pLua, 3);
    pGpu->SetFogColor(color);
    return 0;
}

// ***********************************************************************

int DrawSprite(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    Vec2f position;
    position.x = (float)luaL_checknumber(pLua, 2);
    position.y = (float)luaL_checknumber(pLua, 3);
    pGpu->DrawSprite(pImage, position);
    return 0;
}

// ***********************************************************************

int DrawSpriteRect(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    Vec4f rect;
    rect.x = (float)luaL_checknumber(pLua, 2);
    rect.y = (float)luaL_checknumber(pLua, 3);
    rect.z = (float)luaL_checknumber(pLua, 4);
    rect.w = (float)luaL_checknumber(pLua, 5);
    Vec2f position;
    position.x = (float)luaL_checknumber(pLua, 6);
    position.y = (float)luaL_checknumber(pLua, 7);
    pGpu->DrawSpriteRect(pImage, rect, position);
    return 0;
}

// ***********************************************************************

int DrawText(lua_State* pLua) {
    size_t len;
    const char* str = luaL_checklstring(pLua, 1, &len);
    Vec2f position;
    position.x = (float)luaL_checknumber(pLua, 2);
    position.y = (float)luaL_checknumber(pLua, 3);
    float size = (float)luaL_checknumber(pLua, 4);

    pGpu->DrawText(str, position, size);
    return 0;
}

// ***********************************************************************

int DrawTextEx(lua_State* pLua) {
    size_t len;
    const char* str = luaL_checklstring(pLua, 1, &len);
    Vec2f position;
    position.x = (float)luaL_checknumber(pLua, 2);
    position.y = (float)luaL_checknumber(pLua, 3);
    Vec4f color;
    color.x = (float)luaL_checknumber(pLua, 4);
    color.y = (float)luaL_checknumber(pLua, 5);
    color.z = (float)luaL_checknumber(pLua, 6);
    color.w = (float)luaL_checknumber(pLua, 7);
    Font* pFont = *(Font**)luaL_checkudata(pLua, 8, "Font");
    float size = (float)luaL_checknumber(pLua, 9);

    pGpu->DrawTextEx(str, position, color, pFont, size);
    return 0;
}

int DrawPixel(lua_State* pLua) {
    Vec2f position;
    position.x = (float)luaL_checknumber(pLua, 1);
    position.y = (float)luaL_checknumber(pLua, 2);
    Vec4f color;
    color.x = (float)luaL_checknumber(pLua, 3);
    color.y = (float)luaL_checknumber(pLua, 4);
    color.z = (float)luaL_checknumber(pLua, 5);
    color.w = (float)luaL_checknumber(pLua, 6);

    pGpu->DrawPixel(position, color);
    return 0;
}

int DrawLine(lua_State* pLua) {
    Vec2f start;
    start.x = (float)luaL_checknumber(pLua, 1);
    start.y = (float)luaL_checknumber(pLua, 2);
    Vec2f end;
    end.x = (float)luaL_checknumber(pLua, 3);
    end.y = (float)luaL_checknumber(pLua, 4);
    Vec4f color;
    color.x = (float)luaL_checknumber(pLua, 5);
    color.y = (float)luaL_checknumber(pLua, 6);
    color.z = (float)luaL_checknumber(pLua, 7);
    color.w = (float)luaL_checknumber(pLua, 8);

    pGpu->DrawLine(start, end, color);
    return 0;
}

int DrawCircle(lua_State* pLua) {
    Vec2f center;
    center.x = (float)luaL_checknumber(pLua, 1);
    center.y = (float)luaL_checknumber(pLua, 2);
    float radius = (float)luaL_checknumber(pLua, 3);
    Vec4f color;
    color.x = (float)luaL_checknumber(pLua, 4);
    color.y = (float)luaL_checknumber(pLua, 5);
    color.z = (float)luaL_checknumber(pLua, 6);
    color.w = (float)luaL_checknumber(pLua, 7);

    pGpu->DrawCircle(center, radius, color);
    return 0;
}

int DrawCircleOutline(lua_State* pLua) {
    Vec2f center;
    center.x = (float)luaL_checknumber(pLua, 1);
    center.y = (float)luaL_checknumber(pLua, 2);
    float radius = (float)luaL_checknumber(pLua, 3);
    Vec4f color;
    color.x = (float)luaL_checknumber(pLua, 4);
    color.y = (float)luaL_checknumber(pLua, 5);
    color.z = (float)luaL_checknumber(pLua, 6);
    color.w = (float)luaL_checknumber(pLua, 7);

    pGpu->DrawCircleOutline(center, radius, color);
    return 0;
}

int DrawRectangle(lua_State* pLua) {
    Vec2f bottomLeft;
    bottomLeft.x = (float)luaL_checknumber(pLua, 1);
    bottomLeft.y = (float)luaL_checknumber(pLua, 2);
    Vec2f topRight;
    topRight.x = (float)luaL_checknumber(pLua, 3);
    topRight.y = (float)luaL_checknumber(pLua, 4);
    Vec4f color;
    color.x = (float)luaL_checknumber(pLua, 5);
    color.y = (float)luaL_checknumber(pLua, 6);
    color.z = (float)luaL_checknumber(pLua, 7);
    color.w = (float)luaL_checknumber(pLua, 8);

    pGpu->DrawRectangle(bottomLeft, topRight, color);
    return 0;
}

int DrawRectangleOutline(lua_State* pLua) {
    Vec2f bottomLeft;
    bottomLeft.x = (float)luaL_checknumber(pLua, 1);
    bottomLeft.y = (float)luaL_checknumber(pLua, 2);
    Vec2f topRight;
    topRight.x = (float)luaL_checknumber(pLua, 3);
    topRight.y = (float)luaL_checknumber(pLua, 4);
    Vec4f color;
    color.x = (float)luaL_checknumber(pLua, 5);
    color.y = (float)luaL_checknumber(pLua, 6);
    color.z = (float)luaL_checknumber(pLua, 7);
    color.w = (float)luaL_checknumber(pLua, 8);

    pGpu->DrawRectangleOutline(bottomLeft, topRight, color);
    return 0;
}

// ***********************************************************************

int NewImage(lua_State* pLua) {
    size_t len;
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
    lua_pushinteger(pLua, pImage->m_width);
    return 1;
}

// ***********************************************************************

int Image_GetHeight(lua_State* pLua) {
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    lua_pushinteger(pLua, pImage->m_height);
    return 1;
}

// ***********************************************************************

int NewFont(lua_State* pLua) {
    size_t len;
    const char* path = luaL_checklstring(pLua, 1, &len);
    bool antialiasing = luax_checkboolean(pLua, 2);
    float weight = (float)luaL_checknumber(pLua, 3);

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
