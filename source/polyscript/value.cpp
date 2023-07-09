// Copyright 2020-2022 David Colson. All rights reserved.

#include "value.h"
#include "lexer.h"

#include <resizable_array.inl>

namespace {
Operator::Enum tokenToOperatorMap[TokenType::Count];
ResizableArray<TypeInfo*> typeTable;
}

void FreeFunction(Function* pFunc) {
    pFunc->constants.Free();
    pFunc->code.Free();
	pFunc->dbgLineInfo.Free();
	pFunc->dbgConstantsTypes.Free();
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
		} else if (pInfo->tag == TypeInfo::TypeTag::Struct && pNewType->tag == pInfo->tag) {
			TypeInfoStruct* pNewStruct = (TypeInfoStruct*)pNewType;
			TypeInfoStruct* pOldStruct = (TypeInfoStruct*)pInfo;

			if (pNewStruct->name == pOldStruct->name) {
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
		case TypeInfo::TypeTag::Struct: {
			pToAddType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfoStruct));
			memcpy(pToAddType, pNewType, sizeof(TypeInfoStruct));
			break;
		}
        default: {
			pToAddType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
            memcpy(pToAddType, pNewType, sizeof(TypeInfo));
            break;
        }
    }
    typeTable.PushBack(pToAddType);
    return typeTable[typeTable.count - 1];
}

TypeInfo* FindTypeByName(String identifier) {
	for (TypeInfo* pInfo : typeTable) {
		if (pInfo->name == identifier) {
			return pInfo;
		}
	}
	return nullptr;
}

void InitTypeTable() {
    TypeInfo* pVoidType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pVoidType->tag = TypeInfo::TypeTag::Void;
    pVoidType->size = 0;
    pVoidType->name = "void";
    typeTable.PushBack(pVoidType);
	MarkNotALeak(pVoidType);

    TypeInfo* pI32Type = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pI32Type->tag = TypeInfo::TypeTag::I32;
    pI32Type->size = 4;
    pI32Type->name = "i32";
    typeTable.PushBack(pI32Type);
	MarkNotALeak(pI32Type);

    TypeInfo* pF32Type = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pF32Type->tag = TypeInfo::TypeTag::F32;
    pF32Type->size = 4;
    pF32Type->name = "f32";
    typeTable.PushBack(pF32Type);
	MarkNotALeak(pF32Type);

    TypeInfo* pBoolType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pBoolType->tag = TypeInfo::TypeTag::Bool;
    pBoolType->size = 1;
    pBoolType->name = "bool";
    typeTable.PushBack(pBoolType);
	MarkNotALeak(pBoolType);

    TypeInfo* pTypeType = (TypeInfo*)g_Allocator.Allocate(sizeof(TypeInfo));
    pTypeType->tag = TypeInfo::TypeTag::Type;
    pTypeType->size = 8;
    pTypeType->name = "Type";
    typeTable.PushBack(pTypeType);
	MarkNotALeak(pTypeType);

    TypeInfoFunction* pEmptyFuncType = (TypeInfoFunction*)g_Allocator.Allocate(sizeof(TypeInfoFunction));
    pEmptyFuncType->tag = TypeInfo::TypeTag::Function;
    pEmptyFuncType->size = 8;
    pEmptyFuncType->name = "()";
    typeTable.PushBack(pEmptyFuncType);
	MarkNotALeak(pEmptyFuncType);

	MarkNotALeak(typeTable.pData);
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

void InitTokenToOperatorMap() {
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
}
