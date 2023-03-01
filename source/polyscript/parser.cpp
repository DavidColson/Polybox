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
    pAlloc = pAlloc;
    errors.pAlloc = pAlloc;
}

// ***********************************************************************

void ErrorState::PushError(Ast::Node* pNode, const char* formatMessage, ...) {
    StringBuilder builder(pAlloc);
    va_list args;
    va_start(args, formatMessage);
    builder.AppendFormatInternal(formatMessage, args);
    va_end(args);

    Assert(pNode != nullptr);

    Error er;
    er.pLocation = pNode->pLocation;
    er.line = pNode->line;
    er.pLineStart = pNode->pLineStart;

    er.message = builder.CreateString();
    errors.PushBack(er);
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
    StringBuilder builder(pAlloc);
    builder.AppendFormatInternal(formatMessage, args);

	Error er;
	er.pLocation = pLocation;
	er.line = line;
	er.pLineStart = pLineStart;

    er.message = builder.CreateString();

    errors.PushBack(er);
}

// ***********************************************************************

bool ErrorState::ReportCompilationResult() {
    bool success = errors.count == 0;
    if (!success) {
        Log::Info("Compilation failed with %i errors", errors.count);

        StringBuilder builder;

        for (size_t i = 0; i < errors.count; i++) {
            Error& err = errors[i];
            builder.AppendFormat("Error At: filename:%i:%i\n", err.line, err.pLocation - err.pLineStart);

            uint32_t lineNumberSpacing = uint32_t(log10f((float)err.line) + 1);
            builder.AppendFormat("%*s|\n", lineNumberSpacing + 2, "");

            String line;
            char* lineEnd;
            if (lineEnd = strchr(err.pLineStart, '\n')) {
                line = CopyCStringRange(err.pLineStart, lineEnd);
            } else if (lineEnd = strchr(err.pLineStart, '\0')) {
                line = CopyCStringRange(err.pLineStart, lineEnd);
            }
            defer(FreeString(line));
            builder.AppendFormat(" %i | %s\n", err.line, line.pData);

            int32_t errorAtColumn = int32_t(err.pLocation - err.pLineStart);
            builder.AppendFormat("%*s|%*s^\n", lineNumberSpacing + 2, "", errorAtColumn + 1, "");
            builder.AppendFormat("%s\n", err.message.pData);

            String output = builder.CreateString();
            Log::Info("%s", output.pData);
        }
    } else {
        Log::Info("Compilation Succeeded");
    }
    return success;
}


// ParsingState

// ***********************************************************************

Token ParsingState::Previous() {
    return *(pCurrent - 1);
}

// ***********************************************************************

Token ParsingState::Advance() {
    pCurrent++;
    return Previous();
}

// ***********************************************************************

bool ParsingState::IsAtEnd() {
    return pCurrent >= pTokensEnd;
}

// ***********************************************************************

Token ParsingState::Peek() {
    return *pCurrent;
}

// ***********************************************************************

bool ParsingState::Check(TokenType::Enum type) {
    if (IsAtEnd())
        return false;

    return Peek().type == type;
}
    
// ***********************************************************************

Token ParsingState::Consume(TokenType::Enum type, String message) {
    if (Check(type))
        return Advance();

    PushError(message.pData);
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
    if (panicMode)
        return;

    panicMode = true;

    va_list args;
    va_start(args, formatMessage);
    pErrorState->PushError(pCurrent->pLocation, pCurrent->pLineStart, pCurrent->line, formatMessage, args);
    va_end(args);
}

// ***********************************************************************

void ParsingState::Synchronize() {
    panicMode = false;

    bool waitForBlock = false;
    while (pCurrent->type != TokenType::EndOfFile) {
        if (pCurrent->type == TokenType::LeftBrace) {
            waitForBlock = true;
        }

        if (waitForBlock) {
            if (pCurrent->type == TokenType::RightBrace) {
                Advance();
                return;
            }
            Advance();
        } else {
            if (pCurrent->type == TokenType::Semicolon) {
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
        pType->nodeKind = Ast::NodeType::Type;
        pType->identifier = CopyCStringRange(identifier.pLocation, identifier.pLocation + identifier.length, pAllocator);
        pType->pLocation = identifier.pLocation;
        pType->pLineStart = identifier.pLineStart;
        pType->line = identifier.line;

        return pType;
    }

    if (Match(1, TokenType::Fn)) {
        Token fn = Previous();

        Ast::FnType* pFnType = (Ast::FnType*)pAllocator->Allocate(sizeof(Ast::FnType));
        pFnType->nodeKind = Ast::NodeType::FnType;
        pFnType->params.pAlloc = pAllocator;

        Consume(TokenType::LeftParen, "Expected left parenthesis to start function signature");

        if (Peek().type != TokenType::RightParen) {
            do {
                pFnType->params.PushBack((Ast::Type*)ParseType());

                if (Peek().type == TokenType::Colon) {
					PushError("Expected a function signature, but this looks like a function header. Potentially replace 'fn' with 'func' from start of expression");
                    return nullptr;
                }
            } while (Match(1, TokenType::Comma));
        }
        Consume(TokenType::RightParen, "Expected right parenthesis to close argument list");

        // Parse return type
        if (Match(1, TokenType::FuncSigReturn)) {
            pFnType->pReturnType = (Ast::Type*)ParseType();
        } else {
            pFnType->pReturnType = (Ast::Type*)pAllocator->Allocate(sizeof(Ast::Type));
            pFnType->pReturnType->nodeKind = Ast::NodeType::Identifier;
            pFnType->pReturnType->identifier = CopyCString("void", pAllocator);
            pFnType->pReturnType->pType = GetVoidType();
            pFnType->pReturnType->pLocation = Previous().pLocation;
            pFnType->pReturnType->pLineStart = Previous().pLineStart;
            pFnType->pReturnType->line = Previous().line;
        }

        pFnType->pLocation = fn.pLocation;
        pFnType->pLineStart = fn.pLineStart;
        pFnType->line = fn.line;
        return pFnType;
    }

    return nullptr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParsePrimary() {

	if (Match(1, TokenType::Func)) {
		Token fn = Previous();

		Ast::Function* pFunc = (Ast::Function*)pAllocator->Allocate(sizeof(Ast::Function));
		pFunc->nodeKind = Ast::NodeType::Function;
		pFunc->params.pAlloc = pAllocator;

		Consume(TokenType::LeftParen, "Expected left parenthesis to start function param list");

		if (Peek().type != TokenType::RightParen) {
			// Parse parameter list
			do {
				Token arg = Consume(TokenType::Identifier, "Expected argument identifier after comma");
				Consume(TokenType::Colon, "Expected colon after argument identifier");

				Ast::Declaration* pParamDecl = (Ast::Declaration*)pAllocator->Allocate(sizeof(Ast::Declaration));
				pParamDecl->nodeKind = Ast::NodeType::Declaration;
				pParamDecl->identifier = CopyCStringRange(arg.pLocation, arg.pLocation + arg.length, pAllocator);
				pParamDecl->pDeclaredType = (Ast::Type*)ParseType();
				pParamDecl->pLocation = arg.pLocation;
				pParamDecl->line = arg.line;
				pParamDecl->pLineStart = arg.pLineStart;

				pFunc->params.PushBack(pParamDecl);
			} while (Match(1, TokenType::Comma));
		}
		Consume(TokenType::RightParen, "Expected right parenthesis to close argument list");

		// Parse return type
		if (Match(1, TokenType::FuncSigReturn)) {
			pFunc->pReturnType = (Ast::Type*)ParseType();
		} else {
			pFunc->pReturnType = (Ast::Type*)pAllocator->Allocate(sizeof(Ast::Type));
			pFunc->pReturnType->nodeKind = Ast::NodeType::Identifier;
			pFunc->pReturnType->identifier = CopyCString("void", pAllocator);
			pFunc->pReturnType->pType = GetVoidType();
			pFunc->pReturnType->pLocation = Previous().pLocation;
			pFunc->pReturnType->pLineStart = Previous().pLineStart;
			pFunc->pReturnType->line = Previous().line;
		}

		if (Match(1, TokenType::LeftBrace)) {
			pFunc->pBody = (Ast::Block*)ParseBlock();
		} else {
			PushError("Expected '{' to open function body");
		}
			
		pFunc->pLocation = fn.pLocation;
		pFunc->pLineStart = fn.pLineStart;
		pFunc->line = fn.line;
		return pFunc;
	}

    if (Match(1, TokenType::LiteralInteger)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->nodeKind = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->pLocation = token.pLocation;
        pLiteralExpr->pLineStart = token.pLineStart;
        pLiteralExpr->line = token.line;

        char* endPtr = token.pLocation + token.length;
        pLiteralExpr->value = MakeValue((int32_t)strtol(token.pLocation, &endPtr, 10));
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LiteralFloat)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->nodeKind = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->pLocation = token.pLocation;
        pLiteralExpr->pLineStart = token.pLineStart;
        pLiteralExpr->line = token.line;

        char* endPtr = token.pLocation + token.length;
        pLiteralExpr->value = MakeValue((float)strtod(token.pLocation, &endPtr));
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LiteralBool)) {
        Ast::Literal* pLiteralExpr = (Ast::Literal*)pAllocator->Allocate(sizeof(Ast::Literal));
        pLiteralExpr->nodeKind = Ast::NodeType::Literal;

        Token token = Previous();
        pLiteralExpr->pLocation = token.pLocation;
        pLiteralExpr->pLineStart = token.pLineStart;
        pLiteralExpr->line = token.line;

        char* endPtr = token.pLocation + token.length;
        if (strncmp("true", token.pLocation, 4) == 0)
            pLiteralExpr->value = MakeValue(true);
        else if (strncmp("false", token.pLocation, 4) == 0)
            pLiteralExpr->value = MakeValue(true);
        return pLiteralExpr;
    }

    if (Match(1, TokenType::LeftParen)) {
        Token startToken = Previous();
        Ast::Expression* pExpr = ParseExpression();
        Consume(TokenType::RightParen, "Expected a closing right parenthesis \")\", but found nothing in this expression");

        if (pExpr) {
            Ast::Grouping* pGroupExpr = (Ast::Grouping*)pAllocator->Allocate(sizeof(Ast::Grouping));
            pGroupExpr->nodeKind = Ast::NodeType::Grouping;

            pGroupExpr->pLocation = startToken.pLocation;
            pGroupExpr->pLineStart = startToken.pLineStart;
            pGroupExpr->line = startToken.line;

            pGroupExpr->pExpression = pExpr;
            return pGroupExpr;
        }
        PushError("Expected valid expression inside parenthesis, but found nothing");
        return nullptr;
    }

    if (Match(1, TokenType::Identifier)) { // Note this could now be a primitive type, need to account for this in the parser structure and typechecker
        Token identifier = Previous();

        // Should become identifier with a type, variable or type or something like that
        Ast::Identifier* pIdentifier = (Ast::Identifier*)pAllocator->Allocate(sizeof(Ast::Identifier));
        pIdentifier->nodeKind = Ast::NodeType::Identifier;
        pIdentifier->identifier = CopyCStringRange(identifier.pLocation, identifier.pLocation + identifier.length, pAllocator);

        pIdentifier->pLocation = identifier.pLocation;
        pIdentifier->pLineStart = identifier.pLineStart;
        pIdentifier->line = identifier.line;

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
        pCall->nodeKind = Ast::NodeType::Call;
        pCall->pCallee = pExpr;
        pCall->args.pAlloc = pAllocator;

        if (!Check(TokenType::RightParen)) {
            do {
                pCall->args.PushBack(ParseExpression());
            } while (Match(1, TokenType::Comma));
        }

        Token closeParen = Consume(TokenType::RightParen, "Expected right parenthesis to end function call");
        
        pCall->pLocation = closeParen.pLocation;
        pCall->pLineStart = closeParen.pLineStart;
        pCall->line = closeParen.line;

        pExpr = pCall;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseUnary() {
    while (Match(3, TokenType::Minus, TokenType::Bang, TokenType::As)) {
		Token prev = Previous();

		// Cast Operator
		if (prev.type == TokenType::As) {
			Consume(TokenType::LeftParen, "Expected '(' before cast target type");

			Ast::Cast* pCastExpr = (Ast::Cast*)pAllocator->Allocate(sizeof(Ast::Cast));
			pCastExpr->nodeKind = Ast::NodeType::Cast;
			pCastExpr->pTargetType = (Ast::Type*)ParseType();
			pCastExpr->pLocation = prev.pLocation;
			pCastExpr->pLineStart = prev.pLineStart;
			pCastExpr->line = prev.line;

			Consume(TokenType::RightParen, "Expected ')' after cast target type");

			pCastExpr->pExprToCast = ParseUnary();
			return pCastExpr;
		}
		// Unary maths op
		else {

			Ast::Unary* pUnaryExpr = (Ast::Unary*)pAllocator->Allocate(sizeof(Ast::Unary));
			pUnaryExpr->nodeKind = Ast::NodeType::Unary;

			pUnaryExpr->pLocation = prev.pLocation;
			pUnaryExpr->pLineStart = prev.pLineStart;
			pUnaryExpr->line = prev.line;

			if (prev.type == TokenType::Minus)
				pUnaryExpr->op = Operator::UnaryMinus;
			else if (prev.type == TokenType::Bang)
				pUnaryExpr->op = Operator::Not;
			pUnaryExpr->pRight = ParseUnary();

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
        pBinaryExpr->nodeKind = Ast::NodeType::Binary;

        Token opToken = Previous();
        pBinaryExpr->pLocation = opToken.pLocation;
        pBinaryExpr->pLineStart = opToken.pLineStart;
        pBinaryExpr->line = opToken.line;

        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
        pBinaryExpr->pRight = ParseUnary();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseAddSub() {
    Ast::Expression* pExpr = ParseMulDiv();

    while (Match(2, TokenType::Minus, TokenType::Plus)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->nodeKind = Ast::NodeType::Binary;

        Token opToken = Previous();
        pBinaryExpr->pLocation = opToken.pLocation;
        pBinaryExpr->pLineStart = opToken.pLineStart;
        pBinaryExpr->line = opToken.line;

        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
        pBinaryExpr->pRight = ParseMulDiv();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseComparison() {
    Ast::Expression* pExpr = ParseAddSub();

    while (Match(4, TokenType::Greater, TokenType::Less, TokenType::GreaterEqual, TokenType::LessEqual)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->nodeKind = Ast::NodeType::Binary;

        Token opToken = Previous();
        pBinaryExpr->pLocation = opToken.pLocation;
        pBinaryExpr->pLineStart = opToken.pLineStart;
        pBinaryExpr->line = opToken.line;

        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
        pBinaryExpr->pRight = ParseAddSub();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseEquality() {
    Ast::Expression* pExpr = ParseComparison();

    while (Match(2, TokenType::EqualEqual, TokenType::BangEqual)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->nodeKind = Ast::NodeType::Binary;

        Token opToken = Previous();
        pBinaryExpr->pLocation = opToken.pLocation;
        pBinaryExpr->pLineStart = opToken.pLineStart;
        pBinaryExpr->line = opToken.line;

        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
        pBinaryExpr->pRight = ParseComparison();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseLogicAnd() {
    Ast::Expression* pExpr = ParseEquality();

    while (Match(1, TokenType::And)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->nodeKind = Ast::NodeType::Binary;

        Token opToken = Previous();
        pBinaryExpr->pLocation = opToken.pLocation;
        pBinaryExpr->pLineStart = opToken.pLineStart;
        pBinaryExpr->line = opToken.line;

        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
        pBinaryExpr->pRight = ParseEquality();

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParsingState::ParseLogicOr() {
    Ast::Expression* pExpr = ParseLogicAnd();

    while (Match(1, TokenType::Or)) {
        Ast::Binary* pBinaryExpr = (Ast::Binary*)pAllocator->Allocate(sizeof(Ast::Binary));
        pBinaryExpr->nodeKind = Ast::NodeType::Binary;

        Token opToken = Previous();
        pBinaryExpr->pLocation = opToken.pLocation;
        pBinaryExpr->pLineStart = opToken.pLineStart;
        pBinaryExpr->line = opToken.line;

        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
        pBinaryExpr->pRight = ParseLogicAnd();

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

        if (pExpr->nodeKind == Ast::NodeType::Identifier) {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pAllocator->Allocate(sizeof(Ast::VariableAssignment));
            pVarAssignment->nodeKind = Ast::NodeType::VariableAssignment;

            pVarAssignment->identifier = ((Ast::Identifier*)pExpr)->identifier;
            pVarAssignment->pAssignment = pAssignment;

            pVarAssignment->pLocation = equal.pLocation;
            pVarAssignment->pLineStart = equal.pLineStart;
            pVarAssignment->line = equal.line;
            return pVarAssignment;
        }
        pErrorState->PushError(equal.pLocation, equal.pLineStart, equal.line, "Expression preceding assignment op is not a variable we can assign to");
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
        if (strncmp("print", Previous().pLocation, 5) == 0) {
            pStmt = ParsePrint();
        } else {
            pCurrent--;
        }
    } 
    
    if (pStmt == nullptr) {
        if (Match(8, TokenType::Identifier, TokenType::LiteralString, TokenType::LiteralInteger, TokenType::LiteralBool, TokenType::LiteralFloat, TokenType::LeftParen, TokenType::Bang, TokenType::Minus)) {
            pCurrent--;
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
    pStmt->nodeKind = Ast::NodeType::ExpressionStmt;

    pStmt->pExpr = pExpr;

    pStmt->pLocation = Previous().pLocation;
    pStmt->pLineStart = Previous().pLineStart;
    pStmt->line = Previous().line;
    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseIf() {
    Ast::If* pIf = (Ast::If*)pAllocator->Allocate(sizeof(Ast::If));
    pIf->nodeKind = Ast::NodeType::If;

    pIf->pLocation = Previous().pLocation;
    pIf->pLineStart = Previous().pLineStart;
    pIf->line = Previous().line;

    pIf->pCondition = ParseExpression();
    pIf->pThenStmt = ParseStatement();

    if (Match(1, TokenType::Else)) {
        pIf->pElseStmt = ParseStatement();
    }
    return pIf;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseWhile() {
    Ast::While* pWhile = (Ast::While*)pAllocator->Allocate(sizeof(Ast::While));
    pWhile->nodeKind = Ast::NodeType::While;

    pWhile->pLocation = Previous().pLocation;
    pWhile->pLineStart = Previous().pLineStart;
    pWhile->line = Previous().line;

    pWhile->pCondition = ParseExpression();

    pWhile->pBody = ParseStatement();

    return pWhile;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParsePrint() {
    Consume(TokenType::LeftParen, "Expected \"(\" following print, before the expression starts");
    Ast::Expression* pExpr = ParseExpression();
    Consume(TokenType::RightParen, "Expected \")\" to close print expression");
    Consume(TokenType::Semicolon, "Expected \";\" at the end of this statement");
    
    Ast::Print* pPrintStmt = (Ast::Print*)pAllocator->Allocate(sizeof(Ast::Print));
    pPrintStmt->nodeKind = Ast::NodeType::Print;

    pPrintStmt->pExpr = pExpr;

    pPrintStmt->pLocation = Previous().pLocation;
    pPrintStmt->pLineStart = Previous().pLineStart;
    pPrintStmt->line = Previous().line;
    return pPrintStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseReturn() {
    Ast::Return* pReturnStmt = (Ast::Return*)pAllocator->Allocate(sizeof(Ast::Return));
    pReturnStmt->nodeKind = Ast::NodeType::Return;

    if (!Check(TokenType::Semicolon)) {
        pReturnStmt->pExpr = ParseExpression();
    } else {
        pReturnStmt->pExpr = nullptr;     
    }
    Consume(TokenType::Semicolon, "Expected \";\" at the end of this statement");

    pReturnStmt->pLocation = Previous().pLocation;
    pReturnStmt->pLineStart = Previous().pLineStart;
    pReturnStmt->line = Previous().line;
    return pReturnStmt;
}

// ***********************************************************************

Ast::Statement* ParsingState::ParseBlock() {
    Ast::Block* pBlock = (Ast::Block*)pAllocator->Allocate(sizeof(Ast::Block));
    pBlock->nodeKind = Ast::NodeType::Block;
    pBlock->declarations.pAlloc = pAllocator;
    pBlock->startToken = Previous();

    pBlock->line = pBlock->startToken.line;
    pBlock->pLineStart = pBlock->startToken.pLineStart;
    pBlock->pLocation = pBlock->startToken.pLocation;

    while (!Check(TokenType::RightBrace) && !IsAtEnd()) {
        pBlock->declarations.PushBack(ParseDeclaration());
    }
    Consume(TokenType::RightBrace, "Expected '}' to end this block");
    pBlock->endToken = Previous();
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
            pDecl->nodeKind = Ast::NodeType::Declaration;
            pDecl->identifier = CopyCStringRange(identifier.pLocation, identifier.pLocation + identifier.length, pAllocator);

            // Optionally Parse type 
            if (Peek().type != TokenType::Equal) {
                pDecl->pDeclaredType = (Ast::Type*)ParseType();

                if (pDecl->pDeclaredType == nullptr)
                    PushError("Expected a type here, potentially missing an equal sign before an initializer?");
            } else {
                pDecl->pDeclaredType = nullptr;
            }

            // Parse initializer
            bool isFunc = false;
            if (Match(1, TokenType::Equal)) {
                pDecl->pInitializerExpr = ParseExpression();
                if (pDecl->pInitializerExpr && pDecl->pInitializerExpr->nodeKind == Ast::NodeType::Function) {  // Required for recursion, function will be able to refer to itself
                    Ast::Function* pFunc = (Ast::Function*)pDecl->pInitializerExpr;
                    pFunc->identifier = pDecl->identifier;
                    isFunc = true;
                }
            }

            if (!isFunc)
                Consume(TokenType::Semicolon, "Expected \";\" at the end of this declaration");

            if (pDecl) {
                pDecl->pLocation = identifier.pLocation;
                pDecl->line = identifier.line;
                pDecl->pLineStart = identifier.pLineStart;
            }
            pStmt = pDecl;
        } else {
            // Wasn't a declaration, backtrack
            pCurrent--;
        }
    }
    
    if (pStmt == nullptr) {
        pStmt = ParseStatement();
    }

    if (panicMode)
        Synchronize();

    return pStmt;
}

// ***********************************************************************

ResizableArray<Ast::Statement*> ParsingState::InitAndParse(ResizableArray<Token>& tokens, ErrorState* pErrors, IAllocator* pAlloc) {
    pTokensStart = tokens.pData;
    pTokensEnd = tokens.pData + tokens.count - 1;
    pCurrent = tokens.pData;
    pAllocator = pAlloc;
    pErrorState = pErrors;

    ResizableArray<Ast::Statement*> statements;
    statements.pAlloc = pAlloc;

    while (!IsAtEnd()) {
        Ast::Statement* pStmt = ParseDeclaration();
        if (pStmt)
            statements.PushBack(pStmt);
    }

    return statements;
}

// ***********************************************************************

void DebugStatement(Ast::Statement* pStmt, int indentationLevel) {
    switch (pStmt->nodeKind) {
        case Ast::NodeType::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;
            Log::Debug("%*s+ Decl (%s)", indentationLevel, "", pDecl->identifier.pData);
            if (pDecl->pDeclaredType) {
                String typeStr = "none";
                if (pDecl->pDeclaredType->pResolvedType) {
                    typeStr = pDecl->pDeclaredType->pResolvedType->name;
                }
                Log::Debug("%*s  Type: %s", indentationLevel + 2, "", typeStr.pData);
            }
            else if (pDecl->pInitializerExpr)
                Log::Debug("%*s  Type: inferred as %s", indentationLevel + 2, "", pDecl->pInitializerExpr->pType ? pDecl->pInitializerExpr->pType->name.pData : "none");

            if (pDecl->pInitializerExpr) {
                DebugExpression(pDecl->pInitializerExpr, indentationLevel + 2);
            }
            break;
        }
        case Ast::NodeType::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            Log::Debug("%*s> PrintStmt", indentationLevel, "");
            DebugExpression(pPrint->pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            Log::Debug("%*s> ReturnStmt", indentationLevel, "");
            if (pReturn->pExpr)
                DebugExpression(pReturn->pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            Log::Debug("%*s> ExpressionStmt", indentationLevel, "");
            DebugExpression(pExprStmt->pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            Log::Debug("%*s> If", indentationLevel, "");
            DebugExpression(pIf->pCondition, indentationLevel + 2);
            DebugStatement(pIf->pThenStmt, indentationLevel + 2);
            if (pIf->pElseStmt)
                DebugStatement(pIf->pElseStmt, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            Log::Debug("%*s> While", indentationLevel, "");
            DebugExpression(pWhile->pCondition, indentationLevel + 2);
            DebugStatement(pWhile->pBody, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            Log::Debug("%*s> Block", indentationLevel, "");
            DebugStatements(pBlock->declarations, indentationLevel + 2);
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void DebugStatements(ResizableArray<Ast::Statement*>& statements, int indentationLevel) {
    for (size_t i = 0; i < statements.count; i++) {
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

    switch (pExpr->nodeKind) {
        case Ast::NodeType::Identifier: {
            Ast::Identifier* pIdentifier = (Ast::Identifier*)pExpr;
            String nodeTypeStr = "none";
            if (pIdentifier->pType) {
                nodeTypeStr = pIdentifier->pType->name;
            }
            
            Log::Debug("%*s- Identifier (%s:%s)", indentationLevel, "", pIdentifier->identifier.pData, nodeTypeStr.pData);
            break;
        }
        case Ast::NodeType::FnType: 
        case Ast::NodeType::Type: {
            Ast::Type* pType = (Ast::Type*)pExpr;
            if (pType->pType && pType->pResolvedType)
                Log::Debug("%*s- Type Literal (%s:%s)", indentationLevel, "", pType->pResolvedType->name.pData, pType->pType->name.pData);
            break;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pAssignment = (Ast::VariableAssignment*)pExpr;
            String nodeTypeStr = "none";
            if (pAssignment->pType) {
                nodeTypeStr = pAssignment->pType->name;
            }

            Log::Debug("%*s- Variable Assignment (%s:%s)", indentationLevel, "", pAssignment->identifier.pData, nodeTypeStr.pData);
            DebugExpression(pAssignment->pAssignment, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            String nodeTypeStr = "none";
            if (pLiteral->pType) {
                nodeTypeStr = pLiteral->pType->name;
            }

            if (pLiteral->value.pType == GetF32Type())
                Log::Debug("%*s- Literal (%f:%s)", indentationLevel, "", pLiteral->value.f32Value, nodeTypeStr.pData);
            else if (pLiteral->value.pType == GetI32Type())
                Log::Debug("%*s- Literal (%i:%s)", indentationLevel, "", pLiteral->value.i32Value, nodeTypeStr.pData);
            else if (pLiteral->value.pType == GetBoolType())
                Log::Debug("%*s- Literal (%s:%s)", indentationLevel, "", pLiteral->value.boolValue ? "true" : "false", nodeTypeStr.pData);
            break;
        }
        case Ast::NodeType::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            Log::Debug("%*s- Function", indentationLevel, "");
            for (Ast::Declaration* pParam : pFunction->params) {
                String typeStr = "none";
                if (pParam->pResolvedType) {
                    typeStr = pParam->pResolvedType->name;
                }

                Log::Debug("%*s- Param (%s:%s)", indentationLevel + 2, "", pParam->identifier.pData, typeStr.pData);
            }
            DebugStatement(pFunction->pBody, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            String typeStr = "none";
            if (pGroup->pType) {
                typeStr = pGroup->pType->name;
            }
            Log::Debug("%*s- Group (:%s)", indentationLevel, "", typeStr.pData);
            DebugExpression(pGroup->pExpression, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            String nodeTypeStr = "none";
            if (pBinary->pType) {
                nodeTypeStr = pBinary->pType->name;
            }
            switch (pBinary->op) {
                case Operator::Add:
                    Log::Debug("%*s- Binary (+:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::Subtract:
                    Log::Debug("%*s- Binary (-:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::Divide:
                    Log::Debug("%*s- Binary (/:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::Multiply:
                    Log::Debug("%*s- Binary (*:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::Greater:
                    Log::Debug("%*s- Binary (>:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::Less:
                    Log::Debug("%*s- Binary (<:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::GreaterEqual:
                    Log::Debug("%*s- Binary (>=:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::LessEqual:
                    Log::Debug("%*s- Binary (<=:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::Equal:
                    Log::Debug("%*s- Binary (==:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::NotEqual:
                    Log::Debug("%*s- Binary (!=:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::And:
                    Log::Debug("%*s- Binary (&&:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                case Operator::Or:
                    Log::Debug("%*s- Binary (||:%s)", indentationLevel, "", nodeTypeStr.pData);
                    break;
                default:
                    break;
            }
            DebugExpression(pBinary->pLeft, indentationLevel + 2);
            DebugExpression(pBinary->pRight, indentationLevel + 2);
            break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            switch (pUnary->op) {
                case Operator::UnaryMinus:
                    Log::Debug("%*s- Unary (-:%s)", indentationLevel, "", pUnary->pType->name.pData);
                    break;
                case Operator::Not:
                    Log::Debug("%*s- Unary (!:%s)", indentationLevel, "", pUnary->pType->name.pData);
                    break;
                default:
                    break;
            }
            DebugExpression(pUnary->pRight, indentationLevel + 2);
            break;
        }
		case Ast::NodeType::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;

			Log::Debug("%*s- Cast (:%s)", indentationLevel, "", pCast->pType->name.pData);
			DebugExpression(pCast->pTargetType, indentationLevel + 2);
			DebugExpression(pCast->pExprToCast, indentationLevel + 2);
			break;
		}
        case Ast::NodeType::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;

            DebugExpression(pCall->pCallee, indentationLevel);
            Log::Debug("%*s- Call", indentationLevel + 2, "");

            for (Ast::Expression* pArg : pCall->args) {
                DebugExpression(pArg, indentationLevel + 4);
            }
            break;
        }
        default:
            break;
    }
}