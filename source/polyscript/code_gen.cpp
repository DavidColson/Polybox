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
    Function* m_pFunc;
};
}

// ***********************************************************************

CodeChunk* CurrentChunk(State& state) {
    return &state.m_pFunc->m_chunk;
}

// ***********************************************************************

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

void PushCode(State& state, uint8_t code, uint32_t line) {
    CurrentChunk(state)->code.PushBack(code);
    CurrentChunk(state)->m_lineInfo.PushBack(line);
    if (line == 0)
        __debugbreak();
}

// ***********************************************************************

int PushJump(State& state, OpCode::Enum jumpCode, uint32_t m_line) {
    PushCode(state, jumpCode, m_line);
    PushCode(state, 0xff, m_line);
    PushCode(state, 0xff, m_line);
    return CurrentChunk(state)->code.m_count - 2;
}

// ***********************************************************************

void PushLoop(State& state, uint8_t loopTarget, uint32_t m_line) {
    PushCode(state, OpCode::Loop, m_line);

    int offset = CurrentChunk(state)->code.m_count - loopTarget + 2;
    PushCode(state, (offset >> 8) & 0xff, m_line);
    PushCode(state, offset & 0xff, m_line);
}

// ***********************************************************************

void PatchJump(State& state, int jumpCodeLocation) {
    int jumpOffset = CurrentChunk(state)->code.m_count - jumpCodeLocation - 2;
    CurrentChunk(state)->code[jumpCodeLocation] = (jumpOffset >> 8) & 0xff;
    CurrentChunk(state)->code[jumpCodeLocation + 1] = jumpOffset & 0xff;
}

// ***********************************************************************

void CodeGenExpression(State& state, Ast::Expression* pExpr) {
    switch (pExpr->m_type) {
        case Ast::NodeType::Variable: {
            Ast::Variable* pVariable = (Ast::Variable*)pExpr;
            int localIndex = ResolveLocal(state, pVariable->m_identifier);
            if (localIndex != -1) {
                PushCode(state, OpCode::GetLocal, pVariable->m_line);
                PushCode(state, localIndex, pVariable->m_line);
            }
            break;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            CodeGenExpression(state, pVarAssignment->m_pAssignment);
            int localIndex = ResolveLocal(state, pVarAssignment->m_identifier);
            if (localIndex != -1) {
                PushCode(state, OpCode::SetLocal, pVarAssignment->m_line);
                PushCode(state, localIndex, pVarAssignment->m_line);
            }
            break;
        }
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;

            CurrentChunk(state)->constants.PushBack(pLiteral->m_value);
            uint8_t constIndex = (uint8_t)CurrentChunk(state)->constants.m_count - 1;

            PushCode(state, OpCode::LoadConstant, pLiteral->m_line);
            PushCode(state, constIndex, pLiteral->m_line);
            break;
        }
        case Ast::NodeType::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            Function* pFunc = CodeGen(pFunction->m_pBody->m_declarations, pFunction->m_identifier, state.m_pErrors);

            Value value;
            value.m_type = ValueType::Function;
            value.m_pFunction = pFunc;
            CurrentChunk(state)->constants.PushBack(value);
            uint8_t constIndex = (uint8_t)CurrentChunk(state)->constants.m_count - 1;

            PushCode(state, OpCode::LoadConstant, pFunction->m_line);
            PushCode(state, constIndex, pFunction->m_line);
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;

            CodeGenExpression(state, pGroup->m_pExpression);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;

            if (pBinary->m_operator == Operator::And) {
                CodeGenExpression(state, pBinary->m_pLeft);
                int andJump = PushJump(state, OpCode::JmpIfFalse, pBinary->m_line);
                PushCode(state, OpCode::Pop, pBinary->m_line);
                CodeGenExpression(state, pBinary->m_pRight);
                PatchJump(state, andJump);
                break;
            }

            if (pBinary->m_operator == Operator::Or) {
                CodeGenExpression(state, pBinary->m_pLeft);
                int andJump = PushJump(state, OpCode::JmpIfTrue, pBinary->m_line);
                PushCode(state, OpCode::Pop, pBinary->m_line);
                CodeGenExpression(state, pBinary->m_pRight);
                PatchJump(state, andJump);
                break;
            }

            CodeGenExpression(state, pBinary->m_pLeft);
            CodeGenExpression(state, pBinary->m_pRight);
            switch (pBinary->m_operator) {
                case Operator::Add:
                    PushCode(state, OpCode::Add, pBinary->m_line);
                    break;
                case Operator::Subtract:
                    PushCode(state, OpCode::Subtract, pBinary->m_line);
                    break;
                case Operator::Divide:
                    PushCode(state, OpCode::Divide, pBinary->m_line);
                    break;
                case Operator::Multiply:
                    PushCode(state, OpCode::Multiply, pBinary->m_line);
                    break;
                case Operator::Greater:
                    PushCode(state, OpCode::Greater, pBinary->m_line);
                    break;
                case Operator::Less:
                    PushCode(state, OpCode::Less, pBinary->m_line);
                    break;
                case Operator::GreaterEqual:
                    PushCode(state, OpCode::GreaterEqual, pBinary->m_line);
                    break;
                case Operator::LessEqual:
                    PushCode(state, OpCode::LessEqual, pBinary->m_line);
                    break;
                case Operator::Equal:
                    PushCode(state, OpCode::Equal, pBinary->m_line);
                    break;
                case Operator::NotEqual:
                    PushCode(state, OpCode::NotEqual, pBinary->m_line);
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
                    PushCode(state, OpCode::Negate, pUnary->m_line);
                    break;
                case Operator::Not:
                    PushCode(state, OpCode::Not, pUnary->m_line);
                    break;
                default:
                    break;
            }
            break;
        }
        case Ast::NodeType::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;
            CodeGenExpression(state, pCall->m_pCallee);

            // TODO: Codegen the arg list to leave values on the stack in the right order

            PushCode(state, OpCode::Call, pCall->m_line);
            PushCode(state, 0, pCall->m_line);  // arg count
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void CodeGenStatements(State& state, ResizableArray<Ast::Statement*>& statements);

void CodeGenStatement(State& state, Ast::Statement* pStmt) {
    switch (pStmt->m_type) {
        case Ast::NodeType::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;

            Local local;
            local.m_depth = state.m_currentScopeDepth;
            local.m_name = pDecl->m_identifier;
            state.m_locals.PushBack(local);

            CodeGenExpression(state, pDecl->m_pInitializerExpr);
            break;
        }
        case Ast::NodeType::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            CodeGenExpression(state, pPrint->m_pExpr);
            PushCode(state, OpCode::Print, pPrint->m_line);
            break;
        }
        case Ast::NodeType::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            CodeGenExpression(state, pExprStmt->m_pExpr);
            // Special case for now, functions with void return type probably should push a nill type or something?
            if (pExprStmt->m_pExpr->m_type != Ast::NodeType::Call)
                PushCode(state, OpCode::Pop, pExprStmt->m_line);
            break;
        }
        case Ast::NodeType::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            CodeGenExpression(state, pIf->m_pCondition);

            int ifJump = PushJump(state, OpCode::JmpIfFalse, pIf->m_line);
            PushCode(state, OpCode::Pop, pIf->m_pThenStmt->m_line);

            CodeGenStatement(state, pIf->m_pThenStmt);
            int elseJump = PushJump(state, OpCode::Jmp, pIf->m_pElseStmt->m_line);
            PatchJump(state, ifJump);

            PushCode(state, OpCode::Pop, pIf->m_pElseStmt->m_line);
            if (pIf->m_pElseStmt) {
                CodeGenStatement(state, pIf->m_pElseStmt);
            }
            PatchJump(state, elseJump);

            break;
        }
        case Ast::NodeType::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            int loopStart = CurrentChunk(state)->code.m_count;
            CodeGenExpression(state, pWhile->m_pCondition);

            int ifJump = PushJump(state, OpCode::JmpIfFalse, pWhile->m_line);
            PushCode(state, OpCode::Pop, pWhile->m_line);

            CodeGenStatement(state, pWhile->m_pBody);
            PushLoop(state, loopStart, pWhile->m_pBody->m_line);

            PatchJump(state, ifJump);
            PushCode(state, OpCode::Pop, pWhile->m_pBody->m_line);
            break;
        }
        case Ast::NodeType::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            state.m_currentScopeDepth++;
            CodeGenStatements(state, pBlock->m_declarations);
            state.m_currentScopeDepth--;
            while (state.m_locals.m_count > 0 && state.m_locals[state.m_locals.m_count - 1].m_depth > state.m_currentScopeDepth) {
                state.m_locals.PopBack();
                PushCode(state, OpCode::Pop, pBlock->m_endToken.m_line);
            }
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void CodeGenStatements(State& state, ResizableArray<Ast::Statement*>& statements) {
    for (size_t i = 0; i < statements.m_count; i++) {
        Ast::Statement* pStmt = statements[i];
        CodeGenStatement(state, pStmt);
    }
}

// ***********************************************************************

Function* CodeGen(ResizableArray<Ast::Statement*>& program, String name, ErrorState* pErrorState) {
    State state;

    Local local;
    local.m_depth = 0;
    local.m_name = name;
    state.m_locals.PushBack(local); // This is the local representing this function, allows it to call itself

    state.m_pFunc = (Function*)g_Allocator.Allocate(sizeof(Function));
    SYS_P_NEW(state.m_pFunc) Function();
    state.m_pFunc->m_name = name;

    CodeGenStatements(state, program);
    Ast::Statement* pEnd = program[program.m_count - 1];
    PushCode(state, OpCode::Return, pEnd->m_line);
    return state.m_pFunc;
}