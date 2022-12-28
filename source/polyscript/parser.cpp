// Copyright 2020-2022 David Colson. All rights reserved.

#include "parser.h"

#include "lexer.h"

#include <log.h>
#include <stdarg.h>
#include <string.h>

struct ParsingState {
    Token* m_pTokensStart { nullptr };
    Token* m_pTokensEnd { nullptr };
    Token* m_pCurrent { nullptr };
    IAllocator* pAllocator { nullptr };

    // ***********************************************************************

    Token Previous() {
        return *(m_pCurrent - 1);
    }

    // ***********************************************************************

    Token Advance() {
        m_pCurrent++;
        return Previous();
    }

    // ***********************************************************************

    bool IsAtEnd() {
        return m_pCurrent > m_pTokensEnd;
    }

    // ***********************************************************************

    Token Peek() {
        return *m_pCurrent;
    }

    // ***********************************************************************

    bool Check(TokenType type) {
        if (IsAtEnd())
            return false;

        return Peek().m_type == type;
    }
    
    // ***********************************************************************

    Token Consume(TokenType type) {
        if (Check(type))
            return Advance();

        Token invalid;
        return invalid;
    }

    // ***********************************************************************

    bool Match(int numTokens, ...) {
        va_list args;

        va_start(args, numTokens);
        for (int i = 0; i < numTokens; i++) {
            if (Check(va_arg(args, TokenType))) {
                Advance();
                return true;
            }
        }
        return false;
    }

    // ***********************************************************************

    Ast::Expression* ParsePrimary() {
        if (Match(1, TokenType::LiteralInteger)) {
            Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
            pLiteralExpr->m_type = Ast::NodeType::Literal;

            Token token = Previous();
            char* endPtr = token.m_pLocation + token.m_length;
            pLiteralExpr->m_value.m_type = ValueType::I32;
            pLiteralExpr->m_value.m_i32Value = strtol(token.m_pLocation, &endPtr, 10);
            return pLiteralExpr;
        }

        if (Match(1, TokenType::LiteralFloat)) {
            Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
            pLiteralExpr->m_type = Ast::NodeType::Literal;

            Token token = Previous();
            char* endPtr = token.m_pLocation + token.m_length;
            pLiteralExpr->m_value.m_type = ValueType::F32;
            pLiteralExpr->m_value.m_f32Value = (float)strtod(token.m_pLocation, &endPtr);
            return pLiteralExpr;
        }

        if (Match(1, TokenType::LiteralBool)) {
            Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
            pLiteralExpr->m_type = Ast::NodeType::Literal;

            Token token = Previous();
            char* endPtr = token.m_pLocation + token.m_length;
            pLiteralExpr->m_value.m_type = ValueType::Bool;
            if (strncmp("true", token.m_pLocation, 4) == 0)
                pLiteralExpr->m_value.m_boolValue = true;
            else if (strncmp("false", token.m_pLocation, 4) == 0)
                pLiteralExpr->m_value.m_boolValue = false;
            return pLiteralExpr;
        }

        if (Match(1, TokenType::LeftParen)) {
            Ast::Expression* pExpr = ParseExpression();
            Consume(TokenType::RightParen);

            if (pExpr) {
                Ast::Grouping* pGroupExpr = (Ast::Grouping*)pAllocator->Allocate(sizeof(Ast::Grouping));
                pGroupExpr->m_type = Ast::NodeType::Grouping;

                pGroupExpr->m_pExpression = pExpr;
                return pGroupExpr;
            }
            return nullptr;
        }

        return nullptr;
    }

    // ***********************************************************************

    Ast::Expression* ParseUnary() {
        while (Match(2, TokenType::Minus, TokenType::Bang)) {
            Ast::Unary* pUnaryExpr = (Ast::Unary*)pAllocator->Allocate(sizeof(Ast::Unary));
            pUnaryExpr->m_type = Ast::NodeType::Unary;

            pUnaryExpr->m_operator = Previous();
            pUnaryExpr->m_pRight = ParseUnary();

            return (Ast::Expression*)pUnaryExpr;
        }
        return ParsePrimary();
    }

    // ***********************************************************************

    Ast::Expression* ParseMulDiv() {
        Ast::Expression* pExpr = ParseUnary();

        while (Match(2, TokenType::Star, TokenType::Slash)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseUnary();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    // ***********************************************************************

    Ast::Expression* ParseAddSub() {
        Ast::Expression* pExpr = ParseMulDiv();

        while (Match(2, TokenType::Minus, TokenType::Plus)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseMulDiv();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    // ***********************************************************************

    Ast::Expression* ParseComparison() {
        Ast::Expression* pExpr = ParseAddSub();

        while (Match(4, TokenType::Greater, TokenType::Less, TokenType::GreaterEqual, TokenType::LessEqual)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseAddSub();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    // ***********************************************************************

    Ast::Expression* ParseEquality() {
        Ast::Expression* pExpr = ParseComparison();

        while (Match(2, TokenType::EqualEqual, TokenType::BangEqual)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseComparison();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    // ***********************************************************************

    Ast::Expression* ParseLogicAnd() {
        Ast::Expression* pExpr = ParseEquality();

        while (Match(1, TokenType::And)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseEquality();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    // ***********************************************************************

    Ast::Expression* ParseLogicOr() {
        Ast::Expression* pExpr = ParseLogicAnd();

        while (Match(1, TokenType::Or)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseLogicAnd();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    // ***********************************************************************

    Ast::Expression* ParseExpression() {
        return ParseLogicOr();
    }
};

// ***********************************************************************

Ast::Expression* ParseToAst(ResizableArray<Token>& tokens, IAllocator* pAlloc) {
    ParsingState parse;
    parse.m_pTokensStart = tokens.m_pData;
    parse.m_pTokensEnd = tokens.m_pData + tokens.m_count - 1;
    parse.m_pCurrent = tokens.m_pData;
    parse.pAllocator = pAlloc;

    return parse.ParseExpression();
}

// ***********************************************************************

void DebugAst(Ast::Expression* pExpr, int indentationLevel) {
    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            if (pLiteral->m_value.m_type == ValueType::F32)
                Log::Debug("%*s- Literal (%f)", indentationLevel, "", pLiteral->m_value.m_f32Value);
            else if (pLiteral->m_value.m_type == ValueType::I32)
                Log::Debug("%*s- Literal (%i)", indentationLevel, "", pLiteral->m_value.m_i32Value);
            else if (pLiteral->m_value.m_type == ValueType::Bool)
                Log::Debug("%*s- Literal (%s)", indentationLevel, "", pLiteral->m_value.m_boolValue ? "true" : "false");
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            Log::Debug("%*s- Group", indentationLevel, "");
            DebugAst(pGroup->m_pExpression, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            switch (pBinary->m_operator.m_type) {
                case TokenType::Plus:
                    Log::Debug("%*s- Binary (+)", indentationLevel, "");
                    break;
                case TokenType::Minus:
                    Log::Debug("%*s- Binary (-)", indentationLevel, "");
                    break;
                case TokenType::Slash:
                    Log::Debug("%*s- Binary (/)", indentationLevel, "");
                    break;
                case TokenType::Star:
                    Log::Debug("%*s- Binary (*)", indentationLevel, "");
                    break;
                case TokenType::Greater:
                    Log::Debug("%*s- Binary (>)", indentationLevel, "");
                    break;
                case TokenType::Less:
                    Log::Debug("%*s- Binary (<)", indentationLevel, "");
                    break;
                case TokenType::GreaterEqual:
                    Log::Debug("%*s- Binary (>=)", indentationLevel, "");
                    break;
                case TokenType::LessEqual:
                    Log::Debug("%*s- Binary (<=)", indentationLevel, "");
                    break;
                case TokenType::EqualEqual:
                    Log::Debug("%*s- Binary (==)", indentationLevel, "");
                    break;
                case TokenType::BangEqual:
                    Log::Debug("%*s- Binary (!=)", indentationLevel, "");
                    break;
                case TokenType::And:
                    Log::Debug("%*s- Binary (&&)", indentationLevel, "");
                    break;
                case TokenType::Or:
                    Log::Debug("%*s- Binary (||)", indentationLevel, "");
                    break;
                default:
                    break;
            }
            DebugAst(pBinary->m_pLeft, indentationLevel + 2);
            DebugAst(pBinary->m_pRight, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            switch (pUnary->m_operator.m_type) {
                case TokenType::Minus:
                    Log::Debug("%*s- Unary (-)", indentationLevel, "");
                    break;
                case TokenType::Bang:
                    Log::Debug("%*s- Unary (!)", indentationLevel, "");
                    break;
                default:
                    break;
            }
            DebugAst(pUnary->m_pRight, indentationLevel + 2);
            break;
        }
        default:
            break;
    }
}