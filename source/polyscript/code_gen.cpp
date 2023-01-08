// Copyright 2020-2022 David Colson. All rights reserved.

#include "code_gen.h"

#include "parser.h"
#include "virtual_machine.h"

#include <resizable_array.inl>
#include <hashmap.inl>

// ***********************************************************************

void CodeGenExpression(Ast::Expression* pExpr, CodeChunk* pChunk, ErrorState* pErrors) {
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

            CodeGenExpression(pGroup->m_pExpression, pChunk, pErrors);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            CodeGenExpression(pBinary->m_pLeft, pChunk, pErrors);
            CodeGenExpression(pBinary->m_pRight, pChunk, pErrors);
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
            CodeGenExpression(pUnary->m_pRight, pChunk, pErrors);
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

// ***********************************************************************

void CodeGen(ResizableArray<Ast::Statement*>& program, CodeChunk* pChunk, ErrorState* pErrors) {
    struct Global {
        int m_index;
    };
    HashMap<String, Global> globalLookup;

    uint8_t globalIndexCounter = 0;

    for (size_t i = 0; i < program.m_count; i++) {
        Ast::Statement* pStmt = program[i];

        switch (pStmt->m_type) {
            case Ast::NodeType::VarDecl: {
                Ast::VariableDeclaration* pVarDecl = (Ast::VariableDeclaration*)pStmt;

                // Put global in our lookup map
                Global newGlobal;
                newGlobal.m_index = globalIndexCounter;
                globalIndexCounter++;
                globalLookup.Add(pVarDecl->m_name, newGlobal);
                
                // codegen the initializer which will leave it's output on the stack
                CodeGenExpression(pVarDecl->m_pInitializerExpr, pChunk, pErrors);
                pChunk->code.PushBack((uint8_t)OpCode::SetGlobal);
                pChunk->code.PushBack(newGlobal.m_index);
                break;
            }
            case Ast::NodeType::PrintStmt: {
                Ast::PrintStatement* pPrint = (Ast::PrintStatement*)pStmt;
                CodeGenExpression(pPrint->m_pExpr, pChunk, pErrors);
                pChunk->code.PushBack((uint8_t)OpCode::Print);
                break;
            }
            case Ast::NodeType::ExpressionStmt: {
                Ast::ExpressionStatement* pExprStmt = (Ast::ExpressionStatement*)pStmt;
                CodeGenExpression(pExprStmt->m_pExpr, pChunk, pErrors);
                pChunk->code.PushBack((uint8_t)OpCode::Pop);
                break;
            }
            default:
                break;
        }
    }

    pChunk->m_globalsCount = globalIndexCounter;
}