// Copyright 2020-2022 David Colson. All rights reserved.

#include <log.h>
#include <resizable_array.inl>
#include <defer.h>
#include <stdint.h>
#include <string_builder.h>
#include <light_string.h>
#include <scanning.h>
#include <linear_allocator.h>
#include <maths.h>

#include "parser.h"
#include "virtual_machine.h"
#include "lexer.h"
#include "code_gen.h"


void TypeCheck(Ast::Expression* pExpr, ParsingState* pParser) {
    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            pLiteral->m_valueType = pLiteral->m_value.m_type;
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            TypeCheck(pGroup->m_pExpression, pParser);
            pGroup->m_valueType = pGroup->m_pExpression->m_valueType;
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            TypeCheck(pBinary->m_pLeft, pParser);
            TypeCheck(pBinary->m_pRight, pParser);
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
            TypeCheck(pUnary->m_pRight, pParser);
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

// TODO: 
// [ ] Move type checking to it's own file
// [ ] Move the error reporting code to it's own place, not sure where?

int main() {
    InitValueTables();

    String actualCode;
    //actualCode = "( 5 - (12+true) * 12 / 3";
    actualCode = "false<3 && 12.4 >= 14 || 9<15";

    ResizableArray<Token> tokens = Tokenize(&g_Allocator, actualCode);
    defer(tokens.Free());

    LinearAllocator astMemory;

    ParsingState parser;
    Ast::Expression* pAst = parser.InitAndParse(tokens, &astMemory);

    TypeCheck(pAst, &parser);

    bool success = parser.m_errors.m_count == 0;
    if (!success) {
        Log::Info("Compilation failed with %i errors", parser.m_errors.m_count);

        StringBuilder builder;

        for (size_t i = 0; i < parser.m_errors.m_count; i++) {
            Error& err = parser.m_errors[i];
            builder.AppendFormat("Error At: filename:%i:%i\n", err.m_line, err.m_pLocation - err.m_pLineStart);
            
            uint32_t lineNumberSpacing = uint32_t(log10f((float)err.m_line) + 1);
            builder.AppendFormat("%*s|\n", lineNumberSpacing + 2, "");

            String line;
            char* lineEnd;
            if (lineEnd = strchr(err.m_pLineStart, '\n')) {
                line = CopyCStringRange(err.m_pLineStart, lineEnd);
            } else if (lineEnd = strchr(err.m_pLineStart, '\0')) {
                line = CopyCStringRange(err.m_pLineStart, lineEnd); 
            }
            defer(FreeString(line));
            builder.AppendFormat(" %i | %s\n", err.m_line, line.m_pData);

            int32_t errorAtColumn = int32_t(err.m_pLocation - err.m_pLineStart);
            builder.AppendFormat("%*s|%*s^\n", lineNumberSpacing + 2, "", errorAtColumn+1, "");
            builder.AppendFormat("%s\n", err.m_message.m_pData);

            String output = builder.CreateString();
            Log::Info("%s", output.m_pData);
        }
    } else {
        Log::Info("Compilation Succeeded");
    }

    Log::Debug("---- AST -----");
    DebugAst(pAst);

    if (success) {
        // Compile to bytecode
        CodeChunk chunk;
        defer(chunk.code.Free());
        defer(chunk.constants.Free());

        CodeGen(pAst, &chunk);
        chunk.code.PushBack((uint8_t)OpCode::Print);
        chunk.code.PushBack((uint8_t)OpCode::Return);

        // For debugging
        Disassemble(chunk);
    
        Log::Info("---- Program Running -----");
        Run(&chunk);
    }

    //astMemory.Finished();

    __debugbreak();
    return 0;
}