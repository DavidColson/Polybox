// Copyright 2020-2022 David Colson. All rights reserved.

#include "GameChip.h"

#include "Core/Json.h"
#include "Core/StringHash.h"
#include "Core/Maths.h"

#include <SDL_scancode.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_events.h>
#include <SDL_joystick.h>

// TODO: remove when we have decent logging
#include <format>
#include <SDL.h>
#include <SDL_syswm.h>
#undef DrawText
#undef DrawTextEx

namespace {
    // Mapping of strings to virtual controller buttons
    std::map<uint32_t, ControllerButton> stringToControllerButton = {
        {"FaceBottom"_hash, ControllerButton::FaceBottom },
        {"FaceRight"_hash, ControllerButton::FaceRight },
        {"FaceLeft"_hash, ControllerButton::FaceLeft },
        {"FaceTop"_hash, ControllerButton::FaceTop },
        {"LeftStick"_hash, ControllerButton::LeftStick },
        {"RightStick"_hash, ControllerButton::RightStick },
        {"LeftShoulder"_hash, ControllerButton::LeftShoulder },
        {"RightShoulder"_hash, ControllerButton::RightShoulder },
        {"DpadDown"_hash, ControllerButton::DpadDown },
        {"DpadLeft"_hash, ControllerButton::DpadLeft },
        {"DpadRight"_hash, ControllerButton::DpadRight },
        {"DpadUp"_hash, ControllerButton::DpadUp },
        {"Start"_hash, ControllerButton::Start },
        {"Select"_hash, ControllerButton::Select },
    };
    // Mapping of strings to virtual controller axis
    std::map<uint32_t, ControllerAxis> stringToControllerAxis = {
        {"LeftX"_hash, ControllerAxis::LeftX },
        {"LeftY"_hash, ControllerAxis::LeftY },
        {"RightX"_hash, ControllerAxis::RightX },
        {"RightY"_hash, ControllerAxis::RightY },
        {"TriggerLeft"_hash, ControllerAxis::TriggerLeft },
        {"TriggerRight"_hash, ControllerAxis::TriggerRight },
    };
    // Mapping of strings to SDL scancodes
    std::map<uint32_t, SDL_Keycode> stringToKeyCode = {
        {"Keycode_A"_hash, SDLK_a },
        {"Keycode_B"_hash, SDLK_b },
        {"Keycode_C"_hash, SDLK_c },
        {"Keycode_D"_hash, SDLK_d },
        {"Keycode_E"_hash, SDLK_e },
        {"Keycode_F"_hash, SDLK_f },
        {"Keycode_G"_hash, SDLK_g },
        {"Keycode_H"_hash, SDLK_h },
        {"Keycode_I"_hash, SDLK_i },
        {"Keycode_J"_hash, SDLK_j },
        {"Keycode_K"_hash, SDLK_k },
        {"Keycode_L"_hash, SDLK_l },
        {"Keycode_M"_hash, SDLK_m },
        {"Keycode_N"_hash, SDLK_n },
        {"Keycode_O"_hash, SDLK_o },
        {"Keycode_P"_hash, SDLK_p },
        {"Keycode_Q"_hash, SDLK_q },
        {"Keycode_R"_hash, SDLK_e },
        {"Keycode_S"_hash, SDLK_s },
        {"Keycode_T"_hash, SDLK_t },
        {"Keycode_U"_hash, SDLK_u },
        {"Keycode_V"_hash, SDLK_v },
        {"Keycode_W"_hash, SDLK_w },
        {"Keycode_X"_hash, SDLK_x },
        {"Keycode_Y"_hash, SDLK_y },
        {"Keycode_Z"_hash, SDLK_z },

        {"Keycode_1"_hash, SDLK_1 },
        {"Keycode_2"_hash, SDLK_2 },
        {"Keycode_3"_hash, SDLK_3 },
        {"Keycode_4"_hash, SDLK_4 },
        {"Keycode_5"_hash, SDLK_5 },
        {"Keycode_6"_hash, SDLK_6 },
        {"Keycode_7"_hash, SDLK_7 },
        {"Keycode_8"_hash, SDLK_8 },
        {"Keycode_9"_hash, SDLK_9 },
        {"Keycode_0"_hash, SDLK_0 },

        {"Keycode_Return"_hash, SDLK_RETURN },
        {"Keycode_Escape"_hash, SDLK_ESCAPE },
        {"Keycode_Backspace"_hash, SDLK_BACKSPACE },
        {"Keycode_Tab"_hash, SDLK_TAB },
        {"Keycode_Space"_hash, SDLK_SPACE },
        {"Keycode_Exclaim"_hash, SDLK_EXCLAIM },
        {"Keycode_QuoteDbl"_hash, SDLK_QUOTEDBL },
        {"Keycode_Hash"_hash, SDLK_HASH },
        {"Keycode_Percent"_hash, SDLK_PERCENT },
        {"Keycode_Dollar"_hash, SDLK_DOLLAR },
        {"Keycode_Ampersand"_hash, SDLK_AMPERSAND },
        {"Keycode_Quote"_hash, SDLK_QUOTE },
        {"Keycode_LeftParen"_hash, SDLK_LEFTPAREN },
        {"Keycode_RightParen"_hash, SDLK_RIGHTPAREN },
        {"Keycode_Asterisk"_hash, SDLK_ASTERISK },
        {"Keycode_Plus"_hash, SDLK_PLUS },
        {"Keycode_Comma"_hash, SDLK_COMMA },
        {"Keycode_Minus"_hash, SDLK_MINUS },
        {"Keycode_Period"_hash, SDLK_PERIOD },
        {"Keycode_Slash"_hash, SDLK_SLASH },
        {"Keycode_Colon"_hash, SDLK_COLON },
        {"Keycode_Semicolon"_hash, SDLK_SEMICOLON },
        {"Keycode_Less"_hash, SDLK_LESS },
        {"Keycode_Equals"_hash, SDLK_EQUALS },
        {"Keycode_Greater"_hash, SDLK_GREATER },
        {"Keycode_Question"_hash, SDLK_QUESTION },
        {"Keycode_At"_hash, SDLK_AT },
        {"Keycode_LeftBracket"_hash, SDLK_LEFTBRACKET },
        {"Keycode_Backslash"_hash, SDLK_BACKSLASH },
        {"Keycode_RightBracket"_hash, SDLK_RIGHTBRACKET },
        {"Keycode_Caret"_hash, SDLK_CARET },
        {"Keycode_Underscore"_hash, SDLK_UNDERSCORE },
        {"Keycode_BackQuote"_hash, SDLK_BACKQUOTE },

        {"Keycode_CapsLock"_hash, SDLK_CAPSLOCK},

        {"Keycode_F1"_hash, SDLK_F1},
        {"Keycode_F2"_hash, SDLK_F2},
        {"Keycode_F3"_hash, SDLK_F3},
        {"Keycode_F4"_hash, SDLK_F4},
        {"Keycode_F5"_hash, SDLK_F5},
        {"Keycode_F6"_hash, SDLK_F6},
        {"Keycode_F7"_hash, SDLK_F7},
        {"Keycode_F8"_hash, SDLK_F8},
        {"Keycode_F9"_hash, SDLK_F9},
        {"Keycode_F10"_hash, SDLK_F10},
        {"Keycode_F11"_hash, SDLK_F11},
        {"Keycode_F12"_hash, SDLK_F12},

        {"Keycode_PrintScreen"_hash, SDLK_PRINTSCREEN},
        {"Keycode_ScrollLock"_hash, SDLK_SCROLLLOCK},
        {"Keycode_Pause"_hash, SDLK_PAUSE},
        {"Keycode_Insert"_hash, SDLK_INSERT},
        {"Keycode_Home"_hash, SDLK_HOME},
        {"Keycode_PageUp"_hash, SDLK_PAGEUP},
        {"Keycode_Delete"_hash, SDLK_DELETE},
        {"Keycode_End"_hash, SDLK_END},
        {"Keycode_PageDown"_hash, SDLK_PAGEDOWN},
        {"Keycode_Right"_hash, SDLK_RIGHT},
        {"Keycode_Left"_hash, SDLK_LEFT},
        {"Keycode_Down"_hash, SDLK_DOWN},
        {"Keycode_Up"_hash, SDLK_UP},

        {"Keycode_NumLock"_hash, SDLK_NUMLOCKCLEAR},
        {"Keycode_KpDivide"_hash, SDLK_KP_DIVIDE},
        {"Keycode_KpMultiply"_hash, SDLK_KP_MULTIPLY},
        {"Keycode_KpMinus"_hash, SDLK_KP_MINUS},
        {"Keycode_KpPlus"_hash, SDLK_KP_PLUS},
        {"Keycode_KpEnter"_hash, SDLK_KP_ENTER},
        {"Keycode_Kp1"_hash, SDLK_KP_1},
        {"Keycode_Kp2"_hash, SDLK_KP_2},
        {"Keycode_Kp3"_hash, SDLK_KP_3},
        {"Keycode_Kp4"_hash, SDLK_KP_4},
        {"Keycode_Kp5"_hash, SDLK_KP_5},
        {"Keycode_Kp6"_hash, SDLK_KP_6},
        {"Keycode_Kp7"_hash, SDLK_KP_7},
        {"Keycode_Kp8"_hash, SDLK_KP_8},
        {"Keycode_Kp9"_hash, SDLK_KP_9},
        {"Keycode_Kp0"_hash, SDLK_KP_0},
        {"Keycode_KpPeriod"_hash, SDLK_KP_PERIOD},

        {"Keycode_LeftCtrl"_hash, SDLK_LCTRL},
        {"Keycode_LeftShift"_hash, SDLK_LSHIFT},
        {"Keycode_LeftAlt"_hash, SDLK_LALT},
        {"Keycode_LeftGui"_hash, SDLK_LGUI},
        {"Keycode_RightCtrl"_hash, SDLK_RCTRL},
        {"Keycode_RightShift"_hash, SDLK_RSHIFT},
        {"Keycode_RightAlt"_hash, SDLK_RALT},
        {"Keycode_RightGui"_hash, SDLK_RGUI}
    };

    // Mapping of strings to SDL scancodes
    std::map<SDL_Keycode, Key> keyCodeToInternalKeyCode = {
        { SDLK_a, Key::A },
        { SDLK_b, Key::B },
        { SDLK_c, Key::C },
        { SDLK_d, Key::D },
        { SDLK_e, Key::E },
        { SDLK_f, Key::F },
        { SDLK_g, Key::G },
        { SDLK_h, Key::H },
        { SDLK_i, Key::I },
        { SDLK_j, Key::J },
        { SDLK_k, Key::K },
        { SDLK_l, Key::L },
        { SDLK_m, Key::M },
        { SDLK_n, Key::N },
        { SDLK_o, Key::O },
        { SDLK_p, Key::P },
        { SDLK_q, Key::Q },
        { SDLK_e, Key::R },
        { SDLK_s, Key::S },
        { SDLK_t, Key::T },
        { SDLK_u, Key::U },
        { SDLK_v, Key::V },
        { SDLK_w, Key::W },
        { SDLK_x, Key::X },
        { SDLK_y, Key::Y },
        { SDLK_z, Key::Z },

        { SDLK_1, Key::No1 },
        { SDLK_2, Key::No2 },
        { SDLK_3, Key::No3 },
        { SDLK_4, Key::No4 },
        { SDLK_5, Key::No5 },
        { SDLK_6, Key::No6 },
        { SDLK_7, Key::No7 },
        { SDLK_8, Key::No8 },
        { SDLK_9, Key::No9 },
        { SDLK_0, Key::No0 },

        { SDLK_RETURN, Key::Return },
        { SDLK_ESCAPE, Key::Escape },
        { SDLK_BACKSPACE, Key::Backspace },
        { SDLK_TAB, Key::Tab },
        { SDLK_SPACE, Key::Space },
        { SDLK_EXCLAIM, Key::Exclaim },
        { SDLK_QUOTEDBL, Key::QuoteDbl },
        { SDLK_HASH, Key::Hash },
        { SDLK_PERCENT, Key::Percent },
        { SDLK_DOLLAR, Key::Dollar },
        { SDLK_AMPERSAND, Key::Ampersand },
        { SDLK_QUOTE, Key::Quote },
        { SDLK_LEFTPAREN, Key::LeftParen },
        { SDLK_RIGHTPAREN, Key::RightParen },
        { SDLK_ASTERISK, Key::Asterisk },
        { SDLK_PLUS, Key::Plus },
        { SDLK_COMMA, Key::Comma },
        { SDLK_MINUS, Key::Minus },
        { SDLK_PERIOD, Key::Period },
        { SDLK_SLASH, Key::Slash },
        { SDLK_COLON, Key::Colon },
        { SDLK_SEMICOLON, Key::Semicolon },
        { SDLK_LESS, Key::Less },
        { SDLK_EQUALS, Key::Equals },
        { SDLK_GREATER, Key::Greater },
        { SDLK_QUESTION, Key::Question },
        { SDLK_AT, Key::At },
        { SDLK_LEFTBRACKET, Key::LeftBracket },
        { SDLK_BACKSLASH, Key::Backslash },
        { SDLK_RIGHTBRACKET, Key::RightBracket },
        { SDLK_CARET, Key::Caret },
        { SDLK_UNDERSCORE, Key::Underscore },
        { SDLK_BACKQUOTE, Key::BackQuote },

        { SDLK_CAPSLOCK, Key::CapsLock },
        
        { SDLK_F1, Key::F1 },
        { SDLK_F2, Key::F2 },
        { SDLK_F3, Key::F3 },
        { SDLK_F4, Key::F4 },
        { SDLK_F5, Key::F5 },
        { SDLK_F6, Key::F6 },
        { SDLK_F7, Key::F7 },
        { SDLK_F8, Key::F8 },
        { SDLK_F9, Key::F9 },
        { SDLK_F10, Key::F10 },
        { SDLK_F11, Key::F11 },
        { SDLK_F12, Key::F12 },

        { SDLK_PRINTSCREEN, Key::PrintScreen },
        { SDLK_SCROLLLOCK, Key::ScrollLock },
        { SDLK_PAUSE, Key::Pause },
        { SDLK_INSERT, Key::Insert },
        { SDLK_HOME, Key::Home },
        { SDLK_PAGEUP, Key::PageUp },
        { SDLK_DELETE, Key::Delete },
        { SDLK_END, Key::End },
        { SDLK_PAGEDOWN, Key::PageDown },
        { SDLK_RIGHT, Key::Right },
        { SDLK_LEFT, Key::Left},
        { SDLK_DOWN, Key::Down},
        { SDLK_UP, Key::Up },

        { SDLK_NUMLOCKCLEAR, Key::NumLock },
        { SDLK_KP_DIVIDE, Key::KpDivide },
        { SDLK_KP_MULTIPLY, Key::KpMultiply },
        { SDLK_KP_MINUS, Key::KpMinus },
        { SDLK_KP_PLUS, Key::KpPlus },
        { SDLK_KP_ENTER, Key::KpEnter },
        { SDLK_KP_1, Key::Kp1 },
        { SDLK_KP_2, Key::Kp2 },
        { SDLK_KP_3, Key::Kp3 },
        { SDLK_KP_4, Key::Kp4 },
        { SDLK_KP_5, Key::Kp5 },
        { SDLK_KP_6, Key::Kp6 },
        { SDLK_KP_7, Key::Kp7 },
        { SDLK_KP_8, Key::Kp8 },
        { SDLK_KP_9, Key::Kp9 },
        { SDLK_KP_0, Key::Kp0 },
        { SDLK_KP_PERIOD, Key::KpPeriod },

        { SDLK_LCTRL, Key::LeftCtrl },
        { SDLK_LSHIFT, Key::LeftShift },
        { SDLK_LALT, Key::LeftAlt },
        { SDLK_LGUI, Key::LeftGui },
        { SDLK_RCTRL, Key::RightCtrl },
        { SDLK_RSHIFT, Key::RightShift },
        { SDLK_RALT, Key::RightAlt },
        { SDLK_RGUI, Key::RightGui }
    };

    // Mapping of strings to SDL mouse codes
    std::map<uint32_t, int> stringToMouseCode = {
        { "Mouse_Button0"_hash, SDL_BUTTON_LEFT },
        { "Mouse_Button1"_hash, SDL_BUTTON_MIDDLE },
        { "Mouse_Button2"_hash, SDL_BUTTON_RIGHT },
        { "Mouse_AxisY"_hash, 128 },
        { "Mouse_AxisX"_hash, 127 },
    };

    // Mapping of strings to SDL controller buttons
    std::map<uint32_t, SDL_GameControllerButton> stringToSDLControllerButton = {
        { "Controller_A"_hash, SDL_CONTROLLER_BUTTON_A },
        { "Controller_B"_hash, SDL_CONTROLLER_BUTTON_B },
        { "Controller_X"_hash, SDL_CONTROLLER_BUTTON_X },
        { "Controller_Y"_hash, SDL_CONTROLLER_BUTTON_Y },
        { "Controller_LeftStick"_hash, SDL_CONTROLLER_BUTTON_LEFTSTICK },
        { "Controller_RightStick"_hash, SDL_CONTROLLER_BUTTON_RIGHTSTICK },
        { "Controller_LeftShoulder"_hash, SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
        { "Controller_RightShoulder"_hash, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
        { "Controller_DpadUp"_hash, SDL_CONTROLLER_BUTTON_DPAD_UP },
        { "Controller_DpadDown"_hash, SDL_CONTROLLER_BUTTON_DPAD_DOWN },
        { "Controller_DpadLeft"_hash, SDL_CONTROLLER_BUTTON_DPAD_LEFT },
        { "Controller_DpadRight"_hash, SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
        { "Controller_Start"_hash, SDL_CONTROLLER_BUTTON_START },
        { "Controller_Select"_hash, SDL_CONTROLLER_BUTTON_BACK },
    };

    // Mapping of strings to SDL controller axes
    std::map<uint32_t, SDL_GameControllerAxis> stringToSDLControllerAxis = {
        { "Controller_LeftX"_hash, SDL_CONTROLLER_AXIS_LEFTX },
        { "Controller_LeftY"_hash, SDL_CONTROLLER_AXIS_LEFTY },
        { "Controller_RightX"_hash, SDL_CONTROLLER_AXIS_RIGHTX },
        { "Controller_RightY"_hash, SDL_CONTROLLER_AXIS_RIGHTY },
        { "Controller_TriggerLeft"_hash, SDL_CONTROLLER_AXIS_TRIGGERLEFT },
        { "Controller_TriggerRight"_hash, SDL_CONTROLLER_AXIS_TRIGGERRIGHT },
    };
}

// TODO: Bring over our string hashing function
// All the key codes need to be put into maps where the stringhash is the key

// ***********************************************************************

void GameChip::Init()
{
    // Load json mapping file
    SDL_RWops* pFileRead = SDL_RWFromFile("Assets/ControllerMapping.json", "rb");
    uint64_t size = SDL_RWsize(pFileRead);
    char* pData = new char[size];
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);
    std::string file(pData, pData + size);
    delete[] pData;

    JsonValue mapping = ParseJsonFile(file);

    // Going through all elements of the json file
    if (mapping.HasKey("Buttons"))
    {
        JsonValue& buttons = mapping["Buttons"];
        for (size_t i = 0; i < buttons.Count(); i++)
        {
            JsonValue& jsonButton = buttons[i];
            ControllerButton button = stringToControllerButton[Fnv1a::Hash(jsonButton["Name"].ToString().c_str())];
            SDL_GameControllerButton primaryBinding = stringToSDLControllerButton[Fnv1a::Hash(jsonButton["Primary"].ToString().c_str())];
            m_primaryBindings[primaryBinding] = button;

            std::string altBindingLabel = jsonButton["Alt"].ToString();
            if (altBindingLabel.substr(0, 5) == "Keyco")
            {
                SDL_Keycode keycode = stringToKeyCode[Fnv1a::Hash(altBindingLabel.c_str())];
                m_keyboardAltBindings[keycode] = button;
            }
            else if (altBindingLabel.substr(0, 5) == "Mouse")
            {
                int mousecode = stringToMouseCode[Fnv1a::Hash(altBindingLabel.c_str())];
                m_mouseAltBindings[mousecode] = button;
            }
        }
    }
    if (mapping.HasKey("Axes"))
    {
        JsonValue& axes = mapping["Axes"];
        for (size_t i = 0; i < axes.Count(); i++)
        {
            JsonValue& jsonAxis = axes[i];
            ControllerAxis axis = stringToControllerAxis[Fnv1a::Hash(jsonAxis["Name"].ToString().c_str())];

            // TODO: What if someone tries to bind a controller button(s) to an axis? Support that probably
            SDL_GameControllerAxis primaryBinding = stringToSDLControllerAxis[Fnv1a::Hash(jsonAxis["Primary"].ToString().c_str())];
            m_primaryAxisBindings[primaryBinding] = axis;

            if (jsonAxis.HasKey("Alt"))
            {
                std::string altBindingLabel = jsonAxis["Alt"].ToString();
                if (altBindingLabel.substr(0, 5) == "Scanc")
                {
                    SDL_Keycode keycode = stringToKeyCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_keyboardAxisBindings[keycode] = axis;
                    m_axes[(size_t)axis].m_positiveScanCode = keycode;
                }
                else if (altBindingLabel.substr(0, 5) == "Mouse")
                {
                    int mousecode = stringToMouseCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_mouseAxisBindings[mousecode] = axis;
                    m_axes[(size_t)axis].m_positiveMouseButton = mousecode;
                }
            }
            if (jsonAxis.HasKey("AltPositive"))
            {
                std::string altBindingLabel = jsonAxis["AltPositive"].ToString();
                if (altBindingLabel.substr(0, 5) == "Scanc")
                {
                    SDL_Keycode keycode = stringToKeyCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_keyboardAxisBindings[keycode] = axis;
                    m_axes[(size_t)axis].m_positiveScanCode = keycode;
                }
                else if (altBindingLabel.substr(0, 5) == "Mouse")
                {
                    int mousecode = stringToMouseCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_mouseAxisBindings[mousecode] = axis;
                    m_axes[(size_t)axis].m_positiveMouseButton = mousecode;
                }
            }
            if (jsonAxis.HasKey("AltNegative"))
            {
                std::string altBindingLabel = jsonAxis["AltNegative"].ToString();
                if (altBindingLabel.substr(0, 5) == "Scanc")
                {
                    SDL_Keycode keycode = stringToKeyCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_keyboardAxisBindings[keycode] = axis;
                    m_axes[(size_t)axis].m_negativeScanCode = keycode;
                }
                else if (altBindingLabel.substr(0, 5) == "Mouse")
                {
                    int mousecode = stringToMouseCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_mouseAxisBindings[mousecode] = axis;
                    m_axes[(size_t)axis].m_negativeMouseButton = mousecode;
                }
            }
        }
    }

    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    SDL_StartTextInput();

    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        if (SDL_IsGameController(i))
        {
            const char *name = SDL_GameControllerNameForIndex(i);
            std::string format = std::format("Using first detected controller: {}\n", name);
			OutputDebugStringA(format.c_str());

            m_pOpenController = SDL_GameControllerOpen(i);
        }
        else
        {
            const char *name = SDL_JoystickNameForIndex(i);
            std::string format = std::format("Detected Joystick: {}\n", name);
			OutputDebugStringA(format.c_str());
        }
    }
}

// ***********************************************************************

void GameChip::ProcessEvent(SDL_Event* event)
{
    // Update Input States
    switch (event->type)
	{
    case SDL_TEXTINPUT:
    {
        m_textInputString += event->text.text;
        break;
    }
    case SDL_KEYDOWN:
    {
        SDL_Keycode keycode = event->key.keysym.sym;
        m_keyDowns[(size_t)keyCodeToInternalKeyCode[keycode]] = true;
        m_keyStates[(size_t)keyCodeToInternalKeyCode[keycode]] = true;

        ControllerButton button = m_keyboardAltBindings[keycode];
        if (button != ControllerButton::Invalid)
        {
            m_buttonDowns[(size_t)button] = true;
            m_buttonStates[(size_t)button] = true;
        }
        ControllerAxis axis = m_keyboardAxisBindings[keycode];
        if (axis != ControllerAxis::Invalid)
        {
            Axis& axisData = m_axes[(size_t)axis];
            if (axisData.m_positiveScanCode == keycode)
                axisData.m_positiveInput = true;
            else if (axisData.m_negativeScanCode == keycode)
                axisData.m_negativeInput = true;
            axisData.m_ignoreVirtual = false;
        }
        break;
    }
    case SDL_KEYUP:
    {
        SDL_Keycode keycode = event->key.keysym.sym;
        m_keyUps[(size_t)keyCodeToInternalKeyCode[keycode]] = true;
        m_keyStates[(size_t)keyCodeToInternalKeyCode[keycode]] = false;

        ControllerButton button = m_keyboardAltBindings[keycode];
        if (button != ControllerButton::Invalid)
        {
            m_buttonUps[(size_t)button] = true;
            m_buttonStates[(size_t)button] = false;
        }
        ControllerAxis axis = m_keyboardAxisBindings[keycode];
        if (axis != ControllerAxis::Invalid)
        {
            Axis& axisData = m_axes[(size_t)axis];
            if (axisData.m_positiveScanCode == keycode)
                axisData.m_positiveInput = false;
            else if (axisData.m_negativeScanCode == keycode)
                axisData.m_negativeInput = false;
            axisData.m_ignoreVirtual = false;
        }
        break;
    }
    case SDL_CONTROLLERBUTTONDOWN:
    {
        SDL_GameControllerButton sdlButton = (SDL_GameControllerButton)event->cbutton.button;
        ControllerButton button = m_primaryBindings[sdlButton];
        if (button != ControllerButton::Invalid)
        {
            m_buttonDowns[(size_t)button] = true;
            m_buttonStates[(size_t)button] = true;
        }
        break;
    }
    case SDL_CONTROLLERBUTTONUP:
    {
        SDL_GameControllerButton sdlButton = (SDL_GameControllerButton)event->cbutton.button;
        ControllerButton button = m_primaryBindings[sdlButton];
        if (button != ControllerButton::Invalid)
        {
            m_buttonUps[(size_t)button] = true;
            m_buttonStates[(size_t)button] = false;
        }
        break;
    }
    case SDL_MOUSEBUTTONDOWN:
    {
        int sdlMouseButton = event->button.button;
        ControllerButton button = m_mouseAltBindings[sdlMouseButton];
        if (button != ControllerButton::Invalid)
        {
            m_buttonDowns[(size_t)button] = true;
            m_buttonStates[(size_t)button] = true;
        }
        ControllerAxis axis = m_mouseAxisBindings[sdlMouseButton];
        if (axis != ControllerAxis::Invalid)
        {
            Axis& axisData = m_axes[(size_t)axis];
            if (axisData.m_positiveMouseButton == sdlMouseButton)
                axisData.m_positiveInput = true;
            else if (axisData.m_negativeMouseButton == sdlMouseButton)
                axisData.m_negativeInput = true;
            axisData.m_ignoreVirtual = false;
        }
        break;
    }
    case SDL_MOUSEBUTTONUP:
    {
        int sdlMouseButton = event->button.button;
        ControllerButton button = m_mouseAltBindings[sdlMouseButton];
        if (button != ControllerButton::Invalid)
        {
            m_buttonUps[(size_t)button] = true;
            m_buttonStates[(size_t)button] = false;
        }
        ControllerAxis axis = m_mouseAxisBindings[sdlMouseButton];
        if (axis != ControllerAxis::Invalid)
        {
            Axis& axisData = m_axes[(size_t)axis];
            if (axisData.m_positiveMouseButton == sdlMouseButton)
                axisData.m_positiveInput = false;
            else if (axisData.m_negativeMouseButton == sdlMouseButton)
                axisData.m_negativeInput = false;
            axisData.m_ignoreVirtual = false;
        }
        break;
    }
    case SDL_MOUSEMOTION:
    {
        float mouseSensitivity = 0.08f; // TODO: Make configurable
        SDL_MouseMotionEvent motionEvent = event->motion;
        ControllerAxis axis;
        if (abs(motionEvent.xrel) > 0)
        {
            axis = m_mouseAxisBindings[stringToMouseCode["Mouse_AxisX"_hash]];
            m_axes[(size_t)axis].m_axisValue = (float)motionEvent.xrel * mouseSensitivity;
            m_axes[(size_t)axis].m_ignoreVirtual = true;
            m_axes[(size_t)axis].m_isMouseDriver = true;
        }
        if (abs(motionEvent.yrel) > 0)
        {
            axis = m_mouseAxisBindings[stringToMouseCode["Mouse_AxisY"_hash]];
            m_axes[(size_t)axis].m_axisValue = (float)motionEvent.yrel * mouseSensitivity;
            m_axes[(size_t)axis].m_ignoreVirtual = true;
            m_axes[(size_t)axis].m_isMouseDriver = true;
        }
        break;
    }
    case SDL_CONTROLLERAXISMOTION:
    {
        SDL_ControllerAxisEvent axisEvent = event->caxis;
        ControllerAxis axis = m_primaryAxisBindings[(SDL_GameControllerAxis)axisEvent.axis];
        m_axes[(size_t)axis].m_axisValue = (float)axisEvent.value / 32768.f; 
        m_axes[(size_t)axis].m_ignoreVirtual = true;
        m_axes[(size_t)axis].m_isMouseDriver = false;
        break;
    }
    default: break;
    }
}

// ***********************************************************************

void GameChip::UpdateInputs(float deltaTime, Vec2f targetRes, Vec2f realWindowRes)
{
    m_targetResolution = targetRes;
    m_windowResolution = realWindowRes;

    // TODO: This should be configurable
    float gravity = 1.0f;
    float sensitivity = 1.0f;
    float deadzone = 0.09f;

    for (size_t axisIndex = 0; axisIndex < (size_t)ControllerAxis::Count; axisIndex++)
    {
        ControllerAxis axisEnum = (ControllerAxis)axisIndex;
        Axis& axis = m_axes[axisIndex];

        if (axis.m_isMouseDriver)
            continue;

        if (axis.m_ignoreVirtual)
        { 
            if (abs(axis.m_axisValue) <= deadzone)
                axis.m_axisValue = 0.0f;
            continue;
        }

        if (axis.m_positiveInput)
        {
            axis.m_axisValue += sensitivity * deltaTime;
        }
        if (axis.m_negativeInput)
        {
            axis.m_axisValue -= sensitivity * deltaTime;
        }
        if (!axis.m_negativeInput && !axis.m_positiveInput)
        {
            axis.m_axisValue += (0 - axis.m_axisValue) * gravity * deltaTime;
            if (abs(axis.m_axisValue) <= deadzone)
                axis.m_axisValue = 0.0f;
        }

        if (axisEnum == ControllerAxis::TriggerLeft || axisEnum == ControllerAxis::TriggerRight) // Triggers are special
            axis.m_axisValue = clamp(axis.m_axisValue, 0.f, 1.f);
        else
            axis.m_axisValue = clamp(axis.m_axisValue, -1.f, 1.f);
    }
}

// ***********************************************************************

void GameChip::ClearStates()
{
    m_keyDowns.reset();
	m_keyUps.reset();
    m_buttonDowns.reset();
	m_buttonUps.reset();
    m_textInputString.clear();
    for (Axis& axis : m_axes)
    {
        if (axis.m_isMouseDriver)
        {
            axis.m_axisValue = 0.0f;
        }
    }
    
}

// ***********************************************************************

void GameChip::Shutdown()
{
    if (m_pOpenController)
    {
        SDL_GameControllerClose(m_pOpenController);
    }
}

// ***********************************************************************

bool GameChip::GetButton(ControllerButton buttonCode)
{
    return m_buttonStates[(size_t)buttonCode];
}

// ***********************************************************************

bool GameChip::GetButtonDown(ControllerButton buttonCode)
{
    return m_buttonDowns[(size_t)buttonCode];
}

// ***********************************************************************

bool GameChip::GetButtonUp(ControllerButton buttonCode)
{
    return m_buttonUps[(size_t)buttonCode];
}

// ***********************************************************************

float GameChip::GetAxis(ControllerAxis axis)
{
    return m_axes[(size_t)axis].m_axisValue;
}

// ***********************************************************************

Vec2i GameChip::GetMousePosition()
{
    // TODO: This becomes wrong at the edge of the screen, must apply screen warning to it as well
    Vec2i mousePosition;
    SDL_GetMouseState(&mousePosition.x, &mousePosition.y);
    float xAdjusted = (float)mousePosition.x / m_windowResolution.x * m_targetResolution.x;
    float yAdjusted = m_targetResolution.y - ((float)mousePosition.y / m_windowResolution.y * m_targetResolution.y);
    return Vec2i((int)xAdjusted, (int)yAdjusted);
}

// ***********************************************************************

void GameChip::EnableMouseRelativeMode(bool enable)
{
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
}

// ***********************************************************************

bool GameChip::GetKey(Key keyCode)
{
    return m_keyStates[(size_t)keyCode];
}

// ***********************************************************************

bool GameChip::GetKeyDown(Key keyCode)
{
    return m_keyDowns[(size_t)keyCode];
}

// ***********************************************************************

bool GameChip::GetKeyUp(Key keyCode)
{
    return m_keyUps[(size_t)keyCode];
}

// ***********************************************************************

std::string GameChip::InputString()
{
    return m_textInputString;
}
