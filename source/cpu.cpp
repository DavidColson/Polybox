// Copyright David Colson. All rights reserved.

static const String polyboxDefinitions = R"POLY_LIBS(
--- Serialization API

@checked declare function serialize(value: any, format: number, metadata: any?): string
@checked declare function deserialize(value: string): (any, any?)

@checked declare function store(filename: string, value: any, format: number, metadata: any?)
@checked declare function load(filename: string): (any, any?)

--- Usedata API

declare class UserData 
	function set(self, index: number, ...: number)
	function set2d(self, x: number, y: number, ...: number)

	function get(self, index: number, count: number): ...number
	function get2d(self, x: number, y: number, count: number): ...number

	function width(self): number
	function height(self): number
	function size(self): number

	function magnitude(self): number
	function distance(self, other: UserData): number
	function dot(self, other: UserData): number

	function __add(self, other: UserData): UserData
	function __sub(self, other: UserData): UserData
	function __mul(self, other: UserData): UserData
	function __div(self, other: UserData): UserData

	x: number
	y: number
	z: number
	w: number
	r: number
	g: number
	b: number
	a: number
end

@checked declare function userdata(type: string, width: number, heightOrDataStr: number|string?, dataStr: string?): UserData
@checked declare function vec(x: number, y: number, z: number): UserData
@checked declare function quat(x: number, y: number, z: number, w: number): UserData

--- Graphics API

@checked declare function begin_object_2d(primitiveType: string)
@checked declare function end_object_2d(primitiveType: string)
@checked declare function vertex(x: number, y: number, z: number?)
@checked declare function begin_object_3d(primitiveType: string)
@checked declare function end_object_3d()
@checked declare function color(r: number, g: number, b: number, a: number)
@checked declare function texcoord(u: number, v: number)
@checked declare function normal(x: number, y: number, z: number)
@checked declare function set_clear_color(r: number, g: number, b: number, a: number)
@checked declare function matrix_mode(mode: string)
@checked declare function push_matrix()
@checked declare function pop_matrix()
@checked declare function load_matrix(mat: UserData)
@checked declare function get_matrix(): UserData
@checked declare function perspective(screenWidth: number, screenHeight: number, nearPlane: number, farPlane: number, fov: number)
@checked declare function translate(x: number, y: number, z: number)
@checked declare function rotate(x: number, y: number, z: number)
@checked declare function scale(x: number, y: number, z: number)
@checked declare function identity()
@checked declare function bind_texture(textureData: UserData)
@checked declare function unbind_texture()
@checked declare function normals_mode(mode: string)
@checked declare function enable_lighting(enable: boolean)
@checked declare function light(id: number, dirX: number, dixY: number, dirZ: number, r: number, g: number, b: number)
@checked declare function ambient(r: number, g: number, b: number)
@checked declare function enable_fog(enable: boolean)
@checked declare function set_fog_start(fogStart: number)
@checked declare function set_fog_end(fogEnd: number)
@checked declare function set_fog_color(r: number, g: number, b: number)
@checked declare function draw_sprite(spriteData: UserData, x: number, y: number)
@checked declare function draw_sprite_rect(spriteData: UserData, x: number, y: number, z: number, w: number, posX: number, posY: number)

--- Input API

declare Button: {
	invalid: number,
	face_bottom: number,
	face_right: number,
	face_left: number,
	face_top: number,
	left_stick: number,
	right_stick: number,
	left_shoulder: number,
	right_shoulder: number,
	dpad_down: number,
	dpad_left: number,
	dpad_right: number,
	dpad_up: number,
	start: number,
	select: number,
}

declare Axis: {
	invalid: number,
	left_x: number,
	left_y: number,
	right_x: number,
	right_y: number,
	trigger_left: number,
	trigger_right: number,
}

declare Key: {
	invalid: number,
	a: number,
	b: number,
	c: number,
	d: number,
	e: number,
	f: number,
	g: number,
	h: number,
	i: number,
	j: number,
	k: number,
	l: number,
	m: number,
	n: number,
	o: number,
	p: number,
	q: number,
	r: number,
	s: number,
	t: number,
	u: number,
	v: number,
	w: number,
	x: number,
	y: number,
	z: number,
	no1: number,
	no2: number,
	no3: number,
	no4: number,
	no5: number,
	no6: number,
	no7: number,
	no8: number,
	no9: number,
	no0: number,
	enter: number,
	escape: number,
	backspace: number,
	tab: number,
	space: number,
	exclaim: number,
	quote_dbl: number,
	hash: number,
	percent: number,
	dollar: number,
	ampersand: number,
	quote: number,
	left_paren: number,
	right_paren: number,
	asterisk: number,
	plus: number,
	comma: number,
	minus: number,
	period: number,
	slash: number,
	colon: number,
	semicolon: number,
	less: number,
	equals: number,
	greater: number,
	question: number,
	at: number,
	left_bracket: number,
	backslash: number,
	right_bracket: number,
	caret: number,
	underscore: number,
	back_quote: number,
	caps_lock: number,
	f1: number,
	f2: number,
	f3: number,
	f4: number,
	f5: number,
	f6: number,
	f7: number,
	f8: number,
	f9: number,
	f10: number,
	f11: number,
	f12: number,
	print_screen: number,
	scroll_lock: number,
	pause: number,
	insert: number,
	home: number,
	pageup: number,
	delete: number,
	_end: number,
	pagedown: number,
	right: number,
	left: number,
	down: number,
	up: number,
	num_lock: number,
	kpdivide: number,
	kpmultiply: number,
	kpminus: number,
	kpplus: number,
	kpenter: number,
	kp1: number,
	kp2: number,
	kp3: number,
	kp4: number,
	kp5: number,
	kp6: number,
	kp7: number,
	kp8: number,
	kp9: number,
	kp0: number,
	kpperiod: number,
	left_ctrl: number,
	left_shift: number,
	left_alt: number,
	left_gui: number,
	right_ctrl: number,
	right_shift: number,
	right_alt: number,
	right_gui: number,
}

@checked declare function get_button(button: number): boolean 
@checked declare function get_button_down(button: number) : boolean 
@checked declare function get_button_up(button: number) : boolean 
@checked declare function get_axis(button: number) : number 
@checked declare function get_mouse_position() : (number, number)
@checked declare function enable_mouse_relative_mode(enable: boolean)
@checked declare function get_key(key: number) : boolean
@checked declare function get_key_down(key: number) : boolean
@checked declare function get_key_up(key: number) : boolean
@checked declare function input_string() : string
)POLY_LIBS";


namespace Cpu {

struct State {
	Arena* pArena;
	lua_State* pProgramState;
};

State* pState = nullptr;

// ***********************************************************************

struct LuaFileResolver : Luau::FileResolver {
    std::optional<Luau::SourceCode> readSource(const Luau::ModuleName& name) override {
        Luau::SourceCode::Type sourceType;
        std::optional<std::string> source = std::nullopt;

		SDL_RWops* pFileRead = SDL_RWFromFile(name.c_str(), "rb");
		u64 sourceSize = SDL_RWsize(pFileRead);
		std::string result(sourceSize, 0);
		SDL_RWread(pFileRead, result.data(), sourceSize, 1);
		SDL_RWclose(pFileRead);

		source = result;
		sourceType = Luau::SourceCode::Module;

        if (!source)
            return std::nullopt;

        return Luau::SourceCode{*source, sourceType};
    }

    std::optional<Luau::ModuleInfo> resolveModule(const Luau::ModuleInfo* context, Luau::AstExpr* node) override {
        if (Luau::AstExprConstantString* expr = node->as<Luau::AstExprConstantString>()) {
            Luau::ModuleName name = std::string(expr->value.data, expr->value.size) + ".luau";
			SDL_RWops* pFileRead = SDL_RWFromFile(name.c_str(), "rb");
            if (pFileRead == nullptr) {
                // fall back to .lua if a module with .luau doesn't exist
                name = std::string(expr->value.data, expr->value.size) + ".lua";
            }

            return {{name}};
        }

        return std::nullopt;
    }

    std::string getHumanReadableModuleName(const Luau::ModuleName& name) const override {
        return name;
    }
};

// ***********************************************************************

static void* LuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize) {
	// TODO: lua VM memory tracking?
    if (nsize == 0) {
		RawFree(ptr);
        return nullptr;
    } else {
        return RawRealloc(ptr, nsize, osize, true);
    }
}

// ***********************************************************************

void Init() {
	Arena* pArena = ArenaCreate();
	pState = New(pArena, State);
	pState->pArena = pArena;
}

// ***********************************************************************

void CompileAndLoadProgram(String path) {
	// eventually we'll support multiple independant programs running on the cpu, 
	// and each will have it's own lua state, but for now there's just one

	pState->pProgramState = lua_newstate(LuaAllocator, nullptr);
	lua_State* L = pState->pProgramState;

	// expose builtin libraries
	luaL_openlibs(L);
	Bind::BindGraphics(L);
	Bind::BindInput(L);
	BindUserData(L);
	Serialization::BindSerialization(L);

	// type checking
	bool wasError = false;
	{
		Luau::FrontendOptions frontendOptions;
		frontendOptions.runLintChecks = true;

		LuaFileResolver fileResolver;
		Luau::NullConfigResolver configResolver;
		Luau::Frontend frontend(&fileResolver, &configResolver, frontendOptions);

		unfreeze(frontend.globals.globalTypes);
		Luau::registerBuiltinGlobals(frontend, frontend.globals);

		// Add our global type definitions
		Luau::LoadDefinitionFileResult loadResult = frontend.loadDefinitionFile(
			frontend.globals, frontend.globals.globalScope, std::string_view(polyboxDefinitions.pData), "@polybox", /* captureComments */ false, false);
		Luau::freeze(frontend.globals.globalTypes); // Marks the type memory as read only

		// give errors on parsing the type definitions
		for (Luau::ParseError& error : loadResult.parseResult.errors) {
			Log::Warn("Builtins SyntaxError at [%d] - %s", error.getLocation().begin.line + 1, error.getMessage().c_str());
		}

		if (loadResult.module) {
			for (Luau::TypeError& error : loadResult.module->errors) {
				Log::Warn("Builtins TypeError at [%d] - %s", error.location.begin.line + 1, Luau::toString(error, Luau::TypeErrorToStringOptions{frontend.fileResolver}).c_str());
			}
		}

		// run check
		Luau::CheckResult result = frontend.check(path.pData);

		if (!result.errors.empty())
			wasError = true;

		// print errors and lint warnings
		for (Luau::TypeError& error : result.errors) {
			std::string humanReadableName = frontend.fileResolver->getHumanReadableModuleName(error.moduleName);
			if (const Luau::SyntaxError* syntaxError = Luau::get_if<Luau::SyntaxError>(&error.data))
				Log::Warn("SyntaxError in [%s:%d] - %s", humanReadableName.c_str(), error.location.begin.line + 1, syntaxError->message.c_str());
			else
				Log::Warn("TypeError in [%s:%d] - %s", humanReadableName.c_str(), error.location.begin.line + 1, Luau::toString(error, Luau::TypeErrorToStringOptions{frontend.fileResolver}).c_str());
		}

		for (Luau::LintWarning& error : result.lintResult.errors) {
			Log::Warn("LintError in [%s:%d] (%s) - %s", path.pData, error.location.begin.line + 1,
				  Luau::LintWarning::getName(error.code), error.text.c_str());
		}
		
		for (Luau::LintWarning& error : result.lintResult.warnings) {
			Log::Warn("LintWarning in [%s:%d] (%s) - %s", path.pData, error.location.begin.line + 1,
				  Luau::LintWarning::getName(error.code), error.text.c_str());
		}
	}

	// Compile and load the program
	if (wasError == false) {
		SDL_RWops* pFileRead = SDL_RWFromFile(path.pData, "rb");
		u64 sourceSize = SDL_RWsize(pFileRead);
		char* pSource = New(g_pArenaFrame, char, sourceSize);
		SDL_RWread(pFileRead, pSource, sourceSize, 1);
		pSource[sourceSize] = '\0';
		SDL_RWclose(pFileRead);

		u64 bytecodeSize = 0;
		char* pBytecode = luau_compile(pSource, sourceSize, nullptr, &bytecodeSize);
		int result = luau_load(L, "assets/game.lua", pBytecode, bytecodeSize, 0);

		if (result != 0) {
			Log::Warn("Lua Load Error: %s", lua_tostring(L, lua_gettop(L)));
			lua_pop(L, 1);
		}

		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			Log::Warn("Lua Runtime Error: %s", lua_tostring(L, lua_gettop(L)));
			lua_pop(L, 1);
		}
	}
}

// ***********************************************************************

void Start() {
	lua_State* L = pState->pProgramState;
	lua_getglobal(L, "start");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			Log::Warn("Lua Runtime Error: %s", lua_tostring(L, lua_gettop(L)));
			lua_pop(L, 1);
		}
	}
}

// ***********************************************************************

void Tick(f32 deltaTime) {
	lua_State* L = pState->pProgramState;
	lua_getglobal(L, "update");
	if (lua_isfunction(L, -1)) {
		lua_pushnumber(L, deltaTime);
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			Log::Warn("Lua Runtime Error: %s", lua_tostring(L, lua_gettop(L)));
		lua_pop(L, 1);
		}
	} else {
		lua_pop(L, 1);
	}
}

// ***********************************************************************

void Close() {
	lua_State* L = pState->pProgramState;
	lua_getglobal(L, "close");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			Log::Warn("Lua Runtime Error: %s", lua_tostring(L, lua_gettop(L)));
		}
	}

	lua_close(L);
}

};
