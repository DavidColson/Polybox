#include "Bind_GraphicsChip.h"

#include "GraphicsChip.h"

namespace
{
    GraphicsChip* pGpu{ nullptr };
}

// ***********************************************************************

int luaBeginObject2D(lua_State* pLua)
{
    const char* primitiveType = luaL_checkstring(pLua, 1);

    EPrimitiveType type;
    if (strcmp(primitiveType, "Points") == 0) type = EPrimitiveType::Points;
    else if (strcmp(primitiveType, "Triangles") == 0) type = EPrimitiveType::Triangles;
    else if (strcmp(primitiveType, "Lines") == 0) type = EPrimitiveType::Lines;
    else if (strcmp(primitiveType, "LineStrip") == 0) type = EPrimitiveType::LineStrip;
    pGpu->BeginObject2D(type);
    return 0;
}

// ***********************************************************************

int luaEndObject2D(lua_State* pLua)
{
    pGpu->EndObject2D();
    return 0;
}

// ***********************************************************************

int luaVertex(lua_State* pLua)
{
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);

    if (lua_isnoneornil(pLua, 3))
    {
        pGpu->Vertex(Vec2f(x, y));
        return 0;
    }

    float z = (float)luaL_checknumber(pLua, 3);
    pGpu->Vertex(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int luaBeginObject3D(lua_State* pLua)
{
    const char* primitiveType = luaL_checkstring(pLua, 1);

    EPrimitiveType type;
    if (strcmp(primitiveType, "Points") == 0) type = EPrimitiveType::Points;
    else if (strcmp(primitiveType, "Triangles") == 0) type = EPrimitiveType::Triangles;
    else if (strcmp(primitiveType, "Lines") == 0) type = EPrimitiveType::Lines;
    else if (strcmp(primitiveType, "LineStrip") == 0) type = EPrimitiveType::LineStrip;
    pGpu->BeginObject3D(type);
    return 0;
}

// ***********************************************************************

int luaColor(lua_State* pLua)
{
    float r = (float)luaL_checknumber(pLua, 1);
    float g = (float)luaL_checknumber(pLua, 2);
    float b = (float)luaL_checknumber(pLua, 3);
    float a = (float)luaL_checknumber(pLua, 4);
    pGpu->Color(Vec4f(r, g, b, a));
    return 0;
}

// ***********************************************************************

int luaTexCoord(lua_State* pLua)
{
    float u = (float)luaL_checknumber(pLua, 1);
    float v = (float)luaL_checknumber(pLua, 2);
    pGpu->TexCoord(Vec2f(u, v));
    return 0;
}

// ***********************************************************************

int luaNormal(lua_State* pLua)
{
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);
    float z = (float)luaL_checknumber(pLua, 3);
    pGpu->Normal(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int luaSetClearColor(lua_State* pLua)
{
    float r = (float)luaL_checknumber(pLua, 1);
    float g = (float)luaL_checknumber(pLua, 2);
    float b = (float)luaL_checknumber(pLua, 3);
    float a = (float)luaL_checknumber(pLua, 4);
    pGpu->SetClearColor(Vec4f(r, g, b, a));
    return 0;
}

// ***********************************************************************

int luaMatrixMode(lua_State* pLua)
{
    const char* matrixMode = luaL_checkstring(pLua, 1);

    EMatrixMode mode;
    if (strcmp(matrixMode, "Model") == 0) mode = EMatrixMode::Model;
    else if (strcmp(matrixMode, "View") == 0) mode = EMatrixMode::View;
    else if (strcmp(matrixMode, "Projection") == 0) mode = EMatrixMode::Projection;
    pGpu->MatrixMode(mode);
    return 0;
}

// ***********************************************************************

int luaPerspective(lua_State* pLua)
{
    float screenWidth = (float)luaL_checknumber(pLua, 1);
    float screenHeight = (float)luaL_checknumber(pLua, 2);
    float nearPlane = (float)luaL_checknumber(pLua, 3);
    float farPlane = (float)luaL_checknumber(pLua, 4);
    float fov = (float)luaL_checknumber(pLua, 5);
    pGpu->Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
    return 0;
}

// ***********************************************************************

int luaTranslate(lua_State* pLua)
{
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);
    float z = (float)luaL_checknumber(pLua, 3);
    pGpu->Translate(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int luaRotate(lua_State* pLua)
{
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);
    float z = (float)luaL_checknumber(pLua, 3);
    pGpu->Rotate(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int luaScale(lua_State* pLua)
{
    float x = (float)luaL_checknumber(pLua, 1);
    float y = (float)luaL_checknumber(pLua, 2);
    float z = (float)luaL_checknumber(pLua, 3);
    pGpu->Scale(Vec3f(x, y, z));
    return 0;
}

// ***********************************************************************

int luaIdentity(lua_State* pLua)
{
    pGpu->Identity();
    return 0;
}

// ***********************************************************************

int luaNewImage(lua_State* pLua)
{
    size_t len;
	const char *str = luaL_checklstring(pLua, 1, &len);
    std::string path(str, len);

    // Create a new userdata for our object
    Image** ppImage = (Image**)lua_newuserdata(pLua, sizeof(Image*));
    *ppImage = new Image(path);

    // Sets the metatable of this new userdata to the type's table
    luaL_getmetatable(pLua, "Image");
    lua_setmetatable(pLua, -2);

    return 1;
}

// ***********************************************************************

int luaImageGetWidth(lua_State* pLua)
{
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    lua_pushinteger(pLua, pImage->m_width);
    return 1;
}

// ***********************************************************************

int luaImageGetHeight(lua_State* pLua)
{
    Image* pImage = *(Image**)luaL_checkudata(pLua, 1, "Image");
    lua_pushinteger(pLua, pImage->m_height);
    return 1;
}

// ***********************************************************************

namespace Bind
{
    int BindGraphicsChip(lua_State* pLua, GraphicsChip* _pGraphics)
    {
        pGpu = _pGraphics;

        // Global functions
        ///////////////////

        const luaL_Reg graphicsFuncs[] = {
            { "BeginObject2D", luaBeginObject2D },
            { "EndObject2D", luaEndObject2D },
            { "Vertex", luaVertex },
            { "BeginObject3D", luaBeginObject3D },
            { "Color", luaColor },
            { "TexCoord", luaTexCoord },
            { "Normal", luaNormal },
            { "SetClearColor", luaSetClearColor },
            { "MatrixMode", luaMatrixMode },
            { "Perspective", luaPerspective },
            { "Translate", luaTranslate },
            { "Rotate", luaRotate },
            { "Scale", luaScale },
            { "Identity", luaIdentity },
            { "NewImage", luaNewImage },
            { NULL, NULL }
        };

        lua_pushglobaltable(pLua);
        luaL_setfuncs(pLua, graphicsFuncs, 0);

        // Types
        ////////

        const luaL_Reg imageMethods[] = {
            { "GetWidth", luaImageGetWidth },
            { "GetHeight", luaImageGetHeight },
            { NULL, NULL }
        };
        Lua::RegisterNewType(pLua, "Image", imageMethods);
    
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