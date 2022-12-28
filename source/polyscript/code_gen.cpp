// Copyright 2020-2022 David Colson. All rights reserved.

#include "code_gen.h"

#include "parser.h"

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
            switch (pBinary->m_operator.m_type) {
                case TokenType::Plus:
                    pChunk->code.PushBack((uint8_t)OpCode::Add);
                    break;
                case TokenType::Minus:
                    pChunk->code.PushBack((uint8_t)OpCode::Subtract);
                    break;
                case TokenType::Slash:
                    pChunk->code.PushBack((uint8_t)OpCode::Divide);
                    break;
                case TokenType::Star:
                    pChunk->code.PushBack((uint8_t)OpCode::Multiply);
                    break;
                case TokenType::Greater:
                    pChunk->code.PushBack((uint8_t)OpCode::Greater);
                    break;
                case TokenType::Less:
                    pChunk->code.PushBack((uint8_t)OpCode::Less);
                    break;
                case TokenType::GreaterEqual:
                    pChunk->code.PushBack((uint8_t)OpCode::Less);
                    pChunk->code.PushBack((uint8_t)OpCode::Not);
                    break;
                case TokenType::LessEqual:
                    pChunk->code.PushBack((uint8_t)OpCode::Greater);
                    pChunk->code.PushBack((uint8_t)OpCode::Not);
                    break;
                case TokenType::EqualEqual:
                    pChunk->code.PushBack((uint8_t)OpCode::Equal);
                    break;
                case TokenType::BangEqual:
                    pChunk->code.PushBack((uint8_t)OpCode::Equal);
                    pChunk->code.PushBack((uint8_t)OpCode::Not);
                    break;
                case TokenType::And:
                    pChunk->code.PushBack((uint8_t)OpCode::And);
                    break;
                case TokenType::Or:
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
            switch (pUnary->m_operator.m_type) {
                case TokenType::Minus:
                    pChunk->code.PushBack((uint8_t)OpCode::Negate);
                    break;
                case TokenType::Bang:
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