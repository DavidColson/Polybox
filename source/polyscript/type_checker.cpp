// Copyright 2020-2022 David Colson. All rights reserved.

#include "type_checker.h"

#include "parser.h"

#include <hashmap.inl>
#include <resizable_array.inl>
#include <string_builder.h>
#include <memory.h>

struct TypeCheckerState {
    HashMap<String, Ast::Declaration*> declarations;
    ErrorState* pErrors { nullptr };
    int currentScopeLevel { 0 };
    bool currentlyDeclaringParams { false };
    Ast::Declaration* pCurrentDeclaration{ nullptr }; // nullptr if we are not currently parsing a declaration
	IAllocator* pAllocator{ nullptr };
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

    switch (pExpr->nodeKind) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
			// Nothing to do
            return pLiteral;
        }
        case Ast::NodeType::Type: {
            Ast::Type* pType = (Ast::FnType*)pExpr;
            pType->pType = GetTypeType();

            // Resolve Type
            if (pType->identifier == "i32") {
                pType->pResolvedType = GetI32Type();
            } else if (pType->identifier == "f32") {
                pType->pResolvedType = GetF32Type(); 
            } else if (pType->identifier == "bool") {
                pType->pResolvedType = GetBoolType();
            } else if (pType->identifier == "Type") {
                pType->pResolvedType = GetTypeType();
            } else {
                pType->pResolvedType = GetVoidType();
            }

            return pType;
        }
        case Ast::NodeType::FnType: {
            Ast::FnType* pFnType = (Ast::FnType*)pExpr;
            pFnType->pType = GetTypeType();

            StringBuilder builder;
            builder.Append("fn (");

            TypeInfoFunction newTypeInfo;
            newTypeInfo.tag = TypeInfo::TypeTag::Function;

            for (size_t i = 0; i < pFnType->params.count; i++) {
                pFnType->params[i] = (Ast::Type*)TypeCheckExpression(state, pFnType->params[i]);
                Ast::Type* pParam = pFnType->params[i];

                newTypeInfo.params.PushBack(pParam->pResolvedType);
                if (i < pFnType->params.count - 1) {
                    builder.AppendFormat("%s, ", pParam->pResolvedType->name.pData);
                } else {
                    builder.AppendFormat("%s", pParam->pResolvedType->name.pData);
                }
            }

            pFnType->pReturnType = (Ast::Type*)TypeCheckExpression(state, pFnType->pReturnType);
            newTypeInfo.pReturnType = pFnType->pReturnType->pResolvedType;
            builder.Append(")");
            if (newTypeInfo.pReturnType)
                builder.AppendFormat(" -> %s", newTypeInfo.pReturnType->name.pData);
            newTypeInfo.name = builder.CreateString();

            pFnType->pResolvedType = FindOrAddType(&newTypeInfo);
            return pFnType;
        }
        case Ast::NodeType::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
			Ast::Declaration* pMyDeclaration = state.pCurrentDeclaration;

            // The params will end up in the same scope as the body, and get automatically yeeted from the declarations list at the end of the block
            state.currentlyDeclaringParams = true;
            state.currentScopeLevel++;
            for (Ast::Declaration* pParamDecl : pFunction->params) {
                TypeCheckStatement(state, pParamDecl);
            }
            state.currentScopeLevel--;
            state.currentlyDeclaringParams = false;

            // Create or find type info for this function
            StringBuilder builder;
            builder.Append("fn (");
            TypeInfoFunction newTypeInfo;
            newTypeInfo.tag = TypeInfo::TypeTag::Function;
            for (size_t i = 0; i < pFunction->params.count; i++) {
                Ast::Declaration* pParam = pFunction->params[i];
                newTypeInfo.params.PushBack(pParam->pResolvedType);
                if (i < pFunction->params.count - 1) {
                    builder.AppendFormat("%s, ", pParam->pResolvedType->name.pData);
                } else {
                    builder.AppendFormat("%s", pParam->pResolvedType->name.pData);
                }
            }
            pFunction->pReturnType = (Ast::Type*)TypeCheckExpression(state, pFunction->pReturnType);
            newTypeInfo.pReturnType = pFunction->pReturnType->pResolvedType;
            builder.Append(")");
            if (newTypeInfo.pReturnType)
                builder.AppendFormat(" -> %s", newTypeInfo.pReturnType->name.pData);
            newTypeInfo.name = builder.CreateString();
            pFunction->pType = FindOrAddType(&newTypeInfo);

            pMyDeclaration->pResolvedType = pFunction->pType;
            TypeCheckStatement(state, pFunction->pBody);

            // TODO: Check maximum number of arguments (255 uint8) and error if above
            return pFunction;
        }
		case Ast::NodeType::Structure: {
			Ast::Structure* pStruct = (Ast::Structure*)pExpr;
			Ast::Declaration* pMyDeclaration = state.pCurrentDeclaration;

			// Typecheck the declarations in this struct

			// Declarations are dealt with differently here because this is not imperative executing code
			// We want to ensure no duplicate identifiers, and ensure the initializers are constant and typematching
			HashMap<String, Ast::Declaration*> internalDeclarations(state.pAllocator);
			for (Ast::Statement* pMemberStmt : pStruct->members) {
				Ast::Declaration* pMember = (Ast::Declaration*)pMemberStmt;
				state.pCurrentDeclaration = pMember;

				pMember->scopeLevel = state.currentScopeLevel + 1;
				
				if (internalDeclarations.Get(pMember->identifier) != nullptr)
					state.pErrors->PushError(pMember, "Redefinition of member '%s'", pMember->identifier.pData);

				// For now we don't add the member declarations to global list, since they aren't accessible to initializers inside this struct, such as functions
				// We will eventually do this for constant values though, since they can be accessed
				internalDeclarations.Add(pMember->identifier, pMember);

				if (pMember->pInitializerExpr) {
					// TODO: This should eventually be, values that are constant and reducible to constants at compile time.
					if (!(pMember->pInitializerExpr->nodeKind == Ast::NodeType::Literal
						|| pMember->pInitializerExpr->nodeKind == Ast::NodeType::Type
						|| pMember->pInitializerExpr->nodeKind == Ast::NodeType::FnType
						|| pMember->pInitializerExpr->nodeKind == Ast::NodeType::Function
						|| pMember->pInitializerExpr->nodeKind == Ast::NodeType::Structure))
					{
						state.pErrors->PushError(pMember, "Unsupported struct member initializer '%s' initializer must be resolvable to a constant at compile time", pMember->identifier.pData);
					}

					pMember->pInitializerExpr = TypeCheckExpression(state, pMember->pInitializerExpr);

					if (pMember->pDeclaredType)
						pMember->pDeclaredType = (Ast::Type*)TypeCheckExpression(state, pMember->pDeclaredType);

					if (pMember->pDeclaredType && pMember->pInitializerExpr->pType != pMember->pDeclaredType->pResolvedType) {
						TypeInfo* pDeclaredType = pMember->pDeclaredType->pResolvedType;
						TypeInfo* pInitType = pMember->pInitializerExpr->pType;
						state.pErrors->PushError(pMember->pDeclaredType, "Type mismatch in declaration, declared as %s and initialized as %s", pDeclaredType->name.pData, pInitType->name.pData);
					} else {
						pMember->pResolvedType = pMember->pInitializerExpr->pType;
					}
				} else {
					pMember->pDeclaredType = (Ast::Type*)TypeCheckExpression(state, pMember->pDeclaredType);
					pMember->pResolvedType = pMember->pDeclaredType->pResolvedType;
				}
				state.pCurrentDeclaration = nullptr;
			}

			// Create the actual type that this struct is
			TypeInfoStruct newTypeInfo;
			newTypeInfo.tag = TypeInfo::TypeTag::Struct;
			for (Ast::Statement* pMemberStmt : pStruct->members) {
				Ast::Declaration* pMember = (Ast::Declaration*)pMemberStmt;
				TypeInfoStruct::Member mem;
				mem.identifier = pMember->identifier;
				mem.pType = pMember->pResolvedType;
				newTypeInfo.members.PushBack(mem);

				if (mem.pType)
					newTypeInfo.size += mem.pType->size;
			}
			newTypeInfo.name = pMyDeclaration->identifier;
			pStruct->pDescribedType = FindOrAddType(&newTypeInfo);
			pStruct->pType = GetTypeType();

			return pStruct;
		}
        case Ast::NodeType::Identifier: {
            Ast::Identifier* pIdentifier = (Ast::Identifier*)pExpr;
    
            // Is this a type? If so need to replace this identifier node with a type node
            if (pIdentifier->identifier == "i32"
                | pIdentifier->identifier == "f32"
                | pIdentifier->identifier == "bool"
                | pIdentifier->identifier == "Type") {
                Ast::Type* pType = (Ast::Type*)state.pAllocator->Allocate(sizeof(Ast::Type));
                pType->nodeKind = Ast::NodeType::Type;
                pType->identifier = pIdentifier->identifier;
                pType->pLocation = pIdentifier->pLocation;
                pType->pLineStart = pIdentifier->pLineStart;
                pType->line = pIdentifier->line;

                pType = (Ast::Type*)TypeCheckExpression(state, pType);
                return pType;
            }

            Ast::Declaration** pDeclEntry = state.declarations.Get(pIdentifier->identifier);
            if (pDeclEntry) {
                Ast::Declaration* pDecl = *pDeclEntry;
                pIdentifier->pType = pDecl->pResolvedType;
            } else {
                state.pErrors->PushError(pIdentifier, "Undeclared variable \'%s\', missing a declaration somewhere before?", pIdentifier->identifier.pData);
            }

            return pIdentifier;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            pVarAssignment->pAssignment = TypeCheckExpression(state, pVarAssignment->pAssignment);

            Ast::Declaration** pDeclEntry = state.declarations.Get(pVarAssignment->identifier);
            if (pDeclEntry) {
                Ast::Declaration* pDecl = *pDeclEntry;

                TypeInfo* pDeclaredVarType = pDecl->pResolvedType;
                TypeInfo* pAssignedVarType = pVarAssignment->pAssignment->pType;
                if (pDeclaredVarType == pAssignedVarType) {
                    pVarAssignment->pType = pDeclaredVarType;
                } else {
                    state.pErrors->PushError(pVarAssignment, "Type mismatch on assignment, \'%s\' has type '%s', but is being assigned a value with type '%s'", pVarAssignment->identifier.pData, pDeclaredVarType->name.pData, pAssignedVarType->name.pData);
                }
            } else {
                state.pErrors->PushError(pVarAssignment, "Assigning to undeclared variable \'%s\', missing a declaration somewhere before?", pVarAssignment->identifier.pData);
            }

            return pVarAssignment;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            pGroup->pExpression = TypeCheckExpression(state, pGroup->pExpression);
            pGroup->pType = pGroup->pExpression->pType;
            return pGroup;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            pBinary->pLeft = TypeCheckExpression(state, pBinary->pLeft);
            pBinary->pRight = TypeCheckExpression(state, pBinary->pRight);
            
			// If mismatch, check if we can do an implicit cast, otherwise fail
			if (pBinary->pLeft->pType != pBinary->pRight->pType) {
				if (IsImplicitlyCastable(pBinary->pLeft->pType, pBinary->pRight->pType)) {
					Ast::Cast* pCastExpr = (Ast::Cast*)state.pAllocator->Allocate(sizeof(Ast::Cast));
					pCastExpr->nodeKind = Ast::NodeType::Cast;
					pCastExpr->pExprToCast = pBinary->pLeft;

					Ast::Type* pType = (Ast::Type*)state.pAllocator->Allocate(sizeof(Ast::Type));
					pType->nodeKind = Ast::NodeType::Type;
					pType->identifier = pBinary->pRight->pType->name;
					pType->pLocation = pBinary->pRight->pLocation;
					pType->pLineStart = pBinary->pRight->pLineStart;
					pType->line = pBinary->pRight->line;
					pCastExpr->pTargetType = (Ast::Type*)TypeCheckExpression(state, pType);

					pCastExpr->pLocation = pBinary->pLeft->pLocation;
					pCastExpr->pLineStart = pBinary->pLeft->pLineStart;
					pCastExpr->line = pBinary->pLeft->line;

					pBinary->pLeft = TypeCheckExpression(state, pCastExpr);

				} else if (IsImplicitlyCastable(pBinary->pRight->pType, pBinary->pLeft->pType)) {
					Ast::Cast* pCastExpr = (Ast::Cast*)state.pAllocator->Allocate(sizeof(Ast::Cast));
					pCastExpr->nodeKind = Ast::NodeType::Cast;
					pCastExpr->pExprToCast = pBinary->pRight;

					Ast::Type* pType = (Ast::Type*)state.pAllocator->Allocate(sizeof(Ast::Type));
					pType->nodeKind = Ast::NodeType::Type;
					pType->identifier = pBinary->pLeft->pType->name;
					pType->pLocation = pBinary->pLeft->pLocation;
					pType->pLineStart = pBinary->pLeft->pLineStart;
					pType->line = pBinary->pLeft->line;
					pCastExpr->pTargetType = (Ast::Type*)TypeCheckExpression(state, pType);

					pCastExpr->pLocation = pBinary->pRight->pLocation;
					pCastExpr->pLineStart = pBinary->pRight->pLineStart;
					pCastExpr->line = pBinary->pRight->line;

					pBinary->pRight = TypeCheckExpression(state, pCastExpr);
				} else {
					String str1 = pBinary->pLeft->pType->name;
					String str2 = pBinary->pRight->pType->name;
					String str3 = Operator::ToString(pBinary->op);
					state.pErrors->PushError(pBinary, "Invalid types (%s, %s) used with op \"%s\"", str1.pData, str2.pData, str3.pData);
				}
			}

			if (pBinary->op == Operator::Subtract
				|| pBinary->op == Operator::Multiply
				|| pBinary->op == Operator::Divide
				|| pBinary->op == Operator::Add
				|| pBinary->op == Operator::Subtract) {
				pBinary->pType = pBinary->pLeft->pType;
			} else {
				pBinary->pType = GetBoolType();
			}
            return pBinary;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            pUnary->pRight = TypeCheckExpression(state, pUnary->pRight);

			if (pUnary->op == Operator::Not) {
				pUnary->pType = GetBoolType();
				if (pUnary->pRight->pType != GetBoolType()) {
					state.pErrors->PushError(pUnary, "Invalid type (%s) used with op \"%s\"", pUnary->pRight->pType->name.pData, Operator::ToString(pUnary->op));
				}
			} else if (pUnary->op == Operator::UnaryMinus) {
				pUnary->pType = pUnary->pRight->pType;
				if (pUnary->pRight->pType != GetBoolType()) {
					state.pErrors->PushError(pUnary, "Invalid type (%s) used with op \"%s\"", pUnary->pRight->pType->name.pData, Operator::ToString(pUnary->op));
				}
			}

            return pUnary;
        }
		case Ast::NodeType::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;
			pCast->pTargetType = (Ast::Type*)TypeCheckExpression(state, pCast->pTargetType);
			pCast->pExprToCast = TypeCheckExpression(state, pCast->pExprToCast);

			TypeInfo* pFrom = pCast->pExprToCast->pType;
			TypeInfo* pTo = pCast->pTargetType->pResolvedType;

			// TODO: Replace this with a function which actually checks their compatibility, in terms of size, etc etc, see umka code
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
				state.pErrors->PushError(pCast, "Cast from \"%s\" to \"%s\" is pointless", pFrom->name.pData, pTo->name.pData);
			} else if (castAllowed == false) {
				state.pErrors->PushError(pCast, "Not possible to cast from type \"%s\" to \"%s\"", pFrom->name.pData, pTo->name.pData);
			}
			
			pCast->pType = pCast->pTargetType->pResolvedType;
			return pCast;
		}
        case Ast::NodeType::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;

            pCall->pCallee = TypeCheckExpression(state, pCall->pCallee);

            Ast::Identifier* pVar = (Ast::Identifier*)pCall->pCallee;
            Ast::Declaration** pDeclEntry = state.declarations.Get(pVar->identifier);

            if (pDeclEntry == nullptr) {
                state.pErrors->PushError(pCall, "Attempt to call a value which is not declared yet");
                return pCall;
            }
            Ast::Declaration* pDecl = *pDeclEntry;

            if (pCall->pCallee->pType->tag != TypeInfo::TypeTag::Function) {
                state.pErrors->PushError(pCall, "Attempt to call a value which is not a function");
            }

            for (int i = 0; i < (int)pCall->args.count; i++) {
                pCall->args[i] = TypeCheckExpression(state, pCall->args[i]);
            }
            
            TypeInfoFunction* pFunctionType = (TypeInfoFunction*)pDecl->pResolvedType;
            int argsCount = pCall->args.count;
            int paramsCount = pFunctionType->params.count;
            if (pCall->args.count != pFunctionType->params.count) {
                state.pErrors->PushError(pCall, "Mismatched number of arguments in call to function '%s', expected %i, got %i", pVar->identifier.pData, paramsCount, argsCount);
            }

            int minArgs = argsCount > paramsCount ? paramsCount : argsCount;
            for (size_t i = 0; i < minArgs; i++) {
                Ast::Expression* arg = pCall->args[i];
                TypeInfo* pArgType = pFunctionType->params[i];
                if (arg->pType != pArgType)
                    state.pErrors->PushError(arg, "Type mismatch in function argument '<argname TODO>', expected %s, got %s", pArgType->name.pData, arg->pType->name.pData);
            }
            pCall->pType = pFunctionType->pReturnType;
            return pCall;
        }
        default:
            return pExpr;
    }
}

// ***********************************************************************

void TypeCheckStatement(TypeCheckerState& state, Ast::Statement* pStmt) {
    switch (pStmt->nodeKind) {
        case Ast::NodeType::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;
			state.pCurrentDeclaration = pDecl;

            pDecl->scopeLevel = state.currentScopeLevel;

            if (state.declarations.Get(pDecl->identifier) != nullptr)
                state.pErrors->PushError(pDecl, "Redefinition of variable '%s'", pDecl->identifier.pData);

            if (pDecl->pInitializerExpr) {
                state.declarations.Add(pDecl->identifier, pDecl);
                pDecl->pInitializerExpr = TypeCheckExpression(state, pDecl->pInitializerExpr);

                if (pDecl->pDeclaredType)
                    pDecl->pDeclaredType = (Ast::Type*)TypeCheckExpression(state, pDecl->pDeclaredType);

                if (pDecl->pDeclaredType && pDecl->pInitializerExpr->pType != pDecl->pDeclaredType->pResolvedType) {
                    TypeInfo* pDeclaredType = pDecl->pDeclaredType->pResolvedType;
                    TypeInfo* pInitType = pDecl->pInitializerExpr->pType;
                    state.pErrors->PushError(pDecl->pDeclaredType, "Type mismatch in declaration, declared as %s and initialized as %s", pDeclaredType->name.pData, pInitType->name.pData);
                } else {
                    pDecl->pResolvedType = pDecl->pInitializerExpr->pType;
                }
            } else {
                state.declarations.Add(pDecl->identifier, pDecl);
                pDecl->pDeclaredType = (Ast::Type*)TypeCheckExpression(state, pDecl->pDeclaredType);
                pDecl->pResolvedType = pDecl->pDeclaredType->pResolvedType;
            }
			state.pCurrentDeclaration = nullptr;
			break;
        }
        case Ast::NodeType::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            pPrint->pExpr = TypeCheckExpression(state, pPrint->pExpr);
            break;
        }
        case Ast::NodeType::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            pReturn->pExpr = TypeCheckExpression(state, pReturn->pExpr);
            break;
        }
        case Ast::NodeType::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            pExprStmt->pExpr = TypeCheckExpression(state, pExprStmt->pExpr);
            break;
        }
        case Ast::NodeType::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            pIf->pCondition = TypeCheckExpression(state, pIf->pCondition);
            if (pIf->pCondition->pType != GetBoolType())
                state.pErrors->PushError(pIf->pCondition, "if conditional expression does not evaluate to a boolean");

            TypeCheckStatement(state, pIf->pThenStmt);

            if (pIf->pElseStmt)
                TypeCheckStatement(state, pIf->pElseStmt);
            break;
        }
        case Ast::NodeType::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            pWhile->pCondition = TypeCheckExpression(state, pWhile->pCondition);
            if (pWhile->pCondition->pType != GetBoolType())
                state.pErrors->PushError(pWhile->pCondition, "while conditional expression does not evaluate to a boolean");

            TypeCheckStatement(state, pWhile->pBody);
            break;
        }
        case Ast::NodeType::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;

            state.currentScopeLevel++;
            TypeCheckStatements(state, pBlock->declarations);
            state.currentScopeLevel--;

            // Remove variable declarations that are now out of scope
            for (size_t i = 0; i < state.declarations.tableSize; i++) {
                if (state.declarations.pTable[i].hash != UNUSED_HASH) {
                    int scopeLevel = state.declarations.pTable[i].value->scopeLevel;

                    if (scopeLevel > state.currentScopeLevel)
                        state.declarations.Erase(state.declarations.pTable[i].key);
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

void TypeCheckProgram(ResizableArray<Ast::Statement*>& program, ErrorState* pErrors, IAllocator* pAlloc) {
    TypeCheckerState state;

    state.pErrors = pErrors;
    state.pAllocator = pAlloc;
	state.declarations.pAlloc = pAlloc;

    TypeCheckStatements(state, program);
}