// Copyright 2020-2022 David Colson. All rights reserved.

#include "parser.h"

#include "lexer.h"
#include "compiler.h"

#include <log.h>
#include <stdarg.h>
#include <string.h>
#include <resizable_array.inl>
#include <string_builder.h>
#include <defer.h>
#include <maths.h>


struct ParsingState {
	Token* pTokensStart { nullptr };
	Token* pTokensEnd { nullptr };
	Token* pCurrent { nullptr };
	IAllocator* pAllocator { nullptr };
	ErrorState* pErrorState { nullptr };
	bool panicMode { false };
};

template<typename T>
T* MakeNode(IAllocator* pAlloc, Token token, Ast::NodeKind kind) {
    T* pNode = (T*)pAlloc->Allocate(sizeof(T));
    pNode->nodeKind = kind;
    pNode->pLocation = token.pLocation;
    pNode->pLineStart = token.pLineStart;
    pNode->line = token.line;
    return pNode;
}

// ErrorState

// ***********************************************************************

void ErrorState::Init(IAllocator* _pAlloc) {
    pAlloc = _pAlloc;
    errors.pAlloc = _pAlloc;
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

    er.message = builder.CreateString(true, pAlloc);
    errors.PushBack(er);
}

// ***********************************************************************

void ErrorState::PushError(char* pLocation, char* pLineStart, usize line, const char* formatMessage, ...) {
    va_list args;
    va_start(args, formatMessage);
    PushError(pLocation, pLineStart, line, formatMessage, args);
    va_end(args);
}

// ***********************************************************************

void ErrorState::PushError(char* pLocation, char* pLineStart, usize line, const char* formatMessage, va_list args) {
    StringBuilder builder(pAlloc);
    builder.AppendFormatInternal(formatMessage, args);

	Error er;
	er.pLocation = pLocation;
	er.line = line;
	er.pLineStart = pLineStart;

    er.message = builder.CreateString(true, pAlloc);

    errors.PushBack(er);
}

// ***********************************************************************

bool ErrorState::ReportCompilationResult() {
    bool success = errors.count == 0;
    if (!success) {
        Log::Info("Compilation failed with %i errors", errors.count);

        StringBuilder builder;

        for (size i = 0; i < errors.count; i++) {
            Error& err = errors[i];
            builder.AppendFormat("Error At: filename:%i:%i\n", err.line, err.pLocation - err.pLineStart);

            u32 lineNumberSpacing = u32(log10f((f32)err.line) + 1);
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

            i32 errorAtColumn = i32(err.pLocation - err.pLineStart);
            builder.AppendFormat("%*s|%*s^\n", lineNumberSpacing + 2, "", errorAtColumn + 1, "");
            builder.AppendFormat("%s\n", err.message.pData);

            String output = builder.CreateString();
			defer(FreeString(output));
            Log::Info("%s", output.pData);
        }
    }
    return success;
}


// ParsingState

void PushError(ParsingState &state, const char* formatMessage, ...);
Ast::Statement* ParseBlock(ParsingState& state);
Ast::Expression* ParseExpression(ParsingState& state);
Ast::Statement* ParseIf(ParsingState& state);
Ast::Statement* ParseWhile(ParsingState& state);
Ast::Statement* ParseReturn(ParsingState& state);
Ast::Statement* ParsePrint(ParsingState& state);
Ast::Statement* ParseExpressionStmt(ParsingState& state);
Ast::Statement* ParseDeclaration(ParsingState& state, bool onlyDeclarations = false);

// ***********************************************************************

Token Previous(ParsingState &state) {
    return *(state.pCurrent - 1);
}

// ***********************************************************************

Token Advance(ParsingState &state) {
    state.pCurrent++;
    return Previous(state);
}

// ***********************************************************************

bool IsAtEnd(ParsingState &state) {
    return state.pCurrent >= state.pTokensEnd;
}

// ***********************************************************************

Token Peek(ParsingState &state) {
    return *state.pCurrent;
}

// ***********************************************************************

bool Check(ParsingState &state, TokenType::Enum type) {
    if (IsAtEnd(state))
        return false;

    return Peek(state).type == type;
}
    
// ***********************************************************************

bool CheckPair(ParsingState &state, TokenType::Enum type, TokenType::Enum type2) {
    if (IsAtEnd(state))
        return false;

    return Peek(state).type == type && (state.pCurrent+1)->type == type2;
}

// ***********************************************************************

Token Consume(ParsingState &state, TokenType::Enum type, String message) {
    if (Check(state, type))
        return Advance(state);

    PushError(state, message.pData);
    return Token();
}

// ***********************************************************************

bool Match(ParsingState &state, i32 numTokens, ...) {
    va_list args;

    va_start(args, numTokens);
    for (int i = 0; i < numTokens; i++) {
        if (Check(state, va_arg(args, TokenType::Enum))) {
            Advance(state);
            return true;
        }
    }
    return false;
}

// ***********************************************************************

void PushError(ParsingState &state, const char* formatMessage, ...) {
    if (state.panicMode)
        return;

    state.panicMode = true;

    va_list args;
    va_start(args, formatMessage);
	state.pErrorState->PushError(state.pCurrent->pLocation, state.pCurrent->pLineStart, state.pCurrent->line, formatMessage, args);
    va_end(args);
}

// ***********************************************************************

void Synchronize(ParsingState &state) {
    state.panicMode = false;

    while (state.pCurrent->type != TokenType::EndOfFile) {
        if (state.pCurrent->type == TokenType::Semicolon) {
            Advance(state); // Consume semicolon we found
            return;
        }
        Advance(state);
    }
}

// ***********************************************************************

Ast::Expression* ParseType(ParsingState& state) {

    // This should parse an identifier, or each of the primitive syntactic types
    // There is redundancy in looking for an identifier (due to ParsePrimary) but it's fine
    if (Match(state, 1, TokenType::Identifier)) {
        Token identifier = Previous(state);
        Ast::Identifier* pIdentifier = MakeNode<Ast::Identifier>(state.pAllocator, identifier, Ast::NodeKind::Identifier);
        pIdentifier->identifier = CopyCStringRange(identifier.pLocation, identifier.pLocation + identifier.length, state.pAllocator);
        return pIdentifier;
    }

    if (Match(state, 1, TokenType::Func)) {
        Ast::FunctionType* pFuncType = MakeNode<Ast::FunctionType>(state.pAllocator, Previous(state), Ast::NodeKind::FunctionType);
        pFuncType->params.pAlloc = state.pAllocator;

		Consume(state, TokenType::LeftParen, "Expected left parenthesis to start function param list");

        if (Peek(state).type != TokenType::RightParen) {
			// Parse parameter list
			do {
                // First item could be identifier for a declaration, or a type
                Ast::Node* pParam = (Ast::Type*)ParseType(state);
                Token arg = Previous(state);

                if (Match(state, 1, TokenType::Colon)) {
                    if (pParam->nodeKind == Ast::NodeKind::Identifier) {
                        // The first item was a declaration, so convert it to one
                        Ast::Declaration* pParamDecl = MakeNode<Ast::Declaration>(state.pAllocator, arg, Ast::NodeKind::Declaration);
                        pParamDecl->identifier = CopyCStringRange(arg.pLocation, arg.pLocation + arg.length, state.pAllocator);
                        pParamDecl->pTypeAnnotation = (Ast::Type*)ParseType(state);
            
				        pFuncType->params.PushBack(pParamDecl);
                    } else {
                        PushError(state, "Expected an identifier on the left side of this declaration");
                    }
                } else {
				        pFuncType->params.PushBack(pParam);
                }
			} while (Match(state, 1, TokenType::Comma));
		}
		Consume(state, TokenType::RightParen, "Expected right parenthesis to close argument list");

        // Parse return type
		if (Match(state, 1, TokenType::FuncSigReturn)) {
			pFuncType->pReturnType = (Ast::Type*)ParseType(state);
		}

        return pFuncType;
    }

    return MakeNode<Ast::BadExpression>(state.pAllocator, *state.pCurrent, Ast::NodeKind::BadExpression);
}

// ***********************************************************************

Ast::Expression* ParsePrimary(ParsingState& state) {

	if (Match(state, 1, TokenType::Func)) {
		Token func = Previous(state);
        state.pCurrent--;
        Ast::FunctionType* pFuncType = (Ast::FunctionType*)ParseType(state);

        if (Match(state, 1, TokenType::LeftBrace)) {
            Ast::Function* pFunc = MakeNode<Ast::Function>(state.pAllocator, func, Ast::NodeKind::Function);
            pFunc->pFuncType = pFuncType;
            pFunc->pBody = (Ast::Block*)ParseBlock(state);
            return pFunc;
		}
		return pFuncType;
	}

	if (Match(state, 1, TokenType::Struct)) {
        Ast::Structure* pStruct = MakeNode<Ast::Structure>(state.pAllocator, Previous(state), Ast::NodeKind::Structure);
		pStruct->members.pAlloc = state.pAllocator;

		Consume(state, TokenType::LeftBrace, "Expected '{' after struct to start member declarations");
        pStruct->startToken = Previous(state);

		while (!Check(state, TokenType::RightBrace) && !IsAtEnd(state)) {
            Ast::Statement* pMember = ParseDeclaration(state);
			pStruct->members.PushBack(pMember);
		}

		Consume(state, TokenType::RightBrace, "Expected '}' to end member declarations of struct");
		pStruct->endToken = Previous(state);
		return pStruct;
	}

    if (Match(state, 1, TokenType::LiteralInteger)) {
        Token token = Previous(state);
        Ast::Literal* pLiteralExpr = MakeNode<Ast::Literal>(state.pAllocator, token, Ast::NodeKind::Literal);
        pLiteralExpr->isConstant = true;

        char* endPtr = token.pLocation + token.length;
        pLiteralExpr->constantValue = MakeValue((i32)strtol(token.pLocation, &endPtr, 10));
		pLiteralExpr->pType = GetI32Type();
        return pLiteralExpr;
    }

    if (Match(state, 1, TokenType::LiteralFloat)) {
        Token token = Previous(state);
        Ast::Literal* pLiteralExpr = MakeNode<Ast::Literal>(state.pAllocator, token, Ast::NodeKind::Literal);
        pLiteralExpr->isConstant = true;

        char* endPtr = token.pLocation + token.length;
        pLiteralExpr->constantValue = MakeValue((f32)strtod(token.pLocation, &endPtr));
		pLiteralExpr->pType = GetF32Type();
		return pLiteralExpr;
    }

    if (Match(state, 1, TokenType::LiteralBool)) {
        Token token = Previous(state);
        Ast::Literal* pLiteralExpr = MakeNode<Ast::Literal>(state.pAllocator, token, Ast::NodeKind::Literal);
        pLiteralExpr->isConstant = true;

        if (strncmp("true", token.pLocation, token.length) == 0)
            pLiteralExpr->constantValue = MakeValue(true);
        else if (strncmp("false", token.pLocation, token.length) == 0)
            pLiteralExpr->constantValue = MakeValue(false);
		pLiteralExpr->pType = GetBoolType();
		return pLiteralExpr;
    }

    if (Match(state, 1, TokenType::LeftParen)) {
        Token startToken = Previous(state);
		Ast::Expression* pExpr = ParseExpression(state);
        Consume(state, TokenType::RightParen, "Expected a closing right parenthesis \")\", but found nothing in this expression");

        if (pExpr) {
            Ast::Grouping* pGroupExpr = MakeNode<Ast::Grouping>(state.pAllocator, startToken, Ast::NodeKind::Grouping);
            pGroupExpr->pExpression = pExpr;
            return pGroupExpr;
        }
        PushError(state, "Expected valid expression inside parenthesis, but found nothing");
        
        return MakeNode<Ast::BadExpression>(state.pAllocator, startToken, Ast::NodeKind::BadExpression);
    }

    if (Match(state, 1, TokenType::Identifier)) {
        Token identifier = Previous(state);
        
		// This might be the start of a struct literal, so covering that case
		if (CheckPair(state, TokenType::Dot, TokenType::LeftBrace)) {
			Advance(state); Advance(state);
			Ast::StructLiteral* pLiteral = MakeNode<Ast::StructLiteral>(state.pAllocator, identifier, Ast::NodeKind::StructLiteral);
			pLiteral->structName = CopyCStringRange(identifier.pLocation, identifier.pLocation + identifier.length, state.pAllocator);
			pLiteral->members.pAlloc = state.pAllocator;
			do {
				pLiteral->members.PushBack(ParseExpression(state));
			} while (Match(state, 1, TokenType::Comma));

			Consume(state, TokenType::RightBrace, "Expected '}' to end struct literal expression. Potentially you forgot a ',' between members?");
			return pLiteral;
		} else {
			Ast::Identifier* pIdentifier = MakeNode<Ast::Identifier>(state.pAllocator, identifier, Ast::NodeKind::Identifier);
			pIdentifier->identifier = CopyCStringRange(identifier.pLocation, identifier.pLocation + identifier.length, state.pAllocator);
			return pIdentifier;
		}
    }

	if (CheckPair(state, TokenType::Dot, TokenType::LeftBrace)) {
		Advance(state);
		Token dot = Previous(state);
		Advance(state);

		Ast::StructLiteral* pLiteral = MakeNode<Ast::StructLiteral>(state.pAllocator, dot, Ast::NodeKind::StructLiteral);
		pLiteral->structName = "";
		pLiteral->members.pAlloc = state.pAllocator;
		do {
			pLiteral->members.PushBack(ParseExpression(state));
		} while (Match(state, 1, TokenType::Comma));

		Consume(state, TokenType::RightBrace, "Expected '}' to end struct literal expression. Potentially you forgot a ',' between members?");
		return pLiteral;
	}

	if (Ast::Type* pType = (Ast::Type*)ParseType(state)) {
        return pType;
    }

    return MakeNode<Ast::BadExpression>(state.pAllocator, *state.pCurrent, Ast::NodeKind::BadExpression);
}

// ***********************************************************************

Ast::Expression* ParseCall(ParsingState& state) {
	Ast::Expression* pExpr = ParsePrimary(state);

	while (true) {
		if (Match(state, 1, TokenType::LeftParen)) {
            Token openParen = Previous(state);
            Ast::Call* pCall = MakeNode<Ast::Call>(state.pAllocator, openParen, Ast::NodeKind::Call);
			pCall->pCallee = pExpr;
			pCall->args.pAlloc = state.pAllocator;

			if (!Check(state, TokenType::RightParen)) {
				do {
					pCall->args.PushBack(ParseExpression(state));
				} while (Match(state, 1, TokenType::Comma));
			}

			Token closeParen = Consume(state, TokenType::RightParen, "Expected right parenthesis to end function call");
			pExpr = pCall;
		// here is your selector parsing, equal precedence to function calls
		} else if (Match(state, 1, TokenType::Dot)) {
			Token dot = Previous(state);
            Ast::Selector* pSelector = MakeNode<Ast::Selector>(state.pAllocator, dot, Ast::NodeKind::Selector);
			pSelector->pTarget = pExpr;

			Token fieldName = Consume(state, TokenType::Identifier, "Expected identifier after '.' to access a named field");
			pSelector->fieldName = CopyCStringRange(fieldName.pLocation, fieldName.pLocation + fieldName.length, state.pAllocator);

			pExpr = pSelector;
		} else {
			break;
		}
	}
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParseUnary(ParsingState& state) {
    while (Match(state, 3, TokenType::Minus, TokenType::Bang, TokenType::As)) {
		Token prev = Previous(state);

		// Cast Operator
		if (prev.type == TokenType::As) {
			Consume(state, TokenType::LeftParen, "Expected '(' before cast target type");

            Ast::Cast* pCastExpr = MakeNode<Ast::Cast>(state.pAllocator, prev, Ast::NodeKind::Cast);
			pCastExpr->pTypeExpr = (Ast::Type*)ParseType(state);

			Consume(state, TokenType::RightParen, "Expected ')' after cast target type");

			pCastExpr->pExprToCast = ParseUnary(state);
			return pCastExpr;
		}
		// Unary maths op
		else {
            Ast::Unary* pUnaryExpr = MakeNode<Ast::Unary>(state.pAllocator, prev, Ast::NodeKind::Unary);

			if (prev.type == TokenType::Minus)
				pUnaryExpr->op = Operator::UnaryMinus;
			else if (prev.type == TokenType::Bang)
				pUnaryExpr->op = Operator::Not;
			pUnaryExpr->pRight = ParseUnary(state);

			return (Ast::Expression*)pUnaryExpr;
		}
    }
	return ParseCall(state);
}

// ***********************************************************************

Ast::Expression* ParseMulDiv(ParsingState& state) {
	Ast::Expression* pExpr = ParseUnary(state);

    while (Match(state, 2, TokenType::Star, TokenType::Slash)) {
        Ast::Binary* pBinaryExpr = MakeNode<Ast::Binary>(state.pAllocator, Previous(state), Ast::NodeKind::Binary);

        Token opToken = Previous(state);
        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
		pBinaryExpr->pRight = ParseUnary(state);

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParseAddSub(ParsingState& state) {
	Ast::Expression* pExpr = ParseMulDiv(state);

    while (Match(state, 2, TokenType::Minus, TokenType::Plus)) {
        Ast::Binary* pBinaryExpr = MakeNode<Ast::Binary>(state.pAllocator, Previous(state), Ast::NodeKind::Binary);

        Token opToken = Previous(state);
        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
		pBinaryExpr->pRight = ParseMulDiv(state);

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParseComparison(ParsingState& state) {
	Ast::Expression* pExpr = ParseAddSub(state);

    while (Match(state, 4, TokenType::Greater, TokenType::Less, TokenType::GreaterEqual, TokenType::LessEqual)) {
        Ast::Binary* pBinaryExpr = MakeNode<Ast::Binary>(state.pAllocator, Previous(state), Ast::NodeKind::Binary);

        Token opToken = Previous(state);
        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
		pBinaryExpr->pRight = ParseAddSub(state);

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParseEquality(ParsingState& state) {
	Ast::Expression* pExpr = ParseComparison(state);

    while (Match(state, 2, TokenType::EqualEqual, TokenType::BangEqual)) {
        Ast::Binary* pBinaryExpr = MakeNode<Ast::Binary>(state.pAllocator, Previous(state), Ast::NodeKind::Binary);

        Token opToken = Previous(state);
        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
		pBinaryExpr->pRight = ParseComparison(state);

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParseLogicAnd(ParsingState& state) {
	Ast::Expression* pExpr = ParseEquality(state);

    while (Match(state, 1, TokenType::And)) {
        Ast::Binary* pBinaryExpr = MakeNode<Ast::Binary>(state.pAllocator, Previous(state), Ast::NodeKind::Binary);

        Token opToken = Previous(state);
        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
		pBinaryExpr->pRight = ParseEquality(state);

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParseLogicOr(ParsingState& state) {
	Ast::Expression* pExpr = ParseLogicAnd(state);

    while (Match(state, 1, TokenType::Or)) {
        Ast::Binary* pBinaryExpr = MakeNode<Ast::Binary>(state.pAllocator, Previous(state), Ast::NodeKind::Binary);

        Token opToken = Previous(state);
        pBinaryExpr->pLeft = pExpr;
        pBinaryExpr->op = TokenToOperator(opToken.type);
		pBinaryExpr->pRight = ParseLogicAnd(state);

        pExpr = (Ast::Expression*)pBinaryExpr;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParseAssignment(ParsingState& state) {
	// How do we know this pExpr will actually end up being used in an assignment at all?
	Ast::Expression* pExpr = ParseLogicOr(state);

    if (Match(state, 1, TokenType::Equal)) {
        Token equal = Previous(state);
		Ast::Expression* pRight = ParseAssignment(state);

		pExpr->isLValue = true; // Typechecker will check if this is valid
		Ast::Assignment* pAssignment = MakeNode<Ast::Assignment>(state.pAllocator, equal, Ast::NodeKind::Assignment);

		pAssignment->pTarget = pExpr;
		pAssignment->pAssignment = pRight;
		return pAssignment;
    }
    return pExpr;
}

// ***********************************************************************

Ast::Expression* ParseExpression(ParsingState& state) {
	// We start at the highest precedence possible (i.e. executed last)
	return ParseAssignment(state);
}

// ***********************************************************************

Ast::Statement* ParseStatement(ParsingState& state) {

    if (Match(state, 1, TokenType::If)) {
		return ParseIf(state);
    }
    if (Match(state, 1, TokenType::While)) {
		return ParseWhile(state);
    }
    if (Match(state, 1, TokenType::LeftBrace)) {
		return ParseBlock(state);
    }
    if (Match(state, 1, TokenType::Return)) {
		return ParseReturn(state);
    }

    Ast::Statement* pStmt = nullptr;
    if (Match(state, 1, TokenType::Identifier)) {
        if (strncmp("print", Previous(state).pLocation, Previous(state).length) == 0) {
			pStmt = ParsePrint(state);
        } else {
            state.pCurrent--;
        }
    } 
    
    if (pStmt == nullptr) {
        if (Match(state, 8, TokenType::Identifier, TokenType::LiteralString, TokenType::LiteralInteger, TokenType::LiteralBool, TokenType::LiteralFloat, TokenType::LeftParen, TokenType::Bang, TokenType::Minus)) {
            state.pCurrent--;
			pStmt = ParseExpressionStmt(state);
        }
        else if (Match(state, 1, TokenType::Semicolon)) {
            return MakeNode<Ast::BadStatement>(state.pAllocator, Previous(state), Ast::NodeKind::BadStatement);
        }
        else {
            PushError(state, "Unable to parse statement");
        }
    }

    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParseExpressionStmt(ParsingState& state) {
	Ast::Expression* pExpr = ParseExpression(state);
    Consume(state, TokenType::Semicolon, "Expected \";\" at the end of this statement");

    Ast::ExpressionStmt* pStmt = MakeNode<Ast::ExpressionStmt>(state.pAllocator, Previous(state), Ast::NodeKind::ExpressionStmt);
    pStmt->pExpr = pExpr;
    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParseIf(ParsingState& state) {
    Ast::If* pIf = MakeNode<Ast::If>(state.pAllocator, Previous(state), Ast::NodeKind::If);

	pIf->pCondition = ParseExpression(state);
	pIf->pThenStmt = ParseStatement(state);

    if (Match(state, 1, TokenType::Else)) {
		pIf->pElseStmt = ParseStatement(state);
    }
    return pIf;
}

// ***********************************************************************

Ast::Statement* ParseWhile(ParsingState& state) {
    Ast::While* pWhile = MakeNode<Ast::While>(state.pAllocator, Previous(state), Ast::NodeKind::While);

	pWhile->pCondition = ParseExpression(state);
	pWhile->pBody = ParseStatement(state);

    return pWhile;
}

// ***********************************************************************

Ast::Statement* ParsePrint(ParsingState& state) {
    Consume(state, TokenType::LeftParen, "Expected \"(\" following print, before the expression starts");
	Ast::Expression* pExpr = ParseExpression(state);
    Consume(state, TokenType::RightParen, "Expected \")\" to close print expression");
    Consume(state, TokenType::Semicolon, "Expected \";\" at the end of this statement");
    
    Ast::Print* pPrintStmt = MakeNode<Ast::Print>(state.pAllocator, Previous(state), Ast::NodeKind::Print);
    pPrintStmt->pExpr = pExpr;
    return pPrintStmt;
}

// ***********************************************************************

Ast::Statement* ParseReturn(ParsingState& state) {
    Ast::Return* pReturnStmt = MakeNode<Ast::Return>(state.pAllocator, Previous(state), Ast::NodeKind::Return);

	if (!Check(state, TokenType::Semicolon)) {
		pReturnStmt->pExpr = ParseExpression(state);
    } else {
        pReturnStmt->pExpr = nullptr;     
    }
    Consume(state, TokenType::Semicolon, "Expected \";\" at the end of this statement");
    return pReturnStmt;
}

// ***********************************************************************

Ast::Statement* ParseBlock(ParsingState& state) {
    Ast::Block* pBlock = MakeNode<Ast::Block>(state.pAllocator, Previous(state), Ast::NodeKind::Block);
    pBlock->declarations.pAlloc = state.pAllocator;
    pBlock->startToken = Previous(state);

	while (!Check(state, TokenType::RightBrace) && !IsAtEnd(state)) {
		pBlock->declarations.PushBack(ParseDeclaration(state));
    }
    Consume(state, TokenType::RightBrace, "Expected '}' to end this block");
    pBlock->endToken = Previous(state);
    return pBlock;
}

// ***********************************************************************

Ast::Statement* ParseDeclaration(ParsingState& state, bool onlyDeclarations) {
    Ast::Statement* pStmt = nullptr;

    if (Match(state, 1, TokenType::Identifier)) {
        Token identifier = Previous(state);

        if (Match(state, 1, TokenType::Colon)) {
            // We now know we are dealing with a declaration of some kind
            Ast::Declaration* pDecl = MakeNode<Ast::Declaration>(state.pAllocator, identifier, Ast::NodeKind::Declaration);
            pDecl->identifier = CopyCStringRange(identifier.pLocation, identifier.pLocation + identifier.length, state.pAllocator);

            // Optionally Parse type 
            if (Peek(state).type != TokenType::Equal && Peek(state).type != TokenType::Colon) {
				pDecl->pTypeAnnotation = (Ast::Type*)ParseType(state);

                if (pDecl->pTypeAnnotation->nodeKind == Ast::NodeKind::BadExpression)
                    PushError(state, "Expected a type here, potentially missing an equal sign before an initializer?");
            } else {
                pDecl->pTypeAnnotation = nullptr;
            }

            // Parse initializer for constant declaration
            if (Match(state, 1, TokenType::Colon)) {
                pDecl->isConstantDeclaration = true;
                pDecl->pInitializerExpr = ParseExpression(state);
                if (pDecl->pInitializerExpr->nodeKind == Ast::NodeKind::BadExpression)
                    PushError(state, "Need an expression to initialize this constant declaration");
            }

            // Parse initializer
            if (Match(state, 1, TokenType::Equal)) {
                pDecl->isConstantDeclaration = false;
				pDecl->pInitializerExpr = ParseExpression(state);

                if (pDecl->pInitializerExpr->nodeKind == Ast::NodeKind::BadExpression)
                    PushError(state, "Need an expression to initialize this declaration. If you want it uninitialized, leave out the '=' sign");
            }

            // Let the synchronize function get us back to the next statement
            if (!state.panicMode) 
                Consume(state, TokenType::Semicolon, "Expected \";\" to end a previous declaration");

            pStmt = pDecl;
        } else {
            // Wasn't a declaration, backtrack
            state.pCurrent--;
        }
    }
    
	if (!onlyDeclarations && pStmt == nullptr) {
		pStmt = ParseStatement(state);
    }

	if (state.panicMode)
		Synchronize(state);

    return pStmt;
}

// ***********************************************************************

void Parse(Compiler& compilerState) {
    ResizableArray<Token>& tokens = compilerState.tokens;

	ParsingState state;
	state.pTokensStart = tokens.pData;
	state.pTokensEnd = tokens.pData + tokens.count - 1;
	state.pCurrent = tokens.pData;
	state.pAllocator = &compilerState.compilerMemory;
	state.pErrorState = &compilerState.errorState;

    compilerState.syntaxTree = ResizableArray<Ast::Statement*>(&compilerState.compilerMemory);

	while (!IsAtEnd(state)) {
		Ast::Statement* pStmt = ParseDeclaration(state);
        if (pStmt)
            compilerState.syntaxTree.PushBack(pStmt);
    }
}

// ***********************************************************************

void DebugStatement(Ast::Statement* pStmt, i32 indentationLevel) {
    switch (pStmt->nodeKind) {
        case Ast::NodeKind::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;
            Log::Debug("%*s+ Decl (%s)", indentationLevel, "", pDecl->identifier.pData);
            if (pDecl->pTypeAnnotation) {
                DebugExpression(pDecl->pTypeAnnotation, indentationLevel + 2);
            } 
            if (pDecl->pInitializerExpr) {
                DebugExpression(pDecl->pInitializerExpr, indentationLevel + 2);
            }
            break;
        }
        case Ast::NodeKind::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            Log::Debug("%*s> PrintStmt", indentationLevel, "");
            DebugExpression(pPrint->pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeKind::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            Log::Debug("%*s> ReturnStmt", indentationLevel, "");
            if (pReturn->pExpr)
                DebugExpression(pReturn->pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeKind::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            Log::Debug("%*s> ExpressionStmt", indentationLevel, "");
            DebugExpression(pExprStmt->pExpr, indentationLevel + 2);
            break;
        }
        case Ast::NodeKind::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            Log::Debug("%*s> If", indentationLevel, "");
            DebugExpression(pIf->pCondition, indentationLevel + 2);
            DebugStatement(pIf->pThenStmt, indentationLevel + 2);
            if (pIf->pElseStmt)
                DebugStatement(pIf->pElseStmt, indentationLevel + 2);
            break;
        }
        case Ast::NodeKind::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            Log::Debug("%*s> While", indentationLevel, "");
            DebugExpression(pWhile->pCondition, indentationLevel + 2);
            DebugStatement(pWhile->pBody, indentationLevel + 2);
            break;
        }
        case Ast::NodeKind::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            Log::Debug("%*s> Block", indentationLevel, "");
            DebugStatements(pBlock->declarations, indentationLevel + 2);
            break;
        }
        case Ast::NodeKind::BadStatement: {
            Ast::BadStatement* pBad = (Ast::BadStatement*)pStmt;
            Log::Debug("%*s> Bad Statement", indentationLevel, "");
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void DebugStatements(ResizableArray<Ast::Statement*>& statements, i32 indentationLevel) {
    for (size i = 0; i < statements.count; i++) {
        Ast::Statement* pStmt = statements[i];
        DebugStatement(pStmt, indentationLevel);
    }
}

// ***********************************************************************

void DebugExpression(Ast::Expression* pExpr, i32 indentationLevel) {
    switch (pExpr->nodeKind) {
        case Ast::NodeKind::Identifier: {
            Ast::Identifier* pIdentifier = (Ast::Identifier*)pExpr;
            String nodeTypeStr = "none";
            if (pIdentifier->pType) {
                nodeTypeStr = pIdentifier->pType->name;
            }
            
            Log::Debug("%*s- Identifier (%s:%s)", indentationLevel, "", pIdentifier->identifier.pData, nodeTypeStr.pData);
            break;
        }
        case Ast::NodeKind::FunctionType: {}
        case Ast::NodeKind::Type: {
            Ast::Type* pType = (Ast::Type*)pExpr;
			TypeInfo* pTypeInfo = FindTypeByValue(pType->constantValue);
            if (pType->pType && pTypeInfo)
                Log::Debug("%*s- Type Literal (%s:%s)", indentationLevel, "", pTypeInfo->name.pData, pType->pType->name.pData);
            break;
        }
        case Ast::NodeKind::Assignment: {
            Ast::Assignment* pAssignment = (Ast::Assignment*)pExpr;
            String nodeTypeStr = "none";
            if (pAssignment->pType) {
                nodeTypeStr = pAssignment->pType->name;
            }

            Log::Debug("%*s- Assignment (%s)", indentationLevel, "", nodeTypeStr.pData);
            DebugExpression(pAssignment->pTarget, indentationLevel + 2);
            DebugExpression(pAssignment->pAssignment, indentationLevel + 2);
            break;
        }
        case Ast::NodeKind::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            String nodeTypeStr = "none";
            if (pLiteral->pType) {
                nodeTypeStr = pLiteral->pType->name;
            }

            if (CheckTypesIdentical(pLiteral->pType, GetF32Type()))
                Log::Debug("%*s- Literal (%f:%s)", indentationLevel, "", pLiteral->constantValue.f32Value, nodeTypeStr.pData);
            else if (CheckTypesIdentical(pLiteral->pType, GetI32Type()))
                Log::Debug("%*s- Literal (%i:%s)", indentationLevel, "", pLiteral->constantValue.i32Value, nodeTypeStr.pData);
            else if (CheckTypesIdentical(pLiteral->pType, GetBoolType()))
                Log::Debug("%*s- Literal (%s:%s)", indentationLevel, "", pLiteral->constantValue.boolValue ? "true" : "false", nodeTypeStr.pData);
            break;
        }
		case Ast::NodeKind::StructLiteral: {
			Ast::StructLiteral* pStructLiteral = (Ast::StructLiteral*)pExpr;
            String nodeTypeStr = "none";
            if (pStructLiteral->pType) {
                nodeTypeStr = pStructLiteral->pType->name;
            }
            Log::Debug("%*s- Struct Literal (%s)", indentationLevel, "", nodeTypeStr.pData);
            for (Ast::Expression* pMember : pStructLiteral->members) {
                DebugExpression(pMember, indentationLevel + 4);
            }
			break;
		}
        case Ast::NodeKind::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            Log::Debug("%*s- Function", indentationLevel, "");
            DebugExpression(pFunction->pFuncType, indentationLevel + 2);
            DebugStatement(pFunction->pBody, indentationLevel + 2);
            break;
        }
		case Ast::NodeKind::Structure: {
			Ast::Structure* pStruct = (Ast::Structure*)pExpr;
			Log::Debug("%*s- Struct", indentationLevel, "");
			DebugStatements(pStruct->members, indentationLevel + 2);
			break;
		}
        case Ast::NodeKind::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            String typeStr = "none";
            if (pGroup->pType) {
                typeStr = pGroup->pType->name;
            }
            Log::Debug("%*s- Group (:%s)", indentationLevel, "", typeStr.pData);
            DebugExpression(pGroup->pExpression, indentationLevel + 2);
            break;
        }
        case Ast::NodeKind::Binary: {
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
        case Ast::NodeKind::Unary: {
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
		case Ast::NodeKind::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;

			if (pCast->pType) {
				Log::Debug("%*s- Cast (:%s)", indentationLevel, "", pCast->pType->name.pData);
			} else {
				Log::Debug("%*s- Cast (:none)", indentationLevel, "");
			}
			DebugExpression(pCast->pTypeExpr, indentationLevel + 2);
			DebugExpression(pCast->pExprToCast, indentationLevel + 2);
			break;
		}
        case Ast::NodeKind::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;

            DebugExpression(pCall->pCallee, indentationLevel);
            Log::Debug("%*s- Call", indentationLevel + 2, "");

            for (Ast::Expression* pArg : pCall->args) {
                DebugExpression(pArg, indentationLevel + 4);
            }
            break;
        }
		case Ast::NodeKind::Selector: {
			Ast::Selector* pSelector = (Ast::Selector*)pExpr;

			String nodeTypeStr = "none";
			if (pSelector->pType) {
				nodeTypeStr = pSelector->pType->name;
			}
			Log::Debug("%*s- Selector (%s:%s)", indentationLevel, "", pSelector->fieldName.pData, nodeTypeStr.pData);
			DebugExpression(pSelector->pTarget, indentationLevel + 2);
			break;
		}
        case Ast::NodeKind::BadExpression: {
            Ast::BadExpression* pBad = (Ast::BadExpression*)pExpr;
            Log::Debug("%*s> Bad Expression", indentationLevel, "");
            break;
        }
        default:
            break;
    }
}
