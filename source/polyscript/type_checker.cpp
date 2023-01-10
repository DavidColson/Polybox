// Copyright 2020-2022 David Colson. All rights reserved.

#include "type_checker.h"

#include "parser.h"

#include <hashmap.inl>

struct TypeCheckerState {
    HashMap<String, Ast::VariableDeclaration*> m_variableDeclarations;
};

// ***********************************************************************

void TypeCheckExpression(TypeCheckerState* pState, Ast::Expression* pExpr, ErrorState* pErrors) {
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

            Ast::VariableDeclaration** pDecl = pState->m_variableDeclarations.Get(pVariable->m_identifier);
            if (pDecl) {
                pVariable->m_valueType = (*pDecl)->m_pInitializerExpr->m_valueType;
            } else {
                pErrors->PushError(pVariable, "Undeclared variable \'%s\', missing a declaration somewhere before?", pVariable->m_identifier.m_pData);
            }

            break;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            TypeCheckExpression(pState, pVarAssignment->m_pAssignment, pErrors);

            Ast::VariableDeclaration** pDecl = pState->m_variableDeclarations.Get(pVarAssignment->m_identifier);
            if (pDecl) {
                ValueType::Enum declaredVarType = (*pDecl)->m_pInitializerExpr->m_valueType;
                ValueType::Enum assignedVarType = pVarAssignment->m_pAssignment->m_valueType;
                if (declaredVarType == assignedVarType) {
                    pVarAssignment->m_valueType = declaredVarType;
                }
                else {
                    pErrors->PushError(pVarAssignment, "Type mismatch on assignment, \'%s\' has type %s, but is being assigned a value with type %s", pVarAssignment->m_identifier.m_pData, ValueType::ToString(declaredVarType), ValueType::ToString(assignedVarType));
                }
            } else {
                pErrors->PushError(pVarAssignment, "Undeclared variable \'%s\', missing a declaration somewhere before?", pVarAssignment->m_identifier.m_pData);
            }

            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            TypeCheckExpression(pState, pGroup->m_pExpression, pErrors);
            pGroup->m_valueType = pGroup->m_pExpression->m_valueType;
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            TypeCheckExpression(pState, pBinary->m_pLeft, pErrors);
            TypeCheckExpression(pState, pBinary->m_pRight, pErrors);
            pBinary->m_valueType = OperatorReturnType(pBinary->m_operator, pBinary->m_pLeft->m_valueType, pBinary->m_pRight->m_valueType);

            if (pBinary->m_valueType == ValueType::Invalid && pBinary->m_pLeft->m_valueType != ValueType::Invalid && pBinary->m_pRight->m_valueType != ValueType::Invalid) {
                String str1 = ValueType::ToString(pBinary->m_pLeft->m_valueType);
                String str2 = ValueType::ToString(pBinary->m_pRight->m_valueType);
                String str3 = Operator::ToString(pBinary->m_operator);
                pErrors->PushError(pBinary, "Invalid types (%s, %s) used with operator \"%s\"", str1.m_pData, str2.m_pData, str3.m_pData);
            }
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            TypeCheckExpression(pState, pUnary->m_pRight, pErrors);
            pUnary->m_valueType = OperatorReturnType(pUnary->m_operator, pUnary->m_pRight->m_valueType, ValueType::Invalid);

            if (pUnary->m_valueType == ValueType::Invalid && pUnary->m_pRight->m_valueType != ValueType::Invalid) {
                pErrors->PushError(pUnary, "Invalid type (%s) used with operator \"%s\"", ValueType::ToString(pUnary->m_pRight->m_valueType), Operator::ToString(pUnary->m_operator));
            }
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void TypeCheckProgram(ResizableArray<Ast::Statement*>& program, ErrorState* pErrors) {
    TypeCheckerState state;

    for (size_t i = 0; i < program.m_count; i++) {
        Ast::Statement* pStmt = program[i];

        switch (pStmt->m_type) {
            case Ast::NodeType::VarDecl: {
                Ast::VariableDeclaration* pVarDecl = (Ast::VariableDeclaration*)pStmt;
                state.m_variableDeclarations.Add(pVarDecl->m_identifier, pVarDecl);
                TypeCheckExpression(&state, pVarDecl->m_pInitializerExpr, pErrors);
                break;
            }
            case Ast::NodeType::PrintStmt: {
                Ast::PrintStatement* pPrint = (Ast::PrintStatement*)pStmt;
                TypeCheckExpression(&state, pPrint->m_pExpr, pErrors);
                break;
            }
            case Ast::NodeType::ExpressionStmt: {
                Ast::ExpressionStatement* pExprStmt = (Ast::ExpressionStatement*)pStmt;
                TypeCheckExpression(&state, pExprStmt->m_pExpr, pErrors);
                break;
            }
            case Ast::NodeType::Block: {
                Ast::Block* pBlock = (Ast::Block*)pStmt;
                TypeCheckProgram(pBlock->m_declarations, pErrors);
                break;
            }
            default:
                break;
        }
    }
}