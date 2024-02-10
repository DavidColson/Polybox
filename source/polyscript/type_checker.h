// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "value.h"

#include <light_string.h>
#include <hashmap.h>
#include <resizable_array.h>

struct Compiler;
struct TypeInfo;
namespace Ast { struct Declaration; }

// ***********************************************************************

namespace ScopeKind {
enum Enum {
    Invalid,
    // data scope
    Struct,
    Function,
    FunctionType,

    // imperative scope
    Global,
    Block
};
static const char* stringNames[] = { "Invalid", "Struct", "Function", "FunctionType", "Global", "Block"};
static const char* ToString(Enum kind) {
    return stringNames[kind];
}
}

bool CheckIsDataScope(ScopeKind::Enum scopeKind);

struct Entity;
struct Scope {
    Scope* pParent;
    ScopeKind::Enum kind{ ScopeKind::Invalid };
    HashMap<String, Entity*> entities;
    ResizableArray<Scope*> children;

    // If the kind is a function or function type, find the typeinfo here
    TypeInfoFunction* pFunctionType{ nullptr };

    // The stack size when we entered the scope, used for popping locals as we leave
    i32 codeGenStackAtEntry{ 0 }; 
    // Used for tracking the base of the stack frame for the function we're in, relevant only for function scopes
    i32 codeGenStackFrameBase{ 0 }; 

    u32 startLine { 0 };
    u32 endLine { 0 };
};

// ***********************************************************************

namespace EntityKind {
enum Enum {
    Invalid,
    Variable,
    Constant,
    Function
};
static const char* stringNames[] = { "Invalid", "Variable", "Constant", "Function"};
static const char* ToString(Enum kind) {
    return stringNames[kind];
}
}

namespace EntityStatus {
enum Enum {
    Unresolved,
    InProgress,
    Resolved
};
}

// Represents some named object in the language, such as a variable, function, type, or constant
struct Entity {
    EntityKind::Enum kind{ EntityKind::Invalid };
    String name;

    EntityStatus::Enum status{ EntityStatus::Unresolved };
    TypeInfo* pType{ nullptr };
    Ast::Declaration* pDeclaration{ nullptr };
    bool isLive{ false }; // used for non const variables. Means that it's in memory and usable

    // Set only for constant entities, I may change this to be a union for variables etc
    Value constantValue;
    u32 codeGenConstIndex;
    
    // These are instruction indices that await knowing the function pointer value when generated
    // Once the function has been generated, it's pointer is in constantValue
    bool bFunctionHasBeenGenerated{ false };
    ResizableArray<size> pendingFunctionConstants;
};

Entity* FindEntity(Scope* pLowestSearchScope, String name);

// ***********************************************************************

void TypeCheckProgram(Compiler& compilerState);
