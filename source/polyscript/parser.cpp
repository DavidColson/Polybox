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

// In order of increasing precedence, that is top precedence is executed first, the bottom is executed last
namespace Precedence {
enum Enum : u32 {
	None,
	Assignment,
	Or,
	And,
	Equality,
	Comparison,
	AddSub,
	MulDiv,
	UnaryPrefixes,
	CallsAndSelectors,
	Primary
};
}

// ParsingState

void PushError(ParsingState &state, const char* formatMessage, ...);
Ast::Statement* ParseBlock(ParsingState& state);
Ast::Expression* ParseExpression(ParsingState& state, Precedence::Enum prec);
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

Precedence::Enum GetOperatorPrecedence(Operator::Enum op) {
	switch(op) {
		case Operator::Add: return Precedence::AddSub;
		case Operator::Subtract: return Precedence::AddSub;
		case Operator::Multiply: return Precedence::MulDiv;
		case Operator::Divide: return Precedence::MulDiv;
		case Operator::Less: return Precedence::Comparison;
		case Operator::Greater: return Precedence::Comparison;
		case Operator::GreaterEqual: return Precedence::Comparison;
		case Operator::LessEqual: return Precedence::Comparison;
		case Operator::Equal: return Precedence::Equality;
		case Operator::NotEqual: return Precedence::Equality;
		case Operator::And: return Precedence::And;
		case Operator::Or: return Precedence::Or;
		case Operator::UnaryMinus: return Precedence::UnaryPrefixes;
		case Operator::Not: return Precedence::UnaryPrefixes;
		case Operator::AddressOf: return Precedence::UnaryPrefixes;
		case Operator::FieldSelector: return Precedence::CallsAndSelectors;
		case Operator::ArraySubscript: return Precedence::CallsAndSelectors;
		case Operator::PointerDeref: return Precedence::UnaryPrefixes;
		case Operator::Assignment: return Precedence::Assignment;
		case Operator::StructLiteral: return Precedence::Primary;
		case Operator::ArrayLiteral: return Precedence::Primary;
		default: return Precedence::None;
	}
}

// ***********************************************************************

Ast::Expression* ParseType(ParsingState& state);
Ast::Expression* ParseFunctionType(ParsingState& state) {
	Token funcToken = Advance(state);
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

// ***********************************************************************

Ast::Expression* ParseFunction(ParsingState& state) {
	Token func = Peek(state);
	Ast::FunctionType* pFuncType = (Ast::FunctionType*)ParseFunctionType(state);

	if (Match(state, 1, TokenType::LeftBrace)) {
		Ast::Function* pFunc = MakeNode<Ast::Function>(state.pAllocator, func, Ast::NodeKind::Function);
		pFunc->pFuncType = pFuncType;
		pFunc->pBody = (Ast::Block*)ParseBlock(state);
		return pFunc;
	}
	return pFuncType;
}

// ***********************************************************************

Ast::Expression* ParseStruct(ParsingState& state) {
	Consume(state, TokenType::Struct, "Expected 'struct' at start of struct definition");

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

// ***********************************************************************

Ast::Expression* ParseIntegerLiteral(ParsingState& state) {
	Token token = Advance(state);
	Ast::Literal* pLiteralExpr = MakeNode<Ast::Literal>(state.pAllocator, token, Ast::NodeKind::Literal);
	pLiteralExpr->isConstant = true;

	char* endPtr = token.pLocation + token.length;
	pLiteralExpr->constantValue = MakeValue((i32)strtol(token.pLocation, &endPtr, 10));
	pLiteralExpr->pType = GetI32Type();
	return pLiteralExpr;
}

// ***********************************************************************

Ast::Expression* ParseFloatLiteral(ParsingState& state) {
	Token token = Advance(state);
	Ast::Literal* pLiteralExpr = MakeNode<Ast::Literal>(state.pAllocator, token, Ast::NodeKind::Literal);
	pLiteralExpr->isConstant = true;

	char* endPtr = token.pLocation + token.length;
	pLiteralExpr->constantValue = MakeValue((f32)strtod(token.pLocation, &endPtr));
	pLiteralExpr->pType = GetF32Type();
	return pLiteralExpr;
}

// ***********************************************************************

Ast::Expression* ParseBoolLiteral(ParsingState& state) {
	Token token = Advance(state);
	Ast::Literal* pLiteralExpr = MakeNode<Ast::Literal>(state.pAllocator, token, Ast::NodeKind::Literal);
	pLiteralExpr->isConstant = true;

	if (strncmp("true", token.pLocation, token.length) == 0)
		pLiteralExpr->constantValue = MakeValue(true);
	else if (strncmp("false", token.pLocation, token.length) == 0)
		pLiteralExpr->constantValue = MakeValue(false);
	pLiteralExpr->pType = GetBoolType();
	return pLiteralExpr;
}

// ***********************************************************************

Ast::Expression* ParseGrouping(ParsingState& state) {
	Token startToken = Advance(state);
	Ast::Expression* pExpr = ParseExpression(state, Precedence::None);
	Consume(state, TokenType::RightParen, "Expected a closing right parenthesis \")\", but found nothing in this expression");

	if (pExpr) {
		Ast::Grouping* pGroupExpr = MakeNode<Ast::Grouping>(state.pAllocator, startToken, Ast::NodeKind::Grouping);
		pGroupExpr->pExpression = pExpr;
		return pGroupExpr;
	}
	PushError(state, "Expected valid expression inside parenthesis, but found nothing");

	return MakeNode<Ast::BadExpression>(state.pAllocator, startToken, Ast::NodeKind::BadExpression);
}

// ***********************************************************************

Ast::Expression* ParseStructLiteral(ParsingState& state, Ast::Expression* pLeft) {
	Token previous = Previous(state);
	Token op = Advance(state);

	Ast::StructLiteral* pLiteral = MakeNode<Ast::StructLiteral>(state.pAllocator, op, Ast::NodeKind::StructLiteral);
	pLiteral->pStructType = pLeft;
	pLiteral->members.pAlloc = state.pAllocator;
	if (Peek(state).type != TokenType::RightBrace) {
		do {
			pLiteral->members.PushBack(ParseExpression(state, Precedence::None));
		} while (Match(state, 1, TokenType::Comma));
	}

	Consume(state, TokenType::RightBrace, "Expected '}' to end struct literal expression. Potentially you forgot a ',' between members?");
	return pLiteral;
}

// ***********************************************************************

Ast::Expression* ParseArrayLiteral(ParsingState& state, Ast::Expression* pLeft) {
	Token previous = Previous(state);
	Token op = Advance(state);

	Ast::ArrayLiteral* pLiteral = MakeNode<Ast::ArrayLiteral>(state.pAllocator, op, Ast::NodeKind::ArrayLiteral);
	pLiteral->pElementType = pLeft;
	pLiteral->elements.pAlloc = state.pAllocator;
	if (Peek(state).type != TokenType::RightBrace) {
		do {
			pLiteral->elements.PushBack(ParseExpression(state, Precedence::None));
		} while (Match(state, 1, TokenType::Comma));
	}

	Consume(state, TokenType::RightBracket, "Expected ']' to end array literal expression. Potentially you forgot a ',' between members?");
	return pLiteral;
}

// ***********************************************************************

Ast::Expression* ParseIdentifier(ParsingState& state) {
	Token identifier = Advance(state);

	Ast::Identifier* pIdentifier = MakeNode<Ast::Identifier>(state.pAllocator, identifier, Ast::NodeKind::Identifier);
	pIdentifier->identifier = CopyCStringRange(identifier.pLocation, identifier.pLocation + identifier.length, state.pAllocator);
	return pIdentifier;
}

// ***********************************************************************

Ast::Expression* ParseSelector(ParsingState& state, Ast::Expression* pLeft) {
	Token op = Advance(state);
	Ast::Selector* pSelector = MakeNode<Ast::Selector>(state.pAllocator, op, Ast::NodeKind::Selector);
	pSelector->pTarget = pLeft;
	pSelector->op = TokenToOperator(op.type); 
	pSelector->pSelection = ParseExpression(state, Precedence::CallsAndSelectors);

	if (pSelector->op == Operator::ArraySubscript) {
		Consume(state, TokenType::RightBracket, "Expected ']' to end array subscript");
	}
	return pSelector;
}

// ***********************************************************************

Ast::Expression* ParseCall(ParsingState& state, Ast::Expression* pLeft) {
	Token openParen = Advance(state);
	Ast::Call* pCall = MakeNode<Ast::Call>(state.pAllocator, openParen, Ast::NodeKind::Call);
	pCall->pCallee = pLeft;
	pCall->args.pAlloc = state.pAllocator;

	if (!Check(state, TokenType::RightParen)) {
		do {
			pCall->args.PushBack(ParseExpression(state, Precedence::None));
		} while (Match(state, 1, TokenType::Comma));
	}

	Token closeParen = Consume(state, TokenType::RightParen, "Expected right parenthesis to end function call");
	return pCall;
}

// ***********************************************************************

Ast::Expression* ParseCast(ParsingState& state) {
	Token asToken = Advance(state);
	Consume(state, TokenType::LeftParen, "Expected '(' before cast target type");

	Ast::Cast* pCastExpr = MakeNode<Ast::Cast>(state.pAllocator, asToken, Ast::NodeKind::Cast);
	pCastExpr->pTypeExpr = (Ast::Type*)ParseType(state);

	Consume(state, TokenType::RightParen, "Expected ')' after cast target type");

	pCastExpr->pExprToCast = ParseExpression(state, Precedence::UnaryPrefixes);
	return pCastExpr;
}

// ***********************************************************************

Ast::Expression* ParsePointerType(ParsingState& state) {
	Token caret = Advance(state);
	Ast::PointerType* pPointerType = MakeNode<Ast::PointerType>(state.pAllocator, caret, Ast::NodeKind::PointerType);
	pPointerType->pBaseType = (Ast::Type*)ParseType(state);
	return pPointerType;
}

// ***********************************************************************

Ast::Expression* ParseArrayType(ParsingState& state) {
	Token leftBracket = Advance(state);
	Ast::ArrayType* pArrayType = MakeNode<Ast::ArrayType>(state.pAllocator, leftBracket, Ast::NodeKind::ArrayType);
	pArrayType->pDimension = ParseExpression(state, Precedence::None);
	Consume(state, TokenType::RightBracket, "Expected ']' following array dimension in array type declaration");
	pArrayType->pBaseType = (Ast::Type*)ParseType(state);
	return pArrayType;
}

// ***********************************************************************

Ast::Expression* ParseUnary(ParsingState& state) {
	Token op = Advance(state);
	Ast::Unary* pUnaryExpr = MakeNode<Ast::Unary>(state.pAllocator, op, Ast::NodeKind::Unary);

	if (op.type == TokenType::Minus)
		pUnaryExpr->op = Operator::UnaryMinus;
	else if (op.type == TokenType::Bang)
		pUnaryExpr->op = Operator::Not;
	else if (op.type == TokenType::Address)
		pUnaryExpr->op = Operator::AddressOf;

	pUnaryExpr->pRight = ParseExpression(state, Precedence::UnaryPrefixes);
	return pUnaryExpr;
}

// ***********************************************************************

Ast::Expression* ParseDereference(ParsingState& state, Ast::Expression* pLeft) {
	Token caret = Advance(state);
	Ast::Dereference* pDereference = MakeNode<Ast::Dereference>(state.pAllocator, Previous(state), Ast::NodeKind::Dereference);
	pDereference->pExpr = pLeft;
	return pDereference;
}

// ***********************************************************************

Ast::Expression* ParseBinary(ParsingState& state, Ast::Expression* pLeft) {
	Token opToken = Advance(state);
	Ast::Binary* pBinaryExpr = MakeNode<Ast::Binary>(state.pAllocator, Previous(state), Ast::NodeKind::Binary);

	pBinaryExpr->pLeft = pLeft;
	pBinaryExpr->op = TokenToOperator(opToken.type);
	pBinaryExpr->pRight = ParseExpression(state, GetOperatorPrecedence(pBinaryExpr->op));

	return pBinaryExpr;
}

// ***********************************************************************

Ast::Expression* ParseAssignment(ParsingState& state, Ast::Expression* pLeft) {
	Token equal = Advance(state);

	Ast::Assignment* pAssignment = MakeNode<Ast::Assignment>(state.pAllocator, equal, Ast::NodeKind::Assignment);

	pAssignment->pTarget = pLeft;
	pAssignment->pAssignment = ParseExpression(state, Precedence::Assignment);
	return pAssignment;
}

// ***********************************************************************

Ast::Expression* ParseType(ParsingState& state) {
	Token startToken = Peek(state);

	Ast::Expression* pLeft = nullptr;
	switch(startToken.type) {
		case TokenType::Identifier:
			return ParseIdentifier(state);
			break;
		case TokenType::Func:
			return ParseFunctionType(state);
			break;
		case TokenType::Caret:
			return ParsePointerType(state);
			break;
		case TokenType::LeftBracket:
			return ParseArrayType(state);
			break;
		default:
			return MakeNode<Ast::BadExpression>(state.pAllocator, *state.pCurrent, Ast::NodeKind::BadExpression);
	}
}


// ***********************************************************************
Ast::Expression* ParseExpression(ParsingState& state, Precedence::Enum prec) {
	Token prefixToken = Peek(state);

	// First we parse basic elements of expressions and simple unary operators
	Ast::Expression* pLeft = nullptr;
	switch(prefixToken.type) {
		case TokenType::Func:
			pLeft = ParseFunction(state);
			break;
		case TokenType::Struct:
			pLeft = ParseStruct(state);
			break;
		case TokenType::LiteralInteger:
			pLeft = ParseIntegerLiteral(state);
			break;
		case TokenType::LiteralFloat:
			pLeft = ParseFloatLiteral(state);
			break;
		case TokenType::LiteralBool:
			pLeft = ParseBoolLiteral(state);
			break;
		case TokenType::LeftParen:
			pLeft = ParseGrouping(state);
			break;
		case TokenType::Identifier:
			pLeft = ParseIdentifier(state);
			break;
		case TokenType::StructLiteralOp:
			pLeft =	ParseStructLiteral(state, nullptr);
			break;
		case TokenType::ArrayLiteralOp:
			pLeft =	ParseArrayLiteral(state, nullptr);
			break;
		case TokenType::As:
			pLeft = ParseCast(state);
			break;
		case TokenType::Minus:
		case TokenType::Bang:
		case TokenType::Address:
			pLeft = ParseUnary(state);
			break;
		case TokenType::Caret:
			pLeft = ParsePointerType(state);
			break;
		case TokenType::Semicolon:
			pLeft = MakeNode<Ast::BadExpression>(state.pAllocator, *state.pCurrent, Ast::NodeKind::BadExpression);
			break;
		case TokenType::LeftBracket:
			pLeft = ParseArrayType(state);
			break;
		default:
			Advance(state);
			pLeft = MakeNode<Ast::BadExpression>(state.pAllocator, *state.pCurrent, Ast::NodeKind::BadExpression);
	}
		
	// Then as long as we're not increasing in precedence we'll now loop
	// parsing infix and mixfix operators
	while(prec < GetOperatorPrecedence(TokenToOperator(Peek(state).type))) {
		Token infixToken = Peek(state);

		switch(infixToken.type) {
			case TokenType::Or:
			case TokenType::And:
			case TokenType::EqualEqual:
			case TokenType::BangEqual:
			case TokenType::Greater:
			case TokenType::Less:
			case TokenType::GreaterEqual:
			case TokenType::LessEqual:
			case TokenType::Plus:
			case TokenType::Minus:
			case TokenType::Star:
			case TokenType::Slash:
				pLeft = ParseBinary(state, pLeft);
				break;
			case TokenType::LeftParen:
				pLeft = ParseCall(state, pLeft);
				break;
			case TokenType::StructLiteralOp:
				pLeft =	ParseStructLiteral(state, pLeft);
				break;	
			case TokenType::ArrayLiteralOp:
				pLeft =	ParseArrayLiteral(state, pLeft);
				break;	
			case TokenType::Dot:
				pLeft = ParseSelector(state, pLeft);
				break; 
			case TokenType::LeftBracket:
				pLeft = ParseSelector(state, pLeft);
				break;
			case TokenType::Equal:
				pLeft = ParseAssignment(state, pLeft);
				break;
			case TokenType::Caret:
				pLeft = ParseDereference(state, pLeft);
				break;
			case TokenType::Semicolon:
				pLeft = MakeNode<Ast::BadExpression>(state.pAllocator, *state.pCurrent, Ast::NodeKind::BadExpression);
				break;	
			default:
				Advance(state);
				pLeft = MakeNode<Ast::BadExpression>(state.pAllocator, *state.pCurrent, Ast::NodeKind::BadExpression);
		}
	}
	return pLeft;
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
	Ast::Expression* pExpr = ParseExpression(state, Precedence::None);
    Consume(state, TokenType::Semicolon, "Expected \";\" at the end of this statement");

    Ast::ExpressionStmt* pStmt = MakeNode<Ast::ExpressionStmt>(state.pAllocator, Previous(state), Ast::NodeKind::ExpressionStmt);
    pStmt->pExpr = pExpr;
    return pStmt;
}

// ***********************************************************************

Ast::Statement* ParseIf(ParsingState& state) {
    Ast::If* pIf = MakeNode<Ast::If>(state.pAllocator, Previous(state), Ast::NodeKind::If);

	pIf->pCondition = ParseExpression(state, Precedence::None);
	pIf->pThenStmt = ParseStatement(state);

    if (Match(state, 1, TokenType::Else)) {
		pIf->pElseStmt = ParseStatement(state);
    }
    return pIf;
}

// ***********************************************************************

Ast::Statement* ParseWhile(ParsingState& state) {
    Ast::While* pWhile = MakeNode<Ast::While>(state.pAllocator, Previous(state), Ast::NodeKind::While);

	pWhile->pCondition = ParseExpression(state, Precedence::None);
	pWhile->pBody = ParseStatement(state);

    return pWhile;
}

// ***********************************************************************

Ast::Statement* ParsePrint(ParsingState& state) {
    Consume(state, TokenType::LeftParen, "Expected \"(\" following print, before the expression starts");
	Ast::Expression* pExpr = ParseExpression(state, Precedence::None);
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
		pReturnStmt->pExpr = ParseExpression(state, Precedence::None);
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
                pDecl->pInitializerExpr = ParseExpression(state, Precedence::None);
                if (pDecl->pInitializerExpr->nodeKind == Ast::NodeKind::BadExpression)
                    PushError(state, "Need an expression to initialize this constant declaration");
            }

            // Parse initializer
            if (Match(state, 1, TokenType::Equal)) {
                pDecl->isConstantDeclaration = false;
				pDecl->pInitializerExpr = ParseExpression(state, Precedence::None);

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
        case Ast::NodeKind::BadExpression: {
            Ast::BadExpression* pBad = (Ast::BadExpression*)pExpr;
            Log::Debug("%*s> Bad Expression", indentationLevel, "");
            break;
        }
        default:
            break;
    }
}
