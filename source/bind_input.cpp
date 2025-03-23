// Copyright 2020-2022 David Colson. All rights reserved.

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
	luaL_checktype(pLua, -1, LUA_TBOOLEAN);
    bool enable = lua_toboolean(pLua, -1) != 0; 
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
        { "get_button", LuaGetButton },
        { "get_button_down", LuaGetButtonDown },
        { "get_button_up", LuaGetButtonUp },
        { "get_axis", LuaGetAxis },
        { "get_mouse_position", LuaGetMousePosition },
        { "enable_mouse_relative_mode", LuaEnableMouseRelativeMode },
        { "get_key", LuaGetKey },
        { "get_key_down", LuaGetKeyDown },
        { "get_key_up", LuaGetKeyUp },
        { "input_string", LuaInputString },
        { NULL, NULL }
    };

    lua_pushvalue(pLua, LUA_GLOBALSINDEX);
    luaL_register(pLua, NULL, gameGlobalFuncs);
    lua_pop(pLua, 1);

    // push enum tables to global
    lua_newtable(pLua);
    {
        PushEnum(pLua, (int)ControllerButton::Invalid, "invalid");
        PushEnum(pLua, (int)ControllerButton::FaceBottom, "face_bottom");
        PushEnum(pLua, (int)ControllerButton::FaceRight, "face_right");
        PushEnum(pLua, (int)ControllerButton::FaceLeft, "face_left");
        PushEnum(pLua, (int)ControllerButton::FaceTop, "face_top");
        PushEnum(pLua, (int)ControllerButton::LeftStick, "left_stick");
        PushEnum(pLua, (int)ControllerButton::RightStick, "right_stick");
        PushEnum(pLua, (int)ControllerButton::LeftShoulder, "left_shoulder");
        PushEnum(pLua, (int)ControllerButton::RightShoulder, "right_shoulder");
        PushEnum(pLua, (int)ControllerButton::DpadDown, "dpad_down");
        PushEnum(pLua, (int)ControllerButton::DpadLeft, "dpad_left");
        PushEnum(pLua, (int)ControllerButton::DpadRight, "dpad_right");
        PushEnum(pLua, (int)ControllerButton::DpadUp, "dpad_up");
        PushEnum(pLua, (int)ControllerButton::Start, "start");
        PushEnum(pLua, (int)ControllerButton::Select, "select");
    }
    lua_setglobal(pLua, "Button");

    lua_newtable(pLua);
    {
        PushEnum(pLua, (int)ControllerAxis::Invalid, "invalid");
        PushEnum(pLua, (int)ControllerAxis::LeftX, "left_x");
        PushEnum(pLua, (int)ControllerAxis::LeftY, "left_y");
        PushEnum(pLua, (int)ControllerAxis::RightX, "right_x");
        PushEnum(pLua, (int)ControllerAxis::RightY, "right_y");
        PushEnum(pLua, (int)ControllerAxis::TriggerLeft, "trigger_left");
        PushEnum(pLua, (int)ControllerAxis::TriggerRight, "trigger_right");
    }
    lua_setglobal(pLua, "Axis");

    lua_newtable(pLua);
    {
        PushEnum(pLua, (int)Key::Invalid, "invalid");
        PushEnum(pLua, (int)Key::A, "a");
        PushEnum(pLua, (int)Key::B, "b");
        PushEnum(pLua, (int)Key::C, "c");
        PushEnum(pLua, (int)Key::D, "d");
        PushEnum(pLua, (int)Key::E, "e");
        PushEnum(pLua, (int)Key::F, "f");
        PushEnum(pLua, (int)Key::G, "g");
        PushEnum(pLua, (int)Key::H, "h");
        PushEnum(pLua, (int)Key::I, "i");
        PushEnum(pLua, (int)Key::J, "j");
        PushEnum(pLua, (int)Key::K, "k");
        PushEnum(pLua, (int)Key::L, "l");
        PushEnum(pLua, (int)Key::M, "m");
        PushEnum(pLua, (int)Key::N, "n");
        PushEnum(pLua, (int)Key::O, "o");
        PushEnum(pLua, (int)Key::P, "p");
        PushEnum(pLua, (int)Key::Q, "q");
        PushEnum(pLua, (int)Key::R, "r");
        PushEnum(pLua, (int)Key::S, "s");
        PushEnum(pLua, (int)Key::T, "t");
        PushEnum(pLua, (int)Key::U, "u");
        PushEnum(pLua, (int)Key::V, "v");
        PushEnum(pLua, (int)Key::W, "w");
        PushEnum(pLua, (int)Key::X, "x");
        PushEnum(pLua, (int)Key::Y, "y");
        PushEnum(pLua, (int)Key::Z, "z");

        PushEnum(pLua, (int)Key::No1, "no1");
        PushEnum(pLua, (int)Key::No2, "no2");
        PushEnum(pLua, (int)Key::No3, "no3");
        PushEnum(pLua, (int)Key::No4, "no4");
        PushEnum(pLua, (int)Key::No5, "no5");
        PushEnum(pLua, (int)Key::No6, "no6");
        PushEnum(pLua, (int)Key::No7, "no7");
        PushEnum(pLua, (int)Key::No8, "no8");
        PushEnum(pLua, (int)Key::No9, "no9");
        PushEnum(pLua, (int)Key::No0, "no0");

        PushEnum(pLua, (int)Key::Return, "enter");
        PushEnum(pLua, (int)Key::Escape, "escape");
        PushEnum(pLua, (int)Key::Backspace, "backspace");
        PushEnum(pLua, (int)Key::Tab, "tab");
        PushEnum(pLua, (int)Key::Space, "space");
        PushEnum(pLua, (int)Key::Exclaim, "exclaim");
        PushEnum(pLua, (int)Key::QuoteDbl, "quote_dbl");
        PushEnum(pLua, (int)Key::Hash, "hash");
        PushEnum(pLua, (int)Key::Percent, "percent");
        PushEnum(pLua, (int)Key::Dollar, "dollar");
        PushEnum(pLua, (int)Key::Ampersand, "ampersand");
        PushEnum(pLua, (int)Key::Quote, "quote");
        PushEnum(pLua, (int)Key::LeftParen, "left_paren");
        PushEnum(pLua, (int)Key::RightParen, "right_paren");
        PushEnum(pLua, (int)Key::Asterisk, "asterisk");
        PushEnum(pLua, (int)Key::Plus, "plus");
        PushEnum(pLua, (int)Key::Comma, "comma");
        PushEnum(pLua, (int)Key::Minus, "minus");
        PushEnum(pLua, (int)Key::Period, "period");
        PushEnum(pLua, (int)Key::Slash, "slash");
        PushEnum(pLua, (int)Key::Colon, "colon");
        PushEnum(pLua, (int)Key::Semicolon, "semicolon");
        PushEnum(pLua, (int)Key::Less, "less");
        PushEnum(pLua, (int)Key::Equals, "equals");
        PushEnum(pLua, (int)Key::Greater, "greater");
        PushEnum(pLua, (int)Key::Question, "question");
        PushEnum(pLua, (int)Key::At, "at");
        PushEnum(pLua, (int)Key::LeftBracket, "left_bracket");
        PushEnum(pLua, (int)Key::Backslash, "backslash");
        PushEnum(pLua, (int)Key::RightBracket, "right_bracket");
        PushEnum(pLua, (int)Key::Caret, "caret");
        PushEnum(pLua, (int)Key::Underscore, "underscore");
        PushEnum(pLua, (int)Key::BackQuote, "back_quote");

        PushEnum(pLua, (int)Key::CapsLock, "caps_lock");

        PushEnum(pLua, (int)Key::F1, "f1");
        PushEnum(pLua, (int)Key::F2, "f2");
        PushEnum(pLua, (int)Key::F3, "f3");
        PushEnum(pLua, (int)Key::F4, "f4");
        PushEnum(pLua, (int)Key::F5, "f5");
        PushEnum(pLua, (int)Key::F6, "f6");
        PushEnum(pLua, (int)Key::F7, "f7");
        PushEnum(pLua, (int)Key::F8, "f8");
        PushEnum(pLua, (int)Key::F9, "f9");
        PushEnum(pLua, (int)Key::F10, "f10");
        PushEnum(pLua, (int)Key::F11, "f11");
        PushEnum(pLua, (int)Key::F12, "f12");

        PushEnum(pLua, (int)Key::PrintScreen, "print_screen");
        PushEnum(pLua, (int)Key::ScrollLock, "scroll_lock");
        PushEnum(pLua, (int)Key::Pause, "pause");
        PushEnum(pLua, (int)Key::Insert, "insert");
        PushEnum(pLua, (int)Key::Home, "home");
        PushEnum(pLua, (int)Key::PageUp, "pageup");
        PushEnum(pLua, (int)Key::Delete, "delete");
        PushEnum(pLua, (int)Key::End, "_end");
        PushEnum(pLua, (int)Key::PageDown, "pagedown");
        PushEnum(pLua, (int)Key::Right, "right");
        PushEnum(pLua, (int)Key::Left, "left");
        PushEnum(pLua, (int)Key::Down, "down");
        PushEnum(pLua, (int)Key::Up, "up");

        PushEnum(pLua, (int)Key::NumLock, "num_lock");
        PushEnum(pLua, (int)Key::KpDivide, "kpdivide");
        PushEnum(pLua, (int)Key::KpMultiply, "kpmultiply");
        PushEnum(pLua, (int)Key::KpMinus, "kpminus");
        PushEnum(pLua, (int)Key::KpPlus, "kpplus");
        PushEnum(pLua, (int)Key::KpEnter, "kpenter");
        PushEnum(pLua, (int)Key::Kp1, "kp1");
        PushEnum(pLua, (int)Key::Kp2, "kp2");
        PushEnum(pLua, (int)Key::Kp3, "kp3");
        PushEnum(pLua, (int)Key::Kp4, "kp4");
        PushEnum(pLua, (int)Key::Kp5, "kp5");
        PushEnum(pLua, (int)Key::Kp6, "kp6");
        PushEnum(pLua, (int)Key::Kp7, "kp7");
        PushEnum(pLua, (int)Key::Kp8, "kp8");
        PushEnum(pLua, (int)Key::Kp9, "kp9");
        PushEnum(pLua, (int)Key::Kp0, "kp0");
        PushEnum(pLua, (int)Key::KpPeriod, "kpperiod");

        PushEnum(pLua, (int)Key::LeftCtrl, "left_ctrl");
        PushEnum(pLua, (int)Key::LeftShift, "left_shift");
        PushEnum(pLua, (int)Key::LeftAlt, "left_alt");
        PushEnum(pLua, (int)Key::LeftGui, "left_gui");
        PushEnum(pLua, (int)Key::RightCtrl, "right_ctrl");
        PushEnum(pLua, (int)Key::RightShift, "right_shift");
        PushEnum(pLua, (int)Key::RightAlt, "right_alt");
        PushEnum(pLua, (int)Key::RightGui, "right_gui");
    }
    lua_setglobal(pLua, "Key");

    return 0;
}
}
