// Copyright 2020-2022 David Colson. All rights reserved.

#include "value.h"
#include "lexer.h"

#include <resizable_array.inl>

namespace {
TypeInfo* operatorReturnMap[Operator::Count][TypeInfo::TypeTag::Count][TypeInfo::TypeTag::Count];
Value (*operatorMap[Operator::Count][TypeInfo::TypeTag::Count][TypeInfo::TypeTag::Count])(Value v1, Value v2);
Operator::Enum tokenToOperatorMap[TokenType::Count];

ResizableArray<TypeInfo*> typeTable;
}

void FreeFunction(Function* pFunc) {
    pFunc->chunk.constants.Free();
    pFunc->chunk.code.Free();
    pFunc->chunk.lineInfo.Free();
}

void Register(Operator::Enum op, TypeInfo::TypeTag t1, TypeInfo::TypeTag t2, TypeInfo* pRetType, Value (*func)(Value v1, Value v2)) {
    operatorReturnMap[op][t1][t2] = pRetType;
    operatorMap[op][t1][t2] = func;
}

TypeInfo* OperatorReturnType(Operator::Enum op, TypeInfo::TypeTag t1, TypeInfo::TypeTag t2) {
    return operatorReturnMap[op][t1][t2];
}

Value EvaluateOperator(Operator::Enum op, Value v1, Value v2) {
    return operatorMap[op][v1.pType->tag][v2.pType->tag](v1, v2);
}

Operator::Enum TokenToOperator(TokenType::Enum tokenType) {
    return tokenToOperatorMap[tokenType];
}

TypeInfo* FindOrAddType(TypeInfo* pNewType) {
    for (TypeInfo* pInfo : typeTable) {
        if (pInfo->tag == TypeInfo::TypeTag::Function && pNewType->tag == pInfo->tag) {
            TypeInfoFunction* pNewFunc = (TypeInfoFunction*)pNewType;
            TypeInfoFunction* pOldFunc = (TypeInfoFunction*)pInfo;
            // Check return type
            bool returnTypeMatch = pNewFunc->pReturnType == pOldFunc->pReturnType;

            bool paramsMatch = pNewFunc->params.count == pOldFunc->params.count;
            if (paramsMatch && returnTypeMatch) {
                for (size_t i = 0; i < pNewFunc->params.count; i++) {
                    TypeInfo* pParamType1 = pNewFunc->params[i];
                    TypeInfo* pParamType2 = pOldFunc->params[i];

                    if (pParamType1 != pParamType2) {
                        paramsMatch = false;
                    }
                }
            }

            if (paramsMatch && returnTypeMatch) {
                return pInfo;
            }
        } else if (pInfo->tag == pNewType->tag && pInfo->size == pNewType->size) {
            return pInfo;
        }
    }
    TypeInfo* pToAddType;
    switch (pNewType->tag) {
        case TypeInfo::TypeTag::Function: {
            pToAddType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfoFunction));
            memcpy(pToAddType, pNewType, sizeof(TypeInfoFunction));
            break;
        }
        default: {
            pToAddType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfoFunction));
            memcpy(pToAddType, pNewType, sizeof(TypeInfo));
            break;
        }
    }
    typeTable.PushBack(pToAddType);
    return typeTable[typeTable.count - 1];
}

void InitTypeTable() {
    TypeInfo* pVoidType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pVoidType->tag = TypeInfo::TypeTag::Void;
    pVoidType->size = 0;
    pVoidType->name = "void";
    typeTable.PushBack(pVoidType);

    TypeInfo* pI32Type = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pI32Type->tag = TypeInfo::TypeTag::Integer;
    pI32Type->size = 32;
    pI32Type->name = "i32";
    typeTable.PushBack(pI32Type);

    TypeInfo* pF32Type = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pF32Type->tag = TypeInfo::TypeTag::Float;
    pF32Type->size = 32;
    pF32Type->name = "f32";
    typeTable.PushBack(pF32Type);

    TypeInfo* pBoolType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pBoolType->tag = TypeInfo::TypeTag::Bool;
    pBoolType->size = 1;
    pBoolType->name = "bool";
    typeTable.PushBack(pBoolType);

    TypeInfo* pTypeType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pTypeType->tag = TypeInfo::TypeTag::Type;
    pTypeType->size = 0;
    pTypeType->name = "Type";
    typeTable.PushBack(pTypeType);

    TypeInfoFunction* pEmptyFuncType = (TypeInfoFunction*)g_Allocator.Allocate(sizeof(TypeInfoFunction));
    pEmptyFuncType->tag = TypeInfo::TypeTag::Function;
    pEmptyFuncType->size = 0;
    pEmptyFuncType->name = "()";
    typeTable.PushBack(pEmptyFuncType);
}

TypeInfo* GetVoidType() {
    return typeTable[0];
}

TypeInfo* GetI32Type() {
    return typeTable[1];
}

TypeInfo* GetF32Type() {
    return typeTable[2];
}

TypeInfo* GetBoolType() {
    return typeTable[3];
}

TypeInfo* GetTypeType() {
    return typeTable[4];
}

TypeInfo* GetEmptyFuncType() {
    return typeTable[5];
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
        return MakeValue(v1.f32Value + v2.f32Value);
    });
    Register(Operator::Add, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value + (float)v2.i32Value);
    });

    // I32
    Register(Operator::Add, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value + v2.i32Value);
    });
    Register(Operator::Add, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value + v2.f32Value);
    });

    // Sub
    // F32
    Register(Operator::Subtract, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value - v2.f32Value);
    });
    Register(Operator::Subtract, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value - (float)v2.i32Value);
    });

    // I32
    Register(Operator::Subtract, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value - v2.i32Value);
    });
    Register(Operator::Subtract, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value - v2.f32Value);
    });

    // Mul
    // F32
    Register(Operator::Multiply, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value * v2.f32Value);
    });
    Register(Operator::Multiply, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value * (float)v2.i32Value);
    });

    // I32
    Register(Operator::Multiply, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value * v2.i32Value);
    });
    Register(Operator::Multiply, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value * v2.f32Value);
    });

    // Divide
    // F32
    Register(Operator::Divide, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value / v2.f32Value);
    });
    Register(Operator::Divide, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value / (float)v2.i32Value);
    });

    // I32
    Register(Operator::Divide, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value / v2.i32Value);
    });
    Register(Operator::Divide, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value / v2.f32Value);
    });

    // Less
    // F32
    Register(Operator::Less, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value < v2.f32Value);
    });
    Register(Operator::Less, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value < (float)v2.i32Value);
    });

    // I32
    Register(Operator::Less, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value < v2.i32Value);
    });
    Register(Operator::Less, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value < v2.f32Value);
    });

    // Greater
    // F32
    Register(Operator::Greater, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value > v2.f32Value);
    });
    Register(Operator::Greater, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value > (float)v2.i32Value);
    });

    // I32
    Register(Operator::Greater, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value > v2.i32Value);
    });
    Register(Operator::Greater, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value > v2.f32Value);
    });

    // LessEqual
    // F32
    Register(Operator::LessEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value <= v2.f32Value);
    });
    Register(Operator::LessEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value <= (float)v2.i32Value);
    });

    // I32
    Register(Operator::LessEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value <= v2.i32Value);
    });
    Register(Operator::LessEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value <= v2.f32Value);
    });

    // GreaterEqual
    // F32
    Register(Operator::GreaterEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value >= v2.f32Value);
    });
    Register(Operator::GreaterEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value >= (float)v2.i32Value);
    });

    // I32
    Register(Operator::GreaterEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value >= v2.i32Value);
    });
    Register(Operator::GreaterEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value >= v2.f32Value);
    });

    // Equal
    // F32
    Register(Operator::Equal, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value == v2.f32Value);
    });
    Register(Operator::Equal, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value == (float)v2.i32Value);
    });

    // I32
    Register(Operator::Equal, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value == v2.i32Value);
    });
    Register(Operator::Equal, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value == v2.f32Value);
    });

    // Bool
    Register(Operator::Equal, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Bool, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.boolValue == v2.boolValue);
    });

    // NotEqual
    // F32
    Register(Operator::NotEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value != v2.f32Value);
    });
    Register(Operator::NotEqual, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.f32Value != (float)v2.i32Value);
    });

    // I32
    Register(Operator::NotEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Integer, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.i32Value != v2.i32Value);
    });
    Register(Operator::NotEqual, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Float, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue((float)v1.i32Value != v2.f32Value);
    });

    // Bool
    Register(Operator::NotEqual, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Bool, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.boolValue != v2.boolValue);
    });

    // And
    // Bool
    Register(Operator::And, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Bool, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.boolValue && v2.boolValue);
    });

    // Or
    // Bool
    Register(Operator::Or, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Bool, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(v1.boolValue || v2.boolValue);
    });

    // Unary Minus
    // F32
    Register(Operator::UnaryMinus, TypeInfo::TypeTag::Float, TypeInfo::TypeTag::Void, GetF32Type(), [](Value v1, Value v2) {
        return MakeValue(-v1.f32Value);
    });
    // I32
    Register(Operator::UnaryMinus, TypeInfo::TypeTag::Integer, TypeInfo::TypeTag::Void, GetI32Type(), [](Value v1, Value v2) {
        return MakeValue(-v1.i32Value);
    });

    // Not
    // Bool
    Register(Operator::Not, TypeInfo::TypeTag::Bool, TypeInfo::TypeTag::Void, GetBoolType(), [](Value v1, Value v2) {
        return MakeValue(!v1.boolValue);
    });
}
