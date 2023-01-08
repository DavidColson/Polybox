// Copyright 2020-2022 David Colson. All rights reserved.

#include "parser.h"

#include "lexer.h"

#include <log.h>
#include <stdarg.h>
#include <string.h>
#include <resizable_array.inl>
#include <string_builder.h>
#include <defer.h>
#include <maths.h>


// ErrorState

// ***********************************************************************

void ErrorState::Init(IAllocator* pAlloc) {
    m_pAlloc = pAlloc;
    m_errors.m_pAlloc = m_pAlloc;
}

// ***********************************************************************

void ErrorState::PushError(Ast::Expression* pNode, const char* formatMessage, ...) {
    StringBuilder builder(m_pAlloc);
    va_list args;
    va_start(args, formatMessage);
    builder.AppendFormatInternal(formatMessage, args);
    va_end(args);

    Assert(pNode != nullptr);

    Error er;
    er.m_pLocation = pNode->m_pLocation;
    er.m_line = pNode->m_line;
    er.m_pLineStart = pNode->m_pLineStart;

    er.m_message = builder.CreateString();
    m_errors.PushBack(er);
}

// ***********************************************************************

void ErrorState::PushError(char* pLocation, char* pLineStart, size_t line, const char* formatMessage, ...) {
    va_list args;
    va_start(args, formatMessage);
    PushError(pLocation, pLineStart, line, formatMessage, args);
    va_end(args);
}

// ***********************************************************************

void ErrorState::PushError(char* pLocation, char* pLineStart, size_t line, const char* formatMessage, va_list args) {
    StringBuilder builder(m_pAlloc);
    builder.AppendFormatInternal(formatMessage, args);

    Error er;
    er.m_pLocation = pLocation;
    er.m_line = line;
    er.m_pLineStart = pLineStart;

    er.m_message = builder.CreateString();

    m_errors.PushBack(er);
}

// ***********************************************************************

bool ErrorState::ReportCompilationResult() {
    bool success = m_errors.m_count == 0;
    if (!success) {
        Log::Info("Compilation failed with %i errors", m_errors.m_count);

        StringBuilder builder;

        for (size_t i = 0; i < m_errors.m_count; i++) {
            Error& err = m_errors[i];
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
            builder.AppendFormat("%*s|%*s^\n", lineNumberSpacing + 2, "", errorAtColumn + 1, "");
            builder.AppendFormat("%s\n", err.m_message.m_pData);

            String output = builder.CreateString();
            Log::Info("%s", output.m_pData);
        }
    } else {
        Log::Info("Compilation Succeeded");
    }
    return success;
}


// ParsingState

// ***********************************************************************

Token ParsingState::Previous() {
    return *(m_pCurrent - 1);
}

// ***********************************************************************

Token ParsingState::Advance() {
    m_pCurrent++;
    return Previous();
}

// ***********************************************************************

bool ParsingState::IsAtEnd() {
    return m_pCurrent >= m_pTokensEnd;
}

// ***********************************************************************

Token ParsingState::Peek() {
    return *m_pCurrent;
}

// ***********************************************************************

bool ParsingState::Check(TokenType::Enum type) {
    if (IsAtEnd())
        return false;

    return Peek().m_type == type;
}
    
// ***********************************************************************

Token ParsingState::Consume(TokenType::Enum type, String message) {
    if (Check(type))
        return Advance();

    PushError(nullptr, message.m_pData);
    return Token();
}

// ***********************************************************************

bool ParsingState::Match(int numTokens, ...) {
    va_list args;

    va_start(args, numTokens);
    for (int i = 0; i < numTokens; i++) {
        if (Check(va_arg(args, TokenType::Enum))) {
            Advance();
            return true;
        }
    }
    return false;
}

// ***********************************************************************

void ParsingState::PushError(const char* formatMessage, ...) {
    m_panicMode = true;

    va_list args;
    va_start(args, formatMessage);
    m_pErrorState->PushError(m_pCurrent->m_pLocation, m_pCurrent->m_pLineStart, m_pCurrent->m_line, formatMessage, args);
    va_end(args);
}

// ***********************************************************************

void ParsingState::Synchronize() {
    m_panicMode = false;

    while (m_pCurrent->m_type != TokenType::EndOfFile) {
        if (m_pCurrent->m_type == TokenType::Semicolon) {
            Advance(); // Consume semicolon we found
            return;
        }

        Advance();
    }
}

// ***********************************************************************

Ast::Expression* ParsingState::ParsePrimary() {
    if (Match(1, TokenType::LiteralInteger)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->m_type = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->m_pLocation = token.m_pLocation;
        pLiteralExpr->m_pLineStart = token.m_pLineStart;
        pLiteralExpr->m_line = token.m_line;

        char* endPtr = token.m_pLocation + token.m_length;
        pLiteralExpr->m_value.m_type = ValueType::I32;
        pLiteralExpr->m_value.m_i32Value = strtol(token.m_pLocation, &endPtr, 10);
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LiteralFloat)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->m_type = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->m_pLocation = token.m_pLocation;
        pLiteralExpr->m_pLineStart = token.m_pLineStart;
        pLiteralExpr->m_line = token.m_line;

        char* endPtr = token.m_pLocation + token.m_length;
        pLiteralExpr->m_value.m_type = ValueType::F32;
        pLiteralExpr->m_value.m_f32Value = (float)strtod(token.m_pLocation, &endPtr);
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LiteralBool)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->m_type = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->m_pLocation = token.m_pLocation;
        pLiteralExpr->m_pLineStart = token.m_pLineStart;
        pLiteralExpr->m_line = token.m_line;

        char* endPtr = token.m_pLocation + token.m_length;
        pLiteralExpr->m_value.m_type = ValueType::Bool;
        if (strncmp("true", token.m_pLocation, 4) == 0)
            pLiteralExpr->m_value.m_boolValue = true;
        else if (strncmp("false", token.m_pLocation, 4) == 0)
            pLiteralExpr->m_value.m_boolValue = false;
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LeftParen)) {
        Token startToken = Previous();
        Ast::Expression* pExpr = ParseExpression();
        Consume(TokenType::RightParen, "Expected a closing right parenthesis \")\", but found nothing in this expression");

        if (pExpr) {
            Ast::Grouping* pGroupExpr = (Ast::Grouping*)pAllocator->Allocate(sizeof(Ast::Grouping));
            pGroupExpr->m_type = Ast::NodeType::Grouping;

            pGroupExpr->m_pLocation = startToken.m_pLocation;
            pGroupExpr->m_pLineStart = startToken.m_pLineStart;
            pGroupExpr->m_line = startToken.m_line;

            pGroupExpr->m_pExpression = pExpr;
            return pGroupExpr;
        }
        PushError(nullptr, "Expected valid expression inside parenthesis, but found nothing");
        return nullptr;
    }

    return nullptr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseUnary() {
    while (Match(2, TokenType::Minus, TokenType::Bang)) {
        Ast::Unary* pUnaryExpr = (Ast::Unary*)pAllocator->Allocate(sizeof(Ast::Unary));
        pUnaryExpr->m_type = Ast::NodeType::Unary;

        Token operatorToken = Previous();
        pUnaryExpr->m_pLocation = operatorToken.m_pLocation;
        pUnaryExpr->m_pLineStart = operatorToken.m_pLineStart;
        pUnaryExpr->m_line = operatorToken.m_line;

        if (operatorToken.m_type == TokenType::Minus)
            pUnaryExpr->m_operator = Operator::UnaryMinus;
        else if (operatorToken.m_type == TokenType::Bang)
            pUnaryExpr->m_operator = Operator::Not;
        pUnaryExpr->m_pRight = ParseUnary();

        return (Ast::Expression*)pUnaryExpr;
    }
    return ParsePrimary();
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseMulDiv() {
    Ast::Expression* pExpr = ParseUnary();

    while (Match(2, TokenType::Star, TokenType::Slash)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->m_type = Ast::NodeType::Binary;

        Token operatorToken = Previous();
        pBinaryExpr->m_pLocation = operatorToken.m_pLocation;
        pBinaryExpr->m_pLineStart = operatorToken.m_pLineStart;
        pBinaryExpr->m_line = operatorToken.m_line;

        pBinaryExpr->m_pLeft = pExpr;
        pBinaryExpr->m_operator = TokenToOperator(operatorToken.m_type);
        pBinaryExpr->m_pRight = ParseUnary();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseAddSub() {
    Ast::Expression* pExpr = ParseMulDiv();

    while (Match(2, TokenType::Minus, TokenType::Plus)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->m_type = Ast::NodeType::Binary;

        Token operatorToken = Previous();
        pBinaryExpr->m_pLocation = operatorToken.m_pLocation;
        pBinaryExpr->m_pLineStart = operatorToken.m_pLineStart;
        pBinaryExpr->m_line = operatorToken.m_line;

        pBinaryExpr->m_pLeft = pExpr;
        pBinaryExpr->m_operator = TokenToOperator(operatorToken.m_type);
        pBinaryExpr->m_pRight = ParseMulDiv();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseComparison() {
    Ast::Expression* pExpr = ParseAddSub();

    while (Match(4, TokenType::Greater, TokenType::Less, TokenType::GreaterEqual, TokenType::LessEqual)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->m_type = Ast::NodeType::Binary;

        Token operatorToken = Previous();
        pBinaryExpr->m_pLocation = operatorToken.m_pLocation;
        pBinaryExpr->m_pLineStart = operatorToken.m_pLineStart;
        pBinaryExpr->m_line = operatorToken.m_line;

        pBinaryExpr->m_pLeft = pExpr;
        pBinaryExpr->m_operator = TokenToOperator(operatorToken.m_type);
        pBinaryExpr->m_pRight = ParseAddSub();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseEquality() {
    Ast::Expression* pExpr = ParseComparison();

    while (Match(2, TokenType::EqualEqual, TokenType::BangEqual)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->m_type = Ast::NodeType::Binary;

        Token operatorToken = Previous();
        pBinaryExpr->m_pLocation = operatorToken.m_pLocation;
        pBinaryExpr->m_pLineStart = operatorToken.m_pLineStart;
        pBinaryExpr->m_line = operatorToken.m_line;

        pBinaryExpr->m_pLeft = pExpr;
        pBinaryExpr->m_operator = TokenToOperator(operatorToken.m_type);
        pBinaryExpr->m_pRight = ParseComparison();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseLogicAnd() {
    Ast::Expression* pExpr = ParseEquality();

    while (Match(1, TokenType::And)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->m_type = Ast::NodeType::Binary;

        Token operatorToken = Previous();
        pBinaryExpr->m_pLocation = operatorToken.m_pLocation;
        pBinaryExpr->m_pLineStart = operatorToken.m_pLineStart;
        pBinaryExpr->m_line = operatorToken.m_line;

        pBinaryExpr->m_pLeft = pExpr;
        pBinaryExpr->m_operator = TokenToOperator(operatorToken.m_type);
        pBinaryExpr->m_pRight = ParseEquality();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseLogicOr() {
    Ast::Expression* pExpr = ParseLogicAnd();

    while (Match(1, TokenType::Or)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->m_type = Ast::NodeType::Binary;

        Token operatorToken = Previous();
        pBinaryExpr->m_pLocation = operatorToken.m_pLocation;
        pBinaryExpr->m_pLineStart = operatorToken.m_pLineStart;
        pBinaryExpr->m_line = operatorToken.m_line;

        pBinaryExpr->m_pLeft = pExpr;
        pBinaryExpr->m_operator = TokenToOperator(operatorToken.m_type);
        pBinaryExpr->m_pRight = ParseLogicAnd();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseExpression() {
    return ParseLogicOr();
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseStatement() {
    Ast::Statement* pStmt = nullptr;

    if (Match(1, TokenType::Identifier)) {
        if (strncmp("print", Previous().m_pLocation, 5) == 0) {
            pStmt = ParsePrintStatement();
        } else {
            m_pCurrent--;
        }
    } 
    
    if (pStmt == nullptr) {
        if (Match(8, TokenType::Identifier, TokenType::LiteralString, TokenType::LiteralInteger, TokenType::LiteralBool, TokenType::LiteralFloat, TokenType::LeftParen, TokenType::Bang, TokenType::Minus)) {
            m_pCurrent--;
            pStmt = ParseExpressionStatement();
        } else {
            PushError(nullptr, "Unable to parse statement");
        }
    }

    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseExpressionStatement() {
    Ast::Expression* pExpr = ParseExpression();
    Consume(TokenType::Semicolon, "Expected \";\" at the end of this statement");

    Ast::ExpressionStatement* pStmt = (Ast::ExpressionStatement*)pAllocator->Allocate(sizeof(Ast::ExpressionStatement));
    pStmt->m_type = Ast::NodeType::ExpressionStmt;

    pStmt->m_pExpr = pExpr;

    pStmt->m_pLocation = Previous().m_pLocation;
    pStmt->m_pLineStart = Previous().m_pLineStart;
    pStmt->m_line = Previous().m_line;
    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParsePrintStatement() {
    Consume(TokenType::LeftParen, "Expected \"(\" following print, before the expression starts");
    Ast::Expression* pExpr = ParseExpression();
    Consume(TokenType::RightParen, "Expected \")\" to close print expression");
    Consume(TokenType::Semicolon, "Expected \";\" at the end of this statement");
    
    Ast::PrintStatement* pPrintStmt = (Ast::PrintStatement*)pAllocator->Allocate(sizeof(Ast::PrintStatement));
    pPrintStmt->m_type = Ast::NodeType::PrintStmt;

    pPrintStmt->m_pExpr = pExpr;

    pPrintStmt->m_pLocation = Previous().m_pLocation;
    pPrintStmt->m_pLineStart = Previous().m_pLineStart;
    pPrintStmt->m_line = Previous().m_line;
    return pPrintStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseDeclaration() {
    Ast::Statement* pStmt = nullptr;
    
    if (Match(1, TokenType::Identifier)) {
        if (Advance().m_type == TokenType::Colon) {
            pStmt = ParseVarDeclaration();
        } else {
            m_pCurrent -= 2;
        }
    }
    
    if (pStmt == nullptr) {
        pStmt = ParseStatement();
    }

    if (m_panicMode)
        Synchronize();

    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseVarDeclaration() {
    Token name = *(m_pCurrent - 2);
    Consume(TokenType::Equal, "Expected an equal sign before variable initializer");
    Ast::Expression* pExpr = ParseExpression();
    Consume(TokenType::Semicolon, "Expected \";\" at the end of this statement");

    Ast::VariableDeclaration* pVarDecl = (Ast::VariableDeclaration*)pAllocator->Allocate(sizeof(Ast::VariableDeclaration));
    pVarDecl->m_type = Ast::NodeType::VarDecl;

    pVarDecl->m_name = CopyCStringRange(name.m_pLocation, name.m_pLocation + name.m_length, pAllocator);
    pVarDecl->m_pInitializerExpr = pExpr;

    pVarDecl->m_pLocation = name.m_pLocation;
    pVarDecl->m_pLineStart = name.m_pLineStart;
    pVarDecl->m_line = name.m_line;
    return pVarDecl;
}

// ***********************************************************************

ResizableArray<Ast::Statement*> ParsingState::InitAndParse(ResizableArray<Token>& tokens, ErrorState* pErrors, IAllocator* pAlloc) {
    m_pTokensStart = tokens.m_pData;
    m_pTokensEnd = tokens.m_pData + tokens.m_count - 1;
    m_pCurrent = tokens.m_pData;
    pAllocator = pAlloc;
    m_pErrorState = pErrors;

    ResizableArray<Ast::Statement*> statements;
    statements.m_pAlloc = pAlloc;

    while (!IsAtEnd()) {
        Ast::Statement* pStmt = ParseDeclaration();
        if (pStmt)
            statements.PushBack(pStmt);
    }

    return statements;
}

// ***********************************************************************

void DebugAst(ResizableArray<Ast::Statement*>& program) {
    for (size_t i = 0; i < program.m_count; i++) {
        Ast::Statement* pStmt = program[i];

        switch (pStmt->m_type) {
            case Ast::NodeType::VarDecl: {
                Ast::VariableDeclaration* pVarDecl = (Ast::VariableDeclaration*)pStmt;
                Log::Debug("+ VarDecl (%s)", pVarDecl->m_name.m_pData);
                DebugExpression(pVarDecl->m_pInitializerExpr, 2);
                break;
            }
            case Ast::NodeType::PrintStmt: {
                Ast::PrintStatement* pPrint = (Ast::PrintStatement*)pStmt;
                Log::Debug("> PrintStmt");
                DebugExpression(pPrint->m_pExpr, 2);
                break;
            }
            case Ast::NodeType::ExpressionStmt: {
                Ast::ExpressionStatement* pExprStmt = (Ast::ExpressionStatement*)pStmt;
                Log::Debug("> ExpressionStmt");
                DebugExpression(pExprStmt->m_pExpr, 2);
                break;
            }
            default:
                break;
        }
    }
}

// ***********************************************************************

void DebugExpression(Ast::Expression* pExpr, int indentationLevel) {
    if (pExpr == nullptr) {
        Log::Debug("%*s- NULL", indentationLevel, "");
        return;
    }

    switch (pExpr->m_type) {
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            if (pLiteral->m_value.m_type == ValueType::F32)
                Log::Debug("%*s- Literal (%f:%s)", indentationLevel, "", pLiteral->m_value.m_f32Value, ValueType::ToString(pLiteral->m_valueType));
            else if (pLiteral->m_value.m_type == ValueType::I32)
                Log::Debug("%*s- Literal (%i:%s)", indentationLevel, "", pLiteral->m_value.m_i32Value, ValueType::ToString(pLiteral->m_valueType));
            else if (pLiteral->m_value.m_type == ValueType::Bool)
                Log::Debug("%*s- Literal (%s:%s)", indentationLevel, "", pLiteral->m_value.m_boolValue ? "true" : "false", ValueType::ToString(pLiteral->m_valueType));
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            Log::Debug("%*s- Group (:%s)", indentationLevel, "", ValueType::ToString(pGroup->m_valueType));
            DebugExpression(pGroup->m_pExpression, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            switch (pBinary->m_operator) {
                case Operator::Add:
                    Log::Debug("%*s- Binary (+:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::Subtract:
                    Log::Debug("%*s- Binary (-:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::Divide:
                    Log::Debug("%*s- Binary (/:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::Multiply:
                    Log::Debug("%*s- Binary (*:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::Greater:
                    Log::Debug("%*s- Binary (>:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::Less:
                    Log::Debug("%*s- Binary (<:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::GreaterEqual:
                    Log::Debug("%*s- Binary (>=:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::LessEqual:
                    Log::Debug("%*s- Binary (<=:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::Equal:
                    Log::Debug("%*s- Binary (==:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::NotEqual:
                    Log::Debug("%*s- Binary (!=:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::And:
                    Log::Debug("%*s- Binary (&&:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                case Operator::Or:
                    Log::Debug("%*s- Binary (||:%s)", indentationLevel, "", ValueType::ToString(pBinary->m_valueType));
                    break;
                default:
                    break;
            }
            DebugExpression(pBinary->m_pLeft, indentationLevel + 2);
            DebugExpression(pBinary->m_pRight, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            switch (pUnary->m_operator) {
                case Operator::UnaryMinus:
                    Log::Debug("%*s- Unary (-:%s)", indentationLevel, "", ValueType::ToString(pUnary->m_valueType));
                    break;
                case Operator::Not:
                    Log::Debug("%*s- Unary (!:%s)", indentationLevel, "", ValueType::ToString(pUnary->m_valueType));
                    break;
                default:
                    break;
            }
            DebugExpression(pUnary->m_pRight, indentationLevel + 2);
            break;
        }
        default:
            break;
    }
}