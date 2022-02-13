#pragma once

#include <bitset>
#include <array>
#include <map>
#include <SDL_scancode.h>
#include <SDL_gamecontroller.h>

enum class ControllerButton
{
    Invalid,
    FaceBottom,
    FaceRight,
    FaceLeft,
    FaceTop,
    LeftStick,
    RightStick,
    LeftShoulder,
    RightShoulder,
    DpadDown,
    DpadUp,
    DpadLeft,
    DpadRight,
    Select,
    Start,
    Count
};

enum class ControllerAxis
{
    LeftX,
    LeftY,
    RightX,
    RightY,
    TriggerLeft,
    TriggerRight,
    Count
};

union SDL_Event;

class GameChip
{
public:
    void Init();
    void ProcessEvent(SDL_Event* event);
    void ClearStates();
    void Shutdown();

    // API
    bool GetButton(ControllerButton buttonCode);
    bool GetButtonDown(ControllerButton buttonCode);
    bool GetButtonUp(ControllerButton buttonCode);

private:
    std::map<SDL_GameControllerButton, ControllerButton> m_primaryBindings;
    std::map<SDL_GameControllerAxis, ControllerAxis> m_primaryAxisBindings;

    std::map<SDL_Scancode, ControllerButton> m_keyboardAltBindings;
    std::map<int, ControllerButton> m_mouseAltBindings;

    std::map<SDL_Scancode, ControllerAxis> m_keyboardAxisBindings;
    std::map<int, ControllerAxis> m_mouseAxisBindings;

    struct Axis
    {
        float axisValue{ 0.0f };

        // Virtual axis input state
        bool positiveInput{ false };
        bool negativeInput{ false };

        // Virtual axis mapping
        SDL_Scancode m_positiveScanCode{ SDL_SCANCODE_UNKNOWN };
        SDL_Scancode m_negativeScanCode{ SDL_SCANCODE_UNKNOWN };
        int m_positiveMouseButton{ 0 };
        int m_negativeMouseButton{ 0 };
    };

    std::bitset<(size_t)ControllerButton::Count> m_buttonDowns;
    std::bitset<(size_t)ControllerButton::Count> m_buttonUps;
    std::bitset<(size_t)ControllerButton::Count> m_buttonStates;

    std::array<Axis, (size_t)ControllerAxis::Count> m_axes;

    SDL_GameController* m_pOpenController{ nullptr };
};