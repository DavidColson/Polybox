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

void ErrorState::PushError(Ast::Node* pNode, const char* formatMessage, ...) {
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

    PushError(message.m_pData);
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
    if (m_panicMode)
        return;

    m_panicMode = true;

    va_list args;
    va_start(args, formatMessage);
    m_pErrorState->PushError(m_pCurrent->m_pLocation, m_pCurrent->m_pLineStart, m_pCurrent->m_line, formatMessage, args);
    va_end(args);
}

// ***********************************************************************

void ParsingState::Synchronize() {
    m_panicMode = false;

    bool waitForBlock = false;
    while (m_pCurrent->m_type != TokenType::EndOfFile) {
        if (m_pCurrent->m_type == TokenType::LeftBrace) {
            waitForBlock = true;
        }

        if (waitForBlock) {
            if (m_pCurrent->m_type == TokenType::RightBrace) {
                Advance();
                return;
            }
            Advance();
        } else {
            if (m_pCurrent->m_type == TokenType::Semicolon) {
                Advance(); // Consume semicolon we found
                return;
            }
            Advance();
        }
    }
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseType() {

    if (Match(1, TokenType::Identifier)) {
        Token identifier = Previous();

        Ast::Type* pType = (Ast::Type*)pAllocator->Allocate(sizeof(Ast::Type));
        pType->m_nodeKind = Ast::NodeType::Type;
        pType->m_identifier = CopyCStringRange(identifier.m_pLocation, identifier.m_pLocation + identifier.m_length, pAllocator);
        pType->m_pLocation = identifier.m_pLocation;
        pType->m_pLineStart = identifier.m_pLineStart;
        pType->m_line = identifier.m_line;

        return pType;
    }

    if (Match(1, TokenType::Fn)) {
        Token fn = Previous();

        Ast::FnType* pFnType = (Ast::FnType*)pAllocator->Allocate(sizeof(Ast::FnType));
        pFnType->m_nodeKind = Ast::NodeType::FnType;
        pFnType->m_params.m_pAlloc = pAllocator;

        Consume(TokenType::LeftParen, "Expected left parenthesis to start function signature");

        if (Peek().m_type != TokenType::RightParen) {
            do {
                pFnType->m_params.PushBack((Ast::Type*)ParseType());

                if (Peek().m_type == TokenType::Colon) {
					PushError("Expected a function signature, but this looks like a function header. Potentially replace 'fn' with 'func' from start of expression");
                    return nullptr;
                }
            } while (Match(1, TokenType::Comma));
        }
        Consume(TokenType::RightParen, "Expected right parenthesis to close argument list");

        // Parse return type
        if (Match(1, TokenType::FuncSigReturn)) {
            pFnType->m_pReturnType = (Ast::Type*)ParseType();
        } else {
            pFnType->m_pReturnType = (Ast::Type*)pAllocator->Allocate(sizeof(Ast::Type));
            pFnType->m_pReturnType->m_nodeKind = Ast::NodeType::Identifier;
            pFnType->m_pReturnType->m_identifier = CopyCString("void", pAllocator);
            pFnType->m_pReturnType->m_pType = GetVoidType();
            pFnType->m_pReturnType->m_pLocation = Previous().m_pLocation;
            pFnType->m_pReturnType->m_pLineStart = Previous().m_pLineStart;
            pFnType->m_pReturnType->m_line = Previous().m_line;
        }

        pFnType->m_pLocation = fn.m_pLocation;
        pFnType->m_pLineStart = fn.m_pLineStart;
        pFnType->m_line = fn.m_line;
        return pFnType;
    }

    return nullptr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParsePrimary() {

	if (Match(1, TokenType::Func)) {
		Token fn = Previous();

		Ast::Function* pFunc = (Ast::Function*)pAllocator->Allocate(sizeof(Ast::Function));
		pFunc->m_nodeKind = Ast::NodeType::Function;
		pFunc->m_params.m_pAlloc = pAllocator;

		Consume(TokenType::LeftParen, "Expected left parenthesis to start function param list");

		if (Peek().m_type != TokenType::RightParen) {
			// Parse parameter list
			do {
				Token arg = Consume(TokenType::Identifier, "Expected argument identifier after comma");
				Consume(TokenType::Colon, "Expected colon after argument identifier");

				Ast::Declaration* pParamDecl = (Ast::Declaration*)pAllocator->Allocate(sizeof(Ast::Declaration));
				pParamDecl->m_nodeKind = Ast::NodeType::Declaration;
				pParamDecl->m_identifier = CopyCStringRange(arg.m_pLocation, arg.m_pLocation + arg.m_length, pAllocator);
				pParamDecl->m_pDeclaredType = (Ast::Type*)ParseType();
				pParamDecl->m_pLocation = arg.m_pLocation;
				pParamDecl->m_line = arg.m_line;
				pParamDecl->m_pLineStart = arg.m_pLineStart;

				pFunc->m_params.PushBack(pParamDecl);
			} while (Match(1, TokenType::Comma));
		}
		Consume(TokenType::RightParen, "Expected right parenthesis to close argument list");

		// Parse return type
		if (Match(1, TokenType::FuncSigReturn)) {
			pFunc->m_pReturnType = (Ast::Type*)ParseType();
		} else {
			pFunc->m_pReturnType = (Ast::Type*)pAllocator->Allocate(sizeof(Ast::Type));
			pFunc->m_pReturnType->m_nodeKind = Ast::NodeType::Identifier;
			pFunc->m_pReturnType->m_identifier = CopyCString("void", pAllocator);
			pFunc->m_pReturnType->m_pType = GetVoidType();
			pFunc->m_pReturnType->m_pLocation = Previous().m_pLocation;
			pFunc->m_pReturnType->m_pLineStart = Previous().m_pLineStart;
			pFunc->m_pReturnType->m_line = Previous().m_line;
		}

		if (Match(1, TokenType::LeftBrace)) {
			pFunc->m_pBody = (Ast::Block*)ParseBlock();
		} else {
			PushError("Expected '{' to open function body");
		}
			
		pFunc->m_pLocation = fn.m_pLocation;
		pFunc->m_pLineStart = fn.m_pLineStart;
		pFunc->m_line = fn.m_line;
		return pFunc;
	}

    if (Match(1, TokenType::LiteralInteger)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->m_nodeKind = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->m_pLocation = token.m_pLocation;
        pLiteralExpr->m_pLineStart = token.m_pLineStart;
        pLiteralExpr->m_line = token.m_line;

        char* endPtr = token.m_pLocation + token.m_length;
        pLiteralExpr->m_value = MakeValue((int32_t)strtol(token.m_pLocation, &endPtr, 10));
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LiteralFloat)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->m_nodeKind = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->m_pLocation = token.m_pLocation;
        pLiteralExpr->m_pLineStart = token.m_pLineStart;
        pLiteralExpr->m_line = token.m_line;

        char* endPtr = token.m_pLocation + token.m_length;
        pLiteralExpr->m_value = MakeValue((float)strtod(token.m_pLocation, &endPtr));
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LiteralBool)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->m_nodeKind = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->m_pLocation = token.m_pLocation;
        pLiteralExpr->m_pLineStart = token.m_pLineStart;
        pLiteralExpr->m_line = token.m_line;

        char* endPtr = token.m_pLocation + token.m_length;
        if (strncmp("true", token.m_pLocation, 4) == 0)
            pLiteralExpr->m_value = MakeValue(true);
        else if (strncmp("false", token.m_pLocation, 4) == 0)
            pLiteralExpr->m_value = MakeValue(true);
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LeftParen)) {
        Token startToken = Previous();
        Ast::Expression* pExpr = ParseExpression();
        Consume(TokenType::RightParen, "Expected a closing right parenthesis \")\", but found nothing in this expression");

        if (pExpr) {
            Ast::Grouping* pGroupExpr = (Ast::Grouping*)pAllocator->Allocate(sizeof(Ast::Grouping));
            pGroupExpr->m_nodeKind = Ast::NodeType::Grouping;

            pGroupExpr->m_pLocation = startToken.m_pLocation;
            pGroupExpr->m_pLineStart = startToken.m_pLineStart;
            pGroupExpr->m_line = startToken.m_line;

            pGroupExpr->m_pExpression = pExpr;
            return pGroupExpr;
        }
        PushError("Expected valid expression inside parenthesis, but found nothing");
        return nullptr;
    }

    if (Match(1, TokenType::Identifier)) { // Note this could now be a primitive type, need to account for this in the parser structure and typechecker
        Token identifier = Previous();

        // Should become identifier with a type, variable or type or something like that
        Ast::Identifier* pIdentifier = (Ast::Identifier*)pAllocator->Allocate(sizeof(Ast::Identifier));
        pIdentifier->m_nodeKind = Ast::NodeType::Identifier;
        pIdentifier->m_identifier = CopyCStringRange(identifier.m_pLocation, identifier.m_pLocation + identifier.m_length, pAllocator);

        pIdentifier->m_pLocation = identifier.m_pLocation;
        pIdentifier->m_pLineStart = identifier.m_pLineStart;
        pIdentifier->m_line = identifier.m_line;

        return pIdentifier;
    }

    if (Ast::Type* pType = (Ast::Type*)ParseType()) {
        return pType;
    }

    return nullptr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseCall() {
    Ast::Expression* pExpr = ParsePrimary();

    while (Match(1, TokenType::LeftParen)) {
        Ast::Call* pCall = (Ast::Call*)pAllocator->Allocate(sizeof(Ast::Call));
        pCall->m_nodeKind = Ast::NodeType::Call;
        pCall->m_pCallee = pExpr;
        pCall->m_args.m_pAlloc = pAllocator;

        if (!Check(TokenType::RightParen)) {
            do {
                pCall->m_args.PushBack(ParseExpression());
            } while (Match(1, TokenType::Comma));
        }

        Token closeParen = Consume(TokenType::RightParen, "Expected right parenthesis to end function call");
        
        pCall->m_pLocation = closeParen.m_pLocation;
        pCall->m_pLineStart = closeParen.m_pLineStart;
        pCall->m_line = closeParen.m_line;

        pExpr = pCall;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseUnary() {
    while (Match(3, TokenType::Minus, TokenType::Bang, TokenType::As)) {
		Token prev = Previous();

		// Cast Operator
		if (prev.m_type == TokenType::As) {
			Consume(TokenType::LeftParen, "Expected '(' before cast target type");

			Ast::Cast* pCastExpr = (Ast::Cast*)pAllocator->Allocate(sizeof(Ast::Cast));
			pCastExpr->m_nodeKind = Ast::NodeType::Cast;
			pCastExpr->m_pTargetType = (Ast::Type*)ParseType();
			pCastExpr->m_pLocation = prev.m_pLocation;
			pCastExpr->m_pLineStart = prev.m_pLineStart;
			pCastExpr->m_line = prev.m_line;

			Consume(TokenType::RightParen, "Expected ')' after cast target type");

			pCastExpr->m_pExprToCast = ParseUnary();
			return pCastExpr;
		}
		// Unary maths operator
		else {

			Ast::Unary* pUnaryExpr = (Ast::Unary*)pAllocator->Allocate(sizeof(Ast::Unary));
			pUnaryExpr->m_nodeKind = Ast::NodeType::Unary;

			pUnaryExpr->m_pLocation = prev.m_pLocation;
			pUnaryExpr->m_pLineStart = prev.m_pLineStart;
			pUnaryExpr->m_line = prev.m_line;

			if (prev.m_type == TokenType::Minus)
				pUnaryExpr->m_operator = Operator::UnaryMinus;
			else if (prev.m_type == TokenType::Bang)
				pUnaryExpr->m_operator = Operator::Not;
			pUnaryExpr->m_pRight = ParseUnary();

			return (Ast::Expression*)pUnaryExpr;
		}
    }
    return ParseCall();
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseMulDiv() {
    Ast::Expression* pExpr = ParseUnary();

    while (Match(2, TokenType::Star, TokenType::Slash)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->m_nodeKind = Ast::NodeType::Binary;

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
        pBinaryExpr->m_nodeKind = Ast::NodeType::Binary;

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
        pBinaryExpr->m_nodeKind = Ast::NodeType::Binary;

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
        pBinaryExpr->m_nodeKind = Ast::NodeType::Binary;

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
        pBinaryExpr->m_nodeKind = Ast::NodeType::Binary;

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
        pBinaryExpr->m_nodeKind = Ast::NodeType::Binary;

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

Ast::Expression* ParsingState::ParseVarAssignment() {
    Ast::Expression* pExpr = ParseLogicOr();

    if (Match(1, TokenType::Equal)) {
        Token equal = Previous();
        Ast::Expression* pAssignment = ParseVarAssignment();

        if (pExpr->m_nodeKind == Ast::NodeType::Identifier) {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pAllocator->Allocate(sizeof(Ast::VariableAssignment));
            pVarAssignment->m_nodeKind = Ast::NodeType::VariableAssignment;

            pVarAssignment->m_identifier = ((Ast::Identifier*)pExpr)->m_identifier;
            pVarAssignment->m_pAssignment = pAssignment;

            pVarAssignment->m_pLocation = equal.m_pLocation;
            pVarAssignment->m_pLineStart = equal.m_pLineStart;
            pVarAssignment->m_line = equal.m_line;
            return pVarAssignment;
        }
        m_pErrorState->PushError(equal.m_pLocation, equal.m_pLineStart, equal.m_line, "Expression preceding assignment operator is not a variable we can assign to");
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseExpression() {
    return ParseVarAssignment();
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseStatement() {

    if (Match(1, TokenType::If)) {
        return ParseIf();
    }
    if (Match(1, TokenType::While)) {
        return ParseWhile();
    }
    if (Match(1, TokenType::LeftBrace)) {
        return ParseBlock();
    }
    if (Match(1, TokenType::Return)) {
        return ParseReturn();
    }

    Ast::Statement* pStmt = nullptr;
    if (Match(1, TokenType::Identifier)) {
        if (strncmp("print", Previous().m_pLocation, 5) == 0) {
            pStmt = ParsePrint();
        } else {
            m_pCurrent--;
        }
    } 
    
    if (pStmt == nullptr) {
        if (Match(8, TokenType::Identifier, TokenType::LiteralString, TokenType::LiteralInteger, TokenType::LiteralBool, TokenType::LiteralFloat, TokenType::LeftParen, TokenType::Bang, TokenType::Minus)) {
            m_pCurrent--;
            pStmt = ParseExpressionStmt();
        }
        else if (Match(1, TokenType::Semicolon)) {
            return nullptr;
        }
        else {
            PushError("Unable to parse statement");
        }
    }

    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseExpressionStmt() {
    Ast::Expression* pExpr = ParseExpression();
    Consume(TokenType::Semicolon, "Expected \";\" at the end of this statement");

    Ast::ExpressionStmt* pStmt = (Ast::ExpressionStmt*)pAllocator->Allocate(sizeof(Ast::ExpressionStmt));
    pStmt->m_nodeKind = Ast::NodeType::ExpressionStmt;

    pStmt->m_pExpr = pExpr;

    pStmt->m_pLocation = Previous().m_pLocation;
    pStmt->m_pLineStart = Previous().m_pLineStart;
    pStmt->m_line = Previous().m_line;
    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseIf() {
    Ast::If* pIf = (Ast::If*)pAllocator->Allocate(sizeof(Ast::If));
    pIf->m_nodeKind = Ast::NodeType::If;

    pIf->m_pLocation = Previous().m_pLocation;
    pIf->m_pLineStart = Previous().m_pLineStart;
    pIf->m_line = Previous().m_line;

    pIf->m_pCondition = ParseExpression();
    pIf->m_pThenStmt = ParseStatement();

    if (Match(1, TokenType::Else)) {
        pIf->m_pElseStmt = ParseStatement();
    }
    return pIf;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseWhile() {
    Ast::While* pWhile = (Ast::While*)pAllocator->Allocate(sizeof(Ast::While));
    pWhile->m_nodeKind = Ast::NodeType::While;

    pWhile->m_pLocation = Previous().m_pLocation;
    pWhile->m_pLineStart = Previous().m_pLineStart;
    pWhile->m_line = Previous().m_line;

    pWhile->m_pCondition = ParseExpression();

    pWhile->m_pBody = ParseStatement();

    return pWhile;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParsePrint() {
    Consume(TokenType::LeftParen, "Expected \"(\" following print, before the expression starts");
    Ast::Expression* pExpr = ParseExpression();
    Consume(TokenType::RightParen, "Expected \")\" to close print expression");
    Consume(TokenType::Semicolon, "Expected \";\" at the end of this statement");
    
    Ast::Print* pPrintStmt = (Ast::Print*)pAllocator->Allocate(sizeof(Ast::Print));
    pPrintStmt->m_nodeKind = Ast::NodeType::Print;

    pPrintStmt->m_pExpr = pExpr;

    pPrintStmt->m_pLocation = Previous().m_pLocation;
    pPrintStmt->m_pLineStart = Previous().m_pLineStart;
    pPrintStmt->m_line = Previous().m_line;
    return pPrintStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseReturn() {
    Ast::Return* pReturnStmt = (Ast::Return*)pAllocator->Allocate(sizeof(Ast::Return));
    pReturnStmt->m_nodeKind = Ast::NodeType::Return;

    if (!Check(TokenType::Semicolon)) {
        pReturnStmt->m_pExpr = ParseExpression();
    } else {
        pReturnStmt->m_pExpr = nullptr;     
    }
    Consume(TokenType::Semicolon, "Expected \";\" at the end of this statement");

    pReturnStmt->m_pLocation = Previous().m_pLocation;
    pReturnStmt->m_pLineStart = Previous().m_pLineStart;
    pReturnStmt->m_line = Previous().m_line;
    return pReturnStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseBlock() {
    Ast::Block* pBlock = (Ast::Block*)pAllocator->Allocate(sizeof(Ast::Block));
    pBlock->m_nodeKind = Ast::NodeType::Block;
    pBlock->m_declarations.m_pAlloc = pAllocator;
    pBlock->m_startToken = Previous();

    pBlock->m_line = pBlock->m_startToken.m_line;
    pBlock->m_pLineStart = pBlock->m_startToken.m_pLineStart;
    pBlock->m_pLocation = pBlock->m_startToken.m_pLocation;

    while (!Check(TokenType::RightBrace) && !IsAtEnd()) {
        pBlock->m_declarations.PushBack(ParseDeclaration());
    }
    Consume(TokenType::RightBrace, "Expected '}' to end this block");
    pBlock->m_endToken = Previous();
    return pBlock;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseDeclaration() {
    Ast::Statement* pStmt = nullptr;

    if (Match(1, TokenType::Identifier)) {
        Token identifier = Previous();

        if (Match(1, TokenType::Colon)) {
            // We now know we are dealing with a declaration of some kind
            Ast::Declaration* pDecl = (Ast::Declaration*)pAllocator->Allocate(sizeof(Ast::Declaration));
            pDecl->m_nodeKind = Ast::NodeType::Declaration;
            pDecl->m_identifier = CopyCStringRange(identifier.m_pLocation, identifier.m_pLocation + identifier.m_length, pAllocator);

            // Optionally Parse type 
            if (Peek().m_type != TokenType::Equal) {
                pDecl->m_pDeclaredType = (Ast::Type*)ParseType();

                if (pDecl->m_pDeclaredType == nullptr)
                    PushError("Expected a type here, potentially missing an equal sign before an initializer?");
            } else {
                pDecl->m_pDeclaredType = nullptr;
            }

            // Parse initializer
            bool isFunc = false;
            if (Match(1, TokenType::Equal)) {
                pDecl->m_pInitializerExpr = ParseExpression();
                if (pDecl->m_pInitializerExpr && pDecl->m_pInitializerExpr->m_nodeKind == Ast::NodeType::Function) {  // Required for recursion, function will be able to refer to itself
                    Ast::Function* pFunc = (Ast::Function*)pDecl->m_pInitializerExpr;
                    pFunc->m_identifier = pDecl->m_identifier;
                    isFunc = true;
                }
            }

            if (!isFunc)
                Consume(TokenType::Semicolon, "Expected \";\" at the end of this declaration");

            if (pDecl) {
                pDecl->m_pLocation = identifier.m_pLocation;
                pDecl->m_line = identifier.m_line;
                pDecl->m_pLineStart = identifier.m_pLineStart;
            }
            pStmt = pDecl;
        } else {
            // Wasn't a declaration, backtrack
            m_pCurrent--;
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

void DebugStatement(Ast::Statement* pStmt, int indentationLevel) {
    switch (pStmt->m_nodeKind) {
        case Ast::NodeType::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;
            Log::Debug("%*s+ Decl (%s)", indentationLevel, "", pDecl->m_identifier.m_pData);
            if (pDecl->m_pDeclaredType) {
                String typeStr = "none";
                if (pDecl->m_pDeclaredType->m_pResolvedType) {
                    typeStr = pDecl->m_pDeclaredType->m_pResolvedType->name;
                }
                Log::Debug("%*s  Type: %s", indentationLevel + 2, "", typeStr.m_pData);
            }
            else if (pDecl->m_pInitializerExpr)
                Log::Debug("%*s  Type: inferred as %s", indentationLevel + 2, "", pDecl->m_pInitializerExpr->m_pType ? pDecl->m_pInitializerExpr->m_pType->name.m_pData : "none");

            if (pDecl->m_pInitializerExpr) {
                DebugExpression(pDecl->m_pInitializerExpr, indentationLevel + 2);
            }
            break;
        }
        case Ast::NodeType::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            Log::Debug("%*s> PrintStmt", indentationLevel, "");
            DebugExpression(pPrint->m_pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            Log::Debug("%*s> ReturnStmt", indentationLevel, "");
            if (pReturn->m_pExpr)
                DebugExpression(pReturn->m_pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            Log::Debug("%*s> ExpressionStmt", indentationLevel, "");
            DebugExpression(pExprStmt->m_pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            Log::Debug("%*s> If", indentationLevel, "");
            DebugExpression(pIf->m_pCondition, indentationLevel + 2);
            DebugStatement(pIf->m_pThenStmt, indentationLevel + 2);
            if (pIf->m_pElseStmt)
                DebugStatement(pIf->m_pElseStmt, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            Log::Debug("%*s> While", indentationLevel, "");
            DebugExpression(pWhile->m_pCondition, indentationLevel + 2);
            DebugStatement(pWhile->m_pBody, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            Log::Debug("%*s> Block", indentationLevel, "");
            DebugStatements(pBlock->m_declarations, indentationLevel + 2);
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void DebugStatements(ResizableArray<Ast::Statement*>& statements, int indentationLevel) {
    for (size_t i = 0; i < statements.m_count; i++) {
        Ast::Statement* pStmt = statements[i];
        DebugStatement(pStmt, indentationLevel);
    }
}

// ***********************************************************************

void DebugExpression(Ast::Expression* pExpr, int indentationLevel) {
    if (pExpr == nullptr) {
        Log::Debug("%*s- NULL", indentationLevel, "");
        return;
    }

    switch (pExpr->m_nodeKind) {
        case Ast::NodeType::Identifier: {
            Ast::Identifier* pIdentifier = (Ast::Identifier*)pExpr;
            String nodeTypeStr = "none";
            if (pIdentifier->m_pType) {
                nodeTypeStr = pIdentifier->m_pType->name;
            }
            
            Log::Debug("%*s- Identifier (%s:%s)", indentationLevel, "", pIdentifier->m_identifier.m_pData, nodeTypeStr.m_pData);
            break;
        }
        case Ast::NodeType::FnType: 
        case Ast::NodeType::Type: {
            Ast::Type* pType = (Ast::Type*)pExpr;
            if (pType->m_pType && pType->m_pResolvedType)
                Log::Debug("%*s- Type Literal (%s:%s)", indentationLevel, "", pType->m_pResolvedType->name.m_pData, pType->m_pType->name.m_pData);
            break;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pAssignment = (Ast::VariableAssignment*)pExpr;
            String nodeTypeStr = "none";
            if (pAssignment->m_pType) {
                nodeTypeStr = pAssignment->m_pType->name;
            }

            Log::Debug("%*s- Variable Assignment (%s:%s)", indentationLevel, "", pAssignment->m_identifier.m_pData, nodeTypeStr.m_pData);
            DebugExpression(pAssignment->m_pAssignment, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            String nodeTypeStr = "none";
            if (pLiteral->m_pType) {
                nodeTypeStr = pLiteral->m_pType->name;
            }

            if (pLiteral->m_value.m_pType == GetF32Type())
                Log::Debug("%*s- Literal (%f:%s)", indentationLevel, "", pLiteral->m_value.m_f32Value, nodeTypeStr.m_pData);
            else if (pLiteral->m_value.m_pType == GetI32Type())
                Log::Debug("%*s- Literal (%i:%s)", indentationLevel, "", pLiteral->m_value.m_i32Value, nodeTypeStr.m_pData);
            else if (pLiteral->m_value.m_pType == GetBoolType())
                Log::Debug("%*s- Literal (%s:%s)", indentationLevel, "", pLiteral->m_value.m_boolValue ? "true" : "false", nodeTypeStr.m_pData);
            break;
        }
        case Ast::NodeType::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            Log::Debug("%*s- Function", indentationLevel, "");
            for (Ast::Declaration* pParam : pFunction->m_params) {
                String typeStr = "none";
                if (pParam->m_pResolvedType) {
                    typeStr = pParam->m_pResolvedType->name;
                }

                Log::Debug("%*s- Param (%s:%s)", indentationLevel + 2, "", pParam->m_identifier.m_pData, typeStr.m_pData);
            }
            DebugStatement(pFunction->m_pBody, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            String typeStr = "none";
            if (pGroup->m_pType) {
                typeStr = pGroup->m_pType->name;
            }
            Log::Debug("%*s- Group (:%s)", indentationLevel, "", typeStr.m_pData);
            DebugExpression(pGroup->m_pExpression, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            String nodeTypeStr = "none";
            if (pBinary->m_pType) {
                nodeTypeStr = pBinary->m_pType->name;
            }
            switch (pBinary->m_operator) {
                case Operator::Add:
                    Log::Debug("%*s- Binary (+:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::Subtract:
                    Log::Debug("%*s- Binary (-:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::Divide:
                    Log::Debug("%*s- Binary (/:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::Multiply:
                    Log::Debug("%*s- Binary (*:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::Greater:
                    Log::Debug("%*s- Binary (>:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::Less:
                    Log::Debug("%*s- Binary (<:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::GreaterEqual:
                    Log::Debug("%*s- Binary (>=:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::LessEqual:
                    Log::Debug("%*s- Binary (<=:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::Equal:
                    Log::Debug("%*s- Binary (==:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::NotEqual:
                    Log::Debug("%*s- Binary (!=:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::And:
                    Log::Debug("%*s- Binary (&&:%s)", indentationLevel, "", nodeTypeStr.m_pData);
                    break;
                case Operator::Or:
                    Log::Debug("%*s- Binary (||:%s)", indentationLevel, "", nodeTypeStr.m_pData);
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
                    Log::Debug("%*s- Unary (-:%s)", indentationLevel, "", pUnary->m_pType->name.m_pData);
                    break;
                case Operator::Not:
                    Log::Debug("%*s- Unary (!:%s)", indentationLevel, "", pUnary->m_pType->name.m_pData);
                    break;
                default:
                    break;
            }
            DebugExpression(pUnary->m_pRight, indentationLevel + 2);
            break;
        }
		case Ast::NodeType::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;

			Log::Debug("%*s- Cast (:%s)", indentationLevel, "", pCast->m_pType->name.m_pData);
			DebugExpression(pCast->m_pTargetType, indentationLevel + 2);
			DebugExpression(pCast->m_pExprToCast, indentationLevel + 2);
			break;
		}
        case Ast::NodeType::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;

            DebugExpression(pCall->m_pCallee, indentationLevel);
            Log::Debug("%*s- Call", indentationLevel + 2, "");

            for (Ast::Expression* pArg : pCall->m_args) {
                DebugExpression(pArg, indentationLevel + 4);
            }
            break;
        }
        default:
            break;
    }
}