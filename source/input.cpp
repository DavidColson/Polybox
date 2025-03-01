// Copyright David Colson. All rights reserved.

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

	// Mapping of strings to SDL controller pInputState->axes
	HashMap<u32, SDL_GameControllerAxis> stringToSDLControllerAxis;

    HashMap<SDL_GameControllerButton, ControllerButton> primaryBindings;
    HashMap<SDL_GameControllerAxis, ControllerAxis> primaryAxisBindings;

    HashMap<SDL_Keycode, ControllerButton> keyboardAltBindings;
    HashMap<int, ControllerButton> mouseAltBindings;

    HashMap<SDL_Keycode, ControllerAxis> keyboardAxisBindings;
    HashMap<int, ControllerAxis> mouseAxisBindings;

    bool keyDowns[(u64)Key::Count];
    bool keyUps[(u64)Key::Count];
    bool keyStates[(u64)Key::Count];

    bool buttonDowns[(u64)ControllerButton::Count];
    bool buttonUps[(u64)ControllerButton::Count];
    bool buttonStates[(u64)ControllerButton::Count];

    Axis axes[(u64)ControllerAxis::Count];

    SDL_GameController* pOpenController { nullptr };

    StringBuilder textInputString;

    Vec2f targetResolution;
    Vec2f windowResolution;
};

InputState* pInputState;

// ***********************************************************************

void InputInit() {
	Arena* pArena = ArenaCreate();
	pInputState = New(pArena, InputState);
	pInputState->pArena = pArena;
	
	pInputState->stringToControllerButton.pArena = pArena;
	pInputState->stringToControllerAxis.pArena = pArena;
	pInputState->stringToKeyCode.pArena = pArena;
	pInputState->keyCodeToInternalKeyCode.pArena = pArena;
	pInputState->stringToMouseCode.pArena = pArena;
	pInputState->stringToSDLControllerButton.pArena = pArena;
	pInputState->stringToSDLControllerAxis.pArena = pArena;
	pInputState->stringToSDLControllerButton.pArena = pArena;
	pInputState->primaryBindings.pArena = pArena;
	pInputState->primaryAxisBindings.pArena = pArena;
	pInputState->keyboardAltBindings.pArena = pArena;
	pInputState->mouseAltBindings.pArena = pArena;
	pInputState->keyboardAxisBindings.pArena = pArena;
	pInputState->mouseAxisBindings.pArena = pArena;
	pInputState->primaryAxisBindings.pArena = pArena;
	pInputState->textInputString.pArena = pArena;

	// @Todo: macros to help with this nonesense?

    // Setup hashmaps for controller labels
    pInputState->stringToControllerButton.Add("FaceBottom"_hash, ControllerButton::FaceBottom);
    pInputState->stringToControllerButton.Add("FaceRight"_hash, ControllerButton::FaceRight);
    pInputState->stringToControllerButton.Add("FaceLeft"_hash, ControllerButton::FaceLeft);
    pInputState->stringToControllerButton.Add("FaceTop"_hash, ControllerButton::FaceTop);
    pInputState->stringToControllerButton.Add("LeftStick"_hash, ControllerButton::LeftStick);
    pInputState->stringToControllerButton.Add("RightStick"_hash, ControllerButton::RightStick);
    pInputState->stringToControllerButton.Add("LeftShoulder"_hash, ControllerButton::LeftShoulder);
    pInputState->stringToControllerButton.Add("RightShoulder"_hash, ControllerButton::RightShoulder);
    pInputState->stringToControllerButton.Add("DpadDown"_hash, ControllerButton::DpadDown);
    pInputState->stringToControllerButton.Add("DpadLeft"_hash, ControllerButton::DpadLeft);
    pInputState->stringToControllerButton.Add("DpadRight"_hash, ControllerButton::DpadRight);
    pInputState->stringToControllerButton.Add("DpadUp"_hash, ControllerButton::DpadUp);
    pInputState->stringToControllerButton.Add("Start"_hash, ControllerButton::Start);
    pInputState->stringToControllerButton.Add("Select"_hash, ControllerButton::Select);


    pInputState->stringToControllerAxis.Add("LeftX"_hash, ControllerAxis::LeftX);
    pInputState->stringToControllerAxis.Add("LeftY"_hash, ControllerAxis::LeftY);
    pInputState->stringToControllerAxis.Add("RightX"_hash, ControllerAxis::RightX);
    pInputState->stringToControllerAxis.Add("RightY"_hash, ControllerAxis::RightY);
    pInputState->stringToControllerAxis.Add("TriggerLeft"_hash, ControllerAxis::TriggerLeft);
    pInputState->stringToControllerAxis.Add("TriggerRight"_hash, ControllerAxis::TriggerRight);


    pInputState->stringToKeyCode.Add("Keycode_A"_hash, SDLK_a);
    pInputState->stringToKeyCode.Add("Keycode_B"_hash, SDLK_b);
    pInputState->stringToKeyCode.Add("Keycode_C"_hash, SDLK_c);
    pInputState->stringToKeyCode.Add("Keycode_D"_hash, SDLK_d);
    pInputState->stringToKeyCode.Add("Keycode_E"_hash, SDLK_e);
    pInputState->stringToKeyCode.Add("Keycode_F"_hash, SDLK_f);
    pInputState->stringToKeyCode.Add("Keycode_G"_hash, SDLK_g);
    pInputState->stringToKeyCode.Add("Keycode_H"_hash, SDLK_h);
    pInputState->stringToKeyCode.Add("Keycode_I"_hash, SDLK_i);
    pInputState->stringToKeyCode.Add("Keycode_J"_hash, SDLK_j);
    pInputState->stringToKeyCode.Add("Keycode_K"_hash, SDLK_k);
    pInputState->stringToKeyCode.Add("Keycode_L"_hash, SDLK_l);
    pInputState->stringToKeyCode.Add("Keycode_M"_hash, SDLK_m);
    pInputState->stringToKeyCode.Add("Keycode_N"_hash, SDLK_n);
    pInputState->stringToKeyCode.Add("Keycode_O"_hash, SDLK_o);
    pInputState->stringToKeyCode.Add("Keycode_P"_hash, SDLK_p);
    pInputState->stringToKeyCode.Add("Keycode_Q"_hash, SDLK_q);
    pInputState->stringToKeyCode.Add("Keycode_R"_hash, SDLK_e);
    pInputState->stringToKeyCode.Add("Keycode_S"_hash, SDLK_s);
    pInputState->stringToKeyCode.Add("Keycode_T"_hash, SDLK_t);
    pInputState->stringToKeyCode.Add("Keycode_U"_hash, SDLK_u);
    pInputState->stringToKeyCode.Add("Keycode_V"_hash, SDLK_v);
    pInputState->stringToKeyCode.Add("Keycode_W"_hash, SDLK_w);
    pInputState->stringToKeyCode.Add("Keycode_X"_hash, SDLK_x);
    pInputState->stringToKeyCode.Add("Keycode_Y"_hash, SDLK_y);
    pInputState->stringToKeyCode.Add("Keycode_Z"_hash, SDLK_z);

    pInputState->stringToKeyCode.Add("Keycode_1"_hash, SDLK_1);
    pInputState->stringToKeyCode.Add("Keycode_2"_hash, SDLK_2);
    pInputState->stringToKeyCode.Add("Keycode_3"_hash, SDLK_3);
    pInputState->stringToKeyCode.Add("Keycode_4"_hash, SDLK_4);
    pInputState->stringToKeyCode.Add("Keycode_5"_hash, SDLK_5);
    pInputState->stringToKeyCode.Add("Keycode_6"_hash, SDLK_6);
    pInputState->stringToKeyCode.Add("Keycode_7"_hash, SDLK_7);
    pInputState->stringToKeyCode.Add("Keycode_8"_hash, SDLK_8);
    pInputState->stringToKeyCode.Add("Keycode_9"_hash, SDLK_9);
    pInputState->stringToKeyCode.Add("Keycode_0"_hash, SDLK_0);

    pInputState->stringToKeyCode.Add("Keycode_Return"_hash, SDLK_RETURN);
    pInputState->stringToKeyCode.Add("Keycode_Escape"_hash, SDLK_ESCAPE);
    pInputState->stringToKeyCode.Add("Keycode_Backspace"_hash, SDLK_BACKSPACE);
    pInputState->stringToKeyCode.Add("Keycode_Tab"_hash, SDLK_TAB);
    pInputState->stringToKeyCode.Add("Keycode_Space"_hash, SDLK_SPACE);
    pInputState->stringToKeyCode.Add("Keycode_Exclaim"_hash, SDLK_EXCLAIM);
    pInputState->stringToKeyCode.Add("Keycode_QuoteDbl"_hash, SDLK_QUOTEDBL);
    pInputState->stringToKeyCode.Add("Keycode_Hash"_hash, SDLK_HASH);
    pInputState->stringToKeyCode.Add("Keycode_Percent"_hash, SDLK_PERCENT);
    pInputState->stringToKeyCode.Add("Keycode_Dollar"_hash, SDLK_DOLLAR);
    pInputState->stringToKeyCode.Add("Keycode_Ampersand"_hash, SDLK_AMPERSAND);
    pInputState->stringToKeyCode.Add("Keycode_Quote"_hash, SDLK_QUOTE);
    pInputState->stringToKeyCode.Add("Keycode_LeftParen"_hash, SDLK_LEFTPAREN);
    pInputState->stringToKeyCode.Add("Keycode_RightParen"_hash, SDLK_RIGHTPAREN);
    pInputState->stringToKeyCode.Add("Keycode_Asterisk"_hash, SDLK_ASTERISK);
    pInputState->stringToKeyCode.Add("Keycode_Plus"_hash, SDLK_PLUS);
    pInputState->stringToKeyCode.Add("Keycode_Comma"_hash, SDLK_COMMA);
    pInputState->stringToKeyCode.Add("Keycode_Minus"_hash, SDLK_MINUS);
    pInputState->stringToKeyCode.Add("Keycode_Period"_hash, SDLK_PERIOD);
    pInputState->stringToKeyCode.Add("Keycode_Slash"_hash, SDLK_SLASH);
    pInputState->stringToKeyCode.Add("Keycode_Colon"_hash, SDLK_COLON);
    pInputState->stringToKeyCode.Add("Keycode_Semicolon"_hash, SDLK_SEMICOLON);
    pInputState->stringToKeyCode.Add("Keycode_Less"_hash, SDLK_LESS);
    pInputState->stringToKeyCode.Add("Keycode_Equals"_hash, SDLK_EQUALS);
    pInputState->stringToKeyCode.Add("Keycode_Greater"_hash, SDLK_GREATER);
    pInputState->stringToKeyCode.Add("Keycode_Question"_hash, SDLK_QUESTION);
    pInputState->stringToKeyCode.Add("Keycode_At"_hash, SDLK_AT);
    pInputState->stringToKeyCode.Add("Keycode_LeftBracket"_hash, SDLK_LEFTBRACKET);
    pInputState->stringToKeyCode.Add("Keycode_Backslash"_hash, SDLK_BACKSLASH);
    pInputState->stringToKeyCode.Add("Keycode_RightBracket"_hash, SDLK_RIGHTBRACKET);
    pInputState->stringToKeyCode.Add("Keycode_Caret"_hash, SDLK_CARET);
    pInputState->stringToKeyCode.Add("Keycode_Underscore"_hash, SDLK_UNDERSCORE);
    pInputState->stringToKeyCode.Add("Keycode_BackQuote"_hash, SDLK_BACKQUOTE);

    pInputState->stringToKeyCode.Add("Keycode_CapsLock"_hash, SDLK_CAPSLOCK);

    pInputState->stringToKeyCode.Add("Keycode_F1"_hash, SDLK_F1);
    pInputState->stringToKeyCode.Add("Keycode_F2"_hash, SDLK_F2);
    pInputState->stringToKeyCode.Add("Keycode_F3"_hash, SDLK_F3);
    pInputState->stringToKeyCode.Add("Keycode_F4"_hash, SDLK_F4);
    pInputState->stringToKeyCode.Add("Keycode_F5"_hash, SDLK_F5);
    pInputState->stringToKeyCode.Add("Keycode_F6"_hash, SDLK_F6);
    pInputState->stringToKeyCode.Add("Keycode_F7"_hash, SDLK_F7);
    pInputState->stringToKeyCode.Add("Keycode_F8"_hash, SDLK_F8);
    pInputState->stringToKeyCode.Add("Keycode_F9"_hash, SDLK_F9);
    pInputState->stringToKeyCode.Add("Keycode_F10"_hash, SDLK_F10);
    pInputState->stringToKeyCode.Add("Keycode_F11"_hash, SDLK_F11);
    pInputState->stringToKeyCode.Add("Keycode_F12"_hash, SDLK_F12);

    pInputState->stringToKeyCode.Add("Keycode_PrintScreen"_hash, SDLK_PRINTSCREEN);
    pInputState->stringToKeyCode.Add("Keycode_ScrollLock"_hash, SDLK_SCROLLLOCK);
    pInputState->stringToKeyCode.Add("Keycode_Pause"_hash, SDLK_PAUSE);
    pInputState->stringToKeyCode.Add("Keycode_Insert"_hash, SDLK_INSERT);
    pInputState->stringToKeyCode.Add("Keycode_Home"_hash, SDLK_HOME);
    pInputState->stringToKeyCode.Add("Keycode_PageUp"_hash, SDLK_PAGEUP);
    pInputState->stringToKeyCode.Add("Keycode_Delete"_hash, SDLK_DELETE);
    pInputState->stringToKeyCode.Add("Keycode_End"_hash, SDLK_END);
    pInputState->stringToKeyCode.Add("Keycode_PageDown"_hash, SDLK_PAGEDOWN);
    pInputState->stringToKeyCode.Add("Keycode_Right"_hash, SDLK_RIGHT);
    pInputState->stringToKeyCode.Add("Keycode_Left"_hash, SDLK_LEFT);
    pInputState->stringToKeyCode.Add("Keycode_Down"_hash, SDLK_DOWN);
    pInputState->stringToKeyCode.Add("Keycode_Up"_hash, SDLK_UP);

    pInputState->stringToKeyCode.Add("Keycode_NumLock"_hash, SDLK_NUMLOCKCLEAR);
    pInputState->stringToKeyCode.Add("Keycode_KpDivide"_hash, SDLK_KP_DIVIDE);
    pInputState->stringToKeyCode.Add("Keycode_KpMultiply"_hash, SDLK_KP_MULTIPLY);
    pInputState->stringToKeyCode.Add("Keycode_KpMinus"_hash, SDLK_KP_MINUS);
    pInputState->stringToKeyCode.Add("Keycode_KpPlus"_hash, SDLK_KP_PLUS);
    pInputState->stringToKeyCode.Add("Keycode_KpEnter"_hash, SDLK_KP_ENTER);
    pInputState->stringToKeyCode.Add("Keycode_Kp1"_hash, SDLK_KP_1);
    pInputState->stringToKeyCode.Add("Keycode_Kp2"_hash, SDLK_KP_2);
    pInputState->stringToKeyCode.Add("Keycode_Kp3"_hash, SDLK_KP_3);
    pInputState->stringToKeyCode.Add("Keycode_Kp4"_hash, SDLK_KP_4);
    pInputState->stringToKeyCode.Add("Keycode_Kp5"_hash, SDLK_KP_5);
    pInputState->stringToKeyCode.Add("Keycode_Kp6"_hash, SDLK_KP_6);
    pInputState->stringToKeyCode.Add("Keycode_Kp7"_hash, SDLK_KP_7);
    pInputState->stringToKeyCode.Add("Keycode_Kp8"_hash, SDLK_KP_8);
    pInputState->stringToKeyCode.Add("Keycode_Kp9"_hash, SDLK_KP_9);
    pInputState->stringToKeyCode.Add("Keycode_Kp0"_hash, SDLK_KP_0);
    pInputState->stringToKeyCode.Add("Keycode_KpPeriod"_hash, SDLK_KP_PERIOD);

    pInputState->stringToKeyCode.Add("Keycode_LeftCtrl"_hash, SDLK_LCTRL);
    pInputState->stringToKeyCode.Add("Keycode_LeftShift"_hash, SDLK_LSHIFT);
    pInputState->stringToKeyCode.Add("Keycode_LeftAlt"_hash, SDLK_LALT);
    pInputState->stringToKeyCode.Add("Keycode_LeftGui"_hash, SDLK_LGUI);
    pInputState->stringToKeyCode.Add("Keycode_RightCtrl"_hash, SDLK_RCTRL);
    pInputState->stringToKeyCode.Add("Keycode_RightShift"_hash, SDLK_RSHIFT);
    pInputState->stringToKeyCode.Add("Keycode_RightAlt"_hash, SDLK_RALT);
    pInputState->stringToKeyCode.Add("Keycode_RightGui"_hash, SDLK_RGUI);


    pInputState->keyCodeToInternalKeyCode.Add(SDLK_a, Key::A);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_b, Key::B);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_c, Key::C);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_d, Key::D);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_e, Key::E);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_f, Key::F);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_g, Key::G);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_h, Key::H);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_i, Key::I);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_j, Key::J);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_k, Key::K);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_l, Key::L);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_m, Key::M);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_n, Key::N);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_o, Key::O);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_p, Key::P);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_q, Key::Q);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_e, Key::R);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_s, Key::S);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_t, Key::T);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_u, Key::U);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_v, Key::V);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_w, Key::W);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_x, Key::X);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_y, Key::Y);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_z, Key::Z);

    pInputState->keyCodeToInternalKeyCode.Add(SDLK_1, Key::No1);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_2, Key::No2);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_3, Key::No3);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_4, Key::No4);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_5, Key::No5);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_6, Key::No6);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_7, Key::No7);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_8, Key::No8);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_9, Key::No9);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_0, Key::No0);

    pInputState->keyCodeToInternalKeyCode.Add(SDLK_RETURN, Key::Return);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_ESCAPE, Key::Escape);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_BACKSPACE, Key::Backspace);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_TAB, Key::Tab);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_SPACE, Key::Space);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_EXCLAIM, Key::Exclaim);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_QUOTEDBL, Key::QuoteDbl);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_HASH, Key::Hash);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_PERCENT, Key::Percent);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_DOLLAR, Key::Dollar);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_AMPERSAND, Key::Ampersand);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_QUOTE, Key::Quote);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_LEFTPAREN, Key::LeftParen);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_RIGHTPAREN, Key::RightParen);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_ASTERISK, Key::Asterisk);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_PLUS, Key::Plus);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_COMMA, Key::Comma);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_MINUS, Key::Minus);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_PERIOD, Key::Period);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_SLASH, Key::Slash);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_COLON, Key::Colon);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_SEMICOLON, Key::Semicolon);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_LESS, Key::Less);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_EQUALS, Key::Equals);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_GREATER, Key::Greater);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_QUESTION, Key::Question);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_AT, Key::At);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_LEFTBRACKET, Key::LeftBracket);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_BACKSLASH, Key::Backslash);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_RIGHTBRACKET, Key::RightBracket);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_CARET, Key::Caret);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_UNDERSCORE, Key::Underscore);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_BACKQUOTE, Key::BackQuote);

    pInputState->keyCodeToInternalKeyCode.Add(SDLK_CAPSLOCK, Key::CapsLock);

    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F1, Key::F1);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F2, Key::F2);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F3, Key::F3);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F4, Key::F4);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F5, Key::F5);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F6, Key::F6);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F7, Key::F7);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F8, Key::F8);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F9, Key::F9);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F10, Key::F10);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F11, Key::F11);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_F12, Key::F12);

    pInputState->keyCodeToInternalKeyCode.Add(SDLK_PRINTSCREEN, Key::PrintScreen);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_SCROLLLOCK, Key::ScrollLock);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_PAUSE, Key::Pause);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_INSERT, Key::Insert);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_HOME, Key::Home);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_PAGEUP, Key::PageUp);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_DELETE, Key::Delete);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_END, Key::End);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_PAGEDOWN, Key::PageDown);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_RIGHT, Key::Right);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_LEFT, Key::Left);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_DOWN, Key::Down);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_UP, Key::Up);

    pInputState->keyCodeToInternalKeyCode.Add(SDLK_NUMLOCKCLEAR, Key::NumLock);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_DIVIDE, Key::KpDivide);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_MULTIPLY, Key::KpMultiply);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_MINUS, Key::KpMinus);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_PLUS, Key::KpPlus);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_ENTER, Key::KpEnter);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_1, Key::Kp1);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_2, Key::Kp2);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_3, Key::Kp3);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_4, Key::Kp4);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_5, Key::Kp5);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_6, Key::Kp6);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_7, Key::Kp7);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_8, Key::Kp8);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_9, Key::Kp9);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_0, Key::Kp0);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_KP_PERIOD, Key::KpPeriod);

    pInputState->keyCodeToInternalKeyCode.Add(SDLK_LCTRL, Key::LeftCtrl);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_LSHIFT, Key::LeftShift);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_LALT, Key::LeftAlt);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_LGUI, Key::LeftGui);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_RCTRL, Key::RightCtrl);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_RSHIFT, Key::RightShift);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_RALT, Key::RightAlt);
    pInputState->keyCodeToInternalKeyCode.Add(SDLK_RGUI, Key::RightGui);


    pInputState->stringToMouseCode.Add("Mouse_Button0"_hash, SDL_BUTTON_LEFT);
    pInputState->stringToMouseCode.Add("Mouse_Button1"_hash, SDL_BUTTON_MIDDLE);
    pInputState->stringToMouseCode.Add("Mouse_Button2"_hash, SDL_BUTTON_RIGHT);
    pInputState->stringToMouseCode.Add("Mouse_AxisY"_hash, 128);
    pInputState->stringToMouseCode.Add("Mouse_AxisX"_hash, 127);


    pInputState->stringToSDLControllerButton.Add("Controller_A"_hash, SDL_CONTROLLER_BUTTON_A);
    pInputState->stringToSDLControllerButton.Add("Controller_B"_hash, SDL_CONTROLLER_BUTTON_B);
    pInputState->stringToSDLControllerButton.Add("Controller_X"_hash, SDL_CONTROLLER_BUTTON_X);
    pInputState->stringToSDLControllerButton.Add("Controller_Y"_hash, SDL_CONTROLLER_BUTTON_Y);
    pInputState->stringToSDLControllerButton.Add("Controller_LeftStick"_hash, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    pInputState->stringToSDLControllerButton.Add("Controller_RightStick"_hash, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    pInputState->stringToSDLControllerButton.Add("Controller_LeftShoulder"_hash, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    pInputState->stringToSDLControllerButton.Add("Controller_RightShoulder"_hash, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    pInputState->stringToSDLControllerButton.Add("Controller_DpadUp"_hash, SDL_CONTROLLER_BUTTON_DPAD_UP);
    pInputState->stringToSDLControllerButton.Add("Controller_DpadDown"_hash, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    pInputState->stringToSDLControllerButton.Add("Controller_DpadLeft"_hash, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    pInputState->stringToSDLControllerButton.Add("Controller_DpadRight"_hash, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    pInputState->stringToSDLControllerButton.Add("Controller_Start"_hash, SDL_CONTROLLER_BUTTON_START);
    pInputState->stringToSDLControllerButton.Add("Controller_Select"_hash, SDL_CONTROLLER_BUTTON_BACK);


    pInputState->stringToSDLControllerAxis.Add("Controller_LeftX"_hash, SDL_CONTROLLER_AXIS_LEFTX);
    pInputState->stringToSDLControllerAxis.Add("Controller_LeftY"_hash, SDL_CONTROLLER_AXIS_LEFTY);
    pInputState->stringToSDLControllerAxis.Add("Controller_RightX"_hash, SDL_CONTROLLER_AXIS_RIGHTX);
    pInputState->stringToSDLControllerAxis.Add("Controller_RightY"_hash, SDL_CONTROLLER_AXIS_RIGHTY);
    pInputState->stringToSDLControllerAxis.Add("Controller_TriggerLeft"_hash, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    pInputState->stringToSDLControllerAxis.Add("Controller_TriggerRight"_hash, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);


    // Load json mapping file
    SDL_RWops* pFileRead = SDL_RWFromFile("systemroot/shared/base_controller_mapping.json", "rb");
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
        for (u64 i = 0; i < buttons.Count(); i++) {
            JsonValue& jsonButton = buttons[i];
            ControllerButton button = pInputState->stringToControllerButton[Fnv1a::Hash(jsonButton["Name"].ToString().pData)];
            SDL_GameControllerButton primaryBinding = pInputState->stringToSDLControllerButton[Fnv1a::Hash(jsonButton["Primary"].ToString().pData)];
            pInputState->primaryBindings[primaryBinding] = button;

            String altBindingLabel = jsonButton["Alt"].ToString();
            if (altBindingLabel.SubStr(0, 5) == "Keyco") {
                SDL_Keycode keycode = pInputState->stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                pInputState->keyboardAltBindings[keycode] = button;
            } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                i32 mousecode = pInputState->stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                pInputState->mouseAltBindings[mousecode] = button;
            }
        }
    }
    if (mapping.HasKey("Axes")) {
        JsonValue& jsonAxes = mapping["Axes"];
		for (u64 i = 0; i < jsonAxes.Count(); i++) {
			JsonValue& jsonAxis = jsonAxes[i];
            ControllerAxis axis = pInputState->stringToControllerAxis[Fnv1a::Hash(jsonAxis["Name"].ToString().pData)];

            // TODO: What if someone tries to bind a controller button(s) to an axis? Support that probably
            SDL_GameControllerAxis primaryBinding = pInputState->stringToSDLControllerAxis[Fnv1a::Hash(jsonAxis["Primary"].ToString().pData)];
            pInputState->primaryAxisBindings[primaryBinding] = axis;

            if (jsonAxis.HasKey("Alt")) {
                String altBindingLabel = jsonAxis["Alt"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = pInputState->stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pInputState->keyboardAxisBindings[keycode] = axis;
                    pInputState->axes[(u64)axis].positiveScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = pInputState->stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pInputState->mouseAxisBindings[mousecode] = axis;
                    pInputState->axes[(u64)axis].positiveMouseButton = mousecode;
                }
            }
            if (jsonAxis.HasKey("AltPositive")) {
                String altBindingLabel = jsonAxis["AltPositive"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = pInputState->stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pInputState->keyboardAxisBindings[keycode] = axis;
                    pInputState->axes[(u64)axis].positiveScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = pInputState->stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pInputState->mouseAxisBindings[mousecode] = axis;
                    pInputState->axes[(u64)axis].positiveMouseButton = mousecode;
                }
            }
            if (jsonAxis.HasKey("AltNegative")) {
                String altBindingLabel = jsonAxis["AltNegative"].ToString();
                if (altBindingLabel.SubStr(0, 5) == "Scanc") {
                    SDL_Keycode keycode = pInputState->stringToKeyCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pInputState->keyboardAxisBindings[keycode] = axis;
                    pInputState->axes[(u64)axis].negativeScanCode = keycode;
                } else if (altBindingLabel.SubStr(0, 5) == "Mouse") {
                    i32 mousecode = pInputState->stringToMouseCode[Fnv1a::Hash(altBindingLabel.pData)];
                    pInputState->mouseAxisBindings[mousecode] = axis;
                    pInputState->axes[(u64)axis].negativeMouseButton = mousecode;
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
            pInputState->pOpenController = SDL_GameControllerOpen(i);
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
            pInputState->textInputString.Append(event->text.text);
            break;
        }
        case SDL_KEYDOWN: {
            SDL_Keycode keycode = event->key.keysym.sym;
            pInputState->keyDowns[(u64)pInputState->keyCodeToInternalKeyCode[keycode]] = true;
            pInputState->keyStates[(u64)pInputState->keyCodeToInternalKeyCode[keycode]] = true;

            ControllerButton button = pInputState->keyboardAltBindings[keycode];
            if (button != ControllerButton::Invalid) {
                pInputState->buttonDowns[(u64)button] = true;
                pInputState->buttonStates[(u64)button] = true;
            }
            ControllerAxis axis = pInputState->keyboardAxisBindings[keycode];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = pInputState->axes[(u64)axis];
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
            pInputState->keyUps[(u64)pInputState->keyCodeToInternalKeyCode[keycode]] = true;
            pInputState->keyStates[(u64)pInputState->keyCodeToInternalKeyCode[keycode]] = false;

            ControllerButton button = pInputState->keyboardAltBindings[keycode];
            if (button != ControllerButton::Invalid) {
                pInputState->buttonUps[(u64)button] = true;
                pInputState->buttonStates[(u64)button] = false;
            }
            ControllerAxis axis = pInputState->keyboardAxisBindings[keycode];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = pInputState->axes[(u64)axis];
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
            ControllerButton button = pInputState->primaryBindings[sdlButton];
            if (button != ControllerButton::Invalid) {
                pInputState->buttonDowns[(u64)button] = true;
                pInputState->buttonStates[(u64)button] = true;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP: {
            SDL_GameControllerButton sdlButton = (SDL_GameControllerButton)event->cbutton.button;
            ControllerButton button = pInputState->primaryBindings[sdlButton];
            if (button != ControllerButton::Invalid) {
                pInputState->buttonUps[(u64)button] = true;
                pInputState->buttonStates[(u64)button] = false;
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            i32 sdlMouseButton = event->button.button;
            ControllerButton button = pInputState->mouseAltBindings[sdlMouseButton];
            if (button != ControllerButton::Invalid) {
                pInputState->buttonDowns[(u64)button] = true;
                pInputState->buttonStates[(u64)button] = true;
            }
            ControllerAxis axis = pInputState->mouseAxisBindings[sdlMouseButton];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = pInputState->axes[(u64)axis];
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
            ControllerButton button = pInputState->mouseAltBindings[sdlMouseButton];
            if (button != ControllerButton::Invalid) {
                pInputState->buttonUps[(u64)button] = true;
                pInputState->buttonStates[(u64)button] = false;
            }
            ControllerAxis axis = pInputState->mouseAxisBindings[sdlMouseButton];
            if (axis != ControllerAxis::Invalid) {
                Axis& axisData = pInputState->axes[(u64)axis];
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
                axis = pInputState->mouseAxisBindings[pInputState->stringToMouseCode["Mouse_AxisX"_hash]];
                pInputState->axes[(u64)axis].axisValue = (f32)motionEvent.xrel * mouseSensitivity;
                pInputState->axes[(u64)axis].ignoreVirtual = true;
                pInputState->axes[(u64)axis].isMouseDriver = true;
            }
            if (abs(motionEvent.yrel) > 0) {
                axis = pInputState->mouseAxisBindings[pInputState->stringToMouseCode["Mouse_AxisY"_hash]];
                pInputState->axes[(u64)axis].axisValue = (f32)motionEvent.yrel * mouseSensitivity;
                pInputState->axes[(u64)axis].ignoreVirtual = true;
                pInputState->axes[(u64)axis].isMouseDriver = true;
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION: {
            SDL_ControllerAxisEvent axisEvent = event->caxis;
            ControllerAxis axis = pInputState->primaryAxisBindings[(SDL_GameControllerAxis)axisEvent.axis];
            pInputState->axes[(u64)axis].axisValue = (f32)axisEvent.value / 32768.f;
            pInputState->axes[(u64)axis].ignoreVirtual = true;
            pInputState->axes[(u64)axis].isMouseDriver = false;
            break;
        }
        default: break;
    }
}

// ***********************************************************************

void UpdateInputs(f32 deltaTime, Vec2f targetRes, Vec2f realWindowRes) {
    pInputState->targetResolution = targetRes;
    pInputState->windowResolution = realWindowRes;

    // TODO: This should be configurable
    f32 gravity = 1.0f;
    f32 sensitivity = 1.0f;
    f32 deadzone = 0.09f;

    for (u64 axisIndex = 0; axisIndex < (u64)ControllerAxis::Count; axisIndex++) {
        ControllerAxis axisEnum = (ControllerAxis)axisIndex;
        Axis& axis = pInputState->axes[axisIndex];

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
        pInputState->keyDowns[i] = false;
    for (int i = 0; i < (int)Key::Count; i++)
        pInputState->keyUps[i] = false;

    for (int i = 0; i < (int)ControllerButton::Count; i++)
        pInputState->buttonDowns[i] = false;
    for (int i = 0; i < (int)ControllerButton::Count; i++)
        pInputState->buttonUps[i] = false;

    pInputState->textInputString.Reset();
    for (Axis& axis : pInputState->axes) {
        if (axis.isMouseDriver) {
            axis.axisValue = 0.0f;
        }
    }
}

// ***********************************************************************

void Shutdown() {
    if (pInputState->pOpenController) {
        SDL_GameControllerClose(pInputState->pOpenController);
    }
	ArenaFinished(pInputState->pArena);
}

// ***********************************************************************

bool GetButton(ControllerButton buttonCode) {
    return pInputState->buttonStates[(u64)buttonCode];
}

// ***********************************************************************

bool GetButtonDown(ControllerButton buttonCode) {
    return pInputState->buttonDowns[(u64)buttonCode];
}

// ***********************************************************************

bool GetButtonUp(ControllerButton buttonCode) {
    return pInputState->buttonUps[(u64)buttonCode];
}

// ***********************************************************************

f32 GetAxis(ControllerAxis axis) {
    return pInputState->axes[(u64)axis].axisValue;
}

// ***********************************************************************

Vec2i GetMousePosition() {
    // TODO: This becomes wrong at the edge of the screen, must apply screen warning to it as well
    Vec2i mousePosition;
    SDL_GetMouseState(&mousePosition.x, &mousePosition.y);
    f32 xAdjusted = (f32)mousePosition.x / pInputState->windowResolution.x * pInputState->targetResolution.x;
    f32 yAdjusted = pInputState->targetResolution.y - ((f32)mousePosition.y / pInputState->windowResolution.y * pInputState->targetResolution.y);
    return Vec2i((int)xAdjusted, (int)yAdjusted);
}

// ***********************************************************************

void EnableMouseRelativeMode(bool enable) {
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
}

// ***********************************************************************

bool GetKey(Key keyCode) {
    return pInputState->keyStates[(u64)keyCode];
}

// ***********************************************************************

bool GetKeyDown(Key keyCode) {
    return pInputState->keyDowns[(u64)keyCode];
}

// ***********************************************************************

bool GetKeyUp(Key keyCode) {
    return pInputState->keyUps[(u64)keyCode];
}

// ***********************************************************************

String InputString() {
    return pInputState->textInputString.CreateString(g_pArenaFrame);
}
