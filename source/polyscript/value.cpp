// Copyright 2020-2022 David Colson. All rights reserved.

#include "value.h"
#include "lexer.h"

#include <resizable_array.inl>

namespace {
ValueType::Enum operatorReturnMap[Operator::Count][ValueType::Count][ValueType::Count];
Value (*operatorMap[Operator::Count][ValueType::Count][ValueType::Count])(Value v1, Value v2);
Operator::Enum tokenToOperatorMap[TokenType::Count];
const char* valueNames[ValueType::Count];
}

void FreeFunction(Function* pFunc) {
    pFunc->m_chunk.constants.Free();
    pFunc->m_chunk.code.Free();
    pFunc->m_chunk.m_lineInfo.Free();
}

void Register(Operator::Enum op, ValueType::Enum t1, ValueType::Enum t2, ValueType::Enum ret, Value (*func) (Value v1, Value v2)) {
    operatorReturnMap[op][t1][t2] = ret;
    operatorMap[op][t1][t2] = func;
}

ValueType::Enum OperatorReturnType(Operator::Enum op, ValueType::Enum t1, ValueType::Enum t2) {
    return operatorReturnMap[op][t1][t2];
}

Value EvaluateOperator(Operator::Enum op, Value v1, Value v2) {
    return operatorMap[op][v1.m_type][v2.m_type](v1, v2);
}

Operator::Enum TokenToOperator(TokenType::Enum tokenType) {
    return tokenToOperatorMap[tokenType];
}

const char* ValueToString(ValueType::Enum type) {
    return valueNames[type];
}

TypeInfo* FindOrAddType(TypeInfo* pNewType) {
    // Search the array, filtering by tag type, to find a duplicate type
    // if one exists, return it, otherwise add the new type to the end of the array
    return nullptr;
}

void InitTypeTable() {
    // Need an array filled with TypeInfos
    // create the basic types
}

void InitValueTables() {
    for (size_t i = 0; i < Operator::Count; i++) {
        for (size_t j = 0; j < ValueType::Count; j++) {
            for (size_t k = 0; k < ValueType::Count; k++) {
                operatorReturnMap[i][j][k] = ValueType::Void;
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

    // Value names
    valueNames[ValueType::F32] = "f32";
    valueNames[ValueType::I32] = "i32";
    valueNames[ValueType::Bool] = "bool";
    valueNames[ValueType::Void] = "void";

    // Token -> Operator mapping
    tokenToOperatorMap[TokenType::Plus] = Operator::Add;
    tokenToOperatorMap[TokenType::Minus] = Operator::Subtract;
    tokenToOperatorMap[TokenType::Star] = Operator::Multiply;
    tokenToOperatorMap[TokenType::Slash] = Operator::Divide;
    tokenToOperatorMap[TokenType::Less] = Operator::Less;
    tokenToOperatorMap[TokenType::Greater] = Operator::Greater;
    tokenToOperatorMap[TokenType::GreaterEqual] = Operator::GreaterEqual;
    tokenToOperatorMap[TokenType::LessEqual] = Operator::LessEqual;
    tokenToOperatorMap[TokenType::EqualEqual] = Operator::Equal;
    tokenToOperatorMap[TokenType::BangEqual] = Operator::NotEqual;
    tokenToOperatorMap[TokenType::And] = Operator::And;
    tokenToOperatorMap[TokenType::Or] = Operator::Or;
    tokenToOperatorMap[TokenType::Bang] = Operator::Not;

    // TODO: Before adding more types to this table, separate out the conversion of types to types
    // Rather than store the operator for f+i, store a table that tells you if i can be cast to f and if so add a cast to do so

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

    // LessEqual
    // F32
    Register(Operator::LessEqual, ValueType::F32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value <= v2.m_f32Value);
    });
    Register(Operator::LessEqual, ValueType::F32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value <= (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::LessEqual, ValueType::I32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value <= v2.m_i32Value);
    });
    Register(Operator::LessEqual, ValueType::I32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value <= v2.m_f32Value);
    });

    // GreaterEqual
    // F32
    Register(Operator::GreaterEqual, ValueType::F32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value >= v2.m_f32Value);
    });
    Register(Operator::GreaterEqual, ValueType::F32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value >= (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::GreaterEqual, ValueType::I32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value >= v2.m_i32Value);
    });
    Register(Operator::GreaterEqual, ValueType::I32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value >= v2.m_f32Value);
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

    // NotEqual
    // F32
    Register(Operator::NotEqual, ValueType::F32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value != v2.m_f32Value);
    });
    Register(Operator::NotEqual, ValueType::F32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value != (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::NotEqual, ValueType::I32, ValueType::I32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value != v2.m_i32Value);
    });
    Register(Operator::NotEqual, ValueType::I32, ValueType::F32, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value != v2.m_f32Value);
    });

    // Bool
    Register(Operator::NotEqual, ValueType::Bool, ValueType::Bool, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(v1.m_boolValue != v2.m_boolValue);
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
    Register(Operator::UnaryMinus, ValueType::F32, ValueType::Void, ValueType::F32, [](Value v1, Value v2) {
        return MakeValue(-v1.m_f32Value);
    });
    // I32
    Register(Operator::UnaryMinus, ValueType::I32, ValueType::Void, ValueType::I32, [](Value v1, Value v2) {
        return MakeValue(-v1.m_i32Value);
    });

    // Not
    // Bool
    Register(Operator::Not, ValueType::Bool, ValueType::Void, ValueType::Bool, [](Value v1, Value v2) {
        return MakeValue(!v1.m_boolValue);
    });
}

