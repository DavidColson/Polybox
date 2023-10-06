// Copyright 2020-2022 David Colson. All rights reserved.

#include "type_checker.h"

#include "parser.h"
#include "compiler.h"

#include <hashmap.inl>
#include <resizable_array.inl>
#include <string_builder.h>
#include <memory.h>

// ***********************************************************************

bool CheckIsDataScope(ScopeKind::Enum scopeKind) {
    return scopeKind == ScopeKind::Struct || scopeKind == ScopeKind::Function || scopeKind == ScopeKind::FunctionType;
}

// ***********************************************************************

Scope* CreateScope(ScopeKind::Enum kind, Scope* pParent, IAllocator* pAllocator) {
    Scope* pScope = (Scope*)pAllocator->Allocate(sizeof(Scope));
    pScope->entities.pAlloc = pAllocator;
    pScope->children.pAlloc = pAllocator;
    pScope->kind = kind;

    if (pParent)
        pParent->children.PushBack(pScope);
    pScope->pParent = pParent;

    return pScope;
}

// ***********************************************************************

Entity* FindEntity(Scope* pLowestSearchScope, String name) {
    Entity* pEntity = nullptr;
    Scope* pSearchScope = pLowestSearchScope;
    while(pEntity == nullptr && pSearchScope != nullptr) {
        Entity** pEntry = pSearchScope->entities.Get(name);
        if (pEntry == nullptr) {
            pSearchScope = pSearchScope->pParent;
        } else {
            pEntity = *pEntry;
        }
    }
    return pEntity;
}

// ***********************************************************************

struct TypeCheckerState {
    Scope* pGlobalScope{ nullptr };
    Scope* pCurrentScope{ nullptr };

    ErrorState* pErrors { nullptr };
    int currentScopeLevel { 0 };
    Ast::Declaration* pCurrentDeclaration{ nullptr }; // nullptr if we are not currently parsing a declaration
	IAllocator* pAllocator{ nullptr };
};

void TypeCheckStatement(TypeCheckerState& state, Ast::Statement* pStmt);
void TypeCheckStatements(TypeCheckerState& state, ResizableArray<Ast::Statement*>& program);
[[nodiscard]] Ast::Expression* TypeCheckExpression(TypeCheckerState& state, Ast::Expression* pExpr);

// ***********************************************************************

bool IsImplicitlyCastable(TypeInfo* pFrom, TypeInfo* pTo) {
	// TODO: When adding new core types, ensure no loss of signedness and no truncation or loss of precision
	if (CheckTypesIdentical(pFrom, GetI32Type()) && CheckTypesIdentical(pTo, GetF32Type())) {
		return true;
	}
	return false;
}

// ***********************************************************************

Value ComputeBinaryConstant(TypeInfo* pType, Operator::Enum op, Value left, Value right) {
        switch (pType->tag) {
                case TypeInfo::TypeTag::I32:
                    switch (op) {
                        case Operator::Add: return MakeValue(left.i32Value + right.i32Value);
                        case Operator::Subtract: return MakeValue(left.i32Value - right.i32Value);
                        case Operator::Multiply: return MakeValue(left.i32Value * right.i32Value);
                        case Operator::Divide: return MakeValue(left.i32Value / right.i32Value);
                        case Operator::Less: return MakeValue(left.i32Value < right.i32Value); 
                        case Operator::LessEqual: return MakeValue(left.i32Value <= right.i32Value);
                        case Operator::Greater: return MakeValue(left.i32Value > right.i32Value);
                        case Operator::GreaterEqual: return MakeValue(left.i32Value >= right.i32Value);
                        case Operator::Equal: return MakeValue(left.i32Value == right.i32Value);
                        case Operator::NotEqual: return MakeValue(left.i32Value != right.i32Value);
                        default: return MakeValueNill();
                    }
                    break;
                case TypeInfo::TypeTag::F32:
                    switch (op) {
                        case Operator::Add: return MakeValue(left.f32Value + right.f32Value);
                        case Operator::Subtract: return MakeValue(left.f32Value - right.f32Value);
                        case Operator::Multiply: return MakeValue(left.f32Value * right.f32Value);
                        case Operator::Divide: return MakeValue(left.f32Value / right.f32Value);
                        case Operator::Less: return MakeValue(left.f32Value < right.f32Value);
                        case Operator::LessEqual: return MakeValue(left.f32Value <= right.f32Value);
                        case Operator::Greater: return MakeValue(left.f32Value > right.f32Value);
                        case Operator::GreaterEqual: return MakeValue(left.f32Value >= right.f32Value);
                        case Operator::Equal: return MakeValue(left.f32Value == right.f32Value);
                        case Operator::NotEqual: return MakeValue(left.f32Value != right.f32Value);
                        default: return MakeValueNill();
                    }
                    break;
                case TypeInfo::TypeTag::Bool:
                    switch (op) {
                        case Operator::And: return MakeValue(left.boolValue && right.boolValue);
                        case Operator::Or: return MakeValue(left.boolValue || right.boolValue);
                        default: return MakeValueNill();
                    }
                    break;
                default:
                    return MakeValueNill();
                    break;
        }
}

// ***********************************************************************

Value ComputeUnaryConstant(TypeInfo* pType, Operator::Enum op, Value right) {
        switch (pType->tag) {
                case TypeInfo::TypeTag::I32:
                    switch (op) {
                        case Operator::UnaryMinus: return MakeValue(-right.i32Value);
                        default: return MakeValueNill();
                    }
                    break;
                case TypeInfo::TypeTag::F32:
                    switch (op) {
                        case Operator::UnaryMinus: return MakeValue(-right.f32Value);
                        default: return MakeValueNill();
                    }
                    break;
                case TypeInfo::TypeTag::Bool:
                    switch (op) {
                        case Operator::Not: return MakeValue(!right.boolValue);
                        default: return MakeValueNill();
                    }
                    break;
                default:
                    return MakeValueNill();
                    break;
        }
}

// ***********************************************************************

Value ComputeCastConstant(Value value, TypeInfo* pFrom, TypeInfo* pTo) {
        switch (pFrom->tag) {
                case TypeInfo::TypeTag::I32:
                    switch (pTo->tag) {
                        case TypeInfo::TypeTag::F32: return MakeValue((float)value.i32Value);
                        case TypeInfo::TypeTag::Bool: return MakeValue((bool)value.i32Value);
                        default: return MakeValueNill();
                    }
                    break;
                case TypeInfo::TypeTag::F32:
                    switch (pTo->tag) {
                        case TypeInfo::TypeTag::I32: return MakeValue((int32_t)value.f32Value);
                        case TypeInfo::TypeTag::Bool: return MakeValue((bool)value.f32Value);
                        default: return MakeValueNill();
                    }
                    break;
                case TypeInfo::TypeTag::Bool:
                    switch (pTo->tag) {
                        case TypeInfo::TypeTag::I32: return MakeValue((int32_t)value.boolValue);
                        case TypeInfo::TypeTag::F32: return MakeValue((float)value.boolValue);
                        default: return MakeValueNill();
                    }
                    break;
                default:
                    return MakeValueNill();
                    break;
        }
}

// ***********************************************************************

void TypeCheckFunctionType(TypeCheckerState& state, Ast::FunctionType* pFuncType) {
    pFuncType->pType = GetTypeType();
    pFuncType->isConstant = true;

    StringBuilder builder;
    builder.Append("func (");

    TypeInfoFunction* pFunctionTypeInfo = (TypeInfoFunction*)state.pAllocator->Allocate(sizeof(TypeInfoFunction));
    pFunctionTypeInfo->tag = TypeInfo::TypeTag::Function;
    pFunctionTypeInfo->size = 8;
    pFunctionTypeInfo->params = ResizableArray<TypeInfo*>(state.pAllocator);

    for (size_t i = 0; i < pFuncType->params.count; i++) {
        TypeInfo* pParamType = nullptr;
        if (pFuncType->params[i]->nodeKind == Ast::NodeKind::Identifier) {
            Ast::Identifier* pParam = (Ast::Identifier*)pFuncType->params[i];
            pParam = (Ast::Identifier*)TypeCheckExpression(state, pParam);
            pParamType = pParam->constantValue.pTypeInfo;

        } else if (pFuncType->params[i]->nodeKind == Ast::NodeKind::Declaration) {
            Ast::Declaration* pParam = (Ast::Declaration*)pFuncType->params[i];
            TypeCheckStatement(state, pParam);
            pParamType = pParam->pType;
        } else {
            state.pErrors->PushError(pFuncType, "Invalid parameter, expected a typename or a parameter declaration");
        }

        pFunctionTypeInfo->params.PushBack(pParamType);
        if (i < pFuncType->params.count - 1) {
            builder.AppendFormat("%s, ", pParamType->name.pData);
        } else {
            builder.AppendFormat("%s", pParamType->name.pData);
        }
    }

    builder.Append(")");
    if (pFuncType->pReturnType) {
        pFuncType->pReturnType = (Ast::Type*)TypeCheckExpression(state, pFuncType->pReturnType);
        pFunctionTypeInfo->pReturnType = pFuncType->pReturnType->constantValue.pTypeInfo;
        builder.AppendFormat(" -> %s", pFunctionTypeInfo->pReturnType->name.pData);
    }
    pFunctionTypeInfo->name = builder.CreateString(true, state.pAllocator);

    // Types are constant literals
    pFuncType->constantValue = MakeValue(pFunctionTypeInfo);
}

// ***********************************************************************

[[nodiscard]] Ast::Expression* TypeCheckExpression(TypeCheckerState& state, Ast::Expression* pExpr) {
    if (pExpr == nullptr)
        return pExpr;

    switch (pExpr->nodeKind) {
        case Ast::NodeKind::Literal: {
            // Nothing to do
            return pExpr;
        }
        case Ast::NodeKind::Type: {
            pExpr->pType = GetTypeType();
            return pExpr;
        }
        case Ast::NodeKind::FunctionType: {
            Ast::FunctionType* pFuncType = (Ast::FunctionType*)pExpr;

            state.pCurrentScope = pFuncType->pScope;
            TypeCheckFunctionType(state, pFuncType);
            state.pCurrentScope = pFuncType->pScope->pParent;
            return pFuncType;
        }
		case Ast::NodeKind::Structure: {
			Ast::Structure* pStruct = (Ast::Structure*)pExpr;
            pStruct->isConstant = true;

            state.pCurrentScope = pStruct->pScope;
            // Typecheck statements, the declaration typechecking must ensure the initializers are constant, if the scope is a struct type
            TypeCheckStatements(state, pStruct->members);
            state.pCurrentScope = pStruct->pScope->pParent;

			// Create the actual type that this struct is
            TypeInfoStruct* pStructTypeInfo = (TypeInfoStruct*)state.pAllocator->Allocate(sizeof(TypeInfoStruct));
			pStructTypeInfo->members.pAlloc = state.pAllocator;
			pStructTypeInfo->tag = TypeInfo::TypeTag::Struct;

			for (Ast::Statement* pMemberStmt : pStruct->members) {
                if (pMemberStmt->nodeKind != Ast::NodeKind::Declaration)
                    continue;

				Ast::Declaration* pMember = (Ast::Declaration*)pMemberStmt;
				TypeInfoStruct::Member member;

				member.identifier = pMember->identifier; // Note, not a deep string copy, sharing it here
				member.pType = pMember->pType;
				member.offset = pStructTypeInfo->size;
				pStructTypeInfo->members.PushBack(member);

				if (member.pType)
					pStructTypeInfo->size += member.pType->size;
			}
            // TODO: Naming the struct is interesting right, cause the way odin does it is that the struct itself and the named struct are two different things
            // But I don't want to implement named types like that today, so for now we'll store the declaration here and set the name. You wanna come back to this
            // soon though, so anonymous struct types are a thing
			pStructTypeInfo->name = pStruct->pDeclaration->identifier;

			pStruct->pType = GetTypeType();
            pStruct->constantValue = MakeValue(pStructTypeInfo);
			return pStruct;
		}
        case Ast::NodeKind::Identifier: {
            Ast::Identifier* pIdentifier = (Ast::Identifier*)pExpr;

            // Find the entity corresponding to this identifier
            Entity* pEntity = FindEntity(state.pCurrentScope, pIdentifier->identifier);

            if (pEntity == nullptr) {
                state.pErrors->PushError(pIdentifier, "Undeclared identifier \'%s\', not found in any available scope", pIdentifier->identifier.pData);
                pIdentifier->pType = GetVoidType();
                return pIdentifier;
            }

            if (pEntity->status == EntityStatus::InProgress) {
                state.pErrors->PushError(pIdentifier, "Circular dependency detected on identifier \'%s\'", pIdentifier->identifier.pData);
                pIdentifier->pType = GetVoidType();
                return pIdentifier;
            }

            if (pEntity->kind == EntityKind::Constant) {
                if (pEntity->status == EntityStatus::Unresolved) {
                    TypeCheckStatement(state, pEntity->pDeclaration);
                }
                pIdentifier->isConstant = true;
                pIdentifier->constantValue = pEntity->constantValue;
            } else if (pEntity->kind == EntityKind::Variable) {
                pIdentifier->isConstant = false;
                if (!pEntity->isLive) {
                    state.pErrors->PushError(pIdentifier, "Can't use variable \'%s\', it's not defined yet", pIdentifier->identifier.pData);
                }
            }

            pIdentifier->pType = pEntity->pType;
            return pIdentifier;
        }
        case Ast::NodeKind::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            pFunction->isConstant = true;

            state.pCurrentScope = pFunction->pScope;
            TypeCheckFunctionType(state, pFunction->pFuncType);

            // For recursion to work, we have to resolve the function's type before typechecking it's body
            // If it has a declaration (it may not), we'll go get the entity, and resolve it so we can move on.
            if (pFunction->pDeclaration) {
                // Find the entity corresponding to this function
                Entity* pEntity = FindEntity(state.pCurrentScope, pFunction->pDeclaration->identifier);

                pEntity->pType = pFunction->pFuncType->constantValue.pTypeInfo;
                pEntity->status = EntityStatus::Resolved;
            }

            pFunction->pType = pFunction->pFuncType->constantValue.pTypeInfo;
            TypeCheckStatement(state, pFunction->pBody);

            // Pop function scope (where the params live)
            state.pCurrentScope = pFunction->pScope->pParent;
            return pFunction;
        }
        case Ast::NodeKind::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            pVarAssignment->isConstant = false;
            pVarAssignment->pIdentifier = (Ast::Identifier*)TypeCheckExpression(state, pVarAssignment->pIdentifier);
            pVarAssignment->pAssignment = TypeCheckExpression(state, pVarAssignment->pAssignment);

            // Typechecking failed on the identifier, don't report any more errors
            if (CheckTypesIdentical(pVarAssignment->pIdentifier->pType, GetVoidType())) {
                pVarAssignment->pType = GetVoidType();
                return pVarAssignment;
            }

            if (pVarAssignment->pIdentifier->isConstant) {
                state.pErrors->PushError(pVarAssignment, "Can't assign to constant \'%s\'", pVarAssignment->pIdentifier->identifier.pData);
            }

            TypeInfo* pIdentifierType = pVarAssignment->pIdentifier->pType;
            TypeInfo* pAssignedVarType = pVarAssignment->pAssignment->pType;
            if (CheckTypesIdentical(pIdentifierType, pAssignedVarType)) {
                pVarAssignment->pType = pIdentifierType;
            } else {
                state.pErrors->PushError(pVarAssignment, "Type mismatch on assignment, \'%s\' has type '%s', but is being assigned a value with type '%s'", pVarAssignment->pIdentifier->identifier.pData, pIdentifierType->name.pData, pAssignedVarType->name.pData);
            }
            return pVarAssignment;
        }
        case Ast::NodeKind::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            pGroup->pExpression = TypeCheckExpression(state, pGroup->pExpression);
            pGroup->pType = pGroup->pExpression->pType;

            if (pGroup->pExpression->isConstant) {
                pGroup->isConstant = true;
                pGroup->constantValue = pGroup->pExpression->constantValue;
            }
            return pGroup;
        }
        case Ast::NodeKind::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            pBinary->pLeft = TypeCheckExpression(state, pBinary->pLeft);
            pBinary->pRight = TypeCheckExpression(state, pBinary->pRight);

			String str1 = pBinary->pLeft->pType->name;
			String str2 = pBinary->pRight->pType->name;
			String str3 = Operator::ToString(pBinary->op);
			bool skipLeftTypeCheck = false;

			// If mismatch, check if we can do an implicit cast, otherwise fail
			if (!CheckTypesIdentical(pBinary->pLeft->pType, pBinary->pRight->pType)) {
				if (IsImplicitlyCastable(pBinary->pLeft->pType, pBinary->pRight->pType)) {
					Ast::Cast* pCastExpr = (Ast::Cast*)state.pAllocator->Allocate(sizeof(Ast::Cast));
					pCastExpr->nodeKind = Ast::NodeKind::Cast;
					pCastExpr->pExprToCast = pBinary->pLeft;

					Ast::Type* pType = (Ast::Type*)state.pAllocator->Allocate(sizeof(Ast::Type));
					pType->nodeKind = Ast::NodeKind::Type;
                    pType->isConstant = true;
                    pType->constantValue = MakeValue(pBinary->pRight->pType);
					pType->pLocation = pBinary->pRight->pLocation;
					pType->pLineStart = pBinary->pRight->pLineStart;
					pType->line = pBinary->pRight->line;
					pCastExpr->pTypeExpr = TypeCheckExpression(state, pType);

					pCastExpr->pLocation = pBinary->pLeft->pLocation;
					pCastExpr->pLineStart = pBinary->pLeft->pLineStart;
					pCastExpr->line = pBinary->pLeft->line;

					pBinary->pLeft = TypeCheckExpression(state, pCastExpr);

				} else if (IsImplicitlyCastable(pBinary->pRight->pType, pBinary->pLeft->pType)) {
					Ast::Cast* pCastExpr = (Ast::Cast*)state.pAllocator->Allocate(sizeof(Ast::Cast));
					pCastExpr->nodeKind = Ast::NodeKind::Cast;
					pCastExpr->pExprToCast = pBinary->pRight;

					Ast::Type* pType = (Ast::Type*)state.pAllocator->Allocate(sizeof(Ast::Type));
					pType->nodeKind = Ast::NodeKind::Type;
					pType->isConstant = true;
                    pType->constantValue = MakeValue(pBinary->pLeft->pType);
					pType->pLocation = pBinary->pLeft->pLocation;
					pType->pLineStart = pBinary->pLeft->pLineStart;
					pType->line = pBinary->pLeft->line;
					pCastExpr->pTypeExpr = TypeCheckExpression(state, pType);

					pCastExpr->pLocation = pBinary->pRight->pLocation;
					pCastExpr->pLineStart = pBinary->pRight->pLineStart;
					pCastExpr->line = pBinary->pRight->line;

					pBinary->pRight = TypeCheckExpression(state, pCastExpr);
				} else {
					state.pErrors->PushError(pBinary, "Invalid types (%s, %s) used with op \"%s\"", str1.pData, str2.pData, str3.pData);
					skipLeftTypeCheck = true;
				}
			}

			if (   pBinary->op == Operator::And
				|| pBinary->op == Operator::Or) {
				if (!CheckTypesIdentical(pBinary->pLeft->pType, GetBoolType()) && !CheckTypesIdentical(pBinary->pRight->pType, GetBoolType())) {
					state.pErrors->PushError(pBinary, "Invalid types (%s, %s) used with op \"%s\"", str1.pData, str2.pData, str3.pData);
				}
			}

			if (!skipLeftTypeCheck) {
				if (pBinary->op == Operator::Less
					|| pBinary->op == Operator::LessEqual
					|| pBinary->op == Operator::Greater
					|| pBinary->op == Operator::GreaterEqual) {
				    if (!CheckTypesIdentical(pBinary->pLeft->pType, GetF32Type()) && !CheckTypesIdentical(pBinary->pRight->pType, GetI32Type())) {
						state.pErrors->PushError(pBinary, "Invalid types (%s, %s) used with op \"%s\"", str1.pData, str2.pData, str3.pData);
					}
				}
			}

			if (   pBinary->op == Operator::Multiply
				|| pBinary->op == Operator::Divide
				|| pBinary->op == Operator::Add
				|| pBinary->op == Operator::Subtract) {
				pBinary->pType = pBinary->pLeft->pType;
			} else {
				pBinary->pType = GetBoolType();
			}

            // Do this at the end so we include the evaluation of any potential implicit casts that were added
             if (pBinary->pLeft->isConstant && pBinary->pRight->isConstant) {
                pBinary->isConstant = true;
                pBinary->constantValue = ComputeBinaryConstant(pBinary->pLeft->pType, pBinary->op, pBinary->pLeft->constantValue, pBinary->pRight->constantValue);
            }

            return pBinary;
        }
        case Ast::NodeKind::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            pUnary->pRight = TypeCheckExpression(state, pUnary->pRight);

			if (pUnary->op == Operator::Not) {
				pUnary->pType = GetBoolType();
                if (!CheckTypesIdentical(pUnary->pRight->pType, GetBoolType())) {
					state.pErrors->PushError(pUnary, "Invalid type (%s) used with op \"%s\"", pUnary->pRight->pType->name.pData, Operator::ToString(pUnary->op));
				}
			} else if (pUnary->op == Operator::UnaryMinus) {
				pUnary->pType = pUnary->pRight->pType;
				if (CheckTypesIdentical(pUnary->pRight->pType, GetBoolType())) {
					state.pErrors->PushError(pUnary, "Invalid type (%s) used with op \"%s\"", pUnary->pRight->pType->name.pData, Operator::ToString(pUnary->op));
				}
			}

            if (pUnary->isConstant) {
                pUnary->isConstant = true;
                pUnary->constantValue = ComputeUnaryConstant(pUnary->pType, pUnary->op, pUnary->pRight->constantValue);
            }

            return pUnary;
        }
		case Ast::NodeKind::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;
			pCast->pTypeExpr = TypeCheckExpression(state, pCast->pTypeExpr);
			pCast->pExprToCast = TypeCheckExpression(state, pCast->pExprToCast);
            
			TypeInfo* pFrom = pCast->pExprToCast->pType;
			TypeInfo* pTo = pCast->pTypeExpr->constantValue.pTypeInfo;

            if (pTo == nullptr || pFrom == nullptr) {
			    pCast->pType = GetVoidType();
                return pCast;
            }

			// TODO: Replace this with a function which actually checks their compatibility, in terms of size, etc etc, see umka code
			bool castAllowed = false;
			if ((CheckTypesIdentical(pFrom, GetF32Type()) && CheckTypesIdentical(pTo, GetI32Type()))
				| (CheckTypesIdentical(pFrom, GetF32Type()) && CheckTypesIdentical(pTo, GetBoolType()))
				| (CheckTypesIdentical(pFrom, GetI32Type()) && CheckTypesIdentical(pTo, GetF32Type()))
				| (CheckTypesIdentical(pFrom, GetI32Type()) && CheckTypesIdentical(pTo, GetBoolType()))
				| (CheckTypesIdentical(pFrom, GetBoolType()) && CheckTypesIdentical(pTo, GetI32Type()))
				| (CheckTypesIdentical(pFrom, GetBoolType()) && CheckTypesIdentical(pTo, GetF32Type()))) {
				castAllowed = true;
			}

			if (CheckTypesIdentical(pFrom, pTo)) {
				state.pErrors->PushError(pCast, "Cast from \"%s\" to \"%s\" is pointless", pFrom->name.pData, pTo->name.pData);
			} else if (castAllowed == false) {
				state.pErrors->PushError(pCast, "Not possible to cast from type \"%s\" to \"%s\"", pFrom->name.pData, pTo->name.pData);
			}
			
            if (pCast->pExprToCast->isConstant) {
                pCast->isConstant = true;
                pCast->constantValue = ComputeCastConstant(pCast->pExprToCast->constantValue, pFrom, pTo);
            }

			pCast->pType = pCast->pTypeExpr->constantValue.pTypeInfo;
			return pCast;
		}
        case Ast::NodeKind::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;
            pCall->isConstant = false;
            pCall->pCallee = TypeCheckExpression(state, pCall->pCallee);
            // 1. How do we cope with typechecking failing to produce a type? We need to be able to continue, and not crash
            // 2. Technically, function typechecking should produce a type _before_ the body gets typechecked.

			if (pCall->pCallee->nodeKind == Ast::NodeKind::GetField) {
				state.pErrors->PushError(pCall, "Calling fields not currently supported");
				return pCall;
			}

            if (pCall->pCallee->pType->tag != TypeInfo::TypeTag::Function) {
                state.pErrors->PushError(pCall, "Attempt to call a value which is not a function");
            }

            for (int i = 0; i < (int)pCall->args.count; i++) {
                pCall->args[i] = TypeCheckExpression(state, pCall->args[i]);
            }
            
            TypeInfoFunction* pFunctionType = (TypeInfoFunction*)pCall->pCallee->pType;
            int argsCount = pCall->args.count;
            int paramsCount = pFunctionType->tag == TypeInfo::TypeTag::Void ? 0 : pFunctionType->params.count;
            if (pCall->args.count != pFunctionType->params.count) {
                Ast::Identifier* pVar = (Ast::Identifier*)pCall->pCallee;
                state.pErrors->PushError(pCall, "Mismatched number of arguments in call to function '%s', expected %i, got %i", pVar->identifier.pData, paramsCount, argsCount);
            }

            int minArgs = argsCount > paramsCount ? paramsCount : argsCount;
            for (size_t i = 0; i < minArgs; i++) {
                Ast::Expression* arg = pCall->args[i];
                TypeInfo* pArgType = pFunctionType->params[i];
                if (!CheckTypesIdentical(arg->pType, pArgType)) {
                    state.pErrors->PushError(arg, "Type mismatch in function argument '<argname TODO>', expected %s, got %s", pArgType->name.pData, arg->pType->name.pData);
                }
            }
            pCall->pType = pFunctionType->pReturnType;
            return pCall;
        }
		case Ast::NodeKind::GetField: {
			Ast::GetField* pGetField = (Ast::GetField*)pExpr;
			pGetField->pTarget = TypeCheckExpression(state, pGetField->pTarget);
            
            // TODO: This could be constant actually. If the field was declared as a constant, then this can be constant, for later.
            pGetField->isConstant = false;

			if (pGetField->pTarget->pType == nullptr) {
				return pGetField;
			}
			TypeInfo* pTargetTypeInfo = pGetField->pTarget->pType;
			if (pTargetTypeInfo->tag != TypeInfo::Struct) {
				state.pErrors->PushError(pGetField, "Attempting to access a field on type '%s' which is not a struct", pTargetTypeInfo->name.pData);
				return pGetField;
			}

			TypeInfoStruct* pTargetType = (TypeInfoStruct*)pTargetTypeInfo;

			for (size_t i = 0; i < pTargetType->members.count; i++) {
				TypeInfoStruct::Member& mem = pTargetType->members[i];
				if (mem.identifier == pGetField->fieldName) {
					pGetField->pType = mem.pType;
					return pGetField;
				}
			}

			state.pErrors->PushError(pGetField, "Specified field does not exist in struct '%s'", pTargetType->name.pData);
			
			return pGetField;
			break;
		}
		case Ast::NodeKind::SetField: {
			Ast::SetField* pSetField = (Ast::SetField*)pExpr;
			pSetField->pTarget = TypeCheckExpression(state, pSetField->pTarget);
			pSetField->pAssignment = TypeCheckExpression(state, pSetField->pAssignment);
            pSetField->isConstant = false;

			if (pSetField->pTarget->pType == nullptr) {
				return pSetField;
			}
			TypeInfo* pTargetTypeInfo = pSetField->pTarget->pType;
			if (pTargetTypeInfo->tag != TypeInfo::Struct) {
				state.pErrors->PushError(pSetField, "Attempting to set a field on type '%s' which is not a struct", pTargetTypeInfo->name.pData);
				return pSetField;
			}

			TypeInfoStruct* pTargetType = (TypeInfoStruct*)pTargetTypeInfo;
			TypeInfoStruct::Member* pTargetField = nullptr;
			for (size_t i = 0; i < pTargetType->members.count; i++) {
				TypeInfoStruct::Member& mem = pTargetType->members[i];
				if (mem.identifier == pSetField->fieldName) {
					pTargetField = &pTargetType->members[i];
					break;
				}
			}

			if (pTargetField == nullptr) {
				state.pErrors->PushError(pSetField, "Attempting to set a struct field which does not exist in struct '%s'", pTargetType->name.pData);
				return pSetField;
			}

            if (!CheckTypesIdentical(pTargetField->pType, pSetField->pAssignment->pType)) {
				state.pErrors->PushError(pSetField, "Type mismatch on assignment, field \'%s\' has type '%s', but is being assigned a value with type '%s'", pTargetField->identifier.pData, pTargetField->pType->name.pData, pSetField->pAssignment->pType->name.pData);
			}

			pSetField->pType = pTargetField->pType;
			return pSetField;
		}
        default:
            return pExpr;
    }
}

// ***********************************************************************

void TypeCheckStatement(TypeCheckerState& state, Ast::Statement* pStmt) {
    switch (pStmt->nodeKind) {
        case Ast::NodeKind::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;
            Entity* pEntity = FindEntity(state.pCurrentScope, pDecl->identifier);
        
            if (pEntity->status == EntityStatus::Resolved)
                break;

            pEntity->status = EntityStatus::InProgress;

            // Has initializer
            if (pDecl->pInitializerExpr) {
                pDecl->pInitializerExpr = TypeCheckExpression(state, pDecl->pInitializerExpr);
                if (CheckTypesIdentical(pDecl->pInitializerExpr->pType, GetVoidType())) {
                    // Typechecking failed
                    pDecl->pType = GetVoidType();
                    break;
                }

                if (!pDecl->pInitializerExpr->isConstant && CheckIsDataScope(state.pCurrentScope->kind)) {
                    state.pErrors->PushError(pDecl, "Cannot execute non-constant initializers in data scope");
                }

                if (pDecl->isConstantDeclaration) {
                    if (!pDecl->pInitializerExpr->isConstant) 
                        state.pErrors->PushError(pDecl, "Constant declaration '%s' is not initialized with a constant expression", pDecl->identifier.pData);
                    else
                        pEntity->constantValue = pDecl->pInitializerExpr->constantValue;
                }

                TypeInfo* pTypeAnnotationTypeInfo = GetVoidType();
                if (pDecl->pTypeAnnotation) {
                    pDecl->pTypeAnnotation = TypeCheckExpression(state, pDecl->pTypeAnnotation);
                    if (pDecl->pTypeAnnotation->constantValue.pTypeInfo)
                        pTypeAnnotationTypeInfo = pDecl->pTypeAnnotation->constantValue.pTypeInfo;
                }

                // has type annotation, check it matches the initializer, if so, set the type of the declaration
                if (pDecl->pTypeAnnotation && !CheckTypesIdentical(pTypeAnnotationTypeInfo, pDecl->pInitializerExpr->pType)) {
                    TypeInfo* pInitType = pDecl->pInitializerExpr->pType;
                    if (pInitType)
                        state.pErrors->PushError(pDecl->pTypeAnnotation, "Type mismatch in declaration, declared as %s and initialized as %s", pTypeAnnotationTypeInfo->name.pData, pInitType->name.pData);
                } else {
                    pDecl->pType = pDecl->pInitializerExpr->pType;
                }
            // No initializer, must have type annotation
            } else {
                pDecl->pTypeAnnotation = TypeCheckExpression(state, pDecl->pTypeAnnotation);
                if (!pDecl->pTypeAnnotation->isConstant) {
                    state.pErrors->PushError(pDecl->pTypeAnnotation, "Type annotation for declaration must be a constant");
                } else {
                    pDecl->pType = pDecl->pTypeAnnotation->constantValue.pTypeInfo;
                }
            }

            pEntity->pType = pDecl->pType;
            pEntity->status = EntityStatus::Resolved;

            if (pEntity->kind == EntityKind::Variable)
                pEntity->isLive = true;

			break;
        }
        case Ast::NodeKind::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            if (CheckIsDataScope(state.pCurrentScope->kind)) {
                state.pErrors->PushError(pPrint, "Cannot execute imperative code in data scope");
            }
            pPrint->pExpr = TypeCheckExpression(state, pPrint->pExpr);
            break;
        }
        case Ast::NodeKind::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            if (CheckIsDataScope(state.pCurrentScope->kind))
                state.pErrors->PushError(pReturn, "Cannot execute imperative code in data scope");

            pReturn->pExpr = TypeCheckExpression(state, pReturn->pExpr);
            // TODO: Typecheck against the expected return type of the owning function
            break;
        }
        case Ast::NodeKind::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            pExprStmt->pExpr = TypeCheckExpression(state, pExprStmt->pExpr);
            if (CheckIsDataScope(state.pCurrentScope->kind) && !pExprStmt->pExpr->isConstant)
                state.pErrors->PushError(pExprStmt, "Cannot execute imperative code in data scope");
            break;
        }
        case Ast::NodeKind::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            if (CheckIsDataScope(state.pCurrentScope->kind))
                state.pErrors->PushError(pIf, "Cannot execute imperative code in data scope");

            pIf->pCondition = TypeCheckExpression(state, pIf->pCondition);
            if (!CheckTypesIdentical(pIf->pCondition->pType, GetBoolType()))
                state.pErrors->PushError(pIf->pCondition, "if conditional expression does not evaluate to a boolean");

            TypeCheckStatement(state, pIf->pThenStmt);

            if (pIf->pElseStmt)
                TypeCheckStatement(state, pIf->pElseStmt);
            break;
        }
        case Ast::NodeKind::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            if (CheckIsDataScope(state.pCurrentScope->kind))
                state.pErrors->PushError(pWhile, "Cannot execute imperative code in data scope");

            pWhile->pCondition = TypeCheckExpression(state, pWhile->pCondition);
            if (!CheckTypesIdentical(pWhile->pCondition->pType, GetBoolType()))
                state.pErrors->PushError(pWhile->pCondition, "while conditional expression does not evaluate to a boolean");

            TypeCheckStatement(state, pWhile->pBody);
            break;
        }
        case Ast::NodeKind::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            if (CheckIsDataScope(state.pCurrentScope->kind) && state.pCurrentScope->kind != ScopeKind::Function)
                state.pErrors->PushError(pBlock, "Cannot execute imperative code in data scope");

            state.pCurrentScope = pBlock->pScope;
            TypeCheckStatements(state, pBlock->declarations);
            state.pCurrentScope = pBlock->pScope->pParent;

            for (size_t i = 0; i < pBlock->pScope->entities.tableSize; i++) { 
                HashNode<String, Entity*>& node = pBlock->pScope->entities.pTable[i];
                if (node.hash != UNUSED_HASH) {
                    Entity* pEntity = node.value;
                    if (pEntity->kind == EntityKind::Variable)
                        pEntity->isLive = false;
                }
            }

            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void TypeCheckStatements(TypeCheckerState& state, ResizableArray<Ast::Statement*>& program) {
    for (size_t i = 0; i < program.count; i++) {
        Ast::Statement* pStmt = program[i];
        TypeCheckStatement(state, pStmt);
    }
}

// ***********************************************************************

void CollectEntities(TypeCheckerState& state, ResizableArray<Ast::Statement*>& statements);
void CollectEntitiesInStatement(TypeCheckerState& state, Ast::Statement* pStmt);

void CollectEntitiesInExpression(TypeCheckerState& state, Ast::Expression* pExpr) {
    switch (pExpr->nodeKind) {
        case Ast::NodeKind::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            CollectEntitiesInExpression(state, pBinary->pLeft);
            CollectEntitiesInExpression(state, pBinary->pRight);
            break;
        }
        case Ast::NodeKind::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            CollectEntitiesInExpression(state, pUnary->pRight);
            break;
        }
        case Ast::NodeKind::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;
            CollectEntitiesInExpression(state, pCall->pCallee);
            for (size_t i = 0; i < pCall->args.count; i++) {
                CollectEntitiesInExpression(state, pCall->args[i]);
            }
            break;
        }
        case Ast::NodeKind::GetField: {
            Ast::GetField* pGetField = (Ast::GetField*)pExpr;
            CollectEntitiesInExpression(state, pGetField->pTarget);
            break;
        }
        case Ast::NodeKind::SetField: {
            Ast::SetField* pSetField = (Ast::SetField*)pExpr;
            CollectEntitiesInExpression(state, pSetField->pTarget);
            CollectEntitiesInExpression(state, pSetField->pAssignment);
            break;
        }
        case Ast::NodeKind::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            CollectEntitiesInExpression(state, pGroup->pExpression);
            break;
        }
        case Ast::NodeKind::Cast: {
            Ast::Cast* pCast = (Ast::Cast*)pExpr;
            CollectEntitiesInExpression(state, pCast->pExprToCast);
            CollectEntitiesInExpression(state, pCast->pTypeExpr);
            break;
        }
        case Ast::NodeKind::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            CollectEntitiesInExpression(state, pVarAssignment->pAssignment);
            break;
        }
        case Ast::NodeKind::FunctionType: {
            Ast::FunctionType* pFuncType = (Ast::FunctionType*)pExpr;
            pFuncType->pScope = CreateScope(ScopeKind::FunctionType, state.pCurrentScope, state.pAllocator);
            pFuncType->pScope->startLine = pFuncType->line;
            pFuncType->pScope->endLine = pFuncType->line;

            state.pCurrentScope = pFuncType->pScope;
            for (size_t i = 0; i < pFuncType->params.count; i++) {
                if (pFuncType->params[i]->nodeKind == Ast::NodeKind::Identifier) {
                    CollectEntitiesInExpression(state, (Ast::Expression*)pFuncType->params[i]);
                } else if (pFuncType->params[i]->nodeKind == Ast::NodeKind::Declaration) {
                    CollectEntitiesInStatement(state, (Ast::Declaration*)pFuncType->params[i]);
                }
            }
            if (pFuncType->pReturnType)
                CollectEntitiesInExpression(state, pFuncType->pReturnType);
            state.pCurrentScope = pFuncType->pScope->pParent;
            break;
        }
        case Ast::NodeKind::Structure: {
            Ast::Structure* pStruct = (Ast::Structure*)pExpr;
            pStruct->pScope = CreateScope(ScopeKind::Struct, state.pCurrentScope, state.pAllocator);
            pStruct->pScope->startLine = pStruct->startToken.line;
            pStruct->pScope->endLine = pStruct->endToken.line;

            state.pCurrentScope = pStruct->pScope;
            for (size_t i = 0; i < pStruct->members.count; i++) {
                CollectEntitiesInStatement(state, pStruct->members[i]);
            }
            state.pCurrentScope = pStruct->pScope->pParent;
            break;
        }
        case Ast::NodeKind::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            pFunction->pScope = CreateScope(ScopeKind::Function, state.pCurrentScope, state.pAllocator);
            pFunction->pScope->startLine = pFunction->line;
            pFunction->pScope->endLine = pFunction->line;

            state.pCurrentScope = pFunction->pScope;
            for (size_t i = 0; i < pFunction->pFuncType->params.count; i++) {
                if (pFunction->pFuncType->params[i]->nodeKind == Ast::NodeKind::Identifier) {
                    CollectEntitiesInExpression(state, (Ast::Expression*)pFunction->pFuncType->params[i]);
                } else if (pFunction->pFuncType->params[i]->nodeKind == Ast::NodeKind::Declaration) {
                    CollectEntitiesInStatement(state, (Ast::Declaration*)pFunction->pFuncType->params[i]);
                }
            }
            if (pFunction->pFuncType->pReturnType)
                CollectEntitiesInExpression(state, pFunction->pFuncType->pReturnType);

            CollectEntitiesInStatement(state, pFunction->pBody);

            state.pCurrentScope = pFunction->pScope->pParent;
            break;
        }
        case Ast::NodeKind::Identifier:
        case Ast::NodeKind::Literal:
        default:
            break;
    }
}

// ***********************************************************************

void CollectEntitiesInStatement(TypeCheckerState& state, Ast::Statement* pStmt) {
    switch (pStmt->nodeKind) {
        case Ast::NodeKind::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            CollectEntitiesInExpression(state, pExprStmt->pExpr);
            break;
        }
        case Ast::NodeKind::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            // TODO: Create scope here for conditions
            CollectEntitiesInExpression(state, pIf->pCondition);
            CollectEntitiesInStatement(state, pIf->pThenStmt);
            if (pIf->pElseStmt)
                CollectEntitiesInStatement(state, pIf->pElseStmt);
            break;
        }
        case Ast::NodeKind::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            // TODO: Create scope here for condition
            CollectEntitiesInExpression(state, pWhile->pCondition);
            CollectEntitiesInStatement(state, pWhile->pBody);
            break;
        }
        case Ast::NodeKind::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            CollectEntitiesInExpression(state, pPrint->pExpr);
            break;
        }
        case Ast::NodeKind::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            CollectEntitiesInExpression(state, pReturn->pExpr);
            break;
        }
        case Ast::NodeKind::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;

            Entity* pExistingEntity = FindEntity(state.pCurrentScope, pDecl->identifier);
            if (pExistingEntity) {
                // Error for a redefinition, except for function params that are duplicating a variable name, those are fine
                bool isFunctionParam = state.pCurrentScope->kind == ScopeKind::Function || state.pCurrentScope->kind == ScopeKind::FunctionType;
                bool doesntExistInSameScope = state.pCurrentScope->entities.Get(pDecl->identifier) == nullptr;
                if (!(isFunctionParam && doesntExistInSameScope && pExistingEntity->kind == EntityKind::Variable)) {
                    state.pErrors->PushError(pDecl, "Redefinition of variable '%s'", pDecl->identifier.pData);
                    pDecl->pType = GetVoidType(); // invalid type technically
                    break;
                }
            }

            Entity* pEntity = (Entity*)state.pAllocator->Allocate(sizeof(Entity));
            pEntity->pDeclaration = pDecl;
            pEntity->isLive = false;
            pEntity->status = EntityStatus::Unresolved;
            pEntity->pType = nullptr;
            pEntity->name = pDecl->identifier;

            if (pDecl->isConstantDeclaration) {
                pEntity->kind = EntityKind::Constant;
            } else {
                pEntity->kind = EntityKind::Variable;
            }

            state.pCurrentScope->entities[pEntity->name] = pEntity;

            if (pDecl->pTypeAnnotation)
                CollectEntitiesInExpression(state, pDecl->pTypeAnnotation);

            if (pDecl->pInitializerExpr) {
                CollectEntitiesInExpression(state, pDecl->pInitializerExpr);
                if (pDecl->pInitializerExpr->nodeKind == Ast::NodeKind::Function) {
                    Ast::Function* pFunc = (Ast::Function*)pDecl->pInitializerExpr;
                    pFunc->pDeclaration = pDecl;
                }
                if (pDecl->pInitializerExpr->nodeKind == Ast::NodeKind::Structure) {
                    Ast::Structure* pStruct = (Ast::Structure*)pDecl->pInitializerExpr;
                    pStruct->pDeclaration = pDecl;
                }
            }
            break;
        }
        case Ast::NodeKind::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            pBlock->pScope = CreateScope(ScopeKind::Block, state.pCurrentScope, state.pAllocator);
            pBlock->pScope->startLine = pBlock->startToken.line;
            pBlock->pScope->endLine = pBlock->endToken.line;

            // Make scope active
            state.pCurrentScope = pBlock->pScope;
            CollectEntities(state, pBlock->declarations);
            state.pCurrentScope = pBlock->pScope->pParent;
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void CollectEntities(TypeCheckerState& state, ResizableArray<Ast::Statement*>& statements) {
    for (size_t i = 0; i < statements.count; i++) {
        Ast::Statement* pStmt = statements[i];
        CollectEntitiesInStatement(state, pStmt);
    }
}

// ***********************************************************************

void AddCoreTypeEntities(TypeCheckerState& state) {

    {
        Ast::Declaration* pDecl = (Ast::Declaration*)state.pAllocator->Allocate(sizeof(Ast::Declaration));
        pDecl->nodeKind = Ast::NodeKind::Declaration;
        pDecl->identifier = String("i32");
        pDecl->pType = GetTypeType();
        pDecl->isConstantDeclaration = true;

        Entity* pEntity = (Entity*)state.pAllocator->Allocate(sizeof(Entity));
        pEntity->pDeclaration = pDecl;
        pEntity->isLive = false;
        pEntity->status = EntityStatus::Resolved;
        pEntity->pType = GetTypeType();
        pEntity->name = pDecl->identifier;
        pEntity->kind = EntityKind::Constant;
        pEntity->constantValue = MakeValue(GetI32Type());
        
        state.pGlobalScope->entities[GetI32Type()->name] = pEntity;
    }

    {
        Ast::Declaration* pDecl = (Ast::Declaration*)state.pAllocator->Allocate(sizeof(Ast::Declaration));
        pDecl->nodeKind = Ast::NodeKind::Declaration;
        pDecl->identifier = String("f32");
        pDecl->pType = GetTypeType();
        pDecl->isConstantDeclaration = true;

        Entity* pEntity = (Entity*)state.pAllocator->Allocate(sizeof(Entity));
        pEntity->pDeclaration = pDecl;
        pEntity->isLive = false;
        pEntity->status = EntityStatus::Resolved;
        pEntity->pType = GetTypeType();
        pEntity->name = pDecl->identifier;
        pEntity->kind = EntityKind::Constant;
        pEntity->constantValue = MakeValue(GetF32Type());
        
        state.pGlobalScope->entities[GetF32Type()->name] = pEntity;
    }

     {
        Ast::Declaration* pDecl = (Ast::Declaration*)state.pAllocator->Allocate(sizeof(Ast::Declaration));
        pDecl->nodeKind = Ast::NodeKind::Declaration;
        pDecl->identifier = String("bool");
        pDecl->pType = GetTypeType();
        pDecl->isConstantDeclaration = true;

        Entity* pEntity = (Entity*)state.pAllocator->Allocate(sizeof(Entity));
        pEntity->pDeclaration = pDecl;
        pEntity->isLive = false;
        pEntity->status = EntityStatus::Resolved;
        pEntity->pType = GetTypeType();
        pEntity->name = pDecl->identifier;
        pEntity->kind = EntityKind::Constant;
        pEntity->constantValue = MakeValue(GetBoolType());
        
        state.pGlobalScope->entities[GetBoolType()->name] = pEntity;
    }

    {
        Ast::Declaration* pDecl = (Ast::Declaration*)state.pAllocator->Allocate(sizeof(Ast::Declaration));
        pDecl->nodeKind = Ast::NodeKind::Declaration;
        pDecl->identifier = String("void");
        pDecl->pType = GetTypeType();
        pDecl->isConstantDeclaration = true;

        Entity* pEntity = (Entity*)state.pAllocator->Allocate(sizeof(Entity));
        pEntity->pDeclaration = pDecl;
        pEntity->isLive = false;
        pEntity->status = EntityStatus::Resolved;
        pEntity->pType = GetTypeType();
        pEntity->name = pDecl->identifier;
        pEntity->kind = EntityKind::Constant;
        pEntity->constantValue = MakeValue(GetVoidType());
        
        state.pGlobalScope->entities[GetVoidType()->name] = pEntity;
    }

    {
        Ast::Declaration* pDecl = (Ast::Declaration*)state.pAllocator->Allocate(sizeof(Ast::Declaration));
        pDecl->nodeKind = Ast::NodeKind::Declaration;
        pDecl->identifier = String("type");
        pDecl->pType = GetTypeType();
        pDecl->isConstantDeclaration = true;

        Entity* pEntity = (Entity*)state.pAllocator->Allocate(sizeof(Entity));
        pEntity->pDeclaration = pDecl;
        pEntity->isLive = false;
        pEntity->status = EntityStatus::Resolved;
        pEntity->pType = GetTypeType();
        pEntity->name = pDecl->identifier;
        pEntity->kind = EntityKind::Constant;
        pEntity->constantValue = MakeValue(GetTypeType());
        
        state.pGlobalScope->entities[GetTypeType()->name] = pEntity;
    }
}

// ***********************************************************************

void TypeCheckProgram(Compiler& compilerState) {
    if (compilerState.errorState.errors.count == 0) {
        TypeCheckerState state;

        state.pErrors = &compilerState.errorState;
        state.pAllocator = &compilerState.compilerMemory;
        state.pGlobalScope = CreateScope(ScopeKind::Global, nullptr, &compilerState.compilerMemory);
        state.pCurrentScope = state.pGlobalScope;

        // stage 1, collect all entities, tracking scope
        CollectEntities(state, compilerState.syntaxTree);

        // stage 1.5 add core types to global scope
        AddCoreTypeEntities(state);

        // stage 2, actually typecheck program
        TypeCheckStatements(state, compilerState.syntaxTree);

        compilerState.pGlobalScope = state.pGlobalScope;
    }
}