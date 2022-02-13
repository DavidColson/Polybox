// Copyright 2020-2022 David Colson. All rights reserved.

#include "Bind_Mesh.h"

#include "Mesh.h"


namespace Bind
{
    // ***********************************************************************

    int Primitive_GetNumVertices(lua_State* pLua)
    {
        Primitive* pPrimitive = *(Primitive**)luaL_checkudata(pLua, 1, "Primitive");
        lua_pushinteger(pLua, pPrimitive->GetNumVertices());
        return 1;
    }

    // ***********************************************************************

    int Primitive_GetVertexPosition(lua_State* pLua)
    {
        Primitive* pPrimitive = *(Primitive**)luaL_checkudata(pLua, 1, "Primitive");
        int index = (int)luaL_checkinteger(pLua, 2) - 1;

        Vec3f pos = pPrimitive->GetVertexPosition(index);
        lua_pushnumber(pLua, pos.x);
        lua_pushnumber(pLua, pos.y);
        lua_pushnumber(pLua, pos.z);
        return 3;
    }

    // ***********************************************************************

    int Primitive_GetVertexColor(lua_State* pLua)
    {
        Primitive* pPrimitive = *(Primitive**)luaL_checkudata(pLua, 1, "Primitive");
        int index = (int)luaL_checkinteger(pLua, 2) - 1;

        Vec4f col = pPrimitive->GetVertexColor(index);
        lua_pushnumber(pLua, col.x);
        lua_pushnumber(pLua, col.y);
        lua_pushnumber(pLua, col.z);
        lua_pushnumber(pLua, col.w);
        return 4;
    }

    // ***********************************************************************

    int Primitive_GetVertexTexCoord(lua_State* pLua)
    {
        Primitive* pPrimitive = *(Primitive**)luaL_checkudata(pLua, 1, "Primitive");
        int index = (int)luaL_checkinteger(pLua, 2) - 1;

        Vec2f tex = pPrimitive->GetVertexTexCoord(index);
        lua_pushnumber(pLua, tex.x);
        lua_pushnumber(pLua, tex.y);
        return 2;
    }

    // ***********************************************************************

    int Primitive_GetVertexNormal(lua_State* pLua)
    {
        Primitive* pPrimitive = *(Primitive**)luaL_checkudata(pLua, 1, "Primitive");
        int index = (int)luaL_checkinteger(pLua, 2) - 1;

        Vec3f norm = pPrimitive->GetVertexNormal(index);
        lua_pushnumber(pLua, norm.x);
        lua_pushnumber(pLua, norm.y);
        lua_pushnumber(pLua, norm.z);
        return 3;
    }

    // ***********************************************************************

    int Primitive_GetMaterialTextureId(lua_State* pLua)
    {
        Primitive* pPrimitive = *(Primitive**)luaL_checkudata(pLua, 1, "Primitive");
        lua_pushinteger(pLua, pPrimitive->GetMaterialTextureId() + 1);
        return 1;
    }

    // ***********************************************************************

    int Mesh_GetName(lua_State* pLua)
    {
        Mesh* pMesh = *(Mesh**)luaL_checkudata(pLua, 1, "Mesh");
        lua_pushlstring(pLua, pMesh->m_name.data(), pMesh->m_name.size());
        return 1;
    }

    // ***********************************************************************

    int Mesh_GetNumPrimitives(lua_State* pLua)
    {
        Mesh* pMesh = *(Mesh**)luaL_checkudata(pLua, 1, "Mesh");
        lua_pushinteger(pLua, pMesh->GetNumPrimitives());
        return 1;
    }

    // ***********************************************************************

    int Mesh_GetPrimitive(lua_State* pLua)
    {
        Mesh* pMesh = *(Mesh**)luaL_checkudata(pLua, 1, "Mesh");
        int index = (int)luaL_checkinteger(pLua, 2);
        
        Primitive** ppPrimitive = (Primitive**)lua_newuserdata(pLua, sizeof(Primitive*));
        *ppPrimitive = pMesh->GetPrimitive(index - 1);
        (*ppPrimitive)->Retain(); // The primitive data already existed, so this is now a new reference to it

        luaL_getmetatable(pLua, "Primitive");
        lua_setmetatable(pLua, -2);
        return 1;
    }

    // ***********************************************************************

    int LoadMeshes(lua_State* pLua)
    {
        size_t len;
        const char *str = luaL_checklstring(pLua, 1, &len);
        std::string path(str, len);

        std::vector<Mesh*> meshes = Mesh::LoadMeshes(path.c_str());

        // Create a table, and put all these mesh userdatas in there.
        lua_createtable(pLua, (int)meshes.size(), 0);

        for (int i = 0; i < meshes.size(); i++)
        {
            // Create a new userdata for our object
            Mesh** ppMesh = (Mesh**)lua_newuserdata(pLua, sizeof(Mesh*));
            *ppMesh = meshes[i];

            // Sets the metatable of this new userdata to the type's table
            luaL_getmetatable(pLua, "Mesh");
            lua_setmetatable(pLua, -2);

            lua_seti(pLua, -2, i+1);  // Sets the userdata to index i in the table, and removes the udata from the stack
        }

        // Returns out table of meshes we left on the stack
        return 1;
    }

    // ***********************************************************************

    int LoadTextures(lua_State* pLua)
    {
        size_t len;
        const char *str = luaL_checklstring(pLua, 1, &len);
        std::string path(str, len);

        std::vector<Image*> images = Mesh::LoadTextures(path.c_str());

        // Create a table, and put all these mesh userdatas in there.
        lua_createtable(pLua, (int)images.size(), 0);

        for (int i = 0; i < images.size(); i++)
        {
            // Create a new userdata for our object
            Image** ppImage = (Image**)lua_newuserdata(pLua, sizeof(Image*));
            *ppImage = images[i];

            // Sets the metatable of this new userdata to the type's table
            luaL_getmetatable(pLua, "Image");
            lua_setmetatable(pLua, -2);

            lua_seti(pLua, -2, i+1);  // Sets the userdata to index i in the table, and removes the udata from the stack
        }

        // Returns out table of meshes we left on the stack
        return 1;
    }

    // ***********************************************************************

    int BindMesh(lua_State* pLua)
    {
        // Bind static mesh functions
        const luaL_Reg meshGlobalFuncs[] = {
            { "LoadMeshes", LoadMeshes },
            { "LoadTextures", LoadTextures },
            { NULL, NULL }
        };

        lua_pushglobaltable(pLua);
        luaL_setfuncs(pLua, meshGlobalFuncs, 0);

        // Types
        ////////

        const luaL_Reg meshMethods[] = {
            { "GetName", Mesh_GetName },
            { "GetNumPrimitives", Mesh_GetNumPrimitives },
            { "GetPrimitive", Mesh_GetPrimitive },
            { NULL, NULL }
        };
        luax_registertype(pLua, "Mesh", meshMethods);


        const luaL_Reg primitiveMethods[] = {
            { "GetNumVertices", Primitive_GetNumVertices },
            { "GetVertexPosition", Primitive_GetVertexPosition },
            { "GetVertexColor", Primitive_GetVertexColor },
            { "GetVertexTexCoord", Primitive_GetVertexTexCoord },
            { "GetVertexNormal", Primitive_GetVertexNormal },
            { "GetMaterialTextureId", Primitive_GetMaterialTextureId },
            { NULL, NULL }
        };
        luax_registertype(pLua, "Primitive", primitiveMethods);

        return 0;
    }
}