
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

#define DEBUG_TRACE


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
    double m_value;
};

struct Grouping : public Expression {
    Expression* m_pExpression;
};

}


struct ParsingState {
    Token* m_pTokensStart { nullptr };
    Token* m_pTokensEnd { nullptr };
    Token* m_pCurrent { nullptr };

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
            pLiteralExpr->m_value = (double)strtol(token.m_pLocation, &endPtr, 10);
            return pLiteralExpr;
        }

        if (Match(1, TokenType::LiteralFloat)) {
            Ast::Literal* pLiteralExpr = (Ast::Literal*)g_Allocator.Allocate(sizeof(Ast::Literal));
            pLiteralExpr->m_type = Ast::NodeType::Literal;
            
            Token token = Previous();
            char* endPtr = token.m_pLocation + token.m_length;
            pLiteralExpr->m_value = strtod(token.m_pLocation, &endPtr);
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
        while (Match(1, TokenType::Minus)) {
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

    Ast::Expression* ParseExpression() {
        return ParseAddSub();
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
    StringBuilder builder;

    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            Log::Debug("%*s- Literal (%f)", indentationLevel, "", pLiteral->m_value);
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

// TODO for cleanup of my prototype code

// [ ] Implement a nicer stack structure with custom indexing

int main() {
    // Scanning?
    // Want to reuse scanner from commonLib don't we

    String actualCode;
    actualCode = "( 5 - 2 ) * 12";

    ResizableArray<Token> tokens = Tokenize(&g_Allocator, actualCode);
    defer(tokens.Free());

    Ast::Expression* pAst = ParseToAst(tokens);

    DebugAst(pAst);

    CodeChunk chunk;
    defer(chunk.code.Free());
    defer(chunk.constants.Free());

    // Add constant to table and get it's index
    chunk.constants.PushBack(7);
    uint8_t constIndex7 = (uint8_t)chunk.constants.m_count - 1;

    chunk.constants.PushBack(14);
    uint8_t constIndex14 = (uint8_t)chunk.constants.m_count - 1;

    chunk.constants.PushBack(6);
    uint8_t constIndex6 = (uint8_t)chunk.constants.m_count - 1;

    // Add a load constant opcode
    chunk.code.PushBack((uint8_t)OpCode::LoadConstant);
    chunk.code.PushBack(constIndex6);

    chunk.code.PushBack((uint8_t)OpCode::LoadConstant);
    chunk.code.PushBack(constIndex7);

    chunk.code.PushBack((uint8_t)OpCode::LoadConstant);
    chunk.code.PushBack(constIndex14);

    // return opcode
    chunk.code.PushBack((uint8_t)OpCode::Subtract);
    //chunk.code.PushBack((uint8_t)OpCode::Add);
    chunk.code.PushBack((uint8_t)OpCode::Print);
    chunk.code.PushBack((uint8_t)OpCode::Return);

    Run(&chunk);

    //Disassemble(chunk);

    __debugbreak();
    return 0;
}