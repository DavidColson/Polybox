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
    std::map<uint32_t, SDL_Scancode> stringToScanCode = {
        {"Scancode_A"_hash, SDL_SCANCODE_A },
        {"Scancode_B"_hash, SDL_SCANCODE_B },
        {"Scancode_C"_hash, SDL_SCANCODE_C },
        {"Scancode_D"_hash, SDL_SCANCODE_D },
        {"Scancode_E"_hash, SDL_SCANCODE_E },
        {"Scancode_F"_hash, SDL_SCANCODE_F },
        {"Scancode_G"_hash, SDL_SCANCODE_G },
        {"Scancode_H"_hash, SDL_SCANCODE_H },
        {"Scancode_I"_hash, SDL_SCANCODE_I },
        {"Scancode_J"_hash, SDL_SCANCODE_J },
        {"Scancode_K"_hash, SDL_SCANCODE_K },
        {"Scancode_L"_hash, SDL_SCANCODE_L },
        {"Scancode_M"_hash, SDL_SCANCODE_M },
        {"Scancode_N"_hash, SDL_SCANCODE_N },
        {"Scancode_O"_hash, SDL_SCANCODE_O },
        {"Scancode_P"_hash, SDL_SCANCODE_P },
        {"Scancode_Q"_hash, SDL_SCANCODE_Q },
        {"Scancode_R"_hash, SDL_SCANCODE_R },
        {"Scancode_S"_hash, SDL_SCANCODE_S },
        {"Scancode_T"_hash, SDL_SCANCODE_T },
        {"Scancode_U"_hash, SDL_SCANCODE_U },
        {"Scancode_V"_hash, SDL_SCANCODE_V },
        {"Scancode_W"_hash, SDL_SCANCODE_W },
        {"Scancode_X"_hash, SDL_SCANCODE_X },
        {"Scancode_Y"_hash, SDL_SCANCODE_Y },
        {"Scancode_Z"_hash, SDL_SCANCODE_Z },

        {"Scancode_1"_hash, SDL_SCANCODE_1 },
        {"Scancode_2"_hash, SDL_SCANCODE_2 },
        {"Scancode_3"_hash, SDL_SCANCODE_3 },
        {"Scancode_4"_hash, SDL_SCANCODE_4 },
        {"Scancode_5"_hash, SDL_SCANCODE_5 },
        {"Scancode_6"_hash, SDL_SCANCODE_6 },
        {"Scancode_7"_hash, SDL_SCANCODE_7 },
        {"Scancode_8"_hash, SDL_SCANCODE_8 },
        {"Scancode_9"_hash, SDL_SCANCODE_9 },
        {"Scancode_0"_hash, SDL_SCANCODE_0 },

        {"Scancode_Return"_hash, SDL_SCANCODE_RETURN },
        {"Scancode_Escape"_hash, SDL_SCANCODE_ESCAPE },
        {"Scancode_Backspace"_hash, SDL_SCANCODE_BACKSPACE },
        {"Scancode_Tab"_hash, SDL_SCANCODE_TAB },
        {"Scancode_Space"_hash, SDL_SCANCODE_SPACE },

        {"Scancode_Minus"_hash, SDL_SCANCODE_MINUS},
        {"Scancode_Equals"_hash, SDL_SCANCODE_EQUALS},
        {"Scancode_LeftBracket"_hash, SDL_SCANCODE_LEFTBRACKET},
        {"Scancode_RightBracket"_hash, SDL_SCANCODE_RIGHTBRACKET},
        {"Scancode_Backslash"_hash, SDL_SCANCODE_BACKSLASH},
        {"Scancode_NonUSHash"_hash, SDL_SCANCODE_NONUSHASH},
        {"Scancode_Semicolon"_hash, SDL_SCANCODE_SEMICOLON},
        {"Scancode_Apostrophe"_hash, SDL_SCANCODE_APOSTROPHE},
        {"Scancode_Grave"_hash, SDL_SCANCODE_GRAVE},
        {"Scancode_Comma"_hash, SDL_SCANCODE_COMMA},
        {"Scancode_Period"_hash, SDL_SCANCODE_PERIOD},
        {"Scancode_Slash"_hash, SDL_SCANCODE_SLASH},

        {"Scancode_CapsLock"_hash, SDL_SCANCODE_CAPSLOCK},

        {"Scancode_F1"_hash, SDL_SCANCODE_F1},
        {"Scancode_F2"_hash, SDL_SCANCODE_F2},
        {"Scancode_F3"_hash, SDL_SCANCODE_F3},
        {"Scancode_F4"_hash, SDL_SCANCODE_F4},
        {"Scancode_F5"_hash, SDL_SCANCODE_F5},
        {"Scancode_F6"_hash, SDL_SCANCODE_F6},
        {"Scancode_F7"_hash, SDL_SCANCODE_F7},
        {"Scancode_F8"_hash, SDL_SCANCODE_F8},
        {"Scancode_F9"_hash, SDL_SCANCODE_F9},
        {"Scancode_F10"_hash, SDL_SCANCODE_F10},
        {"Scancode_F11"_hash, SDL_SCANCODE_F11},
        {"Scancode_F12"_hash, SDL_SCANCODE_F12},

        {"Scancode_PrintScreen"_hash, SDL_SCANCODE_PRINTSCREEN},
        {"Scancode_ScrollLock"_hash, SDL_SCANCODE_SCROLLLOCK},
        {"Scancode_Pause"_hash, SDL_SCANCODE_PAUSE},
        {"Scancode_Insert"_hash, SDL_SCANCODE_INSERT},
        {"Scancode_Home"_hash, SDL_SCANCODE_HOME},
        {"Scancode_PageUp"_hash, SDL_SCANCODE_PAGEUP},
        {"Scancode_Delete"_hash, SDL_SCANCODE_DELETE},
        {"Scancode_End"_hash, SDL_SCANCODE_END},
        {"Scancode_PageDown"_hash, SDL_SCANCODE_PAGEDOWN},
        {"Scancode_Right"_hash, SDL_SCANCODE_RIGHT},
        {"Scancode_Left"_hash, SDL_SCANCODE_LEFT},
        {"Scancode_Down"_hash, SDL_SCANCODE_DOWN},
        {"Scancode_Up"_hash, SDL_SCANCODE_UP},

        {"Scancode_NumLock"_hash, SDL_SCANCODE_NUMLOCKCLEAR},
        {"Scancode_KpDivide"_hash, SDL_SCANCODE_KP_DIVIDE},
        {"Scancode_KpMultiply"_hash, SDL_SCANCODE_KP_MULTIPLY},
        {"Scancode_KpMinus"_hash, SDL_SCANCODE_KP_MINUS},
        {"Scancode_KpPlus"_hash, SDL_SCANCODE_KP_PLUS},
        {"Scancode_KpEnter"_hash, SDL_SCANCODE_KP_ENTER},
        {"Scancode_Kp1"_hash, SDL_SCANCODE_KP_1},
        {"Scancode_Kp2"_hash, SDL_SCANCODE_KP_2},
        {"Scancode_Kp3"_hash, SDL_SCANCODE_KP_3},
        {"Scancode_Kp4"_hash, SDL_SCANCODE_KP_4},
        {"Scancode_Kp5"_hash, SDL_SCANCODE_KP_5},
        {"Scancode_Kp6"_hash, SDL_SCANCODE_KP_6},
        {"Scancode_Kp7"_hash, SDL_SCANCODE_KP_7},
        {"Scancode_Kp8"_hash, SDL_SCANCODE_KP_8},
        {"Scancode_Kp9"_hash, SDL_SCANCODE_KP_9},
        {"Scancode_Kp0"_hash, SDL_SCANCODE_KP_0},
        {"Scancode_KpPeriod"_hash, SDL_SCANCODE_KP_PERIOD},

        {"Scancode_LeftCtrl"_hash, SDL_SCANCODE_LCTRL},
        {"Scancode_LeftShift"_hash, SDL_SCANCODE_LSHIFT},
        {"Scancode_LeftAlt"_hash, SDL_SCANCODE_LALT},
        {"Scancode_LeftGui"_hash, SDL_SCANCODE_LGUI},
        {"Scancode_RightCtrl"_hash, SDL_SCANCODE_RCTRL},
        {"Scancode_RightShift"_hash, SDL_SCANCODE_RSHIFT},
        {"Scancode_RightAlt"_hash, SDL_SCANCODE_RALT},
        {"Scancode_RightGui"_hash, SDL_SCANCODE_RGUI}
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
            if (altBindingLabel.substr(0, 5) == "Scanc")
            {
                SDL_Scancode scancode = stringToScanCode[Fnv1a::Hash(altBindingLabel.c_str())];
                m_keyboardAltBindings[scancode] = button;
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
                    SDL_Scancode scancode = stringToScanCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_keyboardAxisBindings[scancode] = axis;
                    m_axes[(size_t)axis].m_positiveScanCode = scancode;
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
                    SDL_Scancode scancode = stringToScanCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_keyboardAxisBindings[scancode] = axis;
                    m_axes[(size_t)axis].m_positiveScanCode = scancode;
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
                    SDL_Scancode scancode = stringToScanCode[Fnv1a::Hash(altBindingLabel.c_str())];
                    m_keyboardAxisBindings[scancode] = axis;
                    m_axes[(size_t)axis].m_negativeScanCode = scancode;
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
    case SDL_KEYDOWN:
    {
        SDL_Scancode scancode = event->key.keysym.scancode;
        ControllerButton button = m_keyboardAltBindings[scancode];
        if (button != ControllerButton::Invalid)
        {
            m_buttonDowns[(size_t)button] = true;
            m_buttonStates[(size_t)button] = true;
        }
        ControllerAxis axis = m_keyboardAxisBindings[scancode];
        if (axis != ControllerAxis::Invalid)
        {
            Axis& axisData = m_axes[(size_t)axis];
            if (axisData.m_positiveScanCode == scancode)
                axisData.m_positiveInput = true;
            else if (axisData.m_negativeScanCode == scancode)
                axisData.m_negativeInput = true;
            axisData.m_ignoreVirtual = false;
        }
        break;
    }
    case SDL_KEYUP:
    {
        SDL_Scancode scancode = event->key.keysym.scancode;
        ControllerButton button = m_keyboardAltBindings[scancode];
        if (button != ControllerButton::Invalid)
        {
            m_buttonUps[(size_t)button] = true;
            m_buttonStates[(size_t)button] = false;
        }
        ControllerAxis axis = m_keyboardAxisBindings[scancode];
        if (axis != ControllerAxis::Invalid)
        {
            Axis& axisData = m_axes[(size_t)axis];
            if (axisData.m_positiveScanCode == scancode)
                axisData.m_positiveInput = false;
            else if (axisData.m_negativeScanCode == scancode)
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

void GameChip::UpdateAxes(float deltaTime)
{
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
    m_buttonDowns.reset();
	m_buttonUps.reset();

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

void GameChip::EnableMouseRelativeMode(bool enable)
{
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
}