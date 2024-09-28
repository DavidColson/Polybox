// Copyright 2020-2022 David Colson. All rights reserved.

#include "bind_input.h"

#include "input.h"
#include "light_string.h"
#include "lua_common.h"

#include <lua.h>
#include <lualib.h>
#include <defer.h>


namespace Bind {
// ***********************************************************************

int LuaGetButton(lua_State* pLua) {
    ControllerButton button = (ControllerButton)luaL_checkinteger(pLua, 1);
    lua_pushboolean(pLua, GetButton(button));
    return 1;
}

// ***********************************************************************

int LuaGetButtonDown(lua_State* pLua) {
    ControllerButton button = (ControllerButton)luaL_checkinteger(pLua, 1);
    lua_pushboolean(pLua, GetButtonDown(button));
    return 1;
}

// ***********************************************************************

int LuaGetButtonUp(lua_State* pLua) {
    ControllerButton button = (ControllerButton)luaL_checkinteger(pLua, 1);
    lua_pushboolean(pLua, GetButtonUp(button));
    return 1;
}

// ***********************************************************************

int LuaGetAxis(lua_State* pLua) {
    ControllerAxis axis = (ControllerAxis)luaL_checkinteger(pLua, 1);
    lua_pushnumber(pLua, GetAxis(axis));
    return 1;
}

// ***********************************************************************

int LuaGetMousePosition(lua_State* pLua) {
    Vec2i mousePos = GetMousePosition();
    lua_pushinteger(pLua, mousePos.x);
    lua_pushinteger(pLua, mousePos.y);
    return 2;
}

// ***********************************************************************

int LuaEnableMouseRelativeMode(lua_State* pLua) {
    bool enable = luax_checkboolean(pLua, 1);
    EnableMouseRelativeMode(enable);
    return 0;
}

// ***********************************************************************

int LuaGetKey(lua_State* pLua) {
    Key key = (Key)luaL_checkinteger(pLua, 1);
    lua_pushboolean(pLua, GetKey(key));
    return 1;
}

// ***********************************************************************

int LuaGetKeyDown(lua_State* pLua) {
    Key key = (Key)luaL_checkinteger(pLua, 1);
    lua_pushboolean(pLua, GetKeyDown(key));
    return 1;
}

// ***********************************************************************

int LuaGetKeyUp(lua_State* pLua) {
    Key key = (Key)luaL_checkinteger(pLua, 1);
    lua_pushboolean(pLua, GetKeyUp(key));
    return 1;
}

// ***********************************************************************

int LuaInputString(lua_State* pLua) {
    String input = InputString();
    lua_pushstring(pLua, input.pData);
    return 1;
}

// ***********************************************************************

void PushEnum(lua_State* pLua, i32 value, const char* label) {
    lua_pushinteger(pLua, value);
    lua_setfield(pLua, -2, label);
}

// ***********************************************************************

int BindInput(lua_State* pLua) {

    // Bind static mesh functions
    const luaL_Reg gameGlobalFuncs[] = {
        { "GetButton", LuaGetButton },
        { "GetButtonDown", LuaGetButtonDown },
        { "GetButtonUp", LuaGetButtonUp },
        { "GetAxis", LuaGetAxis },
        { "GetMousePosition", LuaGetMousePosition },
        { "EnableMouseRelativeMode", LuaEnableMouseRelativeMode },
        { "GetKey", LuaGetKey },
        { "GetKeyDown", LuaGetKeyDown },
        { "GetKeyUp", LuaGetKeyUp },
        { "InputString", LuaInputString },
        { NULL, NULL }
    };

    lua_pushvalue(pLua, LUA_GLOBALSINDEX);
    luaL_register(pLua, NULL, gameGlobalFuncs);
    lua_pop(pLua, 1);

    // push enum tables to global
    lua_newtable(pLua);
    {
        PushEnum(pLua, (int)ControllerButton::Invalid, "Invalid");
        PushEnum(pLua, (int)ControllerButton::FaceBottom, "FaceBottom");
        PushEnum(pLua, (int)ControllerButton::FaceRight, "FaceRight");
        PushEnum(pLua, (int)ControllerButton::FaceLeft, "FaceLeft");
        PushEnum(pLua, (int)ControllerButton::FaceTop, "FaceTop");
        PushEnum(pLua, (int)ControllerButton::LeftStick, "LeftStick");
        PushEnum(pLua, (int)ControllerButton::RightStick, "RightStick");
        PushEnum(pLua, (int)ControllerButton::LeftShoulder, "LeftShoulder");
        PushEnum(pLua, (int)ControllerButton::RightShoulder, "RightShoulder");
        PushEnum(pLua, (int)ControllerButton::DpadDown, "DpadDown");
        PushEnum(pLua, (int)ControllerButton::DpadLeft, "DpadLeft");
        PushEnum(pLua, (int)ControllerButton::DpadRight, "DpadRight");
        PushEnum(pLua, (int)ControllerButton::DpadUp, "DpadUp");
        PushEnum(pLua, (int)ControllerButton::Start, "Start");
        PushEnum(pLua, (int)ControllerButton::Select, "Select");
    }
    lua_setglobal(pLua, "Button");

    lua_newtable(pLua);
    {
        PushEnum(pLua, (int)ControllerAxis::Invalid, "Invalid");
        PushEnum(pLua, (int)ControllerAxis::LeftX, "LeftX");
        PushEnum(pLua, (int)ControllerAxis::LeftY, "LeftY");
        PushEnum(pLua, (int)ControllerAxis::RightX, "RightX");
        PushEnum(pLua, (int)ControllerAxis::RightY, "RightY");
        PushEnum(pLua, (int)ControllerAxis::TriggerLeft, "TriggerLeft");
        PushEnum(pLua, (int)ControllerAxis::TriggerRight, "TriggerRight");
    }
    lua_setglobal(pLua, "Axis");

    lua_newtable(pLua);
    {
        PushEnum(pLua, (int)Key::Invalid, "Invalid");
        PushEnum(pLua, (int)Key::A, "A");
        PushEnum(pLua, (int)Key::B, "B");
        PushEnum(pLua, (int)Key::C, "C");
        PushEnum(pLua, (int)Key::D, "D");
        PushEnum(pLua, (int)Key::E, "E");
        PushEnum(pLua, (int)Key::F, "F");
        PushEnum(pLua, (int)Key::G, "G");
        PushEnum(pLua, (int)Key::H, "H");
        PushEnum(pLua, (int)Key::I, "I");
        PushEnum(pLua, (int)Key::J, "J");
        PushEnum(pLua, (int)Key::K, "K");
        PushEnum(pLua, (int)Key::L, "L");
        PushEnum(pLua, (int)Key::M, "M");
        PushEnum(pLua, (int)Key::N, "N");
        PushEnum(pLua, (int)Key::O, "O");
        PushEnum(pLua, (int)Key::P, "P");
        PushEnum(pLua, (int)Key::Q, "Q");
        PushEnum(pLua, (int)Key::R, "R");
        PushEnum(pLua, (int)Key::S, "S");
        PushEnum(pLua, (int)Key::T, "T");
        PushEnum(pLua, (int)Key::U, "U");
        PushEnum(pLua, (int)Key::V, "V");
        PushEnum(pLua, (int)Key::W, "W");
        PushEnum(pLua, (int)Key::X, "X");
        PushEnum(pLua, (int)Key::Y, "Y");
        PushEnum(pLua, (int)Key::Z, "Z");

        PushEnum(pLua, (int)Key::No1, "No1");
        PushEnum(pLua, (int)Key::No2, "No2");
        PushEnum(pLua, (int)Key::No3, "No3");
        PushEnum(pLua, (int)Key::No4, "No4");
        PushEnum(pLua, (int)Key::No5, "No5");
        PushEnum(pLua, (int)Key::No6, "No6");
        PushEnum(pLua, (int)Key::No7, "No7");
        PushEnum(pLua, (int)Key::No8, "No8");
        PushEnum(pLua, (int)Key::No9, "No9");
        PushEnum(pLua, (int)Key::No0, "No0");

        PushEnum(pLua, (int)Key::Return, "Return");
        PushEnum(pLua, (int)Key::Escape, "Escape");
        PushEnum(pLua, (int)Key::Backspace, "Backspace");
        PushEnum(pLua, (int)Key::Tab, "Tab");
        PushEnum(pLua, (int)Key::Space, "Space");
        PushEnum(pLua, (int)Key::Exclaim, "Exclaim");
        PushEnum(pLua, (int)Key::QuoteDbl, "QuoteDbl");
        PushEnum(pLua, (int)Key::Hash, "Hash");
        PushEnum(pLua, (int)Key::Percent, "Percent");
        PushEnum(pLua, (int)Key::Dollar, "Dollar");
        PushEnum(pLua, (int)Key::Ampersand, "Ampersand");
        PushEnum(pLua, (int)Key::Quote, "Quote");
        PushEnum(pLua, (int)Key::LeftParen, "LeftParen");
        PushEnum(pLua, (int)Key::RightParen, "RightParen");
        PushEnum(pLua, (int)Key::Asterisk, "Asterisk");
        PushEnum(pLua, (int)Key::Plus, "Plus");
        PushEnum(pLua, (int)Key::Comma, "Comma");
        PushEnum(pLua, (int)Key::Minus, "Minus");
        PushEnum(pLua, (int)Key::Period, "Period");
        PushEnum(pLua, (int)Key::Slash, "Slash");
        PushEnum(pLua, (int)Key::Colon, "Colon");
        PushEnum(pLua, (int)Key::Semicolon, "Semicolon");
        PushEnum(pLua, (int)Key::Less, "Less");
        PushEnum(pLua, (int)Key::Equals, "Equals");
        PushEnum(pLua, (int)Key::Greater, "Greater");
        PushEnum(pLua, (int)Key::Question, "Question");
        PushEnum(pLua, (int)Key::At, "At");
        PushEnum(pLua, (int)Key::LeftBracket, "LeftBracket");
        PushEnum(pLua, (int)Key::Backslash, "Backslash");
        PushEnum(pLua, (int)Key::RightBracket, "RightBracket");
        PushEnum(pLua, (int)Key::Caret, "Caret");
        PushEnum(pLua, (int)Key::Underscore, "Underscore");
        PushEnum(pLua, (int)Key::BackQuote, "BackQuote");

        PushEnum(pLua, (int)Key::CapsLock, "CapsLock");

        PushEnum(pLua, (int)Key::F1, "F1");
        PushEnum(pLua, (int)Key::F2, "F2");
        PushEnum(pLua, (int)Key::F3, "F3");
        PushEnum(pLua, (int)Key::F4, "F4");
        PushEnum(pLua, (int)Key::F5, "F5");
        PushEnum(pLua, (int)Key::F6, "F6");
        PushEnum(pLua, (int)Key::F7, "F7");
        PushEnum(pLua, (int)Key::F8, "F8");
        PushEnum(pLua, (int)Key::F9, "F9");
        PushEnum(pLua, (int)Key::F10, "F10");
        PushEnum(pLua, (int)Key::F11, "F11");
        PushEnum(pLua, (int)Key::F12, "F12");

        PushEnum(pLua, (int)Key::PrintScreen, "PrintScreen");
        PushEnum(pLua, (int)Key::ScrollLock, "ScrollLock");
        PushEnum(pLua, (int)Key::Pause, "Pause");
        PushEnum(pLua, (int)Key::Insert, "Insert");
        PushEnum(pLua, (int)Key::Home, "Home");
        PushEnum(pLua, (int)Key::PageUp, "PageUp");
        PushEnum(pLua, (int)Key::Delete, "Delete");
        PushEnum(pLua, (int)Key::End, "End");
        PushEnum(pLua, (int)Key::PageDown, "PageDown");
        PushEnum(pLua, (int)Key::Right, "Right");
        PushEnum(pLua, (int)Key::Left, "Left");
        PushEnum(pLua, (int)Key::Down, "Down");
        PushEnum(pLua, (int)Key::Up, "Up");

        PushEnum(pLua, (int)Key::NumLock, "NumLock");
        PushEnum(pLua, (int)Key::KpDivide, "KpDivide");
        PushEnum(pLua, (int)Key::KpMultiply, "KpMultiply");
        PushEnum(pLua, (int)Key::KpMinus, "KpMinus");
        PushEnum(pLua, (int)Key::KpPlus, "KpPlus");
        PushEnum(pLua, (int)Key::KpEnter, "KpEnter");
        PushEnum(pLua, (int)Key::Kp1, "Kp1");
        PushEnum(pLua, (int)Key::Kp2, "Kp2");
        PushEnum(pLua, (int)Key::Kp3, "Kp3");
        PushEnum(pLua, (int)Key::Kp4, "Kp4");
        PushEnum(pLua, (int)Key::Kp5, "Kp5");
        PushEnum(pLua, (int)Key::Kp6, "Kp6");
        PushEnum(pLua, (int)Key::Kp7, "Kp7");
        PushEnum(pLua, (int)Key::Kp8, "Kp8");
        PushEnum(pLua, (int)Key::Kp9, "Kp9");
        PushEnum(pLua, (int)Key::Kp0, "Kp0");
        PushEnum(pLua, (int)Key::KpPeriod, "KpPeriod");

        PushEnum(pLua, (int)Key::LeftCtrl, "LeftCtrl");
        PushEnum(pLua, (int)Key::LeftShift, "LeftShift");
        PushEnum(pLua, (int)Key::LeftAlt, "LeftAlt");
        PushEnum(pLua, (int)Key::LeftGui, "LeftGui");
        PushEnum(pLua, (int)Key::RightCtrl, "RightCtrl");
        PushEnum(pLua, (int)Key::RightShift, "RightShift");
        PushEnum(pLua, (int)Key::RightAlt, "RightAlt");
        PushEnum(pLua, (int)Key::RightGui, "RightGui");
    }
    lua_setglobal(pLua, "Key");

    return 0;
}
}
