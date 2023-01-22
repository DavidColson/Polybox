// Copyright 2020-2022 David Colson. All rights reserved.

#include "type_checker.h"

#include "parser.h"

#include <hashmap.inl>
#include <resizable_array.inl>

struct Declaration {
    Ast::Declaration* pNode;
    bool initialized { false };
};

struct TypeCheckerState {
    HashMap<String, Declaration> m_declarations;
    ErrorState* m_pErrors { nullptr };
    int m_currentScopeLevel { 0 };
};

void TypeCheckStatement(TypeCheckerState& state, Ast::Statement* pStmt);
void TypeCheckStatements(TypeCheckerState& state, ResizableArray<Ast::Statement*>& program);


// ***********************************************************************

void TypeCheckExpression(TypeCheckerState& state, Ast::Expression* pExpr) {
    if (pExpr == nullptr)
        return;

    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            pLiteral->m_valueType = pLiteral->m_value.m_type;
            break;
        }
        case Ast::NodeType::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            pFunction->m_valueType = ValueType::Function;

            // TODO: Place into the declarations list the params, so they can resolve inside the body
            TypeCheckStatement(state, pFunction->m_pBody);

            // TODO: Check maximum number of arguments (255 uint8) and error if above
            break;
        }
        case Ast::NodeType::Variable: {
            Ast::Variable* pVariable = (Ast::Variable*)pExpr;

            Declaration* pDeclEntry = state.m_declarations.Get(pVariable->m_identifier);
            if (pDeclEntry) {
                if (pDeclEntry->initialized) {
                    pVariable->m_valueType = pDeclEntry->pNode->m_resolvedType;
                } else {
                    state.m_pErrors->PushError(pVariable, "Cannot use \'%s\', it is not initialized yet", pVariable->m_identifier.m_pData);     
                }
            } else {
                state.m_pErrors->PushError(pVariable, "Undeclared variable \'%s\', missing a declaration somewhere before?", pVariable->m_identifier.m_pData);
            }

            break;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            TypeCheckExpression(state, pVarAssignment->m_pAssignment);

            Declaration* pDeclEntry = state.m_declarations.Get(pVarAssignment->m_identifier);
            if (pDeclEntry) {
                ValueType::Enum declaredVarType = pDeclEntry->pNode->m_resolvedType;
                ValueType::Enum assignedVarType = pVarAssignment->m_pAssignment->m_valueType;
                if (declaredVarType == assignedVarType) {
                    pVarAssignment->m_valueType = declaredVarType;
                } else {
                    state.m_pErrors->PushError(pVarAssignment, "Type mismatch on assignment, \'%s\' has type %s, but is being assigned a value with type %s", pVarAssignment->m_identifier.m_pData, ValueType::ToString(declaredVarType), ValueType::ToString(assignedVarType));
                }
                pDeclEntry->initialized = true;
            } else {
                state.m_pErrors->PushError(pVarAssignment, "Assigning to undeclared variable \'%s\', missing a declaration somewhere before?", pVarAssignment->m_identifier.m_pData);
            }

            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            TypeCheckExpression(state, pGroup->m_pExpression);
            pGroup->m_valueType = pGroup->m_pExpression->m_valueType;
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            TypeCheckExpression(state, pBinary->m_pLeft);
            TypeCheckExpression(state, pBinary->m_pRight);
            pBinary->m_valueType = OperatorReturnType(pBinary->m_operator, pBinary->m_pLeft->m_valueType, pBinary->m_pRight->m_valueType);

            if (pBinary->m_valueType == ValueType::Void && pBinary->m_pLeft->m_valueType != ValueType::Void && pBinary->m_pRight->m_valueType != ValueType::Void) {
                String str1 = ValueType::ToString(pBinary->m_pLeft->m_valueType);
                String str2 = ValueType::ToString(pBinary->m_pRight->m_valueType);
                String str3 = Operator::ToString(pBinary->m_operator);
                state.m_pErrors->PushError(pBinary, "Invalid types (%s, %s) used with operator \"%s\"", str1.m_pData, str2.m_pData, str3.m_pData);
            }
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            TypeCheckExpression(state, pUnary->m_pRight);
            pUnary->m_valueType = OperatorReturnType(pUnary->m_operator, pUnary->m_pRight->m_valueType, ValueType::Void);

            if (pUnary->m_valueType == ValueType::Void && pUnary->m_pRight->m_valueType != ValueType::Void) {
                state.m_pErrors->PushError(pUnary, "Invalid type (%s) used with operator \"%s\"", ValueType::ToString(pUnary->m_pRight->m_valueType), Operator::ToString(pUnary->m_operator));
            }
            break;
        }
        case Ast::NodeType::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;

            TypeCheckExpression(state, pCall->m_pCallee);

            Ast::Variable* pVar = (Ast::Variable*)pCall->m_pCallee;
            Declaration* pDeclEntry = state.m_declarations.Get(pVar->m_identifier);

            if (pCall->m_pCallee->m_valueType != ValueType::Function) {
                state.m_pErrors->PushError(pCall, "Attempt to call a value which is not a function");
            }

            for (Ast::Expression* pArg : pCall->m_args) {
                TypeCheckExpression(state, pArg);
            }

            // TODO: This is kind of temporary and hacky. The type of the declaration should be a complex function type, that you can use
            // to typecheck the function call, no need to access the initializer.
            Ast::Declaration* pDecl = (Ast::Declaration*)pDeclEntry->pNode;
            Ast::Function* pFunc = (Ast::Function*)pDecl->m_pInitializerExpr;

            int argsCount = pCall->m_args.m_count;
            int paramsCount = pFunc->m_params.m_count;
            if (pCall->m_args.m_count != pFunc->m_params.m_count) {
                state.m_pErrors->PushError(pCall, "Mismatched number of arguments in call to function '%s', expected %i, got %i", pFunc->m_identifier.m_pData, paramsCount, argsCount);
            }

            int minArgs = argsCount > paramsCount ? paramsCount : argsCount;
            for (size_t i = 0; i < minArgs; i++) {
                Ast::Expression* arg = pCall->m_args[i];
                Ast::Function::Param& param = pFunc->m_params[i];
                if (param.m_pType) {
                    if (arg->m_valueType != param.m_pType->m_resolvedType)
                        state.m_pErrors->PushError(arg, "Type mismatch in function argument '%s', expected %s, got %s", param.identifier.m_pData, ValueType::ToString(param.m_pType->m_resolvedType), ValueType::ToString(arg->m_valueType));
                }
            }

            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void TypeCheckStatement(TypeCheckerState& state, Ast::Statement* pStmt) {
    switch (pStmt->m_type) {
        case Ast::NodeType::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;
            pDecl->m_scopeLevel = state.m_currentScopeLevel;

            if (state.m_declarations.Get(pDecl->m_identifier) != nullptr)
                state.m_pErrors->PushError(pDecl, "Redefinition of variable '%s'", pDecl->m_identifier.m_pData);

            Declaration dec;
            dec.pNode = pDecl;

            if (pDecl->m_pInitializerExpr) {
                if (pDecl->m_pInitializerExpr->m_type == Ast::NodeType::Function) {
                    // Allows recursion
                    pDecl->m_resolvedType = ValueType::Function;
                    dec.initialized = true;
                }
                Declaration& added = state.m_declarations.Add(pDecl->m_identifier, dec);
                TypeCheckExpression(state, pDecl->m_pInitializerExpr);
                added.initialized = true;

                if (pDecl->m_pDeclaredType && pDecl->m_pInitializerExpr->m_valueType != pDecl->m_pDeclaredType->m_resolvedType) {
                    ValueType::Enum declaredType = pDecl->m_pDeclaredType->m_resolvedType;
                    ValueType::Enum initType = pDecl->m_pInitializerExpr->m_valueType;
                    state.m_pErrors->PushError(pDecl->m_pDeclaredType, "Type mismatch in declaration, declared as %s and initialized as %s", ValueType::ToString(declaredType), ValueType::ToString(initType));
                } else {
                    pDecl->m_resolvedType = pDecl->m_pInitializerExpr->m_valueType;
                }
            } else {
                state.m_declarations.Add(pDecl->m_identifier, dec);
                pDecl->m_resolvedType = pDecl->m_pDeclaredType->m_resolvedType;
            }
            break;
        }
        case Ast::NodeType::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            TypeCheckExpression(state, pPrint->m_pExpr);
            break;
        }
        case Ast::NodeType::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            TypeCheckExpression(state, pExprStmt->m_pExpr);
            break;
        }
        case Ast::NodeType::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            TypeCheckExpression(state, pIf->m_pCondition);
            if (pIf->m_pCondition->m_valueType != ValueType::Bool)
                state.m_pErrors->PushError(pIf->m_pCondition, "if conditional expression does not evaluate to a boolean");

            TypeCheckStatement(state, pIf->m_pThenStmt);

            if (pIf->m_pElseStmt)
                TypeCheckStatement(state, pIf->m_pElseStmt);
            break;
        }
        case Ast::NodeType::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            TypeCheckExpression(state, pWhile->m_pCondition);
            if (pWhile->m_pCondition->m_valueType != ValueType::Bool)
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
                    Ast::Declaration* pVarDecl = state.m_declarations.m_pTable[i].value.pNode;

                    if (pVarDecl->m_scopeLevel > state.m_currentScopeLevel)
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

void TypeCheckProgram(ResizableArray<Ast::Statement*>& program, ErrorState* pErrors) {
    TypeCheckerState state;

    state.m_pErrors = pErrors;

    TypeCheckStatements(state, program);
}