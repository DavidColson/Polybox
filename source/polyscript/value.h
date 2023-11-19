// Copyright 2020-2022 David Colson. All rights reserved.
#pragma once

#include <stdint.h>

#include <light_string.h>
#include <resizable_array.h>


namespace TokenType {
enum Enum : uint32_t;
}

namespace Operator {
enum Enum : uint32_t {
    Add,
    Subtract,
    Multiply,
    Divide,
    Less,
    Greater,
    GreaterEqual,
    LessEqual,
    Equal,
    NotEqual,
    And,
    Or,
    UnaryMinus,
    Not,
    Count
};

static const char* stringNames[] = { "+", "-", "*", "/", "<", ">", ">=", "<=", "==", "!=", "&&", "||", "-", "!", "count" };
static const char* ToString(Operator::Enum type) {
    return stringNames[type];
}
}

struct TypeInfo {
    enum TypeTag : uint8_t {
        Invalid,
        Void,
        F32,
        I32,
        Bool,
        Function,
		Type,
		Struct,
        Count
    };
	static const char* TagToString(TypeTag tag) {
		static const char* stringNames[] = { "Invalid", "Void", "f32", "i32", "bool", "function", "Type" };
		return stringNames[tag];
	}

    TypeTag tag = TypeTag::Void;
    size_t size = 0;
    String name;
};

struct TypeInfoFunction : public TypeInfo {
    ResizableArray<TypeInfo*> params;
    TypeInfo* pReturnType;
};

struct TypeInfoStruct : public TypeInfo {
	struct Member {
		String identifier;
		TypeInfo* pType;
		size_t offset;
	};
	ResizableArray<Member> members;
};

void InitTypeTable();

bool CheckTypesIdentical(TypeInfo* pType1, TypeInfo* pType2);

void AddTypeForRuntime(TypeInfo* pNewType);

TypeInfo* FindTypeByName(String identifier);

TypeInfo* GetInvalidType();

TypeInfo* GetVoidType();

TypeInfo* GetI32Type();

TypeInfo* GetF32Type();

TypeInfo* GetBoolType();

TypeInfo* GetTypeType();

TypeInfo* GetEmptyFuncType();






struct Value {
    union {
        bool boolValue;
        float f32Value;
        int32_t i32Value;
        int64_t functionPointer; // this number is the IP offset of the start of the function
        TypeInfo* pTypeInfo;
		void* pPtr;
    };
};

inline Value MakeValueNill() {
    Value v;
    v.pPtr = nullptr;
    return v;
} 

inline Value MakeValue(bool value) {
    Value v;
    v.boolValue = value;
    return v;
}

inline Value MakeValue(float value) {
    Value v;
    v.f32Value = value;
    return v;
}

inline Value MakeValue(int32_t value) {
    Value v;
    v.i32Value = value;
    return v;
}

inline Value MakeValue(TypeInfo* value) {
    Value v;
    v.pTypeInfo = value;
    return v;
}

inline Value MakeFunctionValue(int64_t ipOffset) {
    Value v;
    v.functionPointer = ipOffset;
    return v;
}

void InitTokenToOperatorMap();

Operator::Enum TokenToOperator(TokenType::Enum tokenType);

