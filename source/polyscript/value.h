// Copyright 2020-2022 David Colson. All rights reserved.

#include <stdint.h>

namespace ValueType {
enum Enum : uint32_t {
    Invalid,
    F32,
    I32,
    Bool,
    Count
};
}

namespace Operator {
enum Enum : uint32_t {
    Add,
    Subtract,
    Multiply,
    Divide,
    Less,
    Greater,
    Equal,
    And,
    Or,
    UnaryMinus,
    Not,
    Count
};
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