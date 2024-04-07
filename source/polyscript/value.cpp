// Copyright 2020-2022 David Colson. All rights reserved.

#include "value.h"
#include "lexer.h"

#include <resizable_array.inl>
#include <linear_allocator.h>

namespace {
LinearAllocator typeTableMemory;
ResizableArray<TypeInfo*> typeTable;
}

// ***********************************************************************

Operator::Enum TokenToOperator(TokenType::Enum tokenType) {
    switch(tokenType) {
		case TokenType::Plus: return Operator::Add;
		case TokenType::Minus: return Operator::Subtract;
		case TokenType::Star: return Operator::Multiply;
		case TokenType::Slash: return Operator::Divide;
		case TokenType::Less: return Operator::Less;
		case TokenType::Greater: return Operator::Greater;
		case TokenType::GreaterEqual: return Operator::GreaterEqual;
		case TokenType::LessEqual: return Operator::LessEqual;
		case TokenType::EqualEqual: return Operator::Equal;
		case TokenType::BangEqual: return Operator::NotEqual;
		case TokenType::And: return Operator::And;
		case TokenType::Or: return Operator::Or;
		case TokenType::Bang: return Operator::Not;
		case TokenType::Address: return Operator::AddressOf;
		case TokenType::Dot: return Operator::FieldSelector;
		case TokenType::LeftBracket: return Operator::ArraySubscript;
		case TokenType::Caret: return Operator::PointerDeref;
		case TokenType::Equal: return Operator::Assignment;
		default: return Operator::Invalid;
	}	
}

// ***********************************************************************

bool CheckTypesIdentical(TypeInfo* pType1, TypeInfo* pType2) {
	if (pType1 == nullptr || pType2 == nullptr)
		return false;

    if (pType1->tag != pType2->tag) {
        return false;
    }

    switch (pType1->tag) {
        case TypeInfo::TypeTag::Invalid:
        case TypeInfo::TypeTag::Void:
        case TypeInfo::TypeTag::I32:
        case TypeInfo::TypeTag::F32:
        case TypeInfo::TypeTag::Bool:
        case TypeInfo::TypeTag::Type:
            if (pType1->size == pType2->size)
                return true;
            else
                return false;
            break;
        case TypeInfo::TypeTag::Function: {
            TypeInfoFunction* pFunc1 = (TypeInfoFunction*)pType1;
            TypeInfoFunction* pFunc2 = (TypeInfoFunction*)pType2;

            if (pFunc1->params.count != pFunc2->params.count) {
                return false;
            }

            if (!CheckTypesIdentical(pFunc1->pReturnType, pFunc2->pReturnType)) {
                return false;
            }
        
            for (size i = 0; i < pFunc1->params.count; i++) {
                if (!CheckTypesIdentical(pFunc1->params[i], pFunc2->params[i])) {
                    return false;
                }
            }
            return true;
        } break;
        case TypeInfo::TypeTag::Struct: {
            // Structs are distinct if they have different names, so no need to check members, if the name differs, it's different
            if (pType1->name == pType2->name) {
                return true;
            }
            return false;
        } break;
		case TypeInfo::TypeTag::Pointer: {
			TypeInfoPointer* pPointer1 = (TypeInfoPointer*)pType1;
			TypeInfoPointer* pPointer2 = (TypeInfoPointer*)pType2;
			// Pointers are the same if they share the same base type

			if (CheckTypesIdentical(pPointer1->pBaseType, pPointer2->pBaseType)) {
				return true;
			}
			return false;
		} break;
		case TypeInfo::TypeTag::Array: {
			TypeInfoArray* pArray1 = (TypeInfoArray*)pType1;
			TypeInfoArray* pArray2 = (TypeInfoArray*)pType2;
			// arrays are identical if the base types are identical and the dimension is the same

			bool baseTypesIdentical = CheckTypesIdentical(pArray1->pBaseType, pArray2->pBaseType);
			bool dimensionIdentical = pArray1->dimension == pArray2->dimension;
			if (baseTypesIdentical && dimensionIdentical)
				return true;
			return false;
		} break;
        default:
            return false;
            break;
    }
}

// ***********************************************************************

TypeInfo* CopyTypeDeep(TypeInfo* pTypeInfo, IAllocator* pAlloc = &g_Allocator) {
    switch (pTypeInfo->tag) {
        case TypeInfo::TypeTag::Void:
        case TypeInfo::TypeTag::I32:
        case TypeInfo::TypeTag::F32:
        case TypeInfo::TypeTag::Bool:
        case TypeInfo::TypeTag::Type: {
            TypeInfo* pNewType = (TypeInfo*)pAlloc->Allocate(sizeof(TypeInfo));
            pNewType->tag = pTypeInfo->tag;
            pNewType->size = pTypeInfo->size;
            pNewType->name = CopyString(pTypeInfo->name, pAlloc);
            return pNewType;
        } break;
        case TypeInfo::TypeTag::Function: {
            TypeInfoFunction* pFuncType = (TypeInfoFunction*)pTypeInfo;
            TypeInfoFunction* pNewFuncType = (TypeInfoFunction*)pAlloc->Allocate(sizeof(TypeInfoFunction));

            pNewFuncType->tag = pTypeInfo->tag;
            pNewFuncType->size = pTypeInfo->size;
            pNewFuncType->name = CopyString(pTypeInfo->name, pAlloc);

			if (pFuncType->pReturnType)
				pNewFuncType->pReturnType = CopyTypeDeep(pFuncType->pReturnType, pAlloc);

            pNewFuncType->params = ResizableArray<TypeInfo*>(pAlloc);
            for (size i = 0; i < pFuncType->params.count; i++) {
                pNewFuncType->params.PushBack(CopyTypeDeep(pFuncType->params[i], pAlloc));
            }
            return pNewFuncType;
        } break;
        case TypeInfo::TypeTag::Struct: {
            TypeInfoStruct* pStructType = (TypeInfoStruct*)pTypeInfo;
            TypeInfoStruct* pNewStructType = (TypeInfoStruct*)pAlloc->Allocate(sizeof(TypeInfoStruct));
            
            pNewStructType->tag = pTypeInfo->tag;
            pNewStructType->size = pTypeInfo->size;
            pNewStructType->name = CopyString(pTypeInfo->name, pAlloc);

            pNewStructType->members = ResizableArray<TypeInfoStruct::Member>(pAlloc);
            for (size i = 0; i < pStructType->members.count; i++) {
                TypeInfoStruct::Member newMember;
                newMember.identifier = CopyString(pStructType->members[i].identifier, pAlloc);
                newMember.pType = CopyTypeDeep(pStructType->members[i].pType, pAlloc);
                newMember.offset = pStructType->members[i].offset;
                pNewStructType->members.PushBack(newMember);
            }
            return pNewStructType;
        } break;
		case TypeInfo::TypeTag::Pointer: {
			TypeInfoPointer* pPointerType = (TypeInfoPointer*)pTypeInfo;
            TypeInfoPointer* pNewPointerType = (TypeInfoPointer*)pAlloc->Allocate(sizeof(TypeInfoPointer));

            pNewPointerType->tag = pPointerType->tag;
            pNewPointerType->size = pPointerType->size;
            pNewPointerType->name = CopyString(pPointerType->name, pAlloc);
			pNewPointerType->pBaseType = CopyTypeDeep(pPointerType->pBaseType, pAlloc);
			return pNewPointerType;
		} break;
		case TypeInfo::TypeTag::Array: {
			TypeInfoArray* pArrayType = (TypeInfoArray*)pTypeInfo;
            TypeInfoArray* pNewArrayType = (TypeInfoArray*)pAlloc->Allocate(sizeof(TypeInfoArray));

            pNewArrayType->tag = pArrayType->tag;
            pNewArrayType->size = pArrayType->size;
            pNewArrayType->name = CopyString(pArrayType->name, pAlloc);
			pNewArrayType->pBaseType = CopyTypeDeep(pArrayType->pBaseType, pAlloc);
			pNewArrayType->dimension = pArrayType->dimension;
			return pNewArrayType;
		} break;
        default: {
            return nullptr;
            break;
        }
    }
}

// ***********************************************************************

Value MakeValue(TypeInfo* pType) {
	for (size i = 0; i < typeTable.count; i++) {
		TypeInfo* pInfo = typeTable[i];
        if (CheckTypesIdentical(pInfo, pType)) {
			Value v;
			v.typeId = (i32)i;
			return v;
        }    
    }

    TypeInfo* copy = CopyTypeDeep(pType, &typeTableMemory);
    typeTable.PushBack(copy);

    Value v;
	v.typeId = (i32)typeTable.count - 1;
    return v;
}

// ***********************************************************************

TypeInfo* FindTypeByValue(Value& v) {
	Assert(v.typeId < typeTable.count);
	return typeTable[v.typeId];
}

// ***********************************************************************

TypeInfo* FindTypeByName(String identifier) {
    for (TypeInfo* pInfo : typeTable) {
        if (pInfo->name == identifier) {
            return pInfo;
        }
    }
    return nullptr;
}

// ***********************************************************************

void InitTypeTable() {
    typeTable = ResizableArray<TypeInfo*>(&typeTableMemory);

    TypeInfo* pInvalidType = (TypeInfo*)typeTableMemory.Allocate(sizeof(TypeInfo));
    pInvalidType->tag = TypeInfo::TypeTag::Invalid;
    pInvalidType->size = 0;
    pInvalidType->name = "invalid";
    typeTable.PushBack(pInvalidType);

    TypeInfo* pVoidType = (TypeInfo*)typeTableMemory.Allocate(sizeof(TypeInfo));
    pVoidType->tag = TypeInfo::TypeTag::Void;
    pVoidType->size = 0;
    pVoidType->name = "void";
    typeTable.PushBack(pVoidType);

    MarkNotALeak(typeTableMemory.pMemoryBase);

    TypeInfo* pI32Type = (TypeInfo*)typeTableMemory.Allocate(sizeof(TypeInfo));
    pI32Type->tag = TypeInfo::TypeTag::I32;
    pI32Type->size = 4;
    pI32Type->name = "i32";
    typeTable.PushBack(pI32Type);

    TypeInfo* pF32Type = (TypeInfo*)typeTableMemory.Allocate(sizeof(TypeInfo));
    pF32Type->tag = TypeInfo::TypeTag::F32;
    pF32Type->size = 4;
    pF32Type->name = "f32";
    typeTable.PushBack(pF32Type);

    TypeInfo* pBoolType = (TypeInfo*)typeTableMemory.Allocate(sizeof(TypeInfo));
    pBoolType->tag = TypeInfo::TypeTag::Bool;
    pBoolType->size = 1;
    pBoolType->name = "bool";
    typeTable.PushBack(pBoolType);

    TypeInfo* pTypeType = (TypeInfo*)typeTableMemory.Allocate(sizeof(TypeInfo));
    pTypeType->tag = TypeInfo::TypeTag::Type;
    pTypeType->size = 4;
    pTypeType->name = "type";
    typeTable.PushBack(pTypeType);

}

// ***********************************************************************

TypeInfo* GetInvalidType() {
    return typeTable[0];
}

// ***********************************************************************

TypeInfo* GetVoidType() {
    return typeTable[1];
}

// ***********************************************************************

TypeInfo* GetI32Type() {
    return typeTable[2];
}

// ***********************************************************************

TypeInfo* GetF32Type() {
    return typeTable[3];
}

// ***********************************************************************

TypeInfo* GetBoolType() {
    return typeTable[4];
}

// ***********************************************************************

TypeInfo* GetTypeType() {
    return typeTable[5];
}

// ***********************************************************************

TypeInfo* GetEmptyFuncType() {
    return typeTable[6];
}
