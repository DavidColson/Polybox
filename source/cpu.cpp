// Copyright David Colson. All rights reserved.

#include "bind_input.h"
#include "bind_graphics.h"
#include "bind_mesh.h"
#include "bind_scene.h"

#include <SDL.h>
#include <lua.h>
#include <luacode.h>
#include <lualib.h>
#include <Luau/Frontend.h>
#include <Luau/BuiltinDefinitions.h>
#include <light_string.h>
#include <log.h>

static const String polyboxDefinitions = R"POLY_LIBS(
--- Graphics API

type Primitive = {
	GetNumVertices: (self: Primitive) -> number,
	GetVertexPosition: (self: Primitive, index: number) -> (number, number, number),
	GetVertexColor: (self: Primitive, index: number) -> (number, number, number, number),
	GetVertexTexCoord: (self: Primitive, index: number) -> (number, number),
	GetVertexNormal: (self: Primitive, index: number) -> (number, number, number),
	GetMaterialTextureId: (self: Primitive) -> number,
 	GetType: (self: Primitive) -> string,
}

type Mesh = {
 	GetName: (self: Mesh) -> string,
	GetNumPrimitives: (self: Mesh) -> number,
	GetPrimitive: (self: Mesh, index: number) -> Primitive,
 	GetType: (self: Mesh) -> string,
}

type Image = {
 	GetWidth: (self: Image) -> number,
	GetHeight: (self: Image) -> number,
 	GetType: (self: Image) -> string,
}

type Font = {
 	GetType: (self: Font) -> string,
}

@checked declare function LoadMeshes(path: string): { [number]: Mesh }
@checked declare function LoadTextures(path: string): { [number]: Image }
@checked declare function BeginObject2D(primitiveType: string)
@checked declare function EndObject2D(primitiveType: string)
@checked declare function Vertex(x: number, y: number, z: number?)
@checked declare function BeginObject3D(primitiveType: string)
@checked declare function EndObject3D()
@checked declare function Color(r: number, g: number, b: number, a: number)
@checked declare function TexCoord(u: number, v: number)
@checked declare function Normal(x: number, y: number, z: number)
@checked declare function SetClearColor(r: number, g: number, b: number, a: number)
@checked declare function MatrixMode(mode: string)
@checked declare function Perspective(screenWidth: number, screenHeight: number, nearPlane: number, farPlane: number, fov: number)
@checked declare function Translate(x: number, y: number, z: number)
@checked declare function Rotate(x: number, y: number, z: number)
@checked declare function Scale(x: number, y: number, z: number)
@checked declare function Identity()
@checked declare function BindTexture(texture: Image)
@checked declare function UnbindTexture()
@checked declare function NormalsMode(mode: string)
@checked declare function EnableLighting(enable: boolean)
@checked declare function Light(id: number, dirX: number, dixY: number, dirZ: number, r: number, g: number, b: number)
@checked declare function Ambient(r: number, g: number, b: number)
@checked declare function EnableFog(enable: boolean)
@checked declare function SetFogStart(fogStart: number)
@checked declare function SetFogEnd(fogEnd: number)
@checked declare function SetFogColor(r: number, g: number, b: number)
@checked declare function DrawSprite(sprite: Image, x: number, y: number)
@checked declare function DrawSpriteRect(sprite: Image, x: number, y: number, z: number, w: number, posX: number, posY: number)
@checked declare function DrawText(text: string, x: number, y: number, size: number)
@checked declare function DrawTextEx(text: string, x: number, y: number, r: number, g: number, b: number, a: number, font: Font, size: number)
@checked declare function DrawPixel(x: number, y: number, r: number, g: number, b: number, a: number)
@checked declare function DrawLine(startx: number, starty: number, endx: number, endy: number, r: number, g: number, b: number, a: number)
@checked declare function DrawCircle(x: number, y: number, radius: number, r: number, g: number, b: number, a: number)
@checked declare function DrawCircleOutline(x: number, y: number, radius: number, r: number, g: number, b: number, a: number)
@checked declare function DrawRectangle(bottomLeftx: number, bottomeLeftx: number, topRightx: number, topRighty: number, r: number, g: number, b: number, a: number)
@checked declare function DrawRectangleOutline(bottomLeftx: number, bottomeLeftx: number, topRightx: number, topRighty: number, r: number, g: number, b: number, a: number)
@checked declare function DrawBox(x: number, y: number, z: number, width: number, height: number, depth: number)
@checked declare function DrawIcosahedron(maxDepth: number)

--- Scene API

export type Node = {
	GetNumChildren: (self: Node) -> number,
	GetChild: (self: Node, index: number) -> Node,
	GetPropertyTable: (self: Node) -> any,
	GetLocalPosition: (self: Node) -> (number, number, number),
	GetWorldPosition: (self: Node) -> (number, number, number),
	SetLocalPosition: (self: Node, number, number, number) -> (),	
	GetLocalRotation: (self: Node) -> (number, number, number),
	GetWorldRotation: (self: Node) -> (number, number, number),
	SetLocalRotation: (self: Node, number, number, number) -> (),
	GetLocalScale: (self: Node) -> (number, number, number),
	GetWorldScale: (self: Node) -> (number, number, number),
	SetLocalScale: (self: Node, number, number, number) -> (),
	GetType: (self: Node) -> string,
}

export type Scene = {
	GetNumNodes: (self: Scene) -> number,
	GetNode: (self: Scene, index: number) -> Node,
	GetType: (self: Scene) -> string,
}

@checked declare function LoadScene(path: string): Scene

--- Input API

declare Button: {
	Invalid: number,
	FaceBottom: number,
	FaceRight: number,
	FaceLeft: number,
	FaceTop: number,
	LeftStick: number,
	RightStick: number,
	LeftShoulder: number,
	RightShoulder: number,
	DpadDown: number,
	DpadLeft: number,
	DpadRight: number,
	DpadUp: number,
	Start: number,
	Select: number,
}

declare Axis: {
	Invalid: number,
	LeftX: number,
	LeftY: number,
	RightX: number,
	RightY: number,
	TriggerLeft: number,
	TriggerRight: number,
}

declare Key: {
	Invalid: number,
	A: number,
	B: number,
	C: number,
	D: number,
	E: number,
	F: number,
	G: number,
	H: number,
	I: number,
	J: number,
	K: number,
	L: number,
	M: number,
	N: number,
	O: number,
	P: number,
	Q: number,
	R: number,
	S: number,
	T: number,
	U: number,
	V: number,
	W: number,
	X: number,
	Y: number,
	Z: number,
	No1: number,
	No2: number,
	No3: number,
	No4: number,
	No5: number,
	No6: number,
	No7: number,
	No8: number,
	No9: number,
	No0: number,
	Return: number,
	Escape: number,
	Backspace: number,
	Tab: number,
	Space: number,
	Exclaim: number,
	QuoteDbl: number,
	Hash: number,
	Percent: number,
	Dollar: number,
	Ampersand: number,
	Quote: number,
	LeftParen: number,
	RightParen: number,
	Asterisk: number,
	Plus: number,
	Comma: number,
	Minus: number,
	Period: number,
	Slash: number,
	Colon: number,
	Semicolon: number,
	Less: number,
	Equals: number,
	Greater: number,
	Question: number,
	At: number,
	LeftBracket: number,
	Backslash: number,
	RightBracket: number,
	Caret: number,
	Underscore: number,
	BackQuote: number,
	CapsLock: number,
	F1: number,
	F2: number,
	F3: number,
	F4: number,
	F5: number,
	F6: number,
	F7: number,
	F8: number,
	F9: number,
	F10: number,
	F11: number,
	F12: number,
	PrintScreen: number,
	ScrollLock: number,
	Pause: number,
	Insert: number,
	Home: number,
	PageUp: number,
	Delete: number,
	End: number,
	PageDown: number,
	Right: number,
	Left: number,
	Down: number,
	Up: number,
	NumLock: number,
	KpDivide: number,
	KpMultiply: number,
	KpMinus: number,
	KpPlus: number,
	KpEnter: number,
	Kp1: number,
	Kp2: number,
	Kp3: number,
	Kp4: number,
	Kp5: number,
	Kp6: number,
	Kp7: number,
	Kp8: number,
	Kp9: number,
	Kp0: number,
	KpPeriod: number,
	LeftCtrl: number,
	LeftShift: number,
	LeftAlt: number,
	LeftGui: number,
	RightCtrl: number,
	RightShift: number,
	RightAlt: number,
	RightGui: number,
}

@checked declare function GetButton(button: number): boolean 
@checked declare function GetButtonDown(button: number) : boolean 
@checked declare function GetButtonUp(button: number) : boolean 
@checked declare function GetAxis(button: number) : number 
@checked declare function GetMousePosition() : (number, number)
@checked declare function EnableMouseRelativeMode(enable: boolean)
@checked declare function GetKey(key: number) : boolean
@checked declare function GetKeyDown(key: number) : boolean
@checked declare function GetKeyUp(key: number) : boolean
@checked declare function InputString() : string
)POLY_LIBS";


namespace Cpu {

struct State {
	lua_State* pProgramState;
};

State* pState = nullptr;

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

static void* LuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    if (nsize == 0) {
		free(ptr);
        return nullptr;
    } else {
        return realloc(ptr, nsize);
    }
}

void Init() {
	pState = (State*)g_Allocator.Allocate(sizeof(State));
    SYS_P_NEW(pState) State();
}

void CompileAndLoadProgram(String path) {
	// eventually we'll support multiple independant programs running on the cpu, 
	// and each will have it's own lua state, but for now there's just one

	pState->pProgramState = lua_newstate(LuaAllocator, nullptr);
	lua_State* L = pState->pProgramState;

	// expose builtin libraries
	luaL_openlibs(L);
	Bind::BindGraphics(L);
	Bind::BindMesh(L);
	Bind::BindScene(L);
	Bind::BindInput(L);

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
		char* pSource = (char*)g_Allocator.Allocate(sourceSize * sizeof(char));
		SDL_RWread(pFileRead, pSource, sourceSize, 1);
		pSource[sourceSize] = '\0';
		SDL_RWclose(pFileRead);

		usize bytecodeSize = 0;
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

void Start() {
	lua_State* L = pState->pProgramState;
	lua_getglobal(L, "Start");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			Log::Warn("Lua Runtime Error: %s", lua_tostring(L, lua_gettop(L)));
			lua_pop(L, 1);
		}
	}
}

void Tick(f32 deltaTime) {
	lua_State* L = pState->pProgramState;
	lua_getglobal(L, "Update");
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

void End() {
	lua_State* L = pState->pProgramState;
	lua_getglobal(L, "End");
	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			Log::Warn("Lua Runtime Error: %s", lua_tostring(L, lua_gettop(L)));
		}
	}

	lua_close(L);
}

};
