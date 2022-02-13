#pragma once

#include "Core/Vec2.h"

#include <bitset>
#include <array>
#include <map>
#include <SDL_keycode.h>
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
    Invalid,
    LeftX,
    LeftY,
    RightX,
    RightY,
    TriggerLeft,
    TriggerRight,
    Count
};

enum class Key
{
    Invalid,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    No1,
    No2,
    No3,
    No4,
    No5,
    No6,
    No7,
    No8,
    No9,
    No0,

    Return,
    Escape,
    Backspace,
    Tab,
    Space,
    Exclaim,
    QuoteDbl,
    Hash,
    Percent,
    Dollar,
    Ampersand,
    Quote,
    LeftParen,
    RightParen,
    Asterisk,
    Plus,
    Comma,
    Minus,
    Period,
    Slash,
    Colon,
    Semicolon,
    Less,
    Equals,
    Greater,
    Question,
    At,
    LeftBracket,
    Backslash,
    RightBracket,
    Caret,
    Underscore,
    BackQuote,

    CapsLock,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    PrintScreen,
    ScrollLock,
    Pause,
    Insert,
    Home,
    PageUp,
    Delete,
    End,
    PageDown,
    Right,
    Left,
    Down,
    Up,

    NumLock,
    KpDivide,
    KpMultiply,
    KpMinus,
    KpPlus,
    KpEnter,
    Kp1,
    Kp2,
    Kp3,
    Kp4,
    Kp5,
    Kp6,
    Kp7,
    Kp8,
    Kp9,
    Kp0,
    KpPeriod,

    LeftCtrl,
    LeftShift,
    LeftAlt,
    LeftGui,
    RightCtrl,
    RightShift,
    RightAlt,
    RightGui,
    Count
};

union SDL_Event;

class GameChip
{
public:
    void Init();
    void ProcessEvent(SDL_Event* event);
    void UpdateAxes(float deltaTime);
    void ClearStates();
    void Shutdown();

    // API
    bool GetButton(ControllerButton buttonCode);
    bool GetButtonDown(ControllerButton buttonCode);
    bool GetButtonUp(ControllerButton buttonCode);
    float GetAxis(ControllerAxis axis);

    Vec2i GetMousePosition();
    void EnableMouseRelativeMode(bool enable);
    
    bool GetKey(Key keyCode);
    bool GetKeyDown(Key keyCode);
    bool GetKeyUp(Key keyCode);

    std::string InputString();
private:
    std::map<SDL_GameControllerButton, ControllerButton> m_primaryBindings;
    std::map<SDL_GameControllerAxis, ControllerAxis> m_primaryAxisBindings;

    std::map<SDL_Keycode, ControllerButton> m_keyboardAltBindings;
    std::map<int, ControllerButton> m_mouseAltBindings;

    std::map<SDL_Keycode, ControllerAxis> m_keyboardAxisBindings;
    std::map<int, ControllerAxis> m_mouseAxisBindings;

    struct Axis
    {
        float m_axisValue{ 0.0f };

        bool m_ignoreVirtual{ false };
        bool m_isMouseDriver{ false };

        // Virtual axis input state
        bool m_positiveInput{ false };
        bool m_negativeInput{ false };

        // Virtual axis mapping
        SDL_Keycode m_positiveScanCode{ SDLK_UNKNOWN };
        SDL_Keycode m_negativeScanCode{ SDLK_UNKNOWN };
        int m_positiveMouseButton{ 0 };
        int m_negativeMouseButton{ 0 };
    };

    std::bitset<(size_t)Key::Count> m_keyDowns;
    std::bitset<(size_t)Key::Count> m_keyUps;
    std::bitset<(size_t)Key::Count> m_keyStates;

    std::bitset<(size_t)ControllerButton::Count> m_buttonDowns;
    std::bitset<(size_t)ControllerButton::Count> m_buttonUps;
    std::bitset<(size_t)ControllerButton::Count> m_buttonStates;

    std::array<Axis, (size_t)ControllerAxis::Count> m_axes;

    SDL_GameController* m_pOpenController{ nullptr };

    std::string m_textInputString;
};