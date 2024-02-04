// Copyright 2020-2022 David Colson. All rights reserved.

#include "game_chip.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_joystick.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>
#include <SDL_syswm.h>
#include <defer.h>
#include <json.h>
#include <maths.h>
#include <string_hash.h>
#include <hashmap.inl>
#include <log.h>
#undef DrawText
#undef DrawTextEx

namespace {
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

// Mapping of strings to SDL controller axes
HashMap<u32, SDL_GameControllerAxis> stringToSDLControllerAxis;
}

// ***********************************************************************

void GameChip::Init() {
    // Setup hashmaps for controller labels
    stringToControllerButton.Add("FaceBottom"_hash, ControllerButton::FaceBottom);
    stringToControllerButton.Add("FaceRight"_hash, ControllerButton::FaceRight);
    stringToControllerButton.Add("FaceLeft"_hash, ControllerButton::FaceLeft);
    stringToControllerButton.Add("FaceTop"_hash, ControllerButton::FaceTop);
    stringToControllerButton.Add("LeftStick"_hash, ControllerButton::LeftStick);
    stringToControllerButton.Add("RightStick"_hash, ControllerButton::RightStick);
    stringToControllerButton.Add("LeftShoulder"_hash, ControllerButton::LeftShoulder);
    stringToControllerButton.Add("RightShoulder"_hash, ControllerButton::RightShoulder);
    stringToControllerButton.Add("DpadDown"_hash, ControllerButton::DpadDown);
    stringToControllerButton.Add("DpadLeft"_hash, ControllerButton::DpadLeft);
    stringToControllerButton.Add("DpadRight"_hash, ControllerButton::DpadRight);
    stringToControllerButton.Add("DpadUp"_hash, ControllerButton::DpadUp);
    stringToControllerButton.Add("Start"_hash, ControllerButton::Start);
    stringToControllerButton.Add("Select"_hash, ControllerButton::Select);


    stringToControllerAxis.Add("LeftX"_hash, ControllerAxis::LeftX);
    stringToControllerAxis.Add("LeftY"_hash, ControllerAxis::LeftY);
    stringToControllerAxis.Add("RightX"_hash, ControllerAxis::RightX);
    stringToControllerAxis.Add("RightY"_hash, ControllerAxis::RightY);
    stringToControllerAxis.Add("TriggerLeft"_hash, ControllerAxis::TriggerLeft);
    stringToControllerAxis.Add("TriggerRight"_hash, ControllerAxis::TriggerRight);


    stringToKeyCode.Add("Keycode_A"_hash, SDLK_a);
    stringToKeyCode.Add("Keycode_B"_hash, SDLK_b);
    stringToKeyCode.Add("Keycode_C"_hash, SDLK_c);
    stringToKeyCode.Add("Keycode_D"_hash, SDLK_d);
    stringToKeyCode.Add("Keycode_E"_hash, SDLK_e);
    stringToKeyCode.Add("Keycode_F"_hash, SDLK_f);
    stringToKeyCode.Add("Keycode_G"_hash, SDLK_g);
    stringToKeyCode.Add("Keycode_H"_hash, SDLK_h);
    stringToKeyCode.Add("Keycode_I"_hash, SDLK_i);
    stringToKeyCode.Add("Keycode_J"_hash, SDLK_j);
    stringToKeyCode.Add("Keycode_K"_hash, SDLK_k);
    stringToKeyCode.Add("Keycode_L"_hash, SDLK_l);
    stringToKeyCode.Add("Keycode_M"_hash, SDLK_m);
    stringToKeyCode.Add("Keycode_N"_hash, SDLK_n);
    stringToKeyCode.Add("Keycode_O"_hash, SDLK_o);
    stringToKeyCode.Add("Keycode_P"_hash, SDLK_p);
    stringToKeyCode.Add("Keycode_Q"_hash, SDLK_q);
    stringToKeyCode.Add("Keycode_R"_hash, SDLK_e);
    stringToKeyCode.Add("Keycode_S"_hash, SDLK_s);
    stringToKeyCode.Add("Keycode_T"_hash, SDLK_t);
    stringToKeyCode.Add("Keycode_U"_hash, SDLK_u);
    stringToKeyCode.Add("Keycode_V"_hash, SDLK_v);
    stringToKeyCode.Add("Keycode_W"_hash, SDLK_w);
    stringToKeyCode.Add("Keycode_X"_hash, SDLK_x);
    stringToKeyCode.Add("Keycode_Y"_hash, SDLK_y);
    stringToKeyCode.Add("Keycode_Z"_hash, SDLK_z);

    stringToKeyCode.Add("Keycode_1"_hash, SDLK_1);
    stringToKeyCode.Add("Keycode_2"_hash, SDLK_2);
    stringToKeyCode.Add("Keycode_3"_hash, SDLK_3);
    stringToKeyCode.Add("Keycode_4"_hash, SDLK_4);
    stringToKeyCode.Add("Keycode_5"_hash, SDLK_5);
    stringToKeyCode.Add("Keycode_6"_hash, SDLK_6);
    stringToKeyCode.Add("Keycode_7"_hash, SDLK_7);
    stringToKeyCode.Add("Keycode_8"_hash, SDLK_8);
    stringToKeyCode.Add("Keycode_9"_hash, SDLK_9);
    stringToKeyCode.Add("Keycode_0"_hash, SDLK_0);

    stringToKeyCode.Add("Keycode_Return"_hash, SDLK_RETURN);
    stringToKeyCode.Add("Keycode_Escape"_hash, SDLK_ESCAPE);
    stringToKeyCode.Add("Keycode_Backspace"_hash, SDLK_BACKSPACE);
    stringToKeyCode.Add("Keycode_Tab"_hash, SDLK_TAB);
    stringToKeyCode.Add("Keycode_Space"_hash, SDLK_SPACE);
    stringToKeyCode.Add("Keycode_Exclaim"_hash, SDLK_EXCLAIM);
    stringToKeyCode.Add("Keycode_QuoteDbl"_hash, SDLK_QUOTEDBL);
    stringToKeyCode.Add("Keycode_Hash"_hash, SDLK_HASH);
    stringToKeyCode.Add("Keycode_Percent"_hash, SDLK_PERCENT);
    stringToKeyCode.Add("Keycode_Dollar"_hash, SDLK_DOLLAR);
    stringToKeyCode.Add("Keycode_Ampersand"_hash, SDLK_AMPERSAND);
    stringToKeyCode.Add("Keycode_Quote"_hash, SDLK_QUOTE);
    stringToKeyCode.Add("Keycode_LeftParen"_hash, SDLK_LEFTPAREN);
    stringToKeyCode.Add("Keycode_RightParen"_hash, SDLK_RIGHTPAREN);
    stringToKeyCode.Add("Keycode_Asterisk"_hash, SDLK_ASTERISK);
    stringToKeyCode.Add("Keycode_Plus"_hash, SDLK_PLUS);
    stringToKeyCode.Add("Keycode_Comma"_hash, SDLK_COMMA);
    stringToKeyCode.Add("Keycode_Minus"_hash, SDLK_MINUS);
    stringToKeyCode.Add("Keycode_Period"_hash, SDLK_PERIOD);
    stringToKeyCode.Add("Keycode_Slash"_hash, SDLK_SLASH);
    stringToKeyCode.Add("Keycode_Colon"_hash, SDLK_COLON);
    stringToKeyCode.Add("Keycode_Semicolon"_hash, SDLK_SEMICOLON);
    stringToKeyCode.Add("Keycode_Less"_hash, SDLK_LESS);
    stringToKeyCode.Add("Keycode_Equals"_hash, SDLK_EQUALS);
    stringToKeyCode.Add("Keycode_Greater"_hash, SDLK_GREATER);
    stringToKeyCode.Add("Keycode_Question"_hash, SDLK_QUESTION);
    stringToKeyCode.Add("Keycode_At"_hash, SDLK_AT);
    stringToKeyCode.Add("Keycode_LeftBracket"_hash, SDLK_LEFTBRACKET);
    stringToKeyCode.Add("Keycode_Backslash"_hash, SDLK_BACKSLASH);
    stringToKeyCode.Add("Keycode_RightBracket"_hash, SDLK_RIGHTBRACKET);
    stringToKeyCode.Add("Keycode_Caret"_hash, SDLK_CARET);
    stringToKeyCode.Add("Keycode_Underscore"_hash, SDLK_UNDERSCORE);
    stringToKeyCode.Add("Keycode_BackQuote"_hash, SDLK_BACKQUOTE);

    stringToKeyCode.Add("Keycode_CapsLock"_hash, SDLK_CAPSLOCK);

    stringToKeyCode.Add("Keycode_F1"_hash, SDLK_F1);
    stringToKeyCode.Add("Keycode_F2"_hash, SDLK_F2);
    stringToKeyCode.Add("Keycode_F3"_hash, SDLK_F3);
    stringToKeyCode.Add("Keycode_F4"_hash, SDLK_F4);
    stringToKeyCode.Add("Keycode_F5"_hash, SDLK_F5);
    stringToKeyCode.Add("Keycode_F6"_hash, SDLK_F6);
    stringToKeyCode.Add("Keycode_F7"_hash, SDLK_F7);
    stringToKeyCode.Add("Keycode_F8"_hash, SDLK_F8);
    stringToKeyCode.Add("Keycode_F9"_hash, SDLK_F9);
    stringToKeyCode.Add("Keycode_F10"_hash, SDLK_F10);
    stringToKeyCode.Add("Keycode_F11"_hash, SDLK_F11);
    stringToKeyCode.Add("Keycode_F12"_hash, SDLK_F12);

    stringToKeyCode.Add("Keycode_PrintScreen"_hash, SDLK_PRINTSCREEN);
    stringToKeyCode.Add("Keycode_ScrollLock"_hash, SDLK_SCROLLLOCK);
    stringToKeyCode.Add("Keycode_Pause"_hash, SDLK_PAUSE);
    stringToKeyCode.Add("Keycode_Insert"_hash, SDLK_INSERT);
    stringToKeyCode.Add("Keycode_Home"_hash, SDLK_HOME);
    stringToKeyCode.Add("Keycode_PageUp"_hash, SDLK_PAGEUP);
    stringToKeyCode.Add("Keycode_Delete"_hash, SDLK_DELETE);
    stringToKeyCode.Add("Keycode_End"_hash, SDLK_END);
    stringToKeyCode.Add("Keycode_PageDown"_hash, SDLK_PAGEDOWN);
    stringToKeyCode.Add("Keycode_Right"_hash, SDLK_RIGHT);
    stringToKeyCode.Add("Keycode_Left"_hash, SDLK_LEFT);
    stringToKeyCode.Add("Keycode_Down"_hash, SDLK_DOWN);
    stringToKeyCode.Add("Keycode_Up"_hash, SDLK_UP);

    stringToKeyCode.Add("Keycode_NumLock"_hash, SDLK_NUMLOCKCLEAR);
    stringToKeyCode.Add("Keycode_KpDivide"_hash, SDLK_KP_DIVIDE);
    stringToKeyCode.Add("Keycode_KpMultiply"_hash, SDLK_KP_MULTIPLY);
    stringToKeyCode.Add("Keycode_KpMinus"_hash, SDLK_KP_MINUS);
    stringToKeyCode.Add("Keycode_KpPlus"_hash, SDLK_KP_PLUS);
    stringToKeyCode.Add("Keycode_KpEnter"_hash, SDLK_KP_ENTER);
    stringToKeyCode.Add("Keycode_Kp1"_hash, SDLK_KP_1);
    stringToKeyCode.Add("Keycode_Kp2"_hash, SDLK_KP_2);
    stringToKeyCode.Add("Keycode_Kp3"_hash, SDLK_KP_3);
    stringToKeyCode.Add("Keycode_Kp4"_hash, SDLK_KP_4);
    stringToKeyCode.Add("Keycode_Kp5"_hash, SDLK_KP_5);
    stringToKeyCode.Add("Keycode_Kp6"_hash, SDLK_KP_6);
    stringToKeyCode.Add("Keycode_Kp7"_hash, SDLK_KP_7);
    stringToKeyCode.Add("Keycode_Kp8"_hash, SDLK_KP_8);
    stringToKeyCode.Add("Keycode_Kp9"_hash, SDLK_KP_9);
    stringToKeyCode.Add("Keycode_Kp0"_hash, SDLK_KP_0);
    stringToKeyCode.Add("Keycode_KpPeriod"_hash, SDLK_KP_PERIOD);

    stringToKeyCode.Add("Keycode_LeftCtrl"_hash, SDLK_LCTRL);
    stringToKeyCode.Add("Keycode_LeftShift"_hash, SDLK_LSHIFT);
    stringToKeyCode.Add("Keycode_LeftAlt"_hash, SDLK_LALT);
    stringToKeyCode.Add("Keycode_LeftGui"_hash, SDLK_LGUI);
    stringToKeyCode.Add("Keycode_RightCtrl"_hash, SDLK_RCTRL);
    stringToKeyCode.Add("Keycode_RightShift"_hash, SDLK_RSHIFT);
    stringToKeyCode.Add("Keycode_RightAlt"_hash, SDLK_RALT);
    stringToKeyCode.Add("Keycode_RightGui"_hash, SDLK_RGUI);


    keyCodeToInternalKeyCode.Add(SDLK_a, Key::A);
    keyCodeToInternalKeyCode.Add(SDLK_b, Key::B);
    keyCodeToInternalKeyCode.Add(SDLK_c, Key::C);
    keyCodeToInternalKeyCode.Add(SDLK_d, Key::D);
    keyCodeToInternalKeyCode.Add(SDLK_e, Key::E);
    keyCodeToInternalKeyCode.Add(SDLK_f, Key::F);
    keyCodeToInternalKeyCode.Add(SDLK_g, Key::G);
    keyCodeToInternalKeyCode.Add(SDLK_h, Key::H);
    keyCodeToInternalKeyCode.Add(SDLK_i, Key::I);
    keyCodeToInternalKeyCode.Add(SDLK_j, Key::J);
    keyCodeToInternalKeyCode.Add(SDLK_k, Key::K);
    keyCodeToInternalKeyCode.Add(SDLK_l, Key::L);
    keyCodeToInternalKeyCode.Add(SDLK_m, Key::M);
    keyCodeToInternalKeyCode.Add(SDLK_n, Key::N);
    keyCodeToInternalKeyCode.Add(SDLK_o, Key::O);
    keyCodeToInternalKeyCode.Add(SDLK_p, Key::P);
    keyCodeToInternalKeyCode.Add(SDLK_q, Key::Q);
    keyCodeToInternalKeyCode.Add(SDLK_e, Key::R);
    keyCodeToInternalKeyCode.Add(SDLK_s, Key::S);
    keyCodeToInternalKeyCode.Add(SDLK_t, Key::T);
    keyCodeToInternalKeyCode.Add(SDLK_u, Key::U);
    keyCodeToInternalKeyCode.Add(SDLK_v, Key::V);
    keyCodeToInternalKeyCode.Add(SDLK_w, Key::W);
    keyCodeToInternalKeyCode.Add(SDLK_x, Key::X);
    keyCodeToInternalKeyCode.Add(SDLK_y, Key::Y);
    keyCodeToInternalKeyCode.Add(SDLK_z, Key::Z);

    keyCodeToInternalKeyCode.Add(SDLK_1, Key::No1);
    keyCodeToInternalKeyCode.Add(SDLK_2, Key::No2);
    keyCodeToInternalKeyCode.Add(SDLK_3, Key::No3);
    keyCodeToInternalKeyCode.Add(SDLK_4, Key::No4);
    keyCodeToInternalKeyCode.Add(SDLK_5, Key::No5);
    keyCodeToInternalKeyCode.Add(SDLK_6, Key::No6);
    keyCodeToInternalKeyCode.Add(SDLK_7, Key::No7);
    keyCodeToInternalKeyCode.Add(SDLK_8, Key::No8);
    keyCodeToInternalKeyCode.Add(SDLK_9, Key::No9);
    keyCodeToInternalKeyCode.Add(SDLK_0, Key::No0);

    keyCodeToInternalKeyCode.Add(SDLK_RETURN, Key::Return);
    keyCodeToInternalKeyCode.Add(SDLK_ESCAPE, Key::Escape);
    keyCodeToInternalKeyCode.Add(SDLK_BACKSPACE, Key::Backspace);
    keyCodeToInternalKeyCode.Add(SDLK_TAB, Key::Tab);
    keyCodeToInternalKeyCode.Add(SDLK_SPACE, Key::Space);
    keyCodeToInternalKeyCode.Add(SDLK_EXCLAIM, Key::Exclaim);
    keyCodeToInternalKeyCode.Add(SDLK_QUOTEDBL, Key::QuoteDbl);
    keyCodeToInternalKeyCode.Add(SDLK_HASH, Key::Hash);
    keyCodeToInternalKeyCode.Add(SDLK_PERCENT, Key::Percent);
    keyCodeToInternalKeyCode.Add(SDLK_DOLLAR, Key::Dollar);
    keyCodeToInternalKeyCode.Add(SDLK_AMPERSAND, Key::Ampersand);
    keyCodeToInternalKeyCode.Add(SDLK_QUOTE, Key::Quote);
    keyCodeToInternalKeyCode.Add(SDLK_LEFTPAREN, Key::LeftParen);
    keyCodeToInternalKeyCode.Add(SDLK_RIGHTPAREN, Key::RightParen);
    keyCodeToInternalKeyCode.Add(SDLK_ASTERISK, Key::Asterisk);
    keyCodeToInternalKeyCode.Add(SDLK_PLUS, Key::Plus);
    keyCodeToInternalKeyCode.Add(SDLK_COMMA, Key::Comma);
    keyCodeToInternalKeyCode.Add(SDLK_MINUS, Key::Minus);
    keyCodeToInternalKeyCode.Add(SDLK_PERIOD, Key::Period);
    keyCodeToInternalKeyCode.Add(SDLK_SLASH, Key::Slash);
    keyCodeToInternalKeyCode.Add(SDLK_COLON, Key::Colon);
    keyCodeToInternalKeyCode.Add(SDLK_SEMICOLON, Key::Semicolon);
    keyCodeToInternalKeyCode.Add(SDLK_LESS, Key::Less);
    keyCodeToInternalKeyCode.Add(SDLK_EQUALS, Key::Equals);
    keyCodeToInternalKeyCode.Add(SDLK_GREATER, Key::Greater);
    keyCodeToInternalKeyCode.Add(SDLK_QUESTION, Key::Question);
    keyCodeToInternalKeyCode.Add(SDLK_AT, Key::At);
    keyCodeToInternalKeyCode.Add(SDLK_LEFTBRACKET, Key::LeftBracket);
    keyCodeToInternalKeyCode.Add(SDLK_BACKSLASH, Key::Backslash);
    keyCodeToInternalKeyCode.Add(SDLK_RIGHTBRACKET, Key::RightBracket);
    keyCodeToInternalKeyCode.Add(SDLK_CARET, Key::Caret);
    keyCodeToInternalKeyCode.Add(SDLK_UNDERSCORE, Key::Underscore);
    keyCodeToInternalKeyCode.Add(SDLK_BACKQUOTE, Key::BackQuote);

    keyCodeToInternalKeyCode.Add(SDLK_CAPSLOCK, Key::CapsLock);

    keyCodeToInternalKeyCode.Add(SDLK_F1, Key::F1);
    keyCodeToInternalKeyCode.Add(SDLK_F2, Key::F2);
    keyCodeToInternalKeyCode.Add(SDLK_F3, Key::F3);
    keyCodeToInternalKeyCode.Add(SDLK_F4, Key::F4);
    keyCodeToInternalKeyCode.Add(SDLK_F5, Key::F5);
    keyCodeToInternalKeyCode.Add(SDLK_F6, Key::F6);
    keyCodeToInternalKeyCode.Add(SDLK_F7, Key::F7);
    keyCodeToInternalKeyCode.Add(SDLK_F8, Key::F8);
    keyCodeToInternalKeyCode.Add(SDLK_F9, Key::F9);
    keyCodeToInternalKeyCode.Add(SDLK_F10, Key::F10);
    keyCodeToInternalKeyCode.Add(SDLK_F11, Key::F11);
    keyCodeToInternalKeyCode.Add(SDLK_F12, Key::F12);

    keyCodeToInternalKeyCode.Add(SDLK_PRINTSCREEN, Key::PrintScreen);
    keyCodeToInternalKeyCode.Add(SDLK_SCROLLLOCK, Key::ScrollLock);
    keyCodeToInternalKeyCode.Add(SDLK_PAUSE, Key::Pause);
    keyCodeToInternalKeyCode.Add(SDLK_INSERT, Key::Insert);
    keyCodeToInternalKeyCode.Add(SDLK_HOME, Key::Home);
    keyCodeToInternalKeyCode.Add(SDLK_PAGEUP, Key::PageUp);
    keyCodeToInternalKeyCode.Add(SDLK_DELETE, Key::Delete);
    keyCodeToInternalKeyCode.Add(SDLK_END, Key::End);
    keyCodeToInternalKeyCode.Add(SDLK_PAGEDOWN, Key::PageDown);
    keyCodeToInternalKeyCode.Add(SDLK_RIGHT, Key::Right);
    keyCodeToInternalKeyCode.Add(SDLK_LEFT, Key::Left);
    keyCodeToInternalKeyCode.Add(SDLK_DOWN, Key::Down);
    keyCodeToInternalKeyCode.Add(SDLK_UP, Key::Up);

    keyCodeToInternalKeyCode.Add(SDLK_NUMLOCKCLEAR, Key::NumLock);
    keyCodeToInternalKeyCode.Add(SDLK_KP_DIVIDE, Key::KpDivide);
    keyCodeToInternalKeyCode.Add(SDLK_KP_MULTIPLY, Key::KpMultiply);
    keyCodeToInternalKeyCode.Add(SDLK_KP_MINUS, Key::KpMinus);
    keyCodeToInternalKeyCode.Add(SDLK_KP_PLUS, Key::KpPlus);
    keyCodeToInternalKeyCode.Add(SDLK_KP_ENTER, Key::KpEnter);
    keyCodeToInternalKeyCode.Add(SDLK_KP_1, Key::Kp1);
    keyCodeToInternalKeyCode.Add(SDLK_KP_2, Key::Kp2);
    keyCodeToInternalKeyCode.Add(SDLK_KP_3, Key::Kp3);
    keyCodeToInternalKeyCode.Add(SDLK_KP_4, Key::Kp4);
    keyCodeToInternalKeyCode.Add(SDLK_KP_5, Key::Kp5);
    keyCodeToInternalKeyCode.Add(SDLK_KP_6, Key::Kp6);
    keyCodeToInternalKeyCode.Add(SDLK_KP_7, Key::Kp7);
    keyCodeToInternalKeyCode.Add(SDLK_KP_8, Key::Kp8);
    keyCodeToInternalKeyCode.Add(SDLK_KP_9, Key::Kp9);
    keyCodeToInternalKeyCode.Add(SDLK_KP_0, Key::Kp0);
    keyCodeToInternalKeyCode.Add(SDLK_KP_PERIOD, Key::KpPeriod);

    keyCodeToInternalKeyCode.Add(SDLK_LCTRL, Key::LeftCtrl);
    keyCodeToInternalKeyCode.Add(SDLK_LSHIFT, Key::LeftShift);
    keyCodeToInternalKeyCode.Add(SDLK_LALT, Key::LeftAlt);
    keyCodeToInternalKeyCode.Add(SDLK_LGUI, Key::LeftGui);
    keyCodeToInternalKeyCode.Add(SDLK_RCTRL, Key::RightCtrl);
    keyCodeToInternalKeyCode.Add(SDLK_RSHIFT, Key::RightShift);
    keyCodeToInternalKeyCode.Add(SDLK_RALT, Key::RightAlt);
    keyCodeToInternalKeyCode.Add(SDLK_RGUI, Key::RightGui);


    stringToMouseCode.Add("Mouse_Button0"_hash, SDL_BUTTON_LEFT);
    stringToMouseCode.Add("Mouse_Button1"_hash, SDL_BUTTON_MIDDLE);
    stringToMouseCode.Add("Mouse_Button2"_hash, SDL_BUTTON_RIGHT);
    stringToMouseCode.Add("Mouse_AxisY"_hash, 128);
    stringToMouseCode.Add("Mouse_AxisX"_hash, 127);


    stringToSDLControllerButton.Add("Controller_A"_hash, SDL_CONTROLLER_BUTTON_A);
    stringToSDLControllerButton.Add("Controller_B"_hash, SDL_CONTROLLER_BUTTON_B);
    stringToSDLControllerButton.Add("Controller_X"_hash, SDL_CONTROLLER_BUTTON_X);
    stringToSDLControllerButton.Add("Controller_Y"_hash, SDL_CONTROLLER_BUTTON_Y);
    stringToSDLControllerButton.Add("Controller_LeftStick"_hash, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    stringToSDLControllerButton.Add("Controller_RightStick"_hash, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    stringToSDLControllerButton.Add("Controller_LeftShoulder"_hash, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    stringToSDLControllerButton.Add("Controller_RightShoulder"_hash, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    stringToSDLControllerButton.Add("Controller_DpadUp"_hash, SDL_CONTROLLER_BUTTON_DPAD_UP);
    stringToSDLControllerButton.Add("Controller_DpadDown"_hash, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    stringToSDLControllerButton.Add("Controller_DpadLeft"_hash, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    stringToSDLControllerButton.Add("Controller_DpadRight"_hash, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    stringToSDLControllerButton.Add("Controller_Start"_hash, SDL_CONTROLLER_BUTTON_START);
    stringToSDLControllerButton.Add("Controller_Select"_hash, SDL_CONTROLLER_BUTTON_BACK);


    stringToSDLControllerAxis.Add("Controller_LeftX"_hash, SDL_CONTROLLER_AXIS_LEFTX);
    stringToSDLControllerAxis.Add("Controller_LeftY"_hash, SDL_CONTROLLER_AXIS_LEFTY);
    stringToSDLControllerAxis.Add("Controller_RightX"_hash, SDL_CONTROLLER_AXIS_RIGHTX);
    stringToSDLControllerAxis.Add("Controller_RightY"_hash, SDL_CONTROLLER_AXIS_RIGHTY);
    stringToSDLControllerAxis.Add("Controller_TriggerLeft"_hash, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    stringToSDLControllerAxis.Add("Controller_TriggerRight"_hash, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);


    // Load json mapping file
    SDL_RWops* pFileRead = SDL_RWFromFile("assets/ControllerMapping.json", "rb");
    u64 size = SDL_RWsize(pFileRead);
    char* pData = (char*)g_Allocator.Allocate(size * sizeof(char));
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    String file;
    defer(FreeString(file));
    file.pData = pData;
    file.length = size;
    JsonValue mapping = ParseJsonFile(file);
    defer(mapping.Free());

    // Going through all elements of the json file
    if (mapping.HasKey("Buttons")) {
        JsonValue& buttons = mapping["Buttons"];
        for (usize i = 0; i < buttons.Count(); i++) {
            JsonValue& jsonButton = buttons[i];
            ControllerButton button = stringToControllerButton[Fnv1a::Hash(jsonButton["Name"].ToString().pData)];
            SDL_GameControllerButton primaryBinding = stringToSDLControllerButton[Fnv1a::Hash(jsonButton["Primary"].ToString().pData)];
            primaryBindings[primaryBinding] = button;

            String altBindingLabel = jsonButton["Alt"].ToString();
            if (altBindingLabel.SubStr(0, 5) == "Keyco") {
                SDL_Keycode keycode = stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                keyboardAltBindings[keycode] = button;
            } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                i32 mousecode = stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                mouseAltBindings[mousecode] = button;
            }
        }
    }
    if (mapping.HasKey("Axes")) {
        JsonValue& jsonAxes = mapping["Axes"];
		for (usize i = 0; i < jsonAxes.Count(); i++) {
			JsonValue& jsonAxis = jsonAxes[i];
            ControllerAxis axis = stringToControllerAxis[Fnv1a::Hash(jsonAxis["Name"].ToString().pData)];

            // TODO: What if someone tries to bind a controller button(s) to an axis? Support that probably
            SDL_GameControllerAxis primaryBinding = stringToSDLControllerAxis[Fnv1a::Hash(jsonAxis["Primary"].ToString().pData)];
            primaryAxisBindings[primaryBinding] = axis;

            if (jsonAxis.HasKey("Alt")) {
                String altBindingLabel = jsonAxis["Alt"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    keyboardAxisBindings[keycode] = axis;
                    axes[(usize)axis].positiveScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    mouseAxisBindings[mousecode] = axis;
                    axes[(usize)axis].positiveMouseButton = mousecode;
                }
            }
            if (jsonAxis.HasKey("AltPositive")) {
                String altBindingLabel = jsonAxis["AltPositive"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    keyboardAxisBindings[keycode] = axis;
                    axes[(usize)axis].positiveScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    mouseAxisBindings[mousecode] = axis;
                    axes[(usize)axis].positiveMouseButton = mousecode;
                }
            }
            if (jsonAxis.HasKey("AltNegative")) {
                String altBindingLabel = jsonAxis["AltNegative"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    keyboardAxisBindings[keycode] = axis;
                    axes[(usize)axis].negativeScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    mouseAxisBindings[mousecode] = axis;
                    axes[(usize)axis].negativeMouseButton = mousecode;
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
            pOpenController = SDL_GameControllerOpen(i);
        } else {
            const char* name = SDL_JoystickNameForIndex(i);
            Log::Info("Detected Joystick: %s", name);
        }
    }
}

// ***********************************************************************

void GameChip::ProcessEvent(SDL_Event* event) {
    // Update Input States
    switch (event->type) {
        case SDL_TEXTINPUT: {
            textInputString.Append(event->text.text);
            break;
        }
        case SDL_KEYDOWN: {
            SDL_Keycode keycode = event->key.keysym.sym;
            keyDowns[(usize)keyCodeToInternalKeyCode[keycode]] = true;
            keyStates[(usize)keyCodeToInternalKeyCode[keycode]] = true;

            ControllerButton button = keyboardAltBindings[keycode];
            if (button != ControllerButton::Invalid) {
                buttonDowns[(usize)button] = true;
                buttonStates[(usize)button] = true;
            }
            ControllerAxis axis = keyboardAxisBindings[keycode];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = axes[(usize)axis];
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
            keyUps[(usize)keyCodeToInternalKeyCode[keycode]] = true;
            keyStates[(usize)keyCodeToInternalKeyCode[keycode]] = false;

            ControllerButton button = keyboardAltBindings[keycode];
            if (button != ControllerButton::Invalid) {
                buttonUps[(usize)button] = true;
                buttonStates[(usize)button] = false;
            }
            ControllerAxis axis = keyboardAxisBindings[keycode];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = axes[(usize)axis];
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
            ControllerButton button = primaryBindings[sdlButton];
            if (button != ControllerButton::Invalid) {
                buttonDowns[(usize)button] = true;
                buttonStates[(usize)button] = true;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP: {
            SDL_GameControllerButton sdlButton = (SDL_GameControllerButton)event->cbutton.button;
            ControllerButton button = primaryBindings[sdlButton];
            if (button != ControllerButton::Invalid) {
                buttonUps[(usize)button] = true;
                buttonStates[(usize)button] = false;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            i32 sdlMouseButton = event->button.button;
            ControllerButton button = mouseAltBindings[sdlMouseButton];
            if (button != ControllerButton::Invalid) {
                buttonDowns[(usize)button] = true;
                buttonStates[(usize)button] = true;
            }
            ControllerAxis axis = mouseAxisBindings[sdlMouseButton];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = axes[(usize)axis];
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
            ControllerButton button = mouseAltBindings[sdlMouseButton];
            if (button != ControllerButton::Invalid) {
                buttonUps[(usize)button] = true;
                buttonStates[(usize)button] = false;
            }
            ControllerAxis axis = mouseAxisBindings[sdlMouseButton];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = axes[(usize)axis];
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
                axis = mouseAxisBindings[stringToMouseCode["Mouse_AxisX"_hash]];
                axes[(usize)axis].axisValue = (f32)motionEvent.xrel * mouseSensitivity;
                axes[(usize)axis].ignoreVirtual = true;
                axes[(usize)axis].isMouseDriver = true;
            }
            if (abs(motionEvent.yrel) > 0) {
                axis = mouseAxisBindings[stringToMouseCode["Mouse_AxisY"_hash]];
                axes[(usize)axis].axisValue = (f32)motionEvent.yrel * mouseSensitivity;
                axes[(usize)axis].ignoreVirtual = true;
                axes[(usize)axis].isMouseDriver = true;
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION: {
            SDL_ControllerAxisEvent axisEvent = event->caxis;
            ControllerAxis axis = primaryAxisBindings[(SDL_GameControllerAxis)axisEvent.axis];
            axes[(usize)axis].axisValue = (f32)axisEvent.value / 32768.f;
            axes[(usize)axis].ignoreVirtual = true;
            axes[(usize)axis].isMouseDriver = false;
            break;
        }
        default: break;
    }
}

// ***********************************************************************

void GameChip::UpdateInputs(f32 deltaTime, Vec2f targetRes, Vec2f realWindowRes) {
    targetResolution = targetRes;
    windowResolution = realWindowRes;

    // TODO: This should be configurable
    f32 gravity = 1.0f;
    f32 sensitivity = 1.0f;
    f32 deadzone = 0.09f;

    for (usize axisIndex = 0; axisIndex < (usize)ControllerAxis::Count; axisIndex++) {
        ControllerAxis axisEnum = (ControllerAxis)axisIndex;
        Axis& axis = axes[axisIndex];

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

void GameChip::ClearStates() {
    for (int i = 0; i < (int)Key::Count; i++)
        keyDowns[i] = false;
    for (int i = 0; i < (int)Key::Count; i++)
        keyUps[i] = false;

    for (int i = 0; i < (int)ControllerButton::Count; i++)
        buttonDowns[i] = false;
    for (int i = 0; i < (int)ControllerButton::Count; i++)
        buttonUps[i] = false;

    textInputString.Reset();
    for (Axis& axis : axes) {
        if (axis.isMouseDriver) {
            axis.axisValue = 0.0f;
        }
    }
}

// ***********************************************************************

void GameChip::Shutdown() {
    // TODO: Mark as not a leak?
    primaryBindings.Free();
    primaryAxisBindings.Free();

    keyboardAltBindings.Free();
    mouseAltBindings.Free();

    keyboardAxisBindings.Free();
    mouseAxisBindings.Free();

    // Might be worth marking these as not leaks
    stringToControllerButton.Free();
    stringToControllerAxis.Free();
    stringToKeyCode.Free();
    keyCodeToInternalKeyCode.Free();
    stringToMouseCode.Free();
    stringToSDLControllerButton.Free();
    stringToSDLControllerAxis.Free();

    if (pOpenController) {
        SDL_GameControllerClose(pOpenController);
    }
}

// ***********************************************************************

bool GameChip::GetButton(ControllerButton buttonCode) {
    return buttonStates[(usize)buttonCode];
}

// ***********************************************************************

bool GameChip::GetButtonDown(ControllerButton buttonCode) {
    return buttonDowns[(usize)buttonCode];
}

// ***********************************************************************

bool GameChip::GetButtonUp(ControllerButton buttonCode) {
    return buttonUps[(usize)buttonCode];
}

// ***********************************************************************

f32 GameChip::GetAxis(ControllerAxis axis) {
    return axes[(usize)axis].axisValue;
}

// ***********************************************************************

Vec2i GameChip::GetMousePosition() {
    // TODO: This becomes wrong at the edge of the screen, must apply screen warning to it as well
    Vec2i mousePosition;
    SDL_GetMouseState(&mousePosition.x, &mousePosition.y);
    f32 xAdjusted = (f32)mousePosition.x / windowResolution.x * targetResolution.x;
    f32 yAdjusted = targetResolution.y - ((f32)mousePosition.y / windowResolution.y * targetResolution.y);
    return Vec2i((int)xAdjusted, (int)yAdjusted);
}

// ***********************************************************************

void GameChip::EnableMouseRelativeMode(bool enable) {
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
}

// ***********************************************************************

bool GameChip::GetKey(Key keyCode) {
    return keyStates[(usize)keyCode];
}

// ***********************************************************************

bool GameChip::GetKeyDown(Key keyCode) {
    return keyDowns[(usize)keyCode];
}

// ***********************************************************************

bool GameChip::GetKeyUp(Key keyCode) {
    return keyUps[(usize)keyCode];
}

// ***********************************************************************

String GameChip::InputString() {
    return textInputString.CreateString();
}
