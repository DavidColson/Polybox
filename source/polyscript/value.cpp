// Copyright 2020-2022 David Colson. All rights reserved.

#include "value.h"
#include "lexer.h"

#include <resizable_array.inl>

namespace {
TypeInfo* operatorReturnMap[Operator::Count][TypeInfo::TypeTag::Count][TypeInfo::TypeTag::Count];
Value (*operatorMap[Operator::Count][TypeInfo::TypeTag::Count][TypeInfo::TypeTag::Count])(Value v1, Value v2);
Operator::Enum tokenToOperatorMap[TokenType::Count];

ResizableArray<TypeInfo> typeTable;
}

void FreeFunction(Function* pFunc) {
    pFunc->m_chunk.constants.Free();
    pFunc->m_chunk.code.Free();
    pFunc->m_chunk.m_lineInfo.Free();
}

void Register(Operator::Enum op, TypeInfo::TypeTag t1, TypeInfo::TypeTag t2, TypeInfo* pRetType, Value (*func)(Value v1, Value v2)) {
    operatorReturnMap[op][t1][t2] = pRetType;
    operatorMap[op][t1][t2] = func;
}

TypeInfo* OperatorReturnType(Operator::Enum op, TypeInfo::TypeTag t1, TypeInfo::TypeTag t2) {
    return operatorReturnMap[op][t1][t2];
}

Value EvaluateOperator(Operator::Enum op, Value v1, Value v2) {
    return operatorMap[op][v1.m_pType->tag][v2.m_pType->tag](v1, v2);
}

Operator::Enum TokenToOperator(TokenType::Enum tokenType) {
    return tokenToOperatorMap[tokenType];
}

TypeInfo* FindOrAddType(TypeInfo* pNewType) {
    for (TypeInfo& info : typeTable) {
        if (info.tag == TypeInfo::TypeTag::Function && pNewType->tag == info.tag) {
            TypeInfoFunction* pNewFunc = (TypeInfoFunction*)pNewType;
            TypeInfoFunction* pOldFunc = (TypeInfoFunction*)&info;
            // Check return type
            bool returnTypeMatch = pNewFunc->returnType == pOldFunc->returnType;

            bool paramsMatch = pNewFunc->params.m_count == pOldFunc->params.m_count;
            if (paramsMatch && returnTypeMatch) {
                for (size_t i = 0; i < pNewFunc->params.m_count; i++) {
                    TypeInfo* pParamType1 = pNewFunc->params[i];
                    TypeInfo* pParamType2 = pOldFunc->params[i];

                    if (pParamType1 != pParamType2) {
                        paramsMatch = false;
                    }
                }
            }

            if (paramsMatch && returnTypeMatch) {
                return &info;
            }
        }
        if (info.tag == pNewType->tag && info.size == pNewType->size) {
            return &info;
        }
    }
    typeTable.PushBack(*pNewType);
    return &typeTable[typeTable.m_count - 1];
}

void InitTypeTable() {
    TypeInfo voidType;
    voidType.tag = TypeInfo::TypeTag::Void;
    voidType.size = 0;
    voidType.name = "void";
    typeTable.PushBack(voidType);

    TypeInfo i32Type;
    i32Type.tag = TypeInfo::TypeTag::Integer;
    i32Type.size = 32;
    voidType.name = "i32";
    typeTable.PushBack(i32Type);

    TypeInfo f32Type;
    f32Type.tag = TypeInfo::TypeTag::Float;
    f32Type.size = 32;
    voidType.name = "f32";
    typeTable.PushBack(f32Type);

    TypeInfo boolType;
    boolType.tag = TypeInfo::TypeTag::Bool;
    boolType.size = 1;
    voidType.name = "bool";
    typeTable.PushBack(boolType);

    TypeInfo typeType;
    typeType.tag = TypeInfo::TypeTag::Type;
    typeType.size = 0;
    voidType.name = "type";
    typeTable.PushBack(typeType);
}

TypeInfo* GetVoidType() {
    return &typeTable[0];
}

TypeInfo* GetI32Type() {
    return &typeTable[1];
}

TypeInfo* GetF32Type() {
    return &typeTable[2];
}

TypeInfo* GetBoolType() {
    return &typeTable[3];
}

TypeInfo* GetTypeType() {
    return &typeTable[4];
}

void InitValueTables() {
    for (size_t i = 0; i < Operator::Count; i++) {
        for (size_t j = 0; j < TypeInfo::TypeTag::Count; j++) {
            for (size_t k = 0; k < TypeInfo::TypeTag::Count; k++) {
                operatorReturnMap[i][j][k] = GetVoidType();
            }
        }
    }

    for (size_t i = 0; i < Operator::Count; i++) {
        for (size_t j = 0; j < TypeInfo::TypeTag::Count; j++) {
            for (size_t k = 0; k < TypeInfo::TypeTag::Count; k++) {
                operatorMap[i][j][k] = nullptr;
            }
        }
    }

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
    Register(Operator::Add, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value + v2.m_f32Value);
    });
    Register(Operator::Add, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value + (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Add, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value + v2.m_i32Value);
    });
    Register(Operator::Add, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value + v2.m_f32Value);
    });

    // Sub
    // F32
    Register(Operator::Subtract, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value - v2.m_f32Value);
    });
    Register(Operator::Subtract, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value - (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Subtract, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value - v2.m_i32Value);
    });
    Register(Operator::Subtract, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value - v2.m_f32Value);
    });

    // Mul
    // F32
    Register(Operator::Multiply, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value * v2.m_f32Value);
    });
    Register(Operator::Multiply, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value * (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Multiply, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value * v2.m_i32Value);
    });
    Register(Operator::Multiply, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value * v2.m_f32Value);
    });

    // Divide
    // F32
    Register(Operator::Divide, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value / v2.m_f32Value);
    });
    Register(Operator::Divide, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value / (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Divide, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value / v2.m_i32Value);
    });
    Register(Operator::Divide, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value / v2.m_f32Value);
    });

    // Less
    // F32
    Register(Operator::Less, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value < v2.m_f32Value);
    });
    Register(Operator::Less, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value < (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Less, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value < v2.m_i32Value);
    });
    Register(Operator::Less, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value < v2.m_f32Value);
    });

    // Greater
    // F32
    Register(Operator::Greater, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value > v2.m_f32Value);
    });
    Register(Operator::Greater, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value > (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Greater, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value > v2.m_i32Value);
    });
    Register(Operator::Greater, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value > v2.m_f32Value);
    });

    // LessEqual
    // F32
    Register(Operator::LessEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value <= v2.m_f32Value);
    });
    Register(Operator::LessEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value <= (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::LessEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value <= v2.m_i32Value);
    });
    Register(Operator::LessEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value <= v2.m_f32Value);
    });

    // GreaterEqual
    // F32
    Register(Operator::GreaterEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value >= v2.m_f32Value);
    });
    Register(Operator::GreaterEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value >= (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::GreaterEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value >= v2.m_i32Value);
    });
    Register(Operator::GreaterEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value >= v2.m_f32Value);
    });

    // Equal
    // F32
    Register(Operator::Equal, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value == v2.m_f32Value);
    });
    Register(Operator::Equal, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value == (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::Equal, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value == v2.m_i32Value);
    });
    Register(Operator::Equal, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value == v2.m_f32Value);
    });

    // Bool
    Register(Operator::Equal, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Bool, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_boolValue == v2.m_boolValue);
    });

    // NotEqual
    // F32
    Register(Operator::NotEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value != v2.m_f32Value);
    });
    Register(Operator::NotEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_f32Value != (float)v2.m_i32Value);
    });

    // I32
    Register(Operator::NotEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_i32Value != v2.m_i32Value);
    });
    Register(Operator::NotEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.m_i32Value != v2.m_f32Value);
    });

    // Bool
    Register(Operator::NotEqual, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Bool, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_boolValue != v2.m_boolValue);
    });

    // And
    // Bool
    Register(Operator::And, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Bool, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_boolValue && v2.m_boolValue);
    });

    // Or
    // Bool
    Register(Operator::Or, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Bool, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.m_boolValue || v2.m_boolValue);
    });

    // Unary Minus
    // F32
    Register(Operator::UnaryMinus, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Void, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(-v1.m_f32Value);
    });
    // I32
    Register(Operator::UnaryMinus, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Void, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(-v1.m_i32Value);
    });

    // Not
    // Bool
    Register(Operator::Not, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Void, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(!v1.m_boolValue);
    });
}
