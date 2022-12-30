// Copyright 2020-2022 David Colson. All rights reserved.
#pragma once

#include <stdint.h>

namespace TokenType {
enum Enum;
}

namespace ValueType {

enum Enum : uint32_t {
    Invalid,
    F32,
    I32,
    Bool,
    Count
};

static const char* stringNames[] = { "invalid", "f32", "i32", "bool", "count" };
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

struct Value {
    ValueType::Enum m_type { ValueType::Invalid };
    union {
        bool m_boolValue;
        float m_f32Value;
        int32_t m_i32Value;
    };
};

inline Value MakeValue(bool value) {
    Value v;
    v.m_type = ValueType::Bool;
    v.m_boolValue = value;
    return v;
}

inline Value MakeValue(float value) {
    Value v;
    v.m_type = ValueType::F32;
    v.m_f32Value = value;
    return v;
}

inline Value MakeValue(int32_t value) {
    Value v;
    v.m_type = ValueType::I32;
    v.m_i32Value = value;
    return v;
}

void InitValueTables();

Value EvaluateOperator(Operator::Enum op, Value v1, Value v2);

ValueType::Enum OperatorReturnType(Operator::Enum op, ValueType::Enum t1, ValueType::Enum t2);

Operator::Enum TokenToOperator(TokenType::Enum tokenType);

