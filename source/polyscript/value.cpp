// Copyright 2020-2022 David Colson. All rights reserved.

#include "value.h"

namespace {
ValueType::Enum operatorReturnMap[Operator::Count][ValueType::Count][ValueType::Count];

Value (*operatorMap[Operator::Count][ValueType::Count][ValueType::Count])(Value v1, Value v2);
}

void Register(Operator::Enum op, ValueType::Enum t1, ValueType::Enum t2, ValueType::Enum ret, Value (*func) (Value v1, Value v2)) {
    operatorReturnMap[op][t1][t2] = ret;
    operatorMap[op][t1][t2] = func;
}

Value EvaluateOperator(Operator::Enum op, Value v1, Value v2) {
    return operatorMap[op][v1.m_type][v2.m_type](v1, v2);
}

void InitValueTables() {
    for (size_t i = 0; i < Operator::Count; i++) {
        for (size_t j = 0; j < ValueType::Count; j++) {
            for (size_t k = 0; k < ValueType::Count; k++) {
                operatorReturnMap[i][j][k] = ValueType::Invalid;
            }
        }
    }

    for (size_t i = 0; i < Operator::Count; i++) {
        for (size_t j = 0; j < ValueType::Count; j++) {
            for (size_t k = 0; k < ValueType::Count; k++) {
                operatorMap[i][j][k] = nullptr;
            }
        }
    }

    // Add
    // F32
    Register(Operator::Add, ValueType::F32, ValueType::F32, ValueType::F32, [](Value v1, Value v2) { 
        return MakeValue(v1.m_f32Value + v2.m_f32Value); 
        });
    Register(Operator::Add, ValueType::F32, ValueType::I32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value + (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Add, ValueType::I32, ValueType::I32, ValueType::I32, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value + v2.m_i32Value);
    });
    Register(Operator::Add, ValueType::I32, ValueType::F32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value + v2.m_f32Value);
    });

    // Sub
    // F32
    Register(Operator::Subtract, ValueType::F32, ValueType::F32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value - v2.m_f32Value);
    });
    Register(Operator::Subtract, ValueType::F32, ValueType::I32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value - (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Subtract, ValueType::I32, ValueType::I32, ValueType::I32, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value - v2.m_i32Value);
    });
    Register(Operator::Subtract, ValueType::I32, ValueType::F32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value - v2.m_f32Value);
    });

    // Mul
    // F32
    Register(Operator::Multiply, ValueType::F32, ValueType::F32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value * v2.m_f32Value);
    });
    Register(Operator::Multiply, ValueType::F32, ValueType::I32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value * (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Multiply, ValueType::I32, ValueType::I32, ValueType::I32, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value * v2.m_i32Value);
    });
    Register(Operator::Multiply, ValueType::I32, ValueType::F32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value * v2.m_f32Value);
    });

    // Divide
    // F32
    Register(Operator::Divide, ValueType::F32, ValueType::F32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value / v2.m_f32Value);
    });
    Register(Operator::Divide, ValueType::F32, ValueType::I32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value / (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Divide, ValueType::I32, ValueType::I32, ValueType::I32, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value / v2.m_i32Value);
    });
    Register(Operator::Divide, ValueType::I32, ValueType::F32, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value / v2.m_f32Value);
    });

    // Less
    // F32
    Register(Operator::Less, ValueType::F32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value < v2.m_f32Value);
    });
    Register(Operator::Less, ValueType::F32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value < (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Less, ValueType::I32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value < v2.m_i32Value);
    });
    Register(Operator::Less, ValueType::I32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value < v2.m_f32Value);
    });

    // Greater
    // F32
    Register(Operator::Greater, ValueType::F32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value > v2.m_f32Value);
    });
    Register(Operator::Greater, ValueType::F32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value > (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Greater, ValueType::I32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value > v2.m_i32Value);
    });
    Register(Operator::Greater, ValueType::I32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value > v2.m_f32Value);
    });
    
    // Equal
    // F32
    Register(Operator::Equal, ValueType::F32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value == v2.m_f32Value);
    });
    Register(Operator::Equal, ValueType::F32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value == (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Equal, ValueType::I32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value == v2.m_i32Value);
    });
    Register(Operator::Equal, ValueType::I32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value == v2.m_f32Value);
    });

    // Bool
    Register(Operator::Equal, ValueType::Bool, ValueType::Bool, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_boolValue == v2.m_boolValue);
    });

    // And
    // Bool
    Register(Operator::And, ValueType::Bool, ValueType::Bool, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_boolValue && v2.m_boolValue);
    });

    // Or
    // Bool
    Register(Operator::Or, ValueType::Bool, ValueType::Bool, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_boolValue || v2.m_boolValue);
    });

    // Unary Minus
    // F32
    Register(Operator::UnaryMinus, ValueType::F32, ValueType::Invalid, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(-v1.m_f32Value);
    });
    // I32
    Register(Operator::UnaryMinus, ValueType::I32, ValueType::Invalid, ValueType::I32, [](Value v1, Value v2) {
        return MakeValue(-v1.m_i32Value);
    });

    // Not
    // Bool
    Register(Operator::Not, ValueType::Bool, ValueType::Invalid, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(!v1.m_boolValue);
    });
}

