// Copyright 2020-2022 David Colson. All rights reserved.

#include <log.h>
#include <resizable_array.inl>
#include <defer.h>
#include <stdint.h>
#include <string_builder.h>
#include <light_string.h>
#include <scanning.h>
#include <linear_allocator.h>

#include "parser.h"
#include "virtual_machine.h"
#include "lexer.h"
#include "code_gen.h"


void TypeCheck(Ast::Expression* pExpr) {
    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            pLiteral->m_valueType = pLiteral->m_value.m_type;
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            TypeCheck(pGroup->m_pExpression);
            pGroup->m_valueType = pGroup->m_pExpression->m_valueType;
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            TypeCheck(pBinary->m_pLeft);
            TypeCheck(pBinary->m_pRight);
            pBinary->m_valueType = OperatorReturnType(pBinary->m_operator, pBinary->m_pLeft->m_valueType, pBinary->m_pRight->m_valueType);
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            TypeCheck(pUnary->m_pRight);
            pUnary->m_valueType = OperatorReturnType(pUnary->m_operator, pUnary->m_pRight->m_valueType, ValueType::Invalid);
            break;
        }
        default:
            break;
    }
}


int main() {
    InitValueTables();

    String actualCode;
    actualCode = "( 5 - (12+false) ) * 12 / 3";
    //actualCode = "5<3 && 12.4 >= 14 || 9<15";

    ResizableArray<Token> tokens = Tokenize(&g_Allocator, actualCode);
    defer(tokens.Free());

    LinearAllocator astMemory;
    Ast::Expression* pAst = ParseToAst(tokens, &astMemory);

    TypeCheck(pAst);

    Log::Debug("---- AST -----");
    DebugAst(pAst);

    CodeChunk chunk;
    defer(chunk.code.Free());
    defer(chunk.constants.Free());

    CodeGen(pAst, &chunk);
    chunk.code.PushBack((uint8_t)OpCode::Print);
    chunk.code.PushBack((uint8_t)OpCode::Return);

    Disassemble(chunk);
    
    Log::Info("---- Program Running -----");

    Run(&chunk);

    //astMemory.Finished();

    __debugbreak();
    return 0;
}