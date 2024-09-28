// Copyright David Colson. All rights reserved.

#include "input.h"

#include <SDL_gamecontroller.h>
#include <string_builder.h>
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_joystick.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>
#include <defer.h>
#include <json.h>
#include <maths.h>
#include <string_hash.h>
#include <string_builder.h>
#include <hashmap.inl>
#include <log.h>
#undef DrawText
#undef DrawTextEx

struct Axis {
	f32 axisValue { 0.0f };

	bool ignoreVirtual { false };
	bool isMouseDriver { false };

	// Virtual axis input state
	bool positiveInput { false };
	bool negativeInput { false };

	// Virtual axis mapping
	SDL_Keycode positiveScanCode { SDLK_UNKNOWN };
	SDL_Keycode negativeScanCode { SDLK_UNKNOWN };
	i32 positiveMouseButton { 0 };
	i32 negativeMouseButton { 0 };
};

struct InputState {
	Arena* pArena;

	// Mapping of strings to virtual controller buttons
	HashMap<u32, ControllerButton> stringToControllerButton;

	// Mapping of strings to virtual controller axis
	HashMap<u32, ControllerAxis> stringToControllerAxis;

	// Mapping of strings to SDL scancodes
	HashMap<u32, SDL_Keycode> stringToKeyCode;

	// Mapping of strings to SDL scancodes
	HashMap<SDL_Keycode, Key> keyCodeToInternalKeyCode;

	// Mapping of strings to SDL mouse codes
	HashMap<u32, i32> stringToMouseCode;

	// Mapping of strings to SDL controller buttons
	HashMap<u32, SDL_GameControllerButton> stringToSDLControllerButton;

	// Mapping of strings to SDL controller pState->axes
	HashMap<u32, SDL_GameControllerAxis> stringToSDLControllerAxis;

    HashMap<SDL_GameControllerButton, ControllerButton> primaryBindings;
    HashMap<SDL_GameControllerAxis, ControllerAxis> primaryAxisBindings;

    HashMap<SDL_Keycode, ControllerButton> keyboardAltBindings;
    HashMap<int, ControllerButton> mouseAltBindings;

    HashMap<SDL_Keycode, ControllerAxis> keyboardAxisBindings;
    HashMap<int, ControllerAxis> mouseAxisBindings;

    bool keyDowns[(usize)Key::Count];
    bool keyUps[(usize)Key::Count];
    bool keyStates[(usize)Key::Count];

    bool buttonDowns[(usize)ControllerButton::Count];
    bool buttonUps[(usize)ControllerButton::Count];
    bool buttonStates[(usize)ControllerButton::Count];

    Axis axes[(usize)ControllerAxis::Count];

    SDL_GameController* pOpenController { nullptr };

    StringBuilder textInputString;

    Vec2f targetResolution;
    Vec2f windowResolution;
};

namespace {
InputState* pState;
}

// ***********************************************************************

void InputInit() {
	Arena* pArena = ArenaCreate();
	pState = New(pArena, InputState);
	pState->pArena = pArena;
	
	pState->stringToControllerButton.pArena = pArena;
	pState->stringToControllerAxis.pArena = pArena;
	pState->stringToKeyCode.pArena = pArena;
	pState->keyCodeToInternalKeyCode.pArena = pArena;
	pState->stringToMouseCode.pArena = pArena;
	pState->stringToSDLControllerButton.pArena = pArena;
	pState->stringToSDLControllerAxis.pArena = pArena;
	pState->stringToSDLControllerButton.pArena = pArena;
	pState->primaryBindings.pArena = pArena;
	pState->primaryAxisBindings.pArena = pArena;
	pState->keyboardAltBindings.pArena = pArena;
	pState->mouseAltBindings.pArena = pArena;
	pState->keyboardAxisBindings.pArena = pArena;
	pState->mouseAxisBindings.pArena = pArena;
	pState->primaryAxisBindings.pArena = pArena;
	pState->textInputString.pArena = pArena;

    // Setup hashmaps for controller labels
    pState->stringToControllerButton.Add("FaceBottom"_hash, ControllerButton::FaceBottom);
    pState->stringToControllerButton.Add("FaceRight"_hash, ControllerButton::FaceRight);
    pState->stringToControllerButton.Add("FaceLeft"_hash, ControllerButton::FaceLeft);
    pState->stringToControllerButton.Add("FaceTop"_hash, ControllerButton::FaceTop);
    pState->stringToControllerButton.Add("LeftStick"_hash, ControllerButton::LeftStick);
    pState->stringToControllerButton.Add("RightStick"_hash, ControllerButton::RightStick);
    pState->stringToControllerButton.Add("LeftShoulder"_hash, ControllerButton::LeftShoulder);
    pState->stringToControllerButton.Add("RightShoulder"_hash, ControllerButton::RightShoulder);
    pState->stringToControllerButton.Add("DpadDown"_hash, ControllerButton::DpadDown);
    pState->stringToControllerButton.Add("DpadLeft"_hash, ControllerButton::DpadLeft);
    pState->stringToControllerButton.Add("DpadRight"_hash, ControllerButton::DpadRight);
    pState->stringToControllerButton.Add("DpadUp"_hash, ControllerButton::DpadUp);
    pState->stringToControllerButton.Add("Start"_hash, ControllerButton::Start);
    pState->stringToControllerButton.Add("Select"_hash, ControllerButton::Select);


    pState->stringToControllerAxis.Add("LeftX"_hash, ControllerAxis::LeftX);
    pState->stringToControllerAxis.Add("LeftY"_hash, ControllerAxis::LeftY);
    pState->stringToControllerAxis.Add("RightX"_hash, ControllerAxis::RightX);
    pState->stringToControllerAxis.Add("RightY"_hash, ControllerAxis::RightY);
    pState->stringToControllerAxis.Add("TriggerLeft"_hash, ControllerAxis::TriggerLeft);
    pState->stringToControllerAxis.Add("TriggerRight"_hash, ControllerAxis::TriggerRight);


    pState->stringToKeyCode.Add("Keycode_A"_hash, SDLK_a);
    pState->stringToKeyCode.Add("Keycode_B"_hash, SDLK_b);
    pState->stringToKeyCode.Add("Keycode_C"_hash, SDLK_c);
    pState->stringToKeyCode.Add("Keycode_D"_hash, SDLK_d);
    pState->stringToKeyCode.Add("Keycode_E"_hash, SDLK_e);
    pState->stringToKeyCode.Add("Keycode_F"_hash, SDLK_f);
    pState->stringToKeyCode.Add("Keycode_G"_hash, SDLK_g);
    pState->stringToKeyCode.Add("Keycode_H"_hash, SDLK_h);
    pState->stringToKeyCode.Add("Keycode_I"_hash, SDLK_i);
    pState->stringToKeyCode.Add("Keycode_J"_hash, SDLK_j);
    pState->stringToKeyCode.Add("Keycode_K"_hash, SDLK_k);
    pState->stringToKeyCode.Add("Keycode_L"_hash, SDLK_l);
    pState->stringToKeyCode.Add("Keycode_M"_hash, SDLK_m);
    pState->stringToKeyCode.Add("Keycode_N"_hash, SDLK_n);
    pState->stringToKeyCode.Add("Keycode_O"_hash, SDLK_o);
    pState->stringToKeyCode.Add("Keycode_P"_hash, SDLK_p);
    pState->stringToKeyCode.Add("Keycode_Q"_hash, SDLK_q);
    pState->stringToKeyCode.Add("Keycode_R"_hash, SDLK_e);
    pState->stringToKeyCode.Add("Keycode_S"_hash, SDLK_s);
    pState->stringToKeyCode.Add("Keycode_T"_hash, SDLK_t);
    pState->stringToKeyCode.Add("Keycode_U"_hash, SDLK_u);
    pState->stringToKeyCode.Add("Keycode_V"_hash, SDLK_v);
    pState->stringToKeyCode.Add("Keycode_W"_hash, SDLK_w);
    pState->stringToKeyCode.Add("Keycode_X"_hash, SDLK_x);
    pState->stringToKeyCode.Add("Keycode_Y"_hash, SDLK_y);
    pState->stringToKeyCode.Add("Keycode_Z"_hash, SDLK_z);

    pState->stringToKeyCode.Add("Keycode_1"_hash, SDLK_1);
    pState->stringToKeyCode.Add("Keycode_2"_hash, SDLK_2);
    pState->stringToKeyCode.Add("Keycode_3"_hash, SDLK_3);
    pState->stringToKeyCode.Add("Keycode_4"_hash, SDLK_4);
    pState->stringToKeyCode.Add("Keycode_5"_hash, SDLK_5);
    pState->stringToKeyCode.Add("Keycode_6"_hash, SDLK_6);
    pState->stringToKeyCode.Add("Keycode_7"_hash, SDLK_7);
    pState->stringToKeyCode.Add("Keycode_8"_hash, SDLK_8);
    pState->stringToKeyCode.Add("Keycode_9"_hash, SDLK_9);
    pState->stringToKeyCode.Add("Keycode_0"_hash, SDLK_0);

    pState->stringToKeyCode.Add("Keycode_Return"_hash, SDLK_RETURN);
    pState->stringToKeyCode.Add("Keycode_Escape"_hash, SDLK_ESCAPE);
    pState->stringToKeyCode.Add("Keycode_Backspace"_hash, SDLK_BACKSPACE);
    pState->stringToKeyCode.Add("Keycode_Tab"_hash, SDLK_TAB);
    pState->stringToKeyCode.Add("Keycode_Space"_hash, SDLK_SPACE);
    pState->stringToKeyCode.Add("Keycode_Exclaim"_hash, SDLK_EXCLAIM);
    pState->stringToKeyCode.Add("Keycode_QuoteDbl"_hash, SDLK_QUOTEDBL);
    pState->stringToKeyCode.Add("Keycode_Hash"_hash, SDLK_HASH);
    pState->stringToKeyCode.Add("Keycode_Percent"_hash, SDLK_PERCENT);
    pState->stringToKeyCode.Add("Keycode_Dollar"_hash, SDLK_DOLLAR);
    pState->stringToKeyCode.Add("Keycode_Ampersand"_hash, SDLK_AMPERSAND);
    pState->stringToKeyCode.Add("Keycode_Quote"_hash, SDLK_QUOTE);
    pState->stringToKeyCode.Add("Keycode_LeftParen"_hash, SDLK_LEFTPAREN);
    pState->stringToKeyCode.Add("Keycode_RightParen"_hash, SDLK_RIGHTPAREN);
    pState->stringToKeyCode.Add("Keycode_Asterisk"_hash, SDLK_ASTERISK);
    pState->stringToKeyCode.Add("Keycode_Plus"_hash, SDLK_PLUS);
    pState->stringToKeyCode.Add("Keycode_Comma"_hash, SDLK_COMMA);
    pState->stringToKeyCode.Add("Keycode_Minus"_hash, SDLK_MINUS);
    pState->stringToKeyCode.Add("Keycode_Period"_hash, SDLK_PERIOD);
    pState->stringToKeyCode.Add("Keycode_Slash"_hash, SDLK_SLASH);
    pState->stringToKeyCode.Add("Keycode_Colon"_hash, SDLK_COLON);
    pState->stringToKeyCode.Add("Keycode_Semicolon"_hash, SDLK_SEMICOLON);
    pState->stringToKeyCode.Add("Keycode_Less"_hash, SDLK_LESS);
    pState->stringToKeyCode.Add("Keycode_Equals"_hash, SDLK_EQUALS);
    pState->stringToKeyCode.Add("Keycode_Greater"_hash, SDLK_GREATER);
    pState->stringToKeyCode.Add("Keycode_Question"_hash, SDLK_QUESTION);
    pState->stringToKeyCode.Add("Keycode_At"_hash, SDLK_AT);
    pState->stringToKeyCode.Add("Keycode_LeftBracket"_hash, SDLK_LEFTBRACKET);
    pState->stringToKeyCode.Add("Keycode_Backslash"_hash, SDLK_BACKSLASH);
    pState->stringToKeyCode.Add("Keycode_RightBracket"_hash, SDLK_RIGHTBRACKET);
    pState->stringToKeyCode.Add("Keycode_Caret"_hash, SDLK_CARET);
    pState->stringToKeyCode.Add("Keycode_Underscore"_hash, SDLK_UNDERSCORE);
    pState->stringToKeyCode.Add("Keycode_BackQuote"_hash, SDLK_BACKQUOTE);

    pState->stringToKeyCode.Add("Keycode_CapsLock"_hash, SDLK_CAPSLOCK);

    pState->stringToKeyCode.Add("Keycode_F1"_hash, SDLK_F1);
    pState->stringToKeyCode.Add("Keycode_F2"_hash, SDLK_F2);
    pState->stringToKeyCode.Add("Keycode_F3"_hash, SDLK_F3);
    pState->stringToKeyCode.Add("Keycode_F4"_hash, SDLK_F4);
    pState->stringToKeyCode.Add("Keycode_F5"_hash, SDLK_F5);
    pState->stringToKeyCode.Add("Keycode_F6"_hash, SDLK_F6);
    pState->stringToKeyCode.Add("Keycode_F7"_hash, SDLK_F7);
    pState->stringToKeyCode.Add("Keycode_F8"_hash, SDLK_F8);
    pState->stringToKeyCode.Add("Keycode_F9"_hash, SDLK_F9);
    pState->stringToKeyCode.Add("Keycode_F10"_hash, SDLK_F10);
    pState->stringToKeyCode.Add("Keycode_F11"_hash, SDLK_F11);
    pState->stringToKeyCode.Add("Keycode_F12"_hash, SDLK_F12);

    pState->stringToKeyCode.Add("Keycode_PrintScreen"_hash, SDLK_PRINTSCREEN);
    pState->stringToKeyCode.Add("Keycode_ScrollLock"_hash, SDLK_SCROLLLOCK);
    pState->stringToKeyCode.Add("Keycode_Pause"_hash, SDLK_PAUSE);
    pState->stringToKeyCode.Add("Keycode_Insert"_hash, SDLK_INSERT);
    pState->stringToKeyCode.Add("Keycode_Home"_hash, SDLK_HOME);
    pState->stringToKeyCode.Add("Keycode_PageUp"_hash, SDLK_PAGEUP);
    pState->stringToKeyCode.Add("Keycode_Delete"_hash, SDLK_DELETE);
    pState->stringToKeyCode.Add("Keycode_End"_hash, SDLK_END);
    pState->stringToKeyCode.Add("Keycode_PageDown"_hash, SDLK_PAGEDOWN);
    pState->stringToKeyCode.Add("Keycode_Right"_hash, SDLK_RIGHT);
    pState->stringToKeyCode.Add("Keycode_Left"_hash, SDLK_LEFT);
    pState->stringToKeyCode.Add("Keycode_Down"_hash, SDLK_DOWN);
    pState->stringToKeyCode.Add("Keycode_Up"_hash, SDLK_UP);

    pState->stringToKeyCode.Add("Keycode_NumLock"_hash, SDLK_NUMLOCKCLEAR);
    pState->stringToKeyCode.Add("Keycode_KpDivide"_hash, SDLK_KP_DIVIDE);
    pState->stringToKeyCode.Add("Keycode_KpMultiply"_hash, SDLK_KP_MULTIPLY);
    pState->stringToKeyCode.Add("Keycode_KpMinus"_hash, SDLK_KP_MINUS);
    pState->stringToKeyCode.Add("Keycode_KpPlus"_hash, SDLK_KP_PLUS);
    pState->stringToKeyCode.Add("Keycode_KpEnter"_hash, SDLK_KP_ENTER);
    pState->stringToKeyCode.Add("Keycode_Kp1"_hash, SDLK_KP_1);
    pState->stringToKeyCode.Add("Keycode_Kp2"_hash, SDLK_KP_2);
    pState->stringToKeyCode.Add("Keycode_Kp3"_hash, SDLK_KP_3);
    pState->stringToKeyCode.Add("Keycode_Kp4"_hash, SDLK_KP_4);
    pState->stringToKeyCode.Add("Keycode_Kp5"_hash, SDLK_KP_5);
    pState->stringToKeyCode.Add("Keycode_Kp6"_hash, SDLK_KP_6);
    pState->stringToKeyCode.Add("Keycode_Kp7"_hash, SDLK_KP_7);
    pState->stringToKeyCode.Add("Keycode_Kp8"_hash, SDLK_KP_8);
    pState->stringToKeyCode.Add("Keycode_Kp9"_hash, SDLK_KP_9);
    pState->stringToKeyCode.Add("Keycode_Kp0"_hash, SDLK_KP_0);
    pState->stringToKeyCode.Add("Keycode_KpPeriod"_hash, SDLK_KP_PERIOD);

    pState->stringToKeyCode.Add("Keycode_LeftCtrl"_hash, SDLK_LCTRL);
    pState->stringToKeyCode.Add("Keycode_LeftShift"_hash, SDLK_LSHIFT);
    pState->stringToKeyCode.Add("Keycode_LeftAlt"_hash, SDLK_LALT);
    pState->stringToKeyCode.Add("Keycode_LeftGui"_hash, SDLK_LGUI);
    pState->stringToKeyCode.Add("Keycode_RightCtrl"_hash, SDLK_RCTRL);
    pState->stringToKeyCode.Add("Keycode_RightShift"_hash, SDLK_RSHIFT);
    pState->stringToKeyCode.Add("Keycode_RightAlt"_hash, SDLK_RALT);
    pState->stringToKeyCode.Add("Keycode_RightGui"_hash, SDLK_RGUI);


    pState->keyCodeToInternalKeyCode.Add(SDLK_a, Key::A);
    pState->keyCodeToInternalKeyCode.Add(SDLK_b, Key::B);
    pState->keyCodeToInternalKeyCode.Add(SDLK_c, Key::C);
    pState->keyCodeToInternalKeyCode.Add(SDLK_d, Key::D);
    pState->keyCodeToInternalKeyCode.Add(SDLK_e, Key::E);
    pState->keyCodeToInternalKeyCode.Add(SDLK_f, Key::F);
    pState->keyCodeToInternalKeyCode.Add(SDLK_g, Key::G);
    pState->keyCodeToInternalKeyCode.Add(SDLK_h, Key::H);
    pState->keyCodeToInternalKeyCode.Add(SDLK_i, Key::I);
    pState->keyCodeToInternalKeyCode.Add(SDLK_j, Key::J);
    pState->keyCodeToInternalKeyCode.Add(SDLK_k, Key::K);
    pState->keyCodeToInternalKeyCode.Add(SDLK_l, Key::L);
    pState->keyCodeToInternalKeyCode.Add(SDLK_m, Key::M);
    pState->keyCodeToInternalKeyCode.Add(SDLK_n, Key::N);
    pState->keyCodeToInternalKeyCode.Add(SDLK_o, Key::O);
    pState->keyCodeToInternalKeyCode.Add(SDLK_p, Key::P);
    pState->keyCodeToInternalKeyCode.Add(SDLK_q, Key::Q);
    pState->keyCodeToInternalKeyCode.Add(SDLK_e, Key::R);
    pState->keyCodeToInternalKeyCode.Add(SDLK_s, Key::S);
    pState->keyCodeToInternalKeyCode.Add(SDLK_t, Key::T);
    pState->keyCodeToInternalKeyCode.Add(SDLK_u, Key::U);
    pState->keyCodeToInternalKeyCode.Add(SDLK_v, Key::V);
    pState->keyCodeToInternalKeyCode.Add(SDLK_w, Key::W);
    pState->keyCodeToInternalKeyCode.Add(SDLK_x, Key::X);
    pState->keyCodeToInternalKeyCode.Add(SDLK_y, Key::Y);
    pState->keyCodeToInternalKeyCode.Add(SDLK_z, Key::Z);

    pState->keyCodeToInternalKeyCode.Add(SDLK_1, Key::No1);
    pState->keyCodeToInternalKeyCode.Add(SDLK_2, Key::No2);
    pState->keyCodeToInternalKeyCode.Add(SDLK_3, Key::No3);
    pState->keyCodeToInternalKeyCode.Add(SDLK_4, Key::No4);
    pState->keyCodeToInternalKeyCode.Add(SDLK_5, Key::No5);
    pState->keyCodeToInternalKeyCode.Add(SDLK_6, Key::No6);
    pState->keyCodeToInternalKeyCode.Add(SDLK_7, Key::No7);
    pState->keyCodeToInternalKeyCode.Add(SDLK_8, Key::No8);
    pState->keyCodeToInternalKeyCode.Add(SDLK_9, Key::No9);
    pState->keyCodeToInternalKeyCode.Add(SDLK_0, Key::No0);

    pState->keyCodeToInternalKeyCode.Add(SDLK_RETURN, Key::Return);
    pState->keyCodeToInternalKeyCode.Add(SDLK_ESCAPE, Key::Escape);
    pState->keyCodeToInternalKeyCode.Add(SDLK_BACKSPACE, Key::Backspace);
    pState->keyCodeToInternalKeyCode.Add(SDLK_TAB, Key::Tab);
    pState->keyCodeToInternalKeyCode.Add(SDLK_SPACE, Key::Space);
    pState->keyCodeToInternalKeyCode.Add(SDLK_EXCLAIM, Key::Exclaim);
    pState->keyCodeToInternalKeyCode.Add(SDLK_QUOTEDBL, Key::QuoteDbl);
    pState->keyCodeToInternalKeyCode.Add(SDLK_HASH, Key::Hash);
    pState->keyCodeToInternalKeyCode.Add(SDLK_PERCENT, Key::Percent);
    pState->keyCodeToInternalKeyCode.Add(SDLK_DOLLAR, Key::Dollar);
    pState->keyCodeToInternalKeyCode.Add(SDLK_AMPERSAND, Key::Ampersand);
    pState->keyCodeToInternalKeyCode.Add(SDLK_QUOTE, Key::Quote);
    pState->keyCodeToInternalKeyCode.Add(SDLK_LEFTPAREN, Key::LeftParen);
    pState->keyCodeToInternalKeyCode.Add(SDLK_RIGHTPAREN, Key::RightParen);
    pState->keyCodeToInternalKeyCode.Add(SDLK_ASTERISK, Key::Asterisk);
    pState->keyCodeToInternalKeyCode.Add(SDLK_PLUS, Key::Plus);
    pState->keyCodeToInternalKeyCode.Add(SDLK_COMMA, Key::Comma);
    pState->keyCodeToInternalKeyCode.Add(SDLK_MINUS, Key::Minus);
    pState->keyCodeToInternalKeyCode.Add(SDLK_PERIOD, Key::Period);
    pState->keyCodeToInternalKeyCode.Add(SDLK_SLASH, Key::Slash);
    pState->keyCodeToInternalKeyCode.Add(SDLK_COLON, Key::Colon);
    pState->keyCodeToInternalKeyCode.Add(SDLK_SEMICOLON, Key::Semicolon);
    pState->keyCodeToInternalKeyCode.Add(SDLK_LESS, Key::Less);
    pState->keyCodeToInternalKeyCode.Add(SDLK_EQUALS, Key::Equals);
    pState->keyCodeToInternalKeyCode.Add(SDLK_GREATER, Key::Greater);
    pState->keyCodeToInternalKeyCode.Add(SDLK_QUESTION, Key::Question);
    pState->keyCodeToInternalKeyCode.Add(SDLK_AT, Key::At);
    pState->keyCodeToInternalKeyCode.Add(SDLK_LEFTBRACKET, Key::LeftBracket);
    pState->keyCodeToInternalKeyCode.Add(SDLK_BACKSLASH, Key::Backslash);
    pState->keyCodeToInternalKeyCode.Add(SDLK_RIGHTBRACKET, Key::RightBracket);
    pState->keyCodeToInternalKeyCode.Add(SDLK_CARET, Key::Caret);
    pState->keyCodeToInternalKeyCode.Add(SDLK_UNDERSCORE, Key::Underscore);
    pState->keyCodeToInternalKeyCode.Add(SDLK_BACKQUOTE, Key::BackQuote);

    pState->keyCodeToInternalKeyCode.Add(SDLK_CAPSLOCK, Key::CapsLock);

    pState->keyCodeToInternalKeyCode.Add(SDLK_F1, Key::F1);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F2, Key::F2);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F3, Key::F3);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F4, Key::F4);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F5, Key::F5);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F6, Key::F6);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F7, Key::F7);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F8, Key::F8);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F9, Key::F9);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F10, Key::F10);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F11, Key::F11);
    pState->keyCodeToInternalKeyCode.Add(SDLK_F12, Key::F12);

    pState->keyCodeToInternalKeyCode.Add(SDLK_PRINTSCREEN, Key::PrintScreen);
    pState->keyCodeToInternalKeyCode.Add(SDLK_SCROLLLOCK, Key::ScrollLock);
    pState->keyCodeToInternalKeyCode.Add(SDLK_PAUSE, Key::Pause);
    pState->keyCodeToInternalKeyCode.Add(SDLK_INSERT, Key::Insert);
    pState->keyCodeToInternalKeyCode.Add(SDLK_HOME, Key::Home);
    pState->keyCodeToInternalKeyCode.Add(SDLK_PAGEUP, Key::PageUp);
    pState->keyCodeToInternalKeyCode.Add(SDLK_DELETE, Key::Delete);
    pState->keyCodeToInternalKeyCode.Add(SDLK_END, Key::End);
    pState->keyCodeToInternalKeyCode.Add(SDLK_PAGEDOWN, Key::PageDown);
    pState->keyCodeToInternalKeyCode.Add(SDLK_RIGHT, Key::Right);
    pState->keyCodeToInternalKeyCode.Add(SDLK_LEFT, Key::Left);
    pState->keyCodeToInternalKeyCode.Add(SDLK_DOWN, Key::Down);
    pState->keyCodeToInternalKeyCode.Add(SDLK_UP, Key::Up);

    pState->keyCodeToInternalKeyCode.Add(SDLK_NUMLOCKCLEAR, Key::NumLock);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_DIVIDE, Key::KpDivide);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_MULTIPLY, Key::KpMultiply);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_MINUS, Key::KpMinus);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_PLUS, Key::KpPlus);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_ENTER, Key::KpEnter);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_1, Key::Kp1);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_2, Key::Kp2);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_3, Key::Kp3);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_4, Key::Kp4);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_5, Key::Kp5);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_6, Key::Kp6);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_7, Key::Kp7);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_8, Key::Kp8);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_9, Key::Kp9);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_0, Key::Kp0);
    pState->keyCodeToInternalKeyCode.Add(SDLK_KP_PERIOD, Key::KpPeriod);

    pState->keyCodeToInternalKeyCode.Add(SDLK_LCTRL, Key::LeftCtrl);
    pState->keyCodeToInternalKeyCode.Add(SDLK_LSHIFT, Key::LeftShift);
    pState->keyCodeToInternalKeyCode.Add(SDLK_LALT, Key::LeftAlt);
    pState->keyCodeToInternalKeyCode.Add(SDLK_LGUI, Key::LeftGui);
    pState->keyCodeToInternalKeyCode.Add(SDLK_RCTRL, Key::RightCtrl);
    pState->keyCodeToInternalKeyCode.Add(SDLK_RSHIFT, Key::RightShift);
    pState->keyCodeToInternalKeyCode.Add(SDLK_RALT, Key::RightAlt);
    pState->keyCodeToInternalKeyCode.Add(SDLK_RGUI, Key::RightGui);


    pState->stringToMouseCode.Add("Mouse_Button0"_hash, SDL_BUTTON_LEFT);
    pState->stringToMouseCode.Add("Mouse_Button1"_hash, SDL_BUTTON_MIDDLE);
    pState->stringToMouseCode.Add("Mouse_Button2"_hash, SDL_BUTTON_RIGHT);
    pState->stringToMouseCode.Add("Mouse_AxisY"_hash, 128);
    pState->stringToMouseCode.Add("Mouse_AxisX"_hash, 127);


    pState->stringToSDLControllerButton.Add("Controller_A"_hash, SDL_CONTROLLER_BUTTON_A);
    pState->stringToSDLControllerButton.Add("Controller_B"_hash, SDL_CONTROLLER_BUTTON_B);
    pState->stringToSDLControllerButton.Add("Controller_X"_hash, SDL_CONTROLLER_BUTTON_X);
    pState->stringToSDLControllerButton.Add("Controller_Y"_hash, SDL_CONTROLLER_BUTTON_Y);
    pState->stringToSDLControllerButton.Add("Controller_LeftStick"_hash, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    pState->stringToSDLControllerButton.Add("Controller_RightStick"_hash, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    pState->stringToSDLControllerButton.Add("Controller_LeftShoulder"_hash, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    pState->stringToSDLControllerButton.Add("Controller_RightShoulder"_hash, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    pState->stringToSDLControllerButton.Add("Controller_DpadUp"_hash, SDL_CONTROLLER_BUTTON_DPAD_UP);
    pState->stringToSDLControllerButton.Add("Controller_DpadDown"_hash, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    pState->stringToSDLControllerButton.Add("Controller_DpadLeft"_hash, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    pState->stringToSDLControllerButton.Add("Controller_DpadRight"_hash, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    pState->stringToSDLControllerButton.Add("Controller_Start"_hash, SDL_CONTROLLER_BUTTON_START);
    pState->stringToSDLControllerButton.Add("Controller_Select"_hash, SDL_CONTROLLER_BUTTON_BACK);


    pState->stringToSDLControllerAxis.Add("Controller_LeftX"_hash, SDL_CONTROLLER_AXIS_LEFTX);
    pState->stringToSDLControllerAxis.Add("Controller_LeftY"_hash, SDL_CONTROLLER_AXIS_LEFTY);
    pState->stringToSDLControllerAxis.Add("Controller_RightX"_hash, SDL_CONTROLLER_AXIS_RIGHTX);
    pState->stringToSDLControllerAxis.Add("Controller_RightY"_hash, SDL_CONTROLLER_AXIS_RIGHTY);
    pState->stringToSDLControllerAxis.Add("Controller_TriggerLeft"_hash, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    pState->stringToSDLControllerAxis.Add("Controller_TriggerRight"_hash, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);


    // Load json mapping file
    SDL_RWops* pFileRead = SDL_RWFromFile("assets/ControllerMapping.json", "rb");
    u64 size = SDL_RWsize(pFileRead);
    char* pData = New(g_pArenaFrame, char, size);
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    String file;
    file.pData = pData;
    file.length = size;
    JsonValue mapping = ParseJsonFile(g_pArenaFrame, file);

    // Going through all elements of the json file
    if (mapping.HasKey("Buttons")) {
        JsonValue& buttons = mapping["Buttons"];
        for (usize i = 0; i < buttons.Count(); i++) {
            JsonValue& jsonButton = buttons[i];
            ControllerButton button = pState->stringToControllerButton[Fnv1a::Hash(jsonButton["Name"].ToString().pData)];
            SDL_GameControllerButton primaryBinding = pState->stringToSDLControllerButton[Fnv1a::Hash(jsonButton["Primary"].ToString().pData)];
            pState->primaryBindings[primaryBinding] = button;

            String altBindingLabel = jsonButton["Alt"].ToString();
            if (altBindingLabel.SubStr(0, 5) == "Keyco") {
                SDL_Keycode keycode = pState->stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                pState->keyboardAltBindings[keycode] = button;
            } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                i32 mousecode = pState->stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                pState->mouseAltBindings[mousecode] = button;
            }
        }
    }
    if (mapping.HasKey("Axes")) {
        JsonValue& jsonAxes = mapping["Axes"];
		for (usize i = 0; i < jsonAxes.Count(); i++) {
			JsonValue& jsonAxis = jsonAxes[i];
            ControllerAxis axis = pState->stringToControllerAxis[Fnv1a::Hash(jsonAxis["Name"].ToString().pData)];

            // TODO: What if someone tries to bind a controller button(s) to an axis? Support that probably
            SDL_GameControllerAxis primaryBinding = pState->stringToSDLControllerAxis[Fnv1a::Hash(jsonAxis["Primary"].ToString().pData)];
            pState->primaryAxisBindings[primaryBinding] = axis;

            if (jsonAxis.HasKey("Alt")) {
                String altBindingLabel = jsonAxis["Alt"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = pState->stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pState->keyboardAxisBindings[keycode] = axis;
                    pState->axes[(usize)axis].positiveScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = pState->stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pState->mouseAxisBindings[mousecode] = axis;
                    pState->axes[(usize)axis].positiveMouseButton = mousecode;
                }
            }
            if (jsonAxis.HasKey("AltPositive")) {
                String altBindingLabel = jsonAxis["AltPositive"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = pState->stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pState->keyboardAxisBindings[keycode] = axis;
                    pState->axes[(usize)axis].positiveScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = pState->stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pState->mouseAxisBindings[mousecode] = axis;
                    pState->axes[(usize)axis].positiveMouseButton = mousecode;
                }
            }
            if (jsonAxis.HasKey("AltNegative")) {
                String altBindingLabel = jsonAxis["AltNegative"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = pState->stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pState->keyboardAxisBindings[keycode] = axis;
                    pState->axes[(usize)axis].negativeScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = pState->stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pState->mouseAxisBindings[mousecode] = axis;
                    pState->axes[(usize)axis].negativeMouseButton = mousecode;
                }
            }
        }
    }

    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    SDL_StartTextInput();

    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            const char* name = SDL_GameControllerNameForIndex(i);
            Log::Info("Using first detected controller: %s", name);
            pState->pOpenController = SDL_GameControllerOpen(i);
        } else {
            const char* name = SDL_JoystickNameForIndex(i);
            Log::Info("Detected Joystick: %s", name);
        }
    }
}

// ***********************************************************************

void ProcessEvent(SDL_Event* event) {
    // Update Input States
    switch (event->type) {
        case SDL_TEXTINPUT: {
            pState->textInputString.Append(event->text.text);
            break;
        }
        case SDL_KEYDOWN: {
            SDL_Keycode keycode = event->key.keysym.sym;
            pState->keyDowns[(usize)pState->keyCodeToInternalKeyCode[keycode]] = true;
            pState->keyStates[(usize)pState->keyCodeToInternalKeyCode[keycode]] = true;

            ControllerButton button = pState->keyboardAltBindings[keycode];
            if (button != ControllerButton::Invalid) {
                pState->buttonDowns[(usize)button] = true;
                pState->buttonStates[(usize)button] = true;
            }
            ControllerAxis axis = pState->keyboardAxisBindings[keycode];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = pState->axes[(usize)axis];
                if (axisData.positiveScanCode == keycode)
                    axisData.positiveInput = true;
                else if (axisData.negativeScanCode == keycode)
                    axisData.negativeInput = true;
                axisData.ignoreVirtual = false;
            }
            break;
        }
        case SDL_KEYUP: {
            SDL_Keycode keycode = event->key.keysym.sym;
            pState->keyUps[(usize)pState->keyCodeToInternalKeyCode[keycode]] = true;
            pState->keyStates[(usize)pState->keyCodeToInternalKeyCode[keycode]] = false;

            ControllerButton button = pState->keyboardAltBindings[keycode];
            if (button != ControllerButton::Invalid) {
                pState->buttonUps[(usize)button] = true;
                pState->buttonStates[(usize)button] = false;
            }
            ControllerAxis axis = pState->keyboardAxisBindings[keycode];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = pState->axes[(usize)axis];
                if (axisData.positiveScanCode == keycode)
                    axisData.positiveInput = false;
                else if (axisData.negativeScanCode == keycode)
                    axisData.negativeInput = false;
                axisData.ignoreVirtual = false;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN: {
            SDL_GameControllerButton sdlButton = (SDL_GameControllerButton)event->cbutton.button;
            ControllerButton button = pState->primaryBindings[sdlButton];
            if (button != ControllerButton::Invalid) {
                pState->buttonDowns[(usize)button] = true;
                pState->buttonStates[(usize)button] = true;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP: {
            SDL_GameControllerButton sdlButton = (SDL_GameControllerButton)event->cbutton.button;
            ControllerButton button = pState->primaryBindings[sdlButton];
            if (button != ControllerButton::Invalid) {
                pState->buttonUps[(usize)button] = true;
                pState->buttonStates[(usize)button] = false;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            i32 sdlMouseButton = event->button.button;
            ControllerButton button = pState->mouseAltBindings[sdlMouseButton];
            if (button != ControllerButton::Invalid) {
                pState->buttonDowns[(usize)button] = true;
                pState->buttonStates[(usize)button] = true;
            }
            ControllerAxis axis = pState->mouseAxisBindings[sdlMouseButton];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = pState->axes[(usize)axis];
                if (axisData.positiveMouseButton == sdlMouseButton)
                    axisData.positiveInput = true;
                else if (axisData.negativeMouseButton == sdlMouseButton)
                    axisData.negativeInput = true;
                axisData.ignoreVirtual = false;
            }
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            i32 sdlMouseButton = event->button.button;
            ControllerButton button = pState->mouseAltBindings[sdlMouseButton];
            if (button != ControllerButton::Invalid) {
                pState->buttonUps[(usize)button] = true;
                pState->buttonStates[(usize)button] = false;
            }
            ControllerAxis axis = pState->mouseAxisBindings[sdlMouseButton];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = pState->axes[(usize)axis];
                if (axisData.positiveMouseButton == sdlMouseButton)
                    axisData.positiveInput = false;
                else if (axisData.negativeMouseButton == sdlMouseButton)
                    axisData.negativeInput = false;
                axisData.ignoreVirtual = false;
            }
            break;
        }
        case SDL_MOUSEMOTION: {
            f32 mouseSensitivity = 0.08f;  // TODO: Make configurable
            SDL_MouseMotionEvent motionEvent = event->motion;
            ControllerAxis axis;
            if (abs(motionEvent.xrel) > 0) {
                axis = pState->mouseAxisBindings[pState->stringToMouseCode["Mouse_AxisX"_hash]];
                pState->axes[(usize)axis].axisValue = (f32)motionEvent.xrel * mouseSensitivity;
                pState->axes[(usize)axis].ignoreVirtual = true;
                pState->axes[(usize)axis].isMouseDriver = true;
            }
            if (abs(motionEvent.yrel) > 0) {
                axis = pState->mouseAxisBindings[pState->stringToMouseCode["Mouse_AxisY"_hash]];
                pState->axes[(usize)axis].axisValue = (f32)motionEvent.yrel * mouseSensitivity;
                pState->axes[(usize)axis].ignoreVirtual = true;
                pState->axes[(usize)axis].isMouseDriver = true;
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION: {
            SDL_ControllerAxisEvent axisEvent = event->caxis;
            ControllerAxis axis = pState->primaryAxisBindings[(SDL_GameControllerAxis)axisEvent.axis];
            pState->axes[(usize)axis].axisValue = (f32)axisEvent.value / 32768.f;
            pState->axes[(usize)axis].ignoreVirtual = true;
            pState->axes[(usize)axis].isMouseDriver = false;
            break;
        }
        default: break;
    }
}

// ***********************************************************************

void UpdateInputs(f32 deltaTime, Vec2f targetRes, Vec2f realWindowRes) {
    pState->targetResolution = targetRes;
    pState->windowResolution = realWindowRes;

    // TODO: This should be configurable
    f32 gravity = 1.0f;
    f32 sensitivity = 1.0f;
    f32 deadzone = 0.09f;

    for (usize axisIndex = 0; axisIndex < (usize)ControllerAxis::Count; axisIndex++) {
        ControllerAxis axisEnum = (ControllerAxis)axisIndex;
        Axis& axis = pState->axes[axisIndex];

        if (axis.isMouseDriver)
            continue;

        if (axis.ignoreVirtual) {
            if (fabs(axis.axisValue) <= deadzone)
                axis.axisValue = 0.0f;
            continue;
        }

        if (axis.positiveInput) {
            axis.axisValue += sensitivity * deltaTime;
        }
        if (axis.negativeInput) {
            axis.axisValue -= sensitivity * deltaTime;
        }
        if (!axis.negativeInput && !axis.positiveInput) {
            axis.axisValue += (0 - axis.axisValue) * gravity * deltaTime;
            if (fabs(axis.axisValue) <= deadzone)
                axis.axisValue = 0.0f;
        }

        if (axisEnum == ControllerAxis::TriggerLeft || axisEnum == ControllerAxis::TriggerRight)  // Triggers are special
            axis.axisValue = clamp(axis.axisValue, 0.f, 1.f);
        else
            axis.axisValue = clamp(axis.axisValue, -1.f, 1.f);
    }
}

// ***********************************************************************

void ClearStates() {
    for (int i = 0; i < (int)Key::Count; i++)
        pState->keyDowns[i] = false;
    for (int i = 0; i < (int)Key::Count; i++)
        pState->keyUps[i] = false;

    for (int i = 0; i < (int)ControllerButton::Count; i++)
        pState->buttonDowns[i] = false;
    for (int i = 0; i < (int)ControllerButton::Count; i++)
        pState->buttonUps[i] = false;

    pState->textInputString.Reset();
    for (Axis& axis : pState->axes) {
        if (axis.isMouseDriver) {
            axis.axisValue = 0.0f;
        }
    }
}

// ***********************************************************************

void Shutdown() {
    if (pState->pOpenController) {
        SDL_GameControllerClose(pState->pOpenController);
    }
	ArenaFinished(pState->pArena);
}

// ***********************************************************************

bool GetButton(ControllerButton buttonCode) {
    return pState->buttonStates[(usize)buttonCode];
}

// ***********************************************************************

bool GetButtonDown(ControllerButton buttonCode) {
    return pState->buttonDowns[(usize)buttonCode];
}

// ***********************************************************************

bool GetButtonUp(ControllerButton buttonCode) {
    return pState->buttonUps[(usize)buttonCode];
}

// ***********************************************************************

f32 GetAxis(ControllerAxis axis) {
    return pState->axes[(usize)axis].axisValue;
}

// ***********************************************************************

Vec2i GetMousePosition() {
    // TODO: This becomes wrong at the edge of the screen, must apply screen warning to it as well
    Vec2i mousePosition;
    SDL_GetMouseState(&mousePosition.x, &mousePosition.y);
    f32 xAdjusted = (f32)mousePosition.x / pState->windowResolution.x * pState->targetResolution.x;
    f32 yAdjusted = pState->targetResolution.y - ((f32)mousePosition.y / pState->windowResolution.y * pState->targetResolution.y);
    return Vec2i((int)xAdjusted, (int)yAdjusted);
}

// ***********************************************************************

void EnableMouseRelativeMode(bool enable) {
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
}

// ***********************************************************************

bool GetKey(Key keyCode) {
    return pState->keyStates[(usize)keyCode];
}

// ***********************************************************************

bool GetKeyDown(Key keyCode) {
    return pState->keyDowns[(usize)keyCode];
}

// ***********************************************************************

bool GetKeyUp(Key keyCode) {
    return pState->keyUps[(usize)keyCode];
}

// ***********************************************************************

String InputString() {
    return pState->textInputString.CreateString(g_pArenaFrame);
}
