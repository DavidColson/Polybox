// Copyright 2020-2022 David Colson. All rights reserved.

#include "type_checker.h"

#include "parser.h"

// ***********************************************************************

void TypeCheckExpression(Ast::Expression* pExpr, ParsingState* pParser) {
    if (pExpr == nullptr)
        return;

    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            pLiteral->m_valueType = pLiteral->m_value.m_type;
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            TypeCheckExpression(pGroup->m_pExpression, pParser);
            pGroup->m_valueType = pGroup->m_pExpression->m_valueType;
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            TypeCheckExpression(pBinary->m_pLeft, pParser);
            TypeCheckExpression(pBinary->m_pRight, pParser);
            pBinary->m_valueType = OperatorReturnType(pBinary->m_operator, pBinary->m_pLeft->m_valueType, pBinary->m_pRight->m_valueType);

            if (pBinary->m_valueType == ValueType::Invalid && pBinary->m_pLeft->m_valueType != ValueType::Invalid && pBinary->m_pRight->m_valueType != ValueType::Invalid) {
                String str1 = ValueType::ToString(pBinary->m_pLeft->m_valueType);
                String str2 = ValueType::ToString(pBinary->m_pRight->m_valueType);
                String str3 = Operator::ToString(pBinary->m_operator);
                pParser->PushError(pBinary, "Invalid types (%s, %s) used with operator \"%s\"", str1.m_pData, str2.m_pData, str3.m_pData);
            }
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            TypeCheckExpression(pUnary->m_pRight, pParser);
            pUnary->m_valueType = OperatorReturnType(pUnary->m_operator, pUnary->m_pRight->m_valueType, ValueType::Invalid);

            if (pUnary->m_valueType == ValueType::Invalid && pUnary->m_pRight->m_valueType != ValueType::Invalid) {
                pParser->PushError(pUnary, "Invalid type (%s) used with operator \"%s\"", ValueType::ToString(pUnary->m_pRight->m_valueType), Operator::ToString(pUnary->m_operator));
            }
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void TypeCheckProgram(ResizableArray<Ast::Statement*>& program, ParsingState* pParser) {
    for (size_t i = 0; i < program.m_count; i++) {
        Ast::Statement* pStmt = program[i];

        switch (pStmt->m_type) {
            case Ast::NodeType::PrintStmt: {
                Ast::PrintStatement* pPrint = (Ast::PrintStatement*)pStmt;
                TypeCheckExpression(pPrint->m_pExpr, pParser);
                break;
            }
            case Ast::NodeType::ExpressionStmt: {
                Ast::ExpressionStatement* pExprStmt = (Ast::ExpressionStatement*)pStmt;
                TypeCheckExpression(pExprStmt->m_pExpr, pParser);
                break;
            }
            default:
                break;
        }
    }
}