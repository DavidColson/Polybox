// Copyright 2020-2022 David Colson. All rights reserved.

#include "type_checker.h"

#include "parser.h"

#include <hashmap.inl>
#include <resizable_array.inl>
#include <string_builder.h>
#include <memory.h>

struct TypeCheckerState {
    HashMap<String, Ast::Declaration*> m_declarations;
    ErrorState* m_pErrors { nullptr };
    int m_currentScopeLevel { 0 };
    bool m_currentlyDeclaringParams { false };
    IAllocator* m_pAllocator{ nullptr };

    Ast::Declaration* m_lastFunctionDeclaration{ nullptr }; // temp until we have constant functions
};

void TypeCheckStatement(TypeCheckerState& state, Ast::Statement* pStmt);
void TypeCheckStatements(TypeCheckerState& state, ResizableArray<Ast::Statement*>& program);

// ***********************************************************************

bool IsImplicitlyCastable(TypeInfo* pFrom, TypeInfo* pTo) {
	// TODO: When adding new core types, ensure no loss of signedness and no truncation or loss of precision
	if (pFrom == GetI32Type() && pTo == GetF32Type()) {
		return true;
	}
	return false;
}

// ***********************************************************************

[[nodiscard]] Ast::Expression* TypeCheckExpression(TypeCheckerState& state, Ast::Expression* pExpr) {
    if (pExpr == nullptr)
        return pExpr;

    switch (pExpr->m_nodeKind) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            pLiteral->m_pType = pLiteral->m_value.m_pType;
            return pLiteral;
        }
        case Ast::NodeType::Type: {
            Ast::Type* pType = (Ast::FnType*)pExpr;
            pType->m_pType = GetTypeType();

            // Resolve Type
            if (pType->m_identifier == "i32") {
                pType->m_pResolvedType = GetI32Type();
            } else if (pType->m_identifier == "f32") {
                pType->m_pResolvedType = GetF32Type(); 
            } else if (pType->m_identifier == "bool") {
                pType->m_pResolvedType = GetBoolType();
            } else if (pType->m_identifier == "Type") {
                pType->m_pResolvedType = GetTypeType();
            } else {
                pType->m_pResolvedType = GetVoidType();
            }

            return pType;
        }
        case Ast::NodeType::FnType: {
            Ast::FnType* pFnType = (Ast::FnType*)pExpr;
            pFnType->m_pType = GetTypeType();

            StringBuilder builder;
            builder.Append("fn (");

            TypeInfoFunction newTypeInfo;
            newTypeInfo.tag = TypeInfo::TypeTag::Function;

            for (size_t i = 0; i < pFnType->m_params.m_count; i++) {
                pFnType->m_params[i] = (Ast::Type*)TypeCheckExpression(state, pFnType->m_params[i]);
                Ast::Type* pParam = pFnType->m_params[i];

                newTypeInfo.params.PushBack(pParam->m_pResolvedType);
                if (i < pFnType->m_params.m_count - 1) {
                    builder.AppendFormat("%s, ", pParam->m_pResolvedType->name.m_pData);
                } else {
                    builder.AppendFormat("%s", pParam->m_pResolvedType->name.m_pData);
                }
            }

            pFnType->m_pReturnType = (Ast::Type*)TypeCheckExpression(state, pFnType->m_pReturnType);
            newTypeInfo.pReturnType = pFnType->m_pReturnType->m_pResolvedType;
            builder.Append(")");
            if (newTypeInfo.pReturnType)
                builder.AppendFormat(" -> %s", newTypeInfo.pReturnType->name.m_pData);
            newTypeInfo.name = builder.CreateString();

            pFnType->m_pResolvedType = FindOrAddType(&newTypeInfo);
            return pFnType;
        }
        case Ast::NodeType::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;

            // The params will end up in the same scope as the body, and get automatically yeeted from the declarations list at the end of the block
            state.m_currentlyDeclaringParams = true;
            state.m_currentScopeLevel++;
            for (Ast::Declaration* pParamDecl : pFunction->m_params) {
                TypeCheckStatement(state, pParamDecl);
            }
            state.m_currentScopeLevel--;
            state.m_currentlyDeclaringParams = false;

            // Create or find type info for this function
            StringBuilder builder;
            builder.Append("fn (");
            TypeInfoFunction newTypeInfo;
            newTypeInfo.tag = TypeInfo::TypeTag::Function;
            for (size_t i = 0; i < pFunction->m_params.m_count; i++) {
                Ast::Declaration* pParam = pFunction->m_params[i];
                newTypeInfo.params.PushBack(pParam->m_pResolvedType);
                if (i < pFunction->m_params.m_count - 1) {
                    builder.AppendFormat("%s, ", pParam->m_pResolvedType->name.m_pData);
                } else {
                    builder.AppendFormat("%s", pParam->m_pResolvedType->name.m_pData);
                }
            }
            pFunction->m_pReturnType = (Ast::Type*)TypeCheckExpression(state, pFunction->m_pReturnType);
            newTypeInfo.pReturnType = pFunction->m_pReturnType->m_pResolvedType;
            builder.Append(")");
            if (newTypeInfo.pReturnType)
                builder.AppendFormat(" -> %s", newTypeInfo.pReturnType->name.m_pData);
            newTypeInfo.name = builder.CreateString();
            pFunction->m_pType = FindOrAddType(&newTypeInfo);

            // temp until we have constant declarations which will already be in declarations table
            // This is required for recursion for now
            state.m_lastFunctionDeclaration->m_pResolvedType = pFunction->m_pType;
            TypeCheckStatement(state, pFunction->m_pBody);

            // TODO: Check maximum number of arguments (255 uint8) and error if above
            return pFunction;
        }
        case Ast::NodeType::Identifier: {
            Ast::Identifier* pIdentifier = (Ast::Identifier*)pExpr;
    
            // Is this a type? If so need to replace this identifier node with a type node
            if (pIdentifier->m_identifier == "i32"
                | pIdentifier->m_identifier == "f32"
                | pIdentifier->m_identifier == "bool"
                | pIdentifier->m_identifier == "Type") {
                Ast::Type* pType = (Ast::Type*)state.m_pAllocator->Allocate(sizeof(Ast::Type));
                pType->m_nodeKind = Ast::NodeType::Type;
                pType->m_identifier = pIdentifier->m_identifier;
                pType->m_pLocation = pIdentifier->m_pLocation;
                pType->m_pLineStart = pIdentifier->m_pLineStart;
                pType->m_line = pIdentifier->m_line;

                pType = (Ast::Type*)TypeCheckExpression(state, pType);
                return pType;
            }

            Ast::Declaration** pDeclEntry = state.m_declarations.Get(pIdentifier->m_identifier);
            if (pDeclEntry) {
                Ast::Declaration* pDecl = *pDeclEntry;
                if (pDecl->m_initialized) {
                    pIdentifier->m_pType = pDecl->m_pResolvedType;
                } else {
                    state.m_pErrors->PushError(pIdentifier, "Cannot use \'%s\', it is not initialized yet", pIdentifier->m_identifier.m_pData);     
                }
            } else {
                state.m_pErrors->PushError(pIdentifier, "Undeclared variable \'%s\', missing a declaration somewhere before?", pIdentifier->m_identifier.m_pData);
            }

            return pIdentifier;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            pVarAssignment->m_pAssignment = TypeCheckExpression(state, pVarAssignment->m_pAssignment);

            Ast::Declaration** pDeclEntry = state.m_declarations.Get(pVarAssignment->m_identifier);
            if (pDeclEntry) {
                Ast::Declaration* pDecl = *pDeclEntry;

                TypeInfo* pDeclaredVarType = pDecl->m_pResolvedType;
                TypeInfo* pAssignedVarType = pVarAssignment->m_pAssignment->m_pType;
                if (pDeclaredVarType == pAssignedVarType) {
                    pVarAssignment->m_pType = pDeclaredVarType;
                } else {
                    state.m_pErrors->PushError(pVarAssignment, "Type mismatch on assignment, \'%s\' has type '%s', but is being assigned a value with type '%s'", pVarAssignment->m_identifier.m_pData, pDeclaredVarType->name.m_pData, pAssignedVarType->name.m_pData);
                }
                pDecl->m_initialized = true;
            } else {
                state.m_pErrors->PushError(pVarAssignment, "Assigning to undeclared variable \'%s\', missing a declaration somewhere before?", pVarAssignment->m_identifier.m_pData);
            }

            return pVarAssignment;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            pGroup->m_pExpression = TypeCheckExpression(state, pGroup->m_pExpression);
            pGroup->m_pType = pGroup->m_pExpression->m_pType;
            return pGroup;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            pBinary->m_pLeft = TypeCheckExpression(state, pBinary->m_pLeft);
            pBinary->m_pRight = TypeCheckExpression(state, pBinary->m_pRight);
            
			// If mismatch, check if we can do an implicit cast, otherwise fail
			if (pBinary->m_pLeft->m_pType != pBinary->m_pRight->m_pType) {
				if (IsImplicitlyCastable(pBinary->m_pLeft->m_pType, pBinary->m_pRight->m_pType)) {
					Ast::Cast* pCastExpr = (Ast::Cast*)state.m_pAllocator->Allocate(sizeof(Ast::Cast));
					pCastExpr->m_nodeKind = Ast::NodeType::Cast;
					pCastExpr->m_pExprToCast = pBinary->m_pLeft;

					Ast::Type* pType = (Ast::Type*)state.m_pAllocator->Allocate(sizeof(Ast::Type));
					pType->m_nodeKind = Ast::NodeType::Type;
					pType->m_identifier = pBinary->m_pRight->m_pType->name;
					pType->m_pLocation = pBinary->m_pRight->m_pLocation;
					pType->m_pLineStart = pBinary->m_pRight->m_pLineStart;
					pType->m_line = pBinary->m_pRight->m_line;
					pCastExpr->m_pTargetType = (Ast::Type*)TypeCheckExpression(state, pType);

					pCastExpr->m_pLocation = pBinary->m_pLeft->m_pLocation;
					pCastExpr->m_pLineStart = pBinary->m_pLeft->m_pLineStart;
					pCastExpr->m_line = pBinary->m_pLeft->m_line;

					pBinary->m_pLeft = TypeCheckExpression(state, pCastExpr);

				} else if (IsImplicitlyCastable(pBinary->m_pRight->m_pType, pBinary->m_pLeft->m_pType)) {
					Ast::Cast* pCastExpr = (Ast::Cast*)state.m_pAllocator->Allocate(sizeof(Ast::Cast));
					pCastExpr->m_nodeKind = Ast::NodeType::Cast;
					pCastExpr->m_pExprToCast = pBinary->m_pRight;

					Ast::Type* pType = (Ast::Type*)state.m_pAllocator->Allocate(sizeof(Ast::Type));
					pType->m_nodeKind = Ast::NodeType::Type;
					pType->m_identifier = pBinary->m_pLeft->m_pType->name;
					pType->m_pLocation = pBinary->m_pLeft->m_pLocation;
					pType->m_pLineStart = pBinary->m_pLeft->m_pLineStart;
					pType->m_line = pBinary->m_pLeft->m_line;
					pCastExpr->m_pTargetType = (Ast::Type*)TypeCheckExpression(state, pType);

					pCastExpr->m_pLocation = pBinary->m_pRight->m_pLocation;
					pCastExpr->m_pLineStart = pBinary->m_pRight->m_pLineStart;
					pCastExpr->m_line = pBinary->m_pRight->m_line;

					pBinary->m_pRight = TypeCheckExpression(state, pCastExpr);
				} else {
					String str1 = pBinary->m_pLeft->m_pType->name;
					String str2 = pBinary->m_pRight->m_pType->name;
					String str3 = Operator::ToString(pBinary->m_operator);
					state.m_pErrors->PushError(pBinary, "Invalid types (%s, %s) used with operator \"%s\"", str1.m_pData, str2.m_pData, str3.m_pData);
				}
			}

			if (pBinary->m_operator == Operator::Subtract
				|| pBinary->m_operator == Operator::Multiply
				|| pBinary->m_operator == Operator::Divide
				|| pBinary->m_operator == Operator::Add
				|| pBinary->m_operator == Operator::Subtract) {
				pBinary->m_pType = pBinary->m_pLeft->m_pType;
			} else {
				pBinary->m_pType = GetBoolType();
			}
            return pBinary;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            pUnary->m_pRight = TypeCheckExpression(state, pUnary->m_pRight);

			if (pUnary->m_operator == Operator::Not) {
				pUnary->m_pType = GetBoolType();
				if (pUnary->m_pRight->m_pType != GetBoolType()) {
					state.m_pErrors->PushError(pUnary, "Invalid type (%s) used with operator \"%s\"", pUnary->m_pRight->m_pType->name.m_pData, Operator::ToString(pUnary->m_operator));
				}
			} else if (pUnary->m_operator == Operator::UnaryMinus) {
				pUnary->m_pType = pUnary->m_pRight->m_pType;
				if (pUnary->m_pRight->m_pType != GetBoolType()) {
					state.m_pErrors->PushError(pUnary, "Invalid type (%s) used with operator \"%s\"", pUnary->m_pRight->m_pType->name.m_pData, Operator::ToString(pUnary->m_operator));
				}
			}

            return pUnary;
        }
		case Ast::NodeType::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;
			pCast->m_pTargetType = (Ast::Type*)TypeCheckExpression(state, pCast->m_pTargetType);
			pCast->m_pExprToCast = TypeCheckExpression(state, pCast->m_pExprToCast);

			TypeInfo* pFrom = pCast->m_pExprToCast->m_pType;
			TypeInfo* pTo = pCast->m_pTargetType->m_pResolvedType;

			bool castAllowed = false;
			if ((pFrom == GetF32Type() && pTo == GetI32Type())
				| (pFrom == GetF32Type() && pTo == GetBoolType())
				| (pFrom == GetI32Type() && pTo == GetF32Type())
				| (pFrom == GetI32Type() && pTo == GetBoolType())
				| (pFrom == GetBoolType() && pTo == GetI32Type())
				| (pFrom == GetBoolType() && pTo == GetF32Type())) {
				castAllowed = true;
			}

			if (pFrom == pTo) {
				state.m_pErrors->PushError(pCast, "Cast from \"%s\" to \"%s\" is pointless", pFrom->name.m_pData, pTo->name.m_pData);
			} else if (castAllowed == false) {
				state.m_pErrors->PushError(pCast, "Not possible to cast from type \"%s\" to \"%s\"", pFrom->name.m_pData, pTo->name.m_pData);
			}
			
			pCast->m_pType = pCast->m_pTargetType->m_pResolvedType;
			return pCast;
		}
        case Ast::NodeType::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;

            pCall->m_pCallee = TypeCheckExpression(state, pCall->m_pCallee);

            Ast::Identifier* pVar = (Ast::Identifier*)pCall->m_pCallee;
            Ast::Declaration** pDeclEntry = state.m_declarations.Get(pVar->m_identifier);

            if (pDeclEntry == nullptr) {
                state.m_pErrors->PushError(pCall, "Attempt to call a value which is not declared yet");
                return pCall;
            }
            Ast::Declaration* pDecl = *pDeclEntry;

            if (pCall->m_pCallee->m_pType->tag != TypeInfo::TypeTag::Function) {
                state.m_pErrors->PushError(pCall, "Attempt to call a value which is not a function");
            }

            for (int i = 0; i < (int)pCall->m_args.m_count; i++) {
                pCall->m_args[i] = TypeCheckExpression(state, pCall->m_args[i]);
            }
            
            TypeInfoFunction* pFunctionType = (TypeInfoFunction*)pDecl->m_pResolvedType;
            int argsCount = pCall->m_args.m_count;
            int paramsCount = pFunctionType->params.m_count;
            if (pCall->m_args.m_count != pFunctionType->params.m_count) {
                state.m_pErrors->PushError(pCall, "Mismatched number of arguments in call to function '%s', expected %i, got %i", pVar->m_identifier.m_pData, paramsCount, argsCount);
            }

            int minArgs = argsCount > paramsCount ? paramsCount : argsCount;
            for (size_t i = 0; i < minArgs; i++) {
                Ast::Expression* arg = pCall->m_args[i];
                TypeInfo* pArgType = pFunctionType->params[i];
                if (arg->m_pType != pArgType)
                    state.m_pErrors->PushError(arg, "Type mismatch in function argument '<argname TODO>', expected %s, got %s", pArgType->name.m_pData, arg->m_pType->name.m_pData);
            }
            pCall->m_pType = pFunctionType->pReturnType;
            return pCall;
        }
        default:
            return pExpr;
    }
}

// ***********************************************************************

void TypeCheckStatement(TypeCheckerState& state, Ast::Statement* pStmt) {
    switch (pStmt->m_nodeKind) {
        case Ast::NodeType::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;
            pDecl->m_scopeLevel = state.m_currentScopeLevel;

            if (state.m_declarations.Get(pDecl->m_identifier) != nullptr)
                state.m_pErrors->PushError(pDecl, "Redefinition of variable '%s'", pDecl->m_identifier.m_pData);

            if (state.m_currentlyDeclaringParams)
                pDecl->m_initialized = true;

            if (pDecl->m_pInitializerExpr) {
                if (pDecl->m_pInitializerExpr->m_nodeKind == Ast::NodeType::Function) {
                    // Temp, Allows recursion without constant functions
                    state.m_lastFunctionDeclaration = pDecl;
                    state.m_lastFunctionDeclaration->m_initialized = true;
                }

                state.m_declarations.Add(pDecl->m_identifier, pDecl);
                pDecl->m_pInitializerExpr = TypeCheckExpression(state, pDecl->m_pInitializerExpr);
                pDecl->m_initialized = true;

                if (pDecl->m_pDeclaredType)
                    pDecl->m_pDeclaredType = (Ast::Type*)TypeCheckExpression(state, pDecl->m_pDeclaredType);

                if (pDecl->m_pDeclaredType && pDecl->m_pInitializerExpr->m_pType != pDecl->m_pDeclaredType->m_pResolvedType) {
                    TypeInfo* pDeclaredType = pDecl->m_pDeclaredType->m_pResolvedType;
                    TypeInfo* pInitType = pDecl->m_pInitializerExpr->m_pType;
                    state.m_pErrors->PushError(pDecl->m_pDeclaredType, "Type mismatch in declaration, declared as %s and initialized as %s", pDeclaredType->name.m_pData, pInitType->name.m_pData);
                } else {
                    pDecl->m_pResolvedType = pDecl->m_pInitializerExpr->m_pType;
                }
            } else {
                state.m_declarations.Add(pDecl->m_identifier, pDecl);
                pDecl->m_pDeclaredType = (Ast::Type*)TypeCheckExpression(state, pDecl->m_pDeclaredType);
                pDecl->m_pResolvedType = pDecl->m_pDeclaredType->m_pResolvedType;
            }
            break;
        }
        case Ast::NodeType::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            pPrint->m_pExpr = TypeCheckExpression(state, pPrint->m_pExpr);
            break;
        }
        case Ast::NodeType::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            pReturn->m_pExpr = TypeCheckExpression(state, pReturn->m_pExpr);
            break;
        }
        case Ast::NodeType::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            pExprStmt->m_pExpr = TypeCheckExpression(state, pExprStmt->m_pExpr);
            break;
        }
        case Ast::NodeType::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            pIf->m_pCondition = TypeCheckExpression(state, pIf->m_pCondition);
            if (pIf->m_pCondition->m_pType != GetBoolType())
                state.m_pErrors->PushError(pIf->m_pCondition, "if conditional expression does not evaluate to a boolean");

            TypeCheckStatement(state, pIf->m_pThenStmt);

            if (pIf->m_pElseStmt)
                TypeCheckStatement(state, pIf->m_pElseStmt);
            break;
        }
        case Ast::NodeType::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            pWhile->m_pCondition = TypeCheckExpression(state, pWhile->m_pCondition);
            if (pWhile->m_pCondition->m_pType != GetBoolType())
                state.m_pErrors->PushError(pWhile->m_pCondition, "while conditional expression does not evaluate to a boolean");

            TypeCheckStatement(state, pWhile->m_pBody);
            break;
        }
        case Ast::NodeType::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;

            state.m_currentScopeLevel++;
            TypeCheckStatements(state, pBlock->m_declarations);
            state.m_currentScopeLevel--;

            // Remove variable declarations that are now out of scope
            for (size_t i = 0; i < state.m_declarations.m_tableSize; i++) {
                if (state.m_declarations.m_pTable[i].hash != UNUSED_HASH) {
                    int scopeLevel = state.m_declarations.m_pTable[i].value->m_scopeLevel;

                    if (scopeLevel > state.m_currentScopeLevel)
                        state.m_declarations.Erase(state.m_declarations.m_pTable[i].key);
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
    for (size_t i = 0; i < program.m_count; i++) {
        Ast::Statement* pStmt = program[i];
        TypeCheckStatement(state, pStmt);
    }
}

// ***********************************************************************

void TypeCheckProgram(ResizableArray<Ast::Statement*>& program, ErrorState* pErrors, IAllocator* pAlloc) {
    TypeCheckerState state;

    state.m_pErrors = pErrors;
    state.m_pAllocator = pAlloc;

    TypeCheckStatements(state, program);
}