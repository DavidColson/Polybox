// Copyright 2020-2022 David Colson. All rights reserved.

#include "code_gen.h"

#include "parser.h"
#include "virtual_machine.h"

#include <resizable_array.inl>
#include <hashmap.inl>

namespace {
struct Local {
    String name;
    int depth;
};

struct State {
    ResizableArray<Local> locals;
    int currentScopeDepth { 0 };
    ErrorState* pErrors;
    Function* pFunc;
};
}

// ***********************************************************************

Function* CurrentFunction(State& state) {
    return state.pFunc;
}

// ***********************************************************************

int ResolveLocal(State& state, String name) {
    for (int i = state.locals.count - 1; i >= 0; i--) {
        Local& local = state.locals[i];
        if (name == local.name) {
            return i;
        }
    }
    return -1;
}

// ***********************************************************************

void PushCode4Byte(State& state, uint32_t code, uint32_t line) {
	CurrentFunction(state)->code.PushBack((code >> 24) & 0xff);
	CurrentFunction(state)->code.PushBack((code >> 16) & 0xff);
	CurrentFunction(state)->code.PushBack((code >> 8) & 0xff);
	CurrentFunction(state)->code.PushBack(code & 0xff);

	CurrentFunction(state)->dbgLineInfo.PushBack(line);
	CurrentFunction(state)->dbgLineInfo.PushBack(line);
	CurrentFunction(state)->dbgLineInfo.PushBack(line);
	CurrentFunction(state)->dbgLineInfo.PushBack(line);
	Assert(line != 0);
}

// ***********************************************************************

void PushCode(State& state, uint8_t code, uint32_t line) {
    CurrentFunction(state)->code.PushBack(code);
    CurrentFunction(state)->dbgLineInfo.PushBack(line);
	Assert(line != 0);
}

// ***********************************************************************

int PushJump(State& state, OpCode::Enum jumpCode, uint32_t line) {
    PushCode(state, jumpCode, line);
    PushCode(state, 0xff, line);
    PushCode(state, 0xff, line);
    return CurrentFunction(state)->code.count - 2;
}

// ***********************************************************************

void PushLoop(State& state, uint8_t loopTarget, uint32_t line) {
    PushCode(state, OpCode::Loop, line);

    int offset = CurrentFunction(state)->code.count - loopTarget + 2;
    PushCode(state, (offset >> 8) & 0xff, line);
    PushCode(state, offset & 0xff, line);
}

// ***********************************************************************

void PatchJump(State& state, int jumpCodeLocation) {
    int jumpOffset = CurrentFunction(state)->code.count - jumpCodeLocation - 2;
    CurrentFunction(state)->code[jumpCodeLocation] = (jumpOffset >> 8) & 0xff;
    CurrentFunction(state)->code[jumpCodeLocation + 1] = jumpOffset & 0xff;
}

// ***********************************************************************

void CodeGenExpression(State& state, Ast::Expression* pExpr) {
    switch (pExpr->nodeKind) {
        case Ast::NodeType::Identifier: {
            Ast::Identifier* pVariable = (Ast::Identifier*)pExpr;
            int localIndex = ResolveLocal(state, pVariable->identifier);
            if (localIndex != -1) {
                PushCode(state, OpCode::GetLocal, pVariable->line);
                PushCode(state, localIndex, pVariable->line);
            }
            break;
        }
        case Ast::NodeType::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            CodeGenExpression(state, pVarAssignment->pAssignment);
            int localIndex = ResolveLocal(state, pVarAssignment->identifier);
            if (localIndex != -1) {
                PushCode(state, OpCode::SetLocal, pVarAssignment->line);
                PushCode(state, localIndex, pVarAssignment->line);
            }
            break;
        }
		case Ast::NodeType::SetField: {
			Ast::SetField* pSetField = (Ast::SetField*)pExpr;

			CodeGenExpression(state, pSetField->pTarget);
			CodeGenExpression(state, pSetField->pAssignment);

			TypeInfoStruct* pTargetType = (TypeInfoStruct*)pSetField->pTarget->pType;
			TypeInfoStruct::Member* pTargetField = nullptr;
			for (size_t i = 0; i < pTargetType->members.count; i++) {
				TypeInfoStruct::Member& mem = pTargetType->members[i];
				if (mem.identifier == pSetField->fieldName) {
					pTargetField = &pTargetType->members[i];
					break;
				}
			}

			if (pTargetField->pType->tag == TypeInfo::TypeTag::Struct) {
				PushCode(state, OpCode::SetFieldStruct, pSetField->line);
			} else {
				PushCode(state, OpCode::SetField, pSetField->line);
			}
			PushCode4Byte(state, (uint32_t)pTargetField->offset, pSetField->line);
			PushCode4Byte(state, (uint32_t)pTargetField->pType->size, pSetField->line);
			break;
		}
		case Ast::NodeType::GetField: {
			Ast::GetField* pGetField = (Ast::GetField*)pExpr;
			
			CodeGenExpression(state, pGetField->pTarget);

			TypeInfoStruct* pTargetType = (TypeInfoStruct*)pGetField->pTarget->pType;
			TypeInfoStruct::Member* pTargetField = nullptr;
			for (size_t i = 0; i < pTargetType->members.count; i++) {
				TypeInfoStruct::Member& mem = pTargetType->members[i];
				if (mem.identifier == pGetField->fieldName) {
					pTargetField = &pTargetType->members[i];
					break;
				}
			}

			if (pTargetField->pType->tag == TypeInfo::TypeTag::Struct) {
				PushCode(state, OpCode::GetFieldStruct, pGetField->line);
			} else {
				PushCode(state, OpCode::GetField, pGetField->line);
			}

			PushCode4Byte(state, (uint32_t)pTargetField->offset, pGetField->line);
			PushCode4Byte(state, (uint32_t)pTargetField->pType->size, pGetField->line);
			break;
		}
        case Ast::NodeType::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;

            CurrentFunction(state)->constants.PushBack(pLiteral->value);
			CurrentFunction(state)->dbgConstantsTypes.PushBack(pLiteral->pType);
            uint8_t constIndex = (uint8_t)CurrentFunction(state)->constants.count - 1;

            PushCode(state, OpCode::LoadConstant, pLiteral->line);
            PushCode(state, constIndex, pLiteral->line);
            break;
        }
        case Ast::NodeType::Type:
        case Ast::NodeType::FnType: {
            Ast::Type* pType = (Ast::Type*)pExpr;

			CurrentFunction(state)->constants.PushBack(MakeValue(pType->pResolvedType));
			CurrentFunction(state)->dbgConstantsTypes.PushBack(GetTypeType());
            uint8_t constIndex = (uint8_t)CurrentFunction(state)->constants.count - 1;

            PushCode(state, OpCode::LoadConstant, pType->line);
            PushCode(state, constIndex, pType->line);
            break;
        }
        case Ast::NodeType::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            Function* pFunc = CodeGen(pFunction->pBody->declarations, pFunction->params, pFunction->identifier, state.pErrors);

            Value value;
            value.pFunction = pFunc;
			CurrentFunction(state)->constants.PushBack(value);
			CurrentFunction(state)->dbgConstantsTypes.PushBack(pFunction->pType);
            uint8_t constIndex = (uint8_t)CurrentFunction(state)->constants.count - 1;

            PushCode(state, OpCode::LoadConstant, pFunction->line);
            PushCode(state, constIndex, pFunction->line);
            break;
        }
		case Ast::NodeType::Structure: {
			Ast::Structure* pStructure = (Ast::Structure*)pExpr;

			// Defines a type which we put on the stack as a local
			CurrentFunction(state)->constants.PushBack(MakeValue(pStructure->pDescribedType));
			CurrentFunction(state)->dbgConstantsTypes.PushBack(pStructure->pDescribedType);
			uint8_t constIndex = (uint8_t)CurrentFunction(state)->constants.count - 1;

			PushCode(state, OpCode::LoadConstant, pStructure->line);
			PushCode(state, constIndex, pStructure->line);
			break;
		}
        case Ast::NodeType::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;

            CodeGenExpression(state, pGroup->pExpression);
            break;
        }
        case Ast::NodeType::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;

            if (pBinary->op == Operator::And) {
                CodeGenExpression(state, pBinary->pLeft);
                int andJump = PushJump(state, OpCode::JmpIfFalse, pBinary->line);
                PushCode(state, OpCode::Pop, pBinary->line);
                CodeGenExpression(state, pBinary->pRight);
                PatchJump(state, andJump);
                break;
            }

            if (pBinary->op == Operator::Or) {
                CodeGenExpression(state, pBinary->pLeft);
                int andJump = PushJump(state, OpCode::JmpIfTrue, pBinary->line);
                PushCode(state, OpCode::Pop, pBinary->line);
                CodeGenExpression(state, pBinary->pRight);
                PatchJump(state, andJump);
                break;
            }

            CodeGenExpression(state, pBinary->pLeft);
            CodeGenExpression(state, pBinary->pRight);
            switch (pBinary->op) {
                case Operator::Add:
					PushCode(state, OpCode::Add, pBinary->line);
                    break;
                case Operator::Subtract:
                    PushCode(state, OpCode::Subtract, pBinary->line);
                    break;
                case Operator::Divide:
                    PushCode(state, OpCode::Divide, pBinary->line);
                    break;
                case Operator::Multiply:
                    PushCode(state, OpCode::Multiply, pBinary->line);
                    break;
                case Operator::Greater:
                    PushCode(state, OpCode::Greater, pBinary->line);
                    break;
                case Operator::Less:
                    PushCode(state, OpCode::Less, pBinary->line);
                    break;
                case Operator::GreaterEqual:
                    PushCode(state, OpCode::GreaterEqual, pBinary->line);
                    break;
                case Operator::LessEqual:
                    PushCode(state, OpCode::LessEqual, pBinary->line);
                    break;
                case Operator::Equal:
                    PushCode(state, OpCode::Equal, pBinary->line);
                    break;
                case Operator::NotEqual:
                    PushCode(state, OpCode::NotEqual, pBinary->line);
                    break;
                default:
                    break;
            }
			PushCode(state, pBinary->pLeft->pType->tag, pBinary->line);
			break;
        }
        case Ast::NodeType::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            CodeGenExpression(state, pUnary->pRight);
            switch (pUnary->op) {
                case Operator::Subtract:
                    PushCode(state, OpCode::Negate, pUnary->line);
                    break;
                case Operator::Not:
                    PushCode(state, OpCode::Not, pUnary->line);
                    break;
                default:
                    break;
            }
			PushCode(state, pUnary->pRight->pType->tag, pUnary->line);
			break;
        }
		case Ast::NodeType::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;
			CodeGenExpression(state, pCast->pExprToCast);

			PushCode(state, OpCode::Cast, pCast->line);

			if (pCast->pExprToCast->pType == GetI32Type()) {
				PushCode(state, (uint8_t)TypeInfo::I32, pCast->line);
			} else if (pCast->pExprToCast->pType == GetF32Type()) {
				PushCode(state, (uint8_t)TypeInfo::F32, pCast->line);
			} else if (pCast->pExprToCast->pType == GetBoolType()) {
				PushCode(state, (uint8_t)TypeInfo::Bool, pCast->line);
			} else {
				AssertMsg(false, "Don't know how to cast non base types yet");
			}

			if (pCast->pTargetType->pResolvedType == GetI32Type()) {
				PushCode(state, (uint8_t)TypeInfo::I32, pCast->line);
			} else if (pCast->pTargetType->pResolvedType == GetF32Type()) {
				PushCode(state, (uint8_t)TypeInfo::F32, pCast->line);
			} else if (pCast->pTargetType->pResolvedType == GetBoolType()) {
				PushCode(state, (uint8_t)TypeInfo::Bool, pCast->line);
			} else {
				AssertMsg(false, "Don't know how to cast non base types yet");
			}
			break;
		}
        case Ast::NodeType::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;
            CodeGenExpression(state, pCall->pCallee);

            for (Ast::Expression* pExpr : pCall->args) {
                CodeGenExpression(state, pExpr);
            }

            PushCode(state, OpCode::Call, pCall->line);
            PushCode(state, pCall->args.count, pCall->line);  // arg count
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void CodeGenStatements(State& state, ResizableArray<Ast::Statement*>& statements);

void CodeGenStatement(State& state, Ast::Statement* pStmt) {
    switch (pStmt->nodeKind) {
        case Ast::NodeType::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;

            Local local;
            local.depth = state.currentScopeDepth;
            local.name = pDecl->identifier;
            state.locals.PushBack(local);

            if (pDecl->pInitializerExpr)
                CodeGenExpression(state, pDecl->pInitializerExpr);
            else {
				if (pDecl->pResolvedType->tag == TypeInfo::Struct)
				{
					// allocate memory for the struct
					PushCode(state, OpCode::StructAlloc, pDecl->line);
					PushCode4Byte(state, (uint32_t)pDecl->pResolvedType->size, pDecl->line); // TODO: do I support 4gb structs?

				} else { // For all non struct values we just push a new value on the operand stack that is zero
					Value v;
					v.pPtr = 0;
					CurrentFunction(state)->constants.PushBack(v);
					CurrentFunction(state)->dbgConstantsTypes.PushBack(pDecl->pResolvedType);
					uint8_t constIndex = (uint8_t)CurrentFunction(state)->constants.count - 1;

					PushCode(state, OpCode::LoadConstant, pDecl->line);
					PushCode(state, constIndex, pDecl->line);
				}
            }
            break;
        }
        case Ast::NodeType::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            CodeGenExpression(state, pPrint->pExpr);
            PushCode(state, OpCode::Print, pPrint->line);
			PushCode(state, pPrint->pExpr->pType->tag, pPrint->line);
			break;
        }
        case Ast::NodeType::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            if (pReturn->pExpr)
                CodeGenExpression(state, pReturn->pExpr);
            PushCode(state, OpCode::Return, pReturn->line);
            break;
        }
        case Ast::NodeType::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            CodeGenExpression(state, pExprStmt->pExpr);
            PushCode(state, OpCode::Pop, pExprStmt->line);
            break;
        }
        case Ast::NodeType::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            CodeGenExpression(state, pIf->pCondition);

            int ifJump = PushJump(state, OpCode::JmpIfFalse, pIf->line);
            PushCode(state, OpCode::Pop, pIf->pThenStmt->line);

            CodeGenStatement(state, pIf->pThenStmt);
            int elseJump = PushJump(state, OpCode::Jmp, pIf->pElseStmt->line);
            PatchJump(state, ifJump);

            PushCode(state, OpCode::Pop, pIf->pElseStmt->line);
            if (pIf->pElseStmt) {
                CodeGenStatement(state, pIf->pElseStmt);
            }
            PatchJump(state, elseJump);

            break;
        }
        case Ast::NodeType::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            int loopStart = CurrentFunction(state)->code.count;
            CodeGenExpression(state, pWhile->pCondition);

            int ifJump = PushJump(state, OpCode::JmpIfFalse, pWhile->line);
            PushCode(state, OpCode::Pop, pWhile->line);

            CodeGenStatement(state, pWhile->pBody);
            PushLoop(state, loopStart, pWhile->pBody->line);

            PatchJump(state, ifJump);
            PushCode(state, OpCode::Pop, pWhile->pBody->line);
            break;
        }
        case Ast::NodeType::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            state.currentScopeDepth++;
            CodeGenStatements(state, pBlock->declarations);
            state.currentScopeDepth--;
            while (state.locals.count > 0 && state.locals[state.locals.count - 1].depth > state.currentScopeDepth) {
                state.locals.PopBack();
                PushCode(state, OpCode::Pop, pBlock->endToken.line);
            }
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void CodeGenStatements(State& state, ResizableArray<Ast::Statement*>& statements) {
    for (size_t i = 0; i < statements.count; i++) {
        Ast::Statement* pStmt = statements[i];
        CodeGenStatement(state, pStmt);
    }
}

// ***********************************************************************

Function* CodeGen(ResizableArray<Ast::Statement*>& program, ResizableArray<Ast::Declaration*>& params, String name, ErrorState* pErrorState) {
    State state;

    Local local;
    local.depth = 0;
    local.name = name;
    state.locals.PushBack(local); // This is the local representing this function, allows it to call itself

    state.pFunc = (Function*)g_Allocator.Allocate(sizeof(Function));
    SYS_P_NEW(state.pFunc) Function();
    state.pFunc->name = name;
    
    // Create locals for the params that have been passed in from the caller
    for (Ast::Declaration* pDecl : params) {
        Local local;
        local.depth = state.currentScopeDepth;
        local.name = pDecl->identifier;
        state.locals.PushBack(local);
    }

    CodeGenStatements(state, program);

    // return void
    Ast::Statement* pEnd = program[program.count - 1];
	CurrentFunction(state)->constants.PushBack(Value());
	CurrentFunction(state)->dbgConstantsTypes.PushBack(GetVoidType());
    uint8_t constIndex = (uint8_t)CurrentFunction(state)->constants.count - 1;
    PushCode(state, OpCode::LoadConstant, pEnd->line);
    PushCode(state, constIndex, pEnd->line);
    PushCode(state, OpCode::Return, pEnd->line);

    return state.pFunc;
}