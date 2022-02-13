#include "Bind_GameChip.h"

#include "GameChip.h"

namespace
{
    GameChip* pGame{ nullptr };
}

namespace Bind
{
    int GetButton(lua_State* pLua)
    {
        ControllerButton button = (ControllerButton)luaL_checkinteger(pLua, 1);
        lua_pushboolean(pLua, pGame->GetButton(button));
        return 1;
    }

    int GetButtonDown(lua_State* pLua)
    {
        ControllerButton button = (ControllerButton)luaL_checkinteger(pLua, 1);
        lua_pushboolean(pLua, pGame->GetButtonDown(button));
        return 1;
    }

    int GetButtonUp(lua_State* pLua)
    {
        ControllerButton button = (ControllerButton)luaL_checkinteger(pLua, 1);
        lua_pushboolean(pLua, pGame->GetButtonUp(button));
        return 1;
    }

    // ***********************************************************************

    int BindGameChip(lua_State* pLua, GameChip* pGameChip)
    {
        pGame = pGameChip;

        // Bind static mesh functions
        const luaL_Reg meshGlobalFuncs[] = {
            { "GetButton", GetButton },
            { "GetButtonDown", GetButtonDown },
            { "GetButtonUp", GetButtonUp },
            { NULL, NULL }
        };

        lua_pushglobaltable(pLua);
        luaL_setfuncs(pLua, meshGlobalFuncs, 0);

         // push enum tables to global
        lua_newtable(pLua);
        {
            lua_pushinteger(pLua, (int)ControllerButton::Invalid); lua_setfield(pLua, -2, "Invalid");
            lua_pushinteger(pLua, (int)ControllerButton::FaceBottom); lua_setfield(pLua, -2, "FaceBottom");
            lua_pushinteger(pLua, (int)ControllerButton::FaceRight); lua_setfield(pLua, -2, "FaceRight");
            lua_pushinteger(pLua, (int)ControllerButton::FaceLeft); lua_setfield(pLua, -2, "FaceLeft");
            lua_pushinteger(pLua, (int)ControllerButton::FaceTop); lua_setfield(pLua, -2, "FaceTop");
            lua_pushinteger(pLua, (int)ControllerButton::LeftStick); lua_setfield(pLua, -2, "LeftStick");
            lua_pushinteger(pLua, (int)ControllerButton::RightStick); lua_setfield(pLua, -2, "RightStick");
            lua_pushinteger(pLua, (int)ControllerButton::LeftShoulder); lua_setfield(pLua, -2, "LeftShoulder");
            lua_pushinteger(pLua, (int)ControllerButton::RightShoulder); lua_setfield(pLua, -2, "RightShoulder");
            lua_pushinteger(pLua, (int)ControllerButton::DpadDown); lua_setfield(pLua, -2, "DpadDown");
            lua_pushinteger(pLua, (int)ControllerButton::DpadLeft); lua_setfield(pLua, -2, "DpadLeft");
            lua_pushinteger(pLua, (int)ControllerButton::DpadRight); lua_setfield(pLua, -2, "DpadRight");
            lua_pushinteger(pLua, (int)ControllerButton::DpadUp); lua_setfield(pLua, -2, "DpadUp");
            lua_pushinteger(pLua, (int)ControllerButton::Start); lua_setfield(pLua, -2, "Start");
            lua_pushinteger(pLua, (int)ControllerButton::Select); lua_setfield(pLua, -2, "Select");
        }
        lua_setglobal(pLua, "Button");

        return 0;
    }
}