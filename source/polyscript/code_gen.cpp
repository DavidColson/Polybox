// Copyright 2020-2022 David Colson. All rights reserved.

#include "code_gen.h"

#include "parser.h"
#include "virtual_machine.h"

#include <resizable_array.inl>
#include <hashmap.inl>

namespace {
struct Local {
    String m_name;
    int m_depth;
};

struct State {
    ResizableArray<Local> m_locals;
    int m_currentScopeDepth { 0 };
    ErrorState* m_pErrors;
    CodeChunk m_chunk;
};
}

int ResolveLocal(State& state, String name) {
    for (int i = state.m_locals.m_count - 1; i >= 0; i--) {
        Local& local = state.m_locals[i];
        if (name == local.m_name) {
            return i;
        }
    }
    return -1;
}

// ***********************************************************************

void CodeGenExpression(State& state, Ast::Expression* pExpr) {
    switch (pExpr->m_type) {
        case Ast::NodeType::Variable: {
            Ast::Variable* pVariable = (Ast::Variable*)pExpr;
            int localIndex = ResolveLocal(state, pVariable->m_identifier);
            if (localIndex != -1) {
                state.m_chunk.code.PushBack((uint8_t)OpCode::GetLocal);
                state.m_chunk.code.PushBack(localIndex);
            }
            break;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            CodeGenExpression(state, pVarAssignment->m_pAssignment);
            int localIndex = ResolveLocal(state, pVarAssignment->m_identifier);
            if (localIndex != -1) {
                state.m_chunk.code.PushBack((uint8_t)OpCode::SetLocal);
                state.m_chunk.code.PushBack(localIndex);
            }
            break;
        }
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;

            state.m_chunk.constants.PushBack(pLiteral->m_value);
            uint8_t constIndex = (uint8_t)state.m_chunk.constants.m_count - 1;

            state.m_chunk.code.PushBack((uint8_t)OpCode::LoadConstant);
            state.m_chunk.code.PushBack(constIndex);
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;

            CodeGenExpression(state, pGroup->m_pExpression);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            CodeGenExpression(state, pBinary->m_pLeft);
            CodeGenExpression(state, pBinary->m_pRight);
            switch (pBinary->m_operator) {
                case Operator::Add:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Add);
                    break;
                case Operator::Subtract:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Subtract);
                    break;
                case Operator::Divide:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Divide);
                    break;
                case Operator::Multiply:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Multiply);
                    break;
                case Operator::Greater:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Greater);
                    break;
                case Operator::Less:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Less);
                    break;
                case Operator::GreaterEqual:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::GreaterEqual);
                    break;
                case Operator::LessEqual:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::LessEqual);
                    break;
                case Operator::Equal:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Equal);
                    break;
                case Operator::NotEqual:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::NotEqual);
                    break;
                case Operator::And:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::And);
                    break;
                case Operator::Or:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Or);
                    break;
                default:
                    break;
            }
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            CodeGenExpression(state, pUnary->m_pRight);
            switch (pUnary->m_operator) {
                case Operator::Subtract:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Negate);
                    break;
                case Operator::Not:
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Not);
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

void CodeGenStatements(State& state, ResizableArray<Ast::Statement*>& program) {
    for (size_t i = 0; i < program.m_count; i++) {
        Ast::Statement* pStmt = program[i];

        switch (pStmt->m_type) {
            case Ast::NodeType::VarDecl: {
                Ast::VariableDeclaration* pVarDecl = (Ast::VariableDeclaration*)pStmt;

                Local local;
                local.m_depth = state.m_currentScopeDepth;
                local.m_name = pVarDecl->m_identifier;
                state.m_locals.PushBack(local);
                
                CodeGenExpression(state, pVarDecl->m_pInitializerExpr);
                break;
            }
            case Ast::NodeType::PrintStmt: {
                Ast::PrintStatement* pPrint = (Ast::PrintStatement*)pStmt;
                CodeGenExpression(state, pPrint->m_pExpr);
                state.m_chunk.code.PushBack((uint8_t)OpCode::Print);
                break;
            }
            case Ast::NodeType::ExpressionStmt: {
                Ast::ExpressionStatement* pExprStmt = (Ast::ExpressionStatement*)pStmt;
                CodeGenExpression(state, pExprStmt->m_pExpr);
                state.m_chunk.code.PushBack((uint8_t)OpCode::Pop);
                break;
            }
            case Ast::NodeType::Block: {
                Ast::Block* pBlock = (Ast::Block*)pStmt;
                state.m_currentScopeDepth++;
                CodeGenStatements(state, pBlock->m_declarations);
                state.m_currentScopeDepth--;
                while (state.m_locals.m_count > 0 && state.m_locals[state.m_locals.m_count - 1].m_depth > state.m_currentScopeDepth) {
                    state.m_locals.PopBack();
                    state.m_chunk.code.PushBack((uint8_t)OpCode::Pop);
                }
                break;
            }
            default:
                break;
        }
    }
}

// ***********************************************************************

CodeChunk CodeGen(ResizableArray<Ast::Statement*>& program, ErrorState* pErrorState) {
    State state;

    CodeGenStatements(state, program);
    return state.m_chunk;
}