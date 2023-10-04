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
    Global,
    Struct,
    Function,
    FunctionType,

    // imperative scope
    Block
};
static const char* stringNames[] = { "Invalid", "Global", "Struct", "Function", "FunctionType", "Block"};
static const char* ToString(Enum kind) {
    return stringNames[kind];
}
}

struct Entity;
struct Scope {
    Scope* pParent;
    ScopeKind::Enum kind{ ScopeKind::Invalid };
    HashMap<String, Entity*> entities;
    ResizableArray<Scope*> children;

    uint32_t startLine { 0 };
    uint32_t endLine { 0 };
};

// ***********************************************************************

namespace EntityKind {
enum Enum {
    Invalid,
    Variable,
    Constant
};
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
};

// ***********************************************************************

void TypeCheckProgram(Compiler& compilerState);