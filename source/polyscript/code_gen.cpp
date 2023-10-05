// Copyright 2020-2022 David Colson. All rights reserved.

#include "code_gen.h"

#include "parser.h"
#include "virtual_machine.h"
#include "type_checker.h"
#include "compiler.h"

#include <resizable_array.inl>
#include <hashmap.inl>

namespace {
struct Local {
    String name;
};

struct State {
    ResizableArray<Local> locals;
    Scope* pCurrentScope;
    ErrorState* pErrors;
    Function* pFunc;
	IAllocator* pAlloc;
};

struct CodeGenState {
    IAllocator* pOutputAllocator;
    IAllocator* pCompilerAllocator;
    
	Scope* pGlobalScope;
	Scope* pCurrentScope;
    ResizableArray<String> localsStack;
    uint32_t currentScopeLocalsBase{ 0 }; // Size of the locals stack when we first entered the current scope
    
    ErrorState* pErrors;
    Program* pCurrentlyCompilingProgram;
    Function* pCurrentlyCompilingFunction;

};
}

// ***********************************************************************

void EnterScope(CodeGenState& state, Scope* pNewScope) {
    state.pCurrentScope = pNewScope;
    state.currentScopeLocalsBase = state.localsStack.count;
}

// ***********************************************************************

void ExitScope(CodeGenState& state, Scope* pOldScope) {
    state.pCurrentScope = pOldScope;
    while (state.localsStack.count > state.currentScopeLocalsBase) {
        state.localsStack.PopBack();
    }
}

// ***********************************************************************

Function* CurrentFunction(CodeGenState& state) {
    return state.pCurrentlyCompilingFunction;
}

// ***********************************************************************

Program* CurrentProgram(CodeGenState& state) {
    return state.pCurrentlyCompilingProgram;
}

// ***********************************************************************

int ResolveLocal(CodeGenState& state, String name) {
    for (int i = state.localsStack.count - 1; i >= 0; i--) {
        String& local = state.localsStack[i];
        if (name == local) {
            return i;
        }
    }
    return -1;
}

// ***********************************************************************

uint8_t CreateConstant(CodeGenState& state, Value constant, TypeInfo* pType) {
    CurrentProgram(state)->constantTable.PushBack(constant);
    CurrentProgram(state)->dbgConstantsTypes.PushBack(pType);
    // TODO: When you refactor instructions, allow space for bigger constant indices
    return (uint8_t)CurrentProgram(state)->constantTable.count - 1;
}

// ***********************************************************************

void PushCode4Byte(CodeGenState& state, uint32_t code, uint32_t line) {
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

void PushCode(CodeGenState& state, uint8_t code, uint32_t line) {
    CurrentFunction(state)->code.PushBack(code);
    CurrentFunction(state)->dbgLineInfo.PushBack(line);
	Assert(line != 0);
}

// ***********************************************************************

int PushJump(CodeGenState& state, OpCode::Enum jumpCode, uint32_t line) {
    PushCode(state, jumpCode, line);
    PushCode(state, 0xff, line);
    PushCode(state, 0xff, line);
    return CurrentFunction(state)->code.count - 2;
}

// ***********************************************************************

void PushLoop(CodeGenState& state, uint32_t loopTarget, uint32_t line) {
    PushCode(state, OpCode::Loop, line);

    int offset = CurrentFunction(state)->code.count - loopTarget + 2;
    PushCode(state, (offset >> 8) & 0xff, line);
    PushCode(state, offset & 0xff, line);
}

// ***********************************************************************

void PatchJump(CodeGenState& state, int jumpCodeLocation) {
    int jumpOffset = CurrentFunction(state)->code.count - jumpCodeLocation - 2;
    CurrentFunction(state)->code[jumpCodeLocation] = (jumpOffset >> 8) & 0xff;
    CurrentFunction(state)->code[jumpCodeLocation + 1] = jumpOffset & 0xff;
}

// ***********************************************************************

void CodeGenStatements(CodeGenState& state, ResizableArray<Ast::Statement*>& statements);

void CodeGenFunction(CodeGenState& state, String identifier, Ast::Function* pFunction) {
    state.pCurrentlyCompilingFunction->name = CopyString(identifier, state.pOutputAllocator);

    // Push local for this function
    state.localsStack.PushBack(identifier);

    // Put input params in locals stack
    for (Ast::Declaration* pDecl : pFunction->pFuncType->params) {
        state.localsStack.PushBack(pDecl->identifier);
    }

    // Codegen the statements in the body
    CodeGenStatements(state, pFunction->pBody->declarations);

    // Put a return instruction at the end of the function
    uint8_t constIndex = CreateConstant(state, Value(), GetVoidType());

    PushCode(state, OpCode::LoadConstant, pFunction->pBody->endToken.line);
    PushCode(state, constIndex, pFunction->pBody->endToken.line);
    PushCode(state, OpCode::Return, pFunction->pBody->endToken.line);
}

// ***********************************************************************

void CodeGenExpression(CodeGenState& state, Ast::Expression* pExpr) {

    // Constant folding
    // If this expression is a constant, there's no need to generate any bytecode, just emit the constant literal
    // Functions need their bodies codegen'd so we let them through
    // Identifiers are already in the constant table, so we let them through and they'll be looked up and reused
    if (pExpr->isConstant && (pExpr->nodeKind != Ast::NodeKind::Function && pExpr->nodeKind != Ast::NodeKind::Identifier)) {
        uint8_t constIndex = CreateConstant(state, pExpr->constantValue, pExpr->pType);
        PushCode(state, OpCode::LoadConstant, pExpr->line);
        PushCode(state, constIndex, pExpr->line);
        return;
    }

    switch (pExpr->nodeKind) {
        case Ast::NodeKind::Identifier: {
            Ast::Identifier* pVariable = (Ast::Identifier*)pExpr;
            if (pVariable->isConstant) {
                // Load from the constant table (it's already been put there)
                // TODO: We've written this "find entity" bit of code a few times now, refactor it out
                Entity* pEntity = nullptr;
                Scope* pSearchScope = state.pGlobalScope;
                while(pEntity == nullptr && pSearchScope != nullptr) {
                    Entity** pEntry = pSearchScope->entities.Get(pVariable->identifier);
                    if (pEntry == nullptr) {
                        pSearchScope = pSearchScope->pParent;
                    } else {
                        pEntity = *pEntry;
                    }
                }

                PushCode(state, OpCode::LoadConstant, pVariable->line);
                PushCode(state, pEntity->codeGenConstIndex, pVariable->line);
            } else {
                // do a load from locals memory
                int localIndex = ResolveLocal(state, pVariable->identifier);
                if (localIndex != -1) {
                    PushCode(state, OpCode::GetLocal, pVariable->line);
                    PushCode(state, localIndex, pVariable->line);
                }
            }
            break;
        }
        case Ast::NodeKind::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            CodeGenExpression(state, pVarAssignment->pAssignment);
            int localIndex = ResolveLocal(state, pVarAssignment->pIdentifier->identifier);
            if (localIndex != -1) {
                PushCode(state, OpCode::SetLocal, pVarAssignment->line);
                PushCode(state, localIndex, pVarAssignment->line);
            }
            break;
        }
		case Ast::NodeKind::SetField: {
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
		case Ast::NodeKind::GetField: {
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
        case Ast::NodeKind::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            uint8_t constIndex = CreateConstant(state, pLiteral->constantValue, pLiteral->pType);
            PushCode(state, OpCode::LoadConstant, pLiteral->line);
            PushCode(state, constIndex, pLiteral->line);
            break;
        }
        case Ast::NodeKind::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            
            Function* pNewFunction = (Function*)state.pOutputAllocator->Allocate(sizeof(Function));
            SYS_P_NEW(pNewFunction) Function();
            pNewFunction->code.pAlloc = state.pOutputAllocator;
            pNewFunction->dbgLineInfo.pAlloc = state.pOutputAllocator;
            
            Scope* pPreviousScope = state.pCurrentScope;
            Function* pParentFunction = CurrentFunction(state);
            state.pCurrentlyCompilingFunction = pNewFunction;
            EnterScope(state, pFunction->pScope);
            CodeGenFunction(state, pFunction->pType->name, pFunction);
            ExitScope(state, pPreviousScope);
            state.pCurrentlyCompilingFunction = pParentFunction;

            uint8_t constIndex = CreateConstant(state, MakeValue(pNewFunction), pFunction->pType);
            PushCode(state, OpCode::LoadConstant, pFunction->line);
            PushCode(state, constIndex, pFunction->line);
        
            break;
        }
        case Ast::NodeKind::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;

            CodeGenExpression(state, pGroup->pExpression);
            break;
        }
        case Ast::NodeKind::Binary: {
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
        case Ast::NodeKind::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            CodeGenExpression(state, pUnary->pRight);
            switch (pUnary->op) {
                case Operator::UnaryMinus:
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
		case Ast::NodeKind::Cast: {
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

			if (pCast->pTypeExpr->constantValue.pTypeInfo == GetI32Type()) {
				PushCode(state, (uint8_t)TypeInfo::I32, pCast->line);
			} else if (pCast->pTypeExpr->constantValue.pTypeInfo == GetF32Type()) {
				PushCode(state, (uint8_t)TypeInfo::F32, pCast->line);
			} else if (pCast->pTypeExpr->constantValue.pTypeInfo == GetBoolType()) {
				PushCode(state, (uint8_t)TypeInfo::Bool, pCast->line);
			} else {
				AssertMsg(false, "Don't know how to cast non base types yet");
			}
			break;
		}
        case Ast::NodeKind::Call: {
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

void CodeGenStatement(CodeGenState& state, Ast::Statement* pStmt) {
    switch (pStmt->nodeKind) {
        case Ast::NodeKind::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;

            if (pDecl->isConstantDeclaration) {
                // Codegen the bodies of functions, but otherwise do nothing here
                if (pDecl->pInitializerExpr->nodeKind == Ast::NodeKind::Function) {
                    Ast::Function* pFunction = (Ast::Function*)pDecl->pInitializerExpr;

                    Entity* pEntity = nullptr;
                    Scope* pSearchScope = state.pCurrentScope;
                    while(pEntity == nullptr && pSearchScope != nullptr) {
                        Entity** pEntry = pSearchScope->entities.Get(pDecl->identifier);
                        if (pEntry == nullptr) {
                            pSearchScope = pSearchScope->pParent;
                        } else {
                            pEntity = *pEntry;
                        }
                    }

                    if (pEntity) {
                        Function* pParentFunction = CurrentFunction(state);
                        state.pCurrentlyCompilingFunction = pEntity->constantValue.pFunction;
                        CodeGenFunction(state, pEntity->name, pFunction);
                        state.pCurrentlyCompilingFunction = pParentFunction;
                    }
                }
                break;
            }

            state.localsStack.PushBack(pDecl->identifier);

            if (pDecl->pInitializerExpr)
                CodeGenExpression(state, pDecl->pInitializerExpr);
            else {
				if (pDecl->pType->tag == TypeInfo::Struct)
				{
					// allocate memory for the struct
					PushCode(state, OpCode::StructAlloc, pDecl->line);
					PushCode4Byte(state, (uint32_t)pDecl->pType->size, pDecl->line); // TODO: do I support 4gb structs?

				} else { // For all non struct values we just push a new value on the operand stack that is zero
					Value v;
					v.pPtr = 0;
                    uint8_t constIndex = CreateConstant(state, v, pDecl->pType);

					PushCode(state, OpCode::LoadConstant, pDecl->line);
					PushCode(state, constIndex, pDecl->line);
				}
            }
            break;
        }
        case Ast::NodeKind::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            CodeGenExpression(state, pPrint->pExpr);
            PushCode(state, OpCode::Print, pPrint->line);
			PushCode(state, pPrint->pExpr->pType->tag, pPrint->line);
			break;
        }
        case Ast::NodeKind::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            if (pReturn->pExpr)
                CodeGenExpression(state, pReturn->pExpr);
            PushCode(state, OpCode::Return, pReturn->line);
            break;
        }
        case Ast::NodeKind::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            CodeGenExpression(state, pExprStmt->pExpr);
            PushCode(state, OpCode::Pop, pExprStmt->line);
            break;
        }
        case Ast::NodeKind::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            CodeGenExpression(state, pIf->pCondition);

            int ifJump = PushJump(state, OpCode::JmpIfFalse, pIf->line);
            PushCode(state, OpCode::Pop, pIf->pThenStmt->line);

            CodeGenStatement(state, pIf->pThenStmt);
            if (pIf->pElseStmt) {
                int elseJump = PushJump(state, OpCode::Jmp, pIf->pElseStmt->line);
                PatchJump(state, ifJump);

                PushCode(state, OpCode::Pop, pIf->pElseStmt->line);
                if (pIf->pElseStmt) {
                    CodeGenStatement(state, pIf->pElseStmt);
                }
                PatchJump(state, elseJump);
            } else {
                PatchJump(state, ifJump);
            }

            break;
        }
        case Ast::NodeKind::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            uint32_t loopStart = CurrentFunction(state)->code.count;
            CodeGenExpression(state, pWhile->pCondition);

            int ifJump = PushJump(state, OpCode::JmpIfFalse, pWhile->line);
            PushCode(state, OpCode::Pop, pWhile->line);

            CodeGenStatement(state, pWhile->pBody);
            PushLoop(state, loopStart, pWhile->pBody->line);

            PatchJump(state, ifJump);
            PushCode(state, OpCode::Pop, pWhile->pBody->line);
            break;
        }
        case Ast::NodeKind::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            Scope* pPreviousScope = state.pCurrentScope;
            EnterScope(state, pBlock->pScope);
            CodeGenStatements(state, pBlock->declarations);
            ExitScope(state, pPreviousScope);
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void CodeGenStatements(CodeGenState& state, ResizableArray<Ast::Statement*>& statements) {
    for (size_t i = 0; i < statements.count; i++) {
        Ast::Statement* pStmt = statements[i];
        CodeGenStatement(state, pStmt);
    }
}

// ***********************************************************************

void GenerateConstantTable(CodeGenState& state, Scope* pScope) {
    for (size_t i = 0; i < pScope->entities.tableSize; i++) { 
            HashNode<String, Entity*>& node = pScope->entities.pTable[i];
            if (node.hash != UNUSED_HASH) {
                Entity* pEntity = node.value;
                if (pEntity->kind == EntityKind::Constant && pEntity->pDeclaration->pInitializerExpr) {
                    if (pEntity->pDeclaration->pInitializerExpr->nodeKind == Ast::NodeKind::Function) {
                        Function* pFunctionLiteral = (Function*)state.pOutputAllocator->Allocate(sizeof(Function));
                        SYS_P_NEW(pFunctionLiteral) Function();
                        pFunctionLiteral->code.pAlloc = state.pOutputAllocator;
                        pFunctionLiteral->dbgLineInfo.pAlloc = state.pOutputAllocator;

                        pEntity->constantValue.pFunction = pFunctionLiteral;
                        pEntity->codeGenConstIndex = CreateConstant(state, MakeValue(pFunctionLiteral), pEntity->pType);
                    } else {
                        pEntity->codeGenConstIndex = CreateConstant(state, pEntity->constantValue, pEntity->pType);
                    }
                }
            }
    }

    for (Scope* pChildScope : pScope->children) {
        GenerateConstantTable(state, pChildScope);
    }
}

void CodeGenProgram(Compiler& compilerState) {
    if (compilerState.errorState.errors.count == 0) {
        
        // Set up state
        CodeGenState state;
        state.pOutputAllocator = &compilerState.outputMemory;
        state.pCompilerAllocator = &compilerState.compilerMemory;
        state.pErrors = &compilerState.errorState;
        state.pGlobalScope = compilerState.pGlobalScope;
        state.pCurrentScope = state.pGlobalScope;
        state.pCurrentlyCompilingProgram = (Program*)state.pOutputAllocator->Allocate(sizeof(Program));

        // Create a program to output to
        SYS_P_NEW(state.pCurrentlyCompilingProgram) Program();
        state.pCurrentlyCompilingProgram->constantTable.pAlloc = state.pOutputAllocator;
        state.pCurrentlyCompilingProgram->dbgConstantsTypes.pAlloc = state.pOutputAllocator;

        GenerateConstantTable(state, state.pGlobalScope);

        // Create the top level script function (corresponding to the primary script file)
        state.pCurrentlyCompilingFunction = (Function*)state.pOutputAllocator->Allocate(sizeof(Function));
        SYS_P_NEW(state.pCurrentlyCompilingFunction) Function();
        state.pCurrentlyCompilingFunction->name = "<main>";
        state.pCurrentlyCompilingFunction->code.pAlloc = state.pOutputAllocator;
        state.pCurrentlyCompilingFunction->dbgLineInfo.pAlloc = state.pOutputAllocator;
        state.pCurrentlyCompilingProgram->pMainModuleFunction = state.pCurrentlyCompilingFunction;

        // Set off actual codegen of the main file, will recursively codegen all functions inside it
        CodeGenStatements(state, compilerState.syntaxTree);

        // Put a return instruction at the end of the program
        Token endToken = compilerState.tokens[compilerState.tokens.count - 1];
        uint8_t constIndex = CreateConstant(state, Value(), GetVoidType());
        PushCode(state, OpCode::LoadConstant, endToken.line);
        PushCode(state, constIndex, endToken.line);
        PushCode(state, OpCode::Return, endToken.line);

        compilerState.pProgram = state.pCurrentlyCompilingProgram;
    }
}