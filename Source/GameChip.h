// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "Core/Vec2.h"

#include <string_builder.h>
#include <hashmap.h>
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
    void UpdateInputs(float deltaTime, Vec2f targetRes, Vec2f realWindowRes);
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

    String InputString();
private:
    HashMap<SDL_GameControllerButton, ControllerButton> m_primaryBindings;
    HashMap<SDL_GameControllerAxis, ControllerAxis> m_primaryAxisBindings;

    HashMap<SDL_Keycode, ControllerButton> m_keyboardAltBindings;
    HashMap<int, ControllerButton> m_mouseAltBindings;

    HashMap<SDL_Keycode, ControllerAxis> m_keyboardAxisBindings;
    HashMap<int, ControllerAxis> m_mouseAxisBindings;

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

    bool m_keyDowns[(size_t)Key::Count];
    bool m_keyUps[(size_t)Key::Count];
    bool m_keyStates[(size_t)Key::Count];

    bool m_buttonDowns[(size_t)ControllerButton::Count];
    bool m_buttonUps[(size_t)ControllerButton::Count];
    bool m_buttonStates[(size_t)ControllerButton::Count];

    Axis m_axes[(size_t)ControllerAxis::Count];

    SDL_GameController* m_pOpenController{ nullptr };

    StringBuilder m_textInputString;

    Vec2f m_targetResolution;
    Vec2f m_windowResolution;
};