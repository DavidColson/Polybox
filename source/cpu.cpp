// Copyright David Colson. All rights reserved.

static const String polyboxDefinitions = R"POLY_LIBS(
--- Serialization

@checked declare function serialize(value: any, format: number, metadata: any?): string
@checked declare function deserialize(value: string): (any, any?)

--- Filesystem

@checked declare function store(filename: string, value: any, format: number, metadata: any?)
@checked declare function load(filename: string): (any, any?)
@checked declare function make_directory(filename: string, makeAll: boolean): boolean
@checked declare function remove(filename: string): boolean
@checked declare function copy(from: string, to: string): boolean
@checked declare function move(from: string, to: string): boolean

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

// ***********************************************************************

struct LuaFileResolver : Luau::FileResolver {
    std::optional<Luau::SourceCode> readSource(const Luau::ModuleName& name) override {
        Luau::SourceCode::Type sourceType;
        std::optional<std::string> source = std::nullopt;

		File file = OpenFile(String(name.c_str()), FM_READ);
		defer(CloseFile(file));

		if (!IsValid(file))
			return std::nullopt;

		i64 sourceSize = GetFileSize(file);
		std::string result(sourceSize, 0);
		ReadFromFile(file, result.data(), sourceSize);

		source = result;
		sourceType = Luau::SourceCode::Module;

        if (!source)
            return std::nullopt;

        return Luau::SourceCode{*source, sourceType};
    }

    std::optional<Luau::ModuleInfo> resolveModule(const Luau::ModuleInfo* context, Luau::AstExpr* node) override {
        if (Luau::AstExprConstantString* expr = node->as<Luau::AstExprConstantString>()) {
			StringBuilder builder(g_pArenaFrame);
			builder.AppendChars(expr->value.data, expr->value.size);

			// we expect include calls to give the exact filename
			// so we won't construct any extensions or what have you
			String realPath;
			VFSPathToRealPath(builder.CreateString(g_pArenaFrame, false), realPath, g_pArenaFrame); 

            Luau::ModuleName name = std::string(realPath.pData, realPath.length);
            return {{name}};
        }

        return std::nullopt;
    }

    std::string getHumanReadableModuleName(const Luau::ModuleName& name) const override {
		String vfsBasePath = TempPrint("system/%s/", Cpu::GetAppName().pData); 
		std::string newName = std::string(name.c_str()+vfsBasePath.length, name.size()-vfsBasePath.length);
		return newName;
    }
};

// ***********************************************************************

struct State {
	Arena* pArena;
	bool appLoaded;

	lua_State* pProgramState;

	LuaFileResolver fileResolver;
	Luau::NullConfigResolver configResolver;
	Luau::Frontend frontend;

	ResizableArray<String> luaFilesToWatch;
	FileWatcher* pWatcher;
	String appName;
};

State* pState = nullptr;
bool CompileAndLoadModule(String modulePath, i32 nReturns);

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

void OnCodeEdit(FileChange change, void* pUserData) {
	for (int i=0; i<pState->luaFilesToWatch.count; i++) {
		if (pState->luaFilesToWatch[i] == change.path) {
			Log::Info("Luau file %s changed, reloading", change.path.pData);
			CompileAndLoadModule(change.path, 0);
		}
	}
}

// ***********************************************************************

int LuaInclude(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

	// unlike require we assume the name is an actual file name
	String realPath;
	VFSPathToRealPath(String(name), realPath, pState->pArena); 
	pState->luaFilesToWatch.PushBack(realPath);


	// @todo: Use luau's requireresolver to cache modules
	// @todo: put modules in another thread and sandbox them
	
	CompileAndLoadModule(realPath, 1);
	return 1;
}

// ***********************************************************************

void Init() {
	Arena* pArena = ArenaCreate();
	pState = New(pArena, State);
	pState->pArena = pArena;
	pState->appLoaded = false;

	pState->luaFilesToWatch.pArena = pArena;
	pState->pWatcher = FileWatcherCreate(OnCodeEdit, FC_MODIFIED, (void*)pState, true);

	// Setup our frontend
	Luau::FrontendOptions frontendOptions;
	frontendOptions.runLintChecks = true;

	// regrettably luau::frontend is very RAII heavy, so we must do this
	PlacementNew(&pState->fileResolver) LuaFileResolver();
	PlacementNew(&pState->configResolver) Luau::NullConfigResolver();
	PlacementNew(&pState->frontend) Luau::Frontend(&pState->fileResolver, &pState->configResolver, frontendOptions);

	unfreeze(pState->frontend.globals.globalTypes);
	Luau::registerBuiltinGlobals(pState->frontend, pState->frontend.globals);

	// Add our global type definitions
	Luau::LoadDefinitionFileResult loadResult = pState->frontend.loadDefinitionFile(
		pState->frontend.globals, pState->frontend.globals.globalScope, std::string_view(polyboxDefinitions.pData), "@polybox", /* captureComments */ false, false);
	Luau::freeze(pState->frontend.globals.globalTypes); // Marks the type memory as read only

	// give errors on parsing the type definitions
	for (Luau::ParseError& error : loadResult.parseResult.errors) {
		Log::Warn("Builtins SyntaxError at [%d] - %s", error.getLocation().begin.line + 1, error.getMessage().c_str());
	}

	if (loadResult.module) {
		for (Luau::TypeError& error : loadResult.module->errors) {
			Log::Warn("Builtins TypeError at [%d] - %s", error.location.begin.line + 1, Luau::toString(error, Luau::TypeErrorToStringOptions{pState->frontend.fileResolver}).c_str());
		}
	}
}

// ***********************************************************************

bool CompileAndLoadModule(String modulePath, i32 nReturns) {
	bool wasError = false;

	// run typechecking
	Luau::CheckResult result = pState->frontend.check(modulePath.pData);

	if (!result.errors.empty())
		wasError = true;

	// print errors and lint warnings
	for (Luau::TypeError& error : result.errors) {
		std::string humanReadableName = pState->frontend.fileResolver->getHumanReadableModuleName(error.moduleName);
		if (const Luau::SyntaxError* syntaxError = Luau::get_if<Luau::SyntaxError>(&error.data))
			Log::Warn("SyntaxError in [%s:%d] - %s", humanReadableName.c_str(), error.location.begin.line + 1, syntaxError->message.c_str());
			else
			Log::Warn("TypeError in [%s:%d] - %s", humanReadableName.c_str(), error.location.begin.line + 1, Luau::toString(error, Luau::TypeErrorToStringOptions{pState->frontend.fileResolver}).c_str());
	}

	for (Luau::LintWarning& error : result.lintResult.errors) {
		Log::Warn("LintError in [%s:%d] (%s) - %s", modulePath.pData, error.location.begin.line + 1,
			Luau::LintWarning::getName(error.code), error.text.c_str());
	}

	for (Luau::LintWarning& error : result.lintResult.warnings) {
		Log::Warn("LintWarning in [%s:%d] (%s) - %s", modulePath.pData, error.location.begin.line + 1,
			Luau::LintWarning::getName(error.code), error.text.c_str());
	}

	// Compile and load
	if (wasError == false) {
		i64 sourceSize;
		char* pSource = ReadWholeFile(modulePath, &sourceSize, g_pArenaFrame);

		u64 bytecodeSize = 0;
		char* pBytecode = luau_compile(pSource, sourceSize, nullptr, &bytecodeSize);
		int result = luau_load(pState->pProgramState, modulePath.pData, pBytecode, bytecodeSize, 0);

		if (result != 0) {
			Log::Warn("Lua Load Error: %s", lua_tostring(pState->pProgramState, lua_gettop(pState->pProgramState)));
			lua_pop(pState->pProgramState, 1);
			return false;
		}

		if (lua_pcall(pState->pProgramState, 0, nReturns, 0) != LUA_OK) {
			Log::Warn("Lua Runtime Error: %s", lua_tostring(pState->pProgramState, lua_gettop(pState->pProgramState)));
			lua_pop(pState->pProgramState, 1);
			return false;
		}
	}
	return !wasError;
}

// ***********************************************************************

void LoadApp(String appName) {
	// eventually we'll support multiple independant programs running on the cpu, 
	// and each will have it's own lua state, but for now there's just one
	if (appName.length == 0) {
		Log::Warn("No app specified to boot");
		return;
	}

	if (appName == "shared") {
		Log::Warn("\"shared\" is not a valid name for a project, this conflicts with system folders");
		return;
	}

	pState->appName= appName;

	// search for a project in system/ with appname
	StringBuilder builder(g_pArenaFrame);
	builder.Append("system/");
	builder.Append(appName);

	// watch the project's directory for luau file changes
	FileWatcherAddDirectory(pState->pWatcher, builder.CreateString(g_pArenaFrame, false));

	builder.Append("/main.luau");
	String appMainPath = builder.CreateString(pState->pArena);

	if (!FileExists(appMainPath)) {
		Log::Warn("A main.lua file for project '%s' could not be found", appName.pData);
		return;
	}
	pState->luaFilesToWatch.PushBack(appMainPath);

	// create a program state
	pState->pProgramState = lua_newstate(LuaAllocator, nullptr);
	lua_State* L = pState->pProgramState;

	// expose builtin libraries
	const luaL_Reg coreFuncs[] = {
		{ "include", LuaInclude },
        { NULL, NULL }
	};
	luaL_openlibs(L);
	Bind::BindGraphics(L);
	Bind::BindInput(L);
	BindUserData(L);
	BindSerialization(L);
	BindFileSystem(L);
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, NULL, coreFuncs);
    lua_pop(L, 1);

	// compile and load main.luau
	pState->appLoaded = CompileAndLoadModule(appMainPath, 0);
}

// ***********************************************************************

void Start() {
	if (!pState->appLoaded) return;

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
	if (!pState->appLoaded) return;

	FileWatcherProcessChanges(pState->pWatcher);

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
	if (!pState->appLoaded) return;

	lua_State* L = pState->pProgramState;
	lua_getglobal(L, "close");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			Log::Warn("Lua Runtime Error: %s", lua_tostring(L, lua_gettop(L)));
		}
	}

	lua_close(L);
}

String GetAppName() {
	return pState->appName;
}
};
