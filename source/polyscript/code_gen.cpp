// Copyright 2020-2022 David Colson. All rights reserved.

#include "code_gen.h"

#include "parser.h"
#include "virtual_machine.h"

// ***********************************************************************

void CodeGen(Ast::Expression* pExpr, CodeChunk* pChunk) {
    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;

            pChunk->constants.PushBack(pLiteral->m_value);
            uint8_t constIndex = (uint8_t)pChunk->constants.m_count - 1;

            pChunk->code.PushBack((uint8_t)OpCode::LoadConstant);
            pChunk->code.PushBack(constIndex);
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;

            CodeGen(pGroup->m_pExpression, pChunk);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            CodeGen(pBinary->m_pLeft, pChunk);
            CodeGen(pBinary->m_pRight, pChunk);
            switch (pBinary->m_operator) {
                case Operator::Add:
                    pChunk->code.PushBack((uint8_t)OpCode::Add);
                    break;
                case Operator::Subtract:
                    pChunk->code.PushBack((uint8_t)OpCode::Subtract);
                    break;
                case Operator::Divide:
                    pChunk->code.PushBack((uint8_t)OpCode::Divide);
                    break;
                case Operator::Multiply:
                    pChunk->code.PushBack((uint8_t)OpCode::Multiply);
                    break;
                case Operator::Greater:
                    pChunk->code.PushBack((uint8_t)OpCode::Greater);
                    break;
                case Operator::Less:
                    pChunk->code.PushBack((uint8_t)OpCode::Less);
                    break;
                case Operator::GreaterEqual:
                    pChunk->code.PushBack((uint8_t)OpCode::GreaterEqual);
                    break;
                case Operator::LessEqual:
                    pChunk->code.PushBack((uint8_t)OpCode::LessEqual);
                    break;
                case Operator::Equal:
                    pChunk->code.PushBack((uint8_t)OpCode::Equal);
                    break;
                case Operator::NotEqual:
                    pChunk->code.PushBack((uint8_t)OpCode::NotEqual);
                    break;
                case Operator::And:
                    pChunk->code.PushBack((uint8_t)OpCode::And);
                    break;
                case Operator::Or:
                    pChunk->code.PushBack((uint8_t)OpCode::Or);
                    break;
                default:
                    break;
            }
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            CodeGen(pUnary->m_pRight, pChunk);
            switch (pUnary->m_operator) {
                case Operator::Subtract:
                    pChunk->code.PushBack((uint8_t)OpCode::Negate);
                    break;
                case Operator::Not:
                    pChunk->code.PushBack((uint8_t)OpCode::Not);
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}