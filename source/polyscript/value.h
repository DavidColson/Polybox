// Copyright 2020-2022 David Colson. All rights reserved.
#pragma once

#include <stdint.h>

#include <light_string.h>
#include <resizable_array.h>


namespace TokenType {
enum Enum : uint32_t;
}

namespace ValueType {

enum Enum : uint32_t {
    Void,
    F32,
    I32,
    Bool,
    Function,
    Type,
    Count
};

static const char* stringNames[] = { "Void", "f32", "i32", "bool", "function", "type", "count" };
static const char* ToString(ValueType::Enum type) {
    return stringNames[type];
}
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
    enum TypeTag : uint32_t {
        Void,
        Float,
        Integer,
        Bool,
        Function,
        Type,
        Count
    };
    TypeTag tag = TypeTag::Void;
    size_t size = 0;
    String name;
};

struct TypeInfoFunction : public TypeInfo {
    ResizableArray<TypeInfo*> params;
    TypeInfo* pReturnType;
};

void InitTypeTable();

TypeInfo* FindOrAddType(TypeInfo* pNewType);

TypeInfo* GetVoidType();

TypeInfo* GetI32Type();

TypeInfo* GetF32Type();

TypeInfo* GetBoolType();

TypeInfo* GetTypeType();

TypeInfo* GetEmptyFuncType();





struct Function;
struct Value {
    //ValueType::Enum m_type { ValueType::Void };
    TypeInfo* m_pType { GetVoidType() };
    union {
        bool m_boolValue;
        float m_f32Value;
        int32_t m_i32Value;
        Function* m_pFunction;
    };
};

inline Value MakeValue(bool value) {
    Value v;
    v.m_pType = GetBoolType();
    v.m_boolValue = value;
    return v;
}

inline Value MakeValue(float value) {
    Value v;
    v.m_pType = GetF32Type();
    v.m_f32Value = value;
    return v;
}

inline Value MakeValue(int32_t value) {
    Value v;
    v.m_pType = GetI32Type();
    v.m_i32Value = value;
    return v;
}


struct CodeChunk {
    ResizableArray<Value> constants;
    ResizableArray<uint8_t> code;
    ResizableArray<uint32_t> m_lineInfo;
};

struct Function {
    String m_name;
    CodeChunk m_chunk;
};

void FreeFunction(Function* pFunc);



void InitValueTables();

Value EvaluateOperator(Operator::Enum op, Value v1, Value v2);

TypeInfo* OperatorReturnType(Operator::Enum op, TypeInfo::TypeTag t1, TypeInfo::TypeTag t2);

Operator::Enum TokenToOperator(TokenType::Enum tokenType);

