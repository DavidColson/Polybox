// Copyright 2020-2022 David Colson. All rights reserved.

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

int LuaSetCullMode(lua_State* pLua) {
    sg_cull_mode m = (sg_cull_mode)luaL_checknumber(pLua, 1);
    SetCullMode(m);
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

int LuaPushMatrix(lua_State* pLua) {
    PushMatrix();
    return 0;
}

// ***********************************************************************

int LuaPopMatrix(lua_State* pLua) {
    PopMatrix();
    return 0;
}

// ***********************************************************************

int LuaLoadMatrix(lua_State* pLua) {
	UserData* pUserData = (UserData*)luaL_checkudata(pLua, 1, "UserData");
	if (pUserData->type != Type::Float32 || pUserData->width != 4 || pUserData->height != 4) {
		luaL_error(pLua, "Invalid matrix provided, need to be f32 type 4x4 matrix");
		return 0;
	}
	Matrixf mat;
	memcpy(mat.m, pUserData->pData, 16*sizeof(f32));
    LoadMatrix(mat);
    return 0;
}

// ***********************************************************************

int LuaGetMatrix(lua_State* pLua) {
    Matrixf mat = GetMatrix();

	UserData* pUserData = AllocUserData(pLua, Type::Float32, 4, 4);
	memcpy((u8*)pUserData->pData, mat.m, 16*sizeof(f32));
    return 1;
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
    f32 angle = (f32)luaL_checknumber(pLua, 1);
    f32 x = (f32)luaL_checknumber(pLua, 2);
    f32 y = (f32)luaL_checknumber(pLua, 3);
    f32 z = (f32)luaL_checknumber(pLua, 4);
    Rotate(angle, Vec3f(x, y, z));
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
	UserData* pUserData = (UserData*)luaL_checkudata(pLua, 1, "UserData");
	UpdateUserDataImage(pUserData);
    BindTexture(pUserData->img);
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
	luaL_checktype(pLua, -1, LUA_TBOOLEAN);
    bool enabled = lua_toboolean(pLua, -1) != 0;
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
	luaL_checktype(pLua, -1, LUA_TBOOLEAN);
    bool enabled = lua_toboolean(pLua, -1) != 0; 
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
	UserData* pUserData = (UserData*)luaL_checkudata(pLua, 1, "UserData");
	UpdateUserDataImage(pUserData);
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 2);
    position.y = (f32)luaL_checknumber(pLua, 3);
    DrawSprite(pUserData->img, position);
    return 0;
}

// ***********************************************************************

int LuaDrawSpriteRect(lua_State* pLua) {
	UserData* pUserData = (UserData*)luaL_checkudata(pLua, 1, "UserData");
	UpdateUserDataImage(pUserData);
    Vec4f rect;
    rect.x = (f32)luaL_checknumber(pLua, 2);
    rect.y = (f32)luaL_checknumber(pLua, 3);
    rect.z = (f32)luaL_checknumber(pLua, 4);
    rect.w = (f32)luaL_checknumber(pLua, 5);
    Vec2f position;
    position.x = (f32)luaL_checknumber(pLua, 6);
    position.y = (f32)luaL_checknumber(pLua, 7);
    DrawSpriteRect(pUserData->img, rect, position);
    return 0;
}

// ***********************************************************************

int BindGraphics(lua_State* pLua) {

    // Global functions
    ///////////////////

    const luaL_Reg graphicsFuncs[] = {
        { "begin_object_2d", LuaBeginObject2D },
        { "end_object_2d", LuaEndObject2D },
        { "vertex", LuaVertex },
        { "begin_object_3d", LuaBeginObject3D },
        { "end_object_3d", LuaEndObject3D },
        { "color", LuaColor },
        { "texcoord", LuaTexCoord },
        { "normal", LuaNormal },
        { "set_cull_mode", LuaSetCullMode },
        { "set_clear_color", LuaSetClearColor },
        { "matrix_mode", LuaMatrixMode },
        { "push_matrix", LuaPushMatrix },
        { "pop_matrix", LuaPopMatrix },
        { "load_matrix", LuaLoadMatrix },
        { "get_matrix", LuaGetMatrix },
        { "perspective", LuaPerspective },
        { "translate", LuaTranslate },
        { "rotate", LuaRotate },
        { "scale", LuaScale },
        { "identity", LuaIdentity },
        { "bind_texture", LuaBindTexture },
        { "unbind_texture", LuaUnbindTexture },
        { "normals_mode", LuaNormalsMode },
        { "enable_lighting", LuaEnableLighting },
        { "light", LuaLight },
        { "ambient", LuaAmbient },
        { "enable_fog", LuaEnableFog },
        { "set_fog_start", LuaSetFogStart },
        { "set_fog_end", LuaSetFogEnd },
        { "set_fog_color", LuaSetFogColor },
        { "draw_sprite", LuaDrawSprite },
        { "draw_sprite_rect", LuaDrawSpriteRect },
        { NULL, NULL }
    };

    lua_pushvalue(pLua, LUA_GLOBALSINDEX);
    luaL_register(pLua, NULL, graphicsFuncs);
    lua_pop(pLua, 1);

    return 0;
}
}
