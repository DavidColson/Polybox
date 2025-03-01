// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

enum class ControllerButton {
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

enum class ControllerAxis {
    Invalid,
    LeftX,
    LeftY,
    RightX,
    RightY,
    TriggerLeft,
    TriggerRight,
    Count
};

enum class Key {
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
struct String;

void InputInit();
void ProcessEvent(SDL_Event* event);
void UpdateInputs(f32 deltaTime, Vec2f targetRes, Vec2f realWindowRes);
void ClearStates();
void Shutdown();

// API
bool GetButton(ControllerButton buttonCode);
bool GetButtonDown(ControllerButton buttonCode);
bool GetButtonUp(ControllerButton buttonCode);
f32 GetAxis(ControllerAxis axis);

Vec2i GetMousePosition();
void EnableMouseRelativeMode(bool enable);

bool GetKey(Key keyCode);
bool GetKeyDown(Key keyCode);
bool GetKeyUp(Key keyCode);

String InputString();

   
