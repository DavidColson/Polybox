
#include <log.h>
#include <resizable_array.inl>
#include <defer.h>
#include <stdint.h>
#include <string_builder.h>
#include <light_string.h>
#include <scanning.h>
#include <stdarg.h>

#include "virtual_machine.h"
#include "lexer.h"


namespace Ast {
enum class NodeType {
    Binary,
    Unary,
    Literal,
    Grouping
};

struct Expression {
    NodeType m_type;
};

struct Binary : public Expression {
    Expression* m_pLeft;
    Token m_operator;
    Expression* m_pRight;
};

struct Unary : public Expression {
    Token m_operator;
    Expression* m_pRight;
};

struct Literal : public Expression {
    Value m_value;
};

struct Grouping : public Expression {
    Expression* m_pExpression;
};

}

struct ParsingState {
    Token* m_pTokensStart { nullptr };
    Token* m_pTokensEnd { nullptr };
    Token* m_pCurrent { nullptr };

    // TODO: Store an allocator in here that can control the whole parse process and AST production
    // Probably do a linear pool, since we probably want to just yeet this entire thing at once

    Token Previous() {
        return *(m_pCurrent - 1);
    }

    Token Advance() {
        m_pCurrent++;
        return Previous();
    }

    bool IsAtEnd() {
        return m_pCurrent > m_pTokensEnd;
    }

    Token Peek() {
        return *m_pCurrent;
    }

    bool Check(TokenType type) {
        if (IsAtEnd())
            return false;

        return Peek().m_type == type;
    }

    Token Consume(TokenType type) {
        if (Check(type))
            return Advance();

        Token invalid;
        return invalid;
    }

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
    
    Ast::Expression* ParsePrimary() {
        if (Match(1, TokenType::LiteralInteger)) {
            Ast::Literal* pLiteralExpr = (Ast::Literal*)g_Allocator.Allocate(sizeof(Ast::Literal));
            pLiteralExpr->m_type = Ast::NodeType::Literal;

            Token token = Previous();
            char* endPtr = token.m_pLocation + token.m_length;
            pLiteralExpr->m_value.m_type = ValueType::Float;
            pLiteralExpr->m_value.m_floatValue = (double)strtol(token.m_pLocation, &endPtr, 10);
            return pLiteralExpr;
        }

        if (Match(1, TokenType::LiteralFloat)) {
            Ast::Literal* pLiteralExpr = (Ast::Literal*)g_Allocator.Allocate(sizeof(Ast::Literal));
            pLiteralExpr->m_type = Ast::NodeType::Literal;
            
            Token token = Previous();
            char* endPtr = token.m_pLocation + token.m_length;
            pLiteralExpr->m_value.m_type = ValueType::Float;
            pLiteralExpr->m_value.m_floatValue = strtod(token.m_pLocation, &endPtr);
            return pLiteralExpr;
        }

        if (Match(1, TokenType::LiteralBool)) {
            Ast::Literal* pLiteralExpr = (Ast::Literal*)g_Allocator.Allocate(sizeof(Ast::Literal));
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
                Ast::Grouping* pGroupExpr = (Ast::Grouping*)g_Allocator.Allocate(sizeof(Ast::Grouping));
                pGroupExpr->m_type = Ast::NodeType::Grouping;

                pGroupExpr->m_pExpression = pExpr;
                return pGroupExpr;
            }
            return nullptr;
        }

        return nullptr;
    }

    Ast::Expression* ParseUnary() {
        while (Match(2, TokenType::Minus, TokenType::Bang)) {
            Ast::Unary* pUnaryExpr = (Ast::Unary*)g_Allocator.Allocate(sizeof(Ast::Unary));
            pUnaryExpr->m_type = Ast::NodeType::Unary;

            pUnaryExpr->m_operator = Previous();
            pUnaryExpr->m_pRight = ParseUnary();

            return (Ast::Expression*)pUnaryExpr;
        }
        return ParsePrimary();
    }

    Ast::Expression* ParseMulDiv() {
        Ast::Expression* pExpr = ParseUnary();

        while (Match(2, TokenType::Star, TokenType::Slash)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)g_Allocator.Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseUnary();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    Ast::Expression* ParseAddSub() {
        Ast::Expression* pExpr = ParseMulDiv();

        while (Match(2, TokenType::Minus, TokenType::Plus)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)g_Allocator.Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseMulDiv();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    Ast::Expression* ParseComparison() {
        Ast::Expression* pExpr = ParseAddSub();

        while (Match(4, TokenType::Greater, TokenType::Less, TokenType::GreaterEqual, TokenType::LessEqual)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)g_Allocator.Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseAddSub();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    Ast::Expression* ParseEquality() {
        Ast::Expression* pExpr = ParseComparison();

        while (Match(2, TokenType::EqualEqual, TokenType::BangEqual)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)g_Allocator.Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseComparison();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    Ast::Expression* ParseLogicAnd() {
        Ast::Expression* pExpr = ParseEquality();

        while (Match(1, TokenType::And)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)g_Allocator.Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseEquality();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }

    Ast::Expression* ParseLogicOr() {
        Ast::Expression* pExpr = ParseLogicAnd();

        while (Match(1, TokenType::Or)) {
            Ast::Binary* pBinaryExpr = (Ast::Binary*)g_Allocator.Allocate(sizeof(Ast::Binary));
            pBinaryExpr->m_type = Ast::NodeType::Binary;

            pBinaryExpr->m_pLeft = pExpr;
            pBinaryExpr->m_operator = Previous();
            pBinaryExpr->m_pRight = ParseLogicAnd();

            pExpr = (Ast::Expression*)pBinaryExpr;
        }
        return pExpr;
    }
    
    Ast::Expression* ParseExpression() {
        return ParseLogicOr();
    }
};


Ast::Expression* ParseToAst(ResizableArray<Token>& tokens) {
    ParsingState parse;
    parse.m_pTokensStart = tokens.m_pData;
    parse.m_pTokensEnd = tokens.m_pData + tokens.m_count - 1;
    parse.m_pCurrent = tokens.m_pData;

    return parse.ParseExpression();
}


void DebugAst(Ast::Expression* pExpr, int indentationLevel = 0) {
    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            if (pLiteral->m_value.m_type == ValueType::Float)
                Log::Debug("%*s- Literal (%f)", indentationLevel, "", pLiteral->m_value.m_floatValue);
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

// TODO for cleanup of my prototype code

// [ ] Implement a nicer stack structure with custom indexing
// [ ] Move AST parser to it's own file

int main() {
    // Scanning?
    // Want to reuse scanner from commonLib don't we

    String actualCode;
    //actualCode = "( 5 - (12+3) ) * 12 / 3";
    actualCode = "true && true";

    ResizableArray<Token> tokens = Tokenize(&g_Allocator, actualCode);
    defer(tokens.Free());

    Ast::Expression* pAst = ParseToAst(tokens);

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

    __debugbreak();
    return 0;
}