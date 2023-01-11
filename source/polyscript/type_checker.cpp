// Copyright 2020-2022 David Colson. All rights reserved.

#include "type_checker.h"

#include "parser.h"

#include <hashmap.inl>

struct TypeCheckerState {
    HashMap<String, Ast::VariableDeclaration*> m_variableDeclarations;
    ErrorState* m_pErrors { nullptr };
    int m_currentScopeLevel { 0 };
};

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
        case Ast::NodeType::Variable: {
            Ast::Variable* pVariable = (Ast::Variable*)pExpr;

            Ast::VariableDeclaration** pDecl = state.m_variableDeclarations.Get(pVariable->m_identifier);
            if (pDecl) {
                pVariable->m_valueType = (*pDecl)->m_pInitializerExpr->m_valueType;
            } else {
                state.m_pErrors->PushError(pVariable, "Undeclared variable \'%s\', missing a declaration somewhere before?", pVariable->m_identifier.m_pData);
            }

            break;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            TypeCheckExpression(state, pVarAssignment->m_pAssignment);

            Ast::VariableDeclaration** pDecl = state.m_variableDeclarations.Get(pVarAssignment->m_identifier);
            if (pDecl) {
                ValueType::Enum declaredVarType = (*pDecl)->m_pInitializerExpr->m_valueType;
                ValueType::Enum assignedVarType = pVarAssignment->m_pAssignment->m_valueType;
                if (declaredVarType == assignedVarType) {
                    pVarAssignment->m_valueType = declaredVarType;
                }
                else {
                    state.m_pErrors->PushError(pVarAssignment, "Type mismatch on assignment, \'%s\' has type %s, but is being assigned a value with type %s", pVarAssignment->m_identifier.m_pData, ValueType::ToString(declaredVarType), ValueType::ToString(assignedVarType));
                }
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

            if (pBinary->m_valueType == ValueType::Invalid && pBinary->m_pLeft->m_valueType != ValueType::Invalid && pBinary->m_pRight->m_valueType != ValueType::Invalid) {
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
            pUnary->m_valueType = OperatorReturnType(pUnary->m_operator, pUnary->m_pRight->m_valueType, ValueType::Invalid);

            if (pUnary->m_valueType == ValueType::Invalid && pUnary->m_pRight->m_valueType != ValueType::Invalid) {
                state.m_pErrors->PushError(pUnary, "Invalid type (%s) used with operator \"%s\"", ValueType::ToString(pUnary->m_pRight->m_valueType), Operator::ToString(pUnary->m_operator));
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

        switch (pStmt->m_type) {
            case Ast::NodeType::VarDecl: {
                Ast::VariableDeclaration* pVarDecl = (Ast::VariableDeclaration*)pStmt;
                pVarDecl->m_scopeLevel = state.m_currentScopeLevel;

                if (state.m_variableDeclarations.Get(pVarDecl->m_identifier) != nullptr)
                    state.m_pErrors->PushError(pVarDecl, "Redefinition of variable '%s'", pVarDecl->m_identifier.m_pData);

                TypeCheckExpression(state, pVarDecl->m_pInitializerExpr);
                state.m_variableDeclarations.Add(pVarDecl->m_identifier, pVarDecl);
                break;
            }
            case Ast::NodeType::PrintStmt: {
                Ast::PrintStatement* pPrint = (Ast::PrintStatement*)pStmt;
                TypeCheckExpression(state, pPrint->m_pExpr);
                break;
            }
            case Ast::NodeType::ExpressionStmt: {
                Ast::ExpressionStatement* pExprStmt = (Ast::ExpressionStatement*)pStmt;
                TypeCheckExpression(state, pExprStmt->m_pExpr);
                break;
            }
            case Ast::NodeType::Block: {
                Ast::Block* pBlock = (Ast::Block*)pStmt;

                state.m_currentScopeLevel++;
                TypeCheckStatements(state, pBlock->m_declarations);
                state.m_currentScopeLevel--;

                // Remove variable declarations that are now out of scope
                for (size_t i = 0; i < state.m_variableDeclarations.m_tableSize; i++) {
                    if (state.m_variableDeclarations.m_pTable[i].hash != UNUSED_HASH) {
                        Ast::VariableDeclaration* pVarDecl = state.m_variableDeclarations.m_pTable[i].value;

                        if (pVarDecl->m_scopeLevel > state.m_currentScopeLevel)
                            state.m_variableDeclarations.Erase(state.m_variableDeclarations.m_pTable[i].key);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

// ***********************************************************************

void TypeCheckProgram(ResizableArray<Ast::Statement*>& program, ErrorState* pErrors) {
    TypeCheckerState state;

    state.m_pErrors = pErrors;

    TypeCheckStatements(state, program);
}