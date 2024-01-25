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
    
    ErrorState* pErrors;
    Program* pCurrentlyCompilingProgram;
};
}

// ***********************************************************************

int PushInstruction(CodeGenState& state, uint32_t line, Instruction instruction, TypeInfo::TypeTag tag = TypeInfo::TypeTag::Invalid) {
    state.pCurrentlyCompilingProgram->code.PushBack(instruction);
    state.pCurrentlyCompilingProgram->dbgLineInfo.PushBack(line);
	if (tag != TypeInfo::TypeTag::Invalid) {
		state.pCurrentlyCompilingProgram->dbgConstantsTypes.Add(state.pCurrentlyCompilingProgram->code.count - 1, tag);
	}
    return state.pCurrentlyCompilingProgram->code.count - 1;
}

// ***********************************************************************

void EnterScope(CodeGenState& state, Scope* pNewScope) {
    state.pCurrentScope = pNewScope;
    if (pNewScope->kind == ScopeKind::Function) {
        state.pCurrentScope->codeGenStackFrameBase = state.localsStack.count;
    } else {
        // If we're not a function scope, then presumably some scope above us is a function scope
        Scope* pParentFunctionScope = pNewScope;
        while (pParentFunctionScope != nullptr && pParentFunctionScope->kind != ScopeKind::Global && pParentFunctionScope->kind != ScopeKind::Function) {
            pParentFunctionScope = pParentFunctionScope->pParent;
        }
        state.pCurrentScope->codeGenStackFrameBase = pParentFunctionScope->codeGenStackFrameBase;
    }
    state.pCurrentScope->codeGenStackAtEntry = state.localsStack.count;
}

// ***********************************************************************

void ExitScope(CodeGenState& state, Scope* pOldScope, Token popToken) {
    while (state.localsStack.count > state.pCurrentScope->codeGenStackAtEntry) {
        state.localsStack.PopBack();

        if (state.pCurrentScope->kind == ScopeKind::Block) {
            PushInstruction(state, popToken.line, { .opcode = OpCode::Pop });
        }
    }
    state.pCurrentScope = pOldScope;
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

void PatchJump(CodeGenState& state, int jumpInstructionIndex) {
    state.pCurrentlyCompilingProgram->code[jumpInstructionIndex].ipOffset = state.pCurrentlyCompilingProgram->code.count - 1; 
}

// ***********************************************************************

void CodeGenStatements(CodeGenState& state, ResizableArray<Ast::Statement*>& statements);

void CodeGenFunction(CodeGenState& state, String identifier, Ast::Function* pFunction) {
    // Push local for this function
    state.localsStack.PushBack(identifier);

    // Put input params in locals stack
    for (Ast::Node* pParam : pFunction->pFuncType->params) {
        if (pParam->nodeKind == Ast::NodeKind::Identifier) {
            Ast::Identifier* pIdentifier = (Ast::Identifier*)pParam;
            state.localsStack.PushBack(pIdentifier->identifier);
        } else if (pParam->nodeKind == Ast::NodeKind::Declaration) {
            Ast::Declaration* pDecl = (Ast::Declaration*)pParam;
            state.localsStack.PushBack(pDecl->identifier);
        }
    }

    // Codegen the statements in the body
    CodeGenStatements(state, pFunction->pBody->declarations);

    // Put a return instruction at the end of the function
    PushInstruction(state, pFunction->pBody->endToken.line, {
                .opcode     = OpCode::PushConstant,
                .constant   = Value() }, TypeInfo::TypeTag::Void);
    PushInstruction(state, pFunction->pBody->endToken.line, { .opcode = OpCode::Return });
}

// ***********************************************************************

void CodeGenExpression(CodeGenState& state, Ast::Expression* pExpr) {

    // Constant folding
    // If this expression is a constant, there's no need to generate any bytecode, just emit the constant literal
    // Functions need their bodies codegen'd so we let them through
    // Identifiers are already in the constant table, so we let them through and they'll be looked up and reused
    if (pExpr->isConstant && (pExpr->nodeKind != Ast::NodeKind::Function && pExpr->nodeKind != Ast::NodeKind::Identifier)) {
        PushInstruction(state, pExpr->line, {
                    .opcode     = OpCode::PushConstant,
                    .constant   = pExpr->constantValue }, pExpr->pType->tag);
        return;
    }

    switch (pExpr->nodeKind) {
        case Ast::NodeKind::Identifier: {
            Ast::Identifier* pVariable = (Ast::Identifier*)pExpr;
            if (pVariable->isConstant) {
                // Load from the constant table (it's already been put there)
                Entity* pEntity = FindEntity(state.pCurrentScope, pVariable->identifier);
                
                if (pEntity->kind == EntityKind::Function && pEntity->bFunctionHasBeenGenerated == false) {
                    PushInstruction(state, pVariable->line, {
                        .opcode     = OpCode::PushConstant,
                        .constant   = 0 }, pEntity->pType->tag);
                    pEntity->pendingFunctionConstants.PushBack(state.pCurrentlyCompilingProgram->code.count-1);
                } else {
                    PushInstruction(state, pVariable->line, {
                        .opcode     = OpCode::PushConstant,
                        .constant   = pEntity->constantValue }, pEntity->pType->tag);
                }
            } else {
                // do a load from locals memory
                int localIndex = ResolveLocal(state, pVariable->identifier);
                if (localIndex != -1) {
                    PushInstruction(state, pVariable->line, {
                        .opcode     = OpCode::PushLocal,
                        .stackIndex = localIndex - state.pCurrentScope->codeGenStackFrameBase });
                }
            }
            break;
        }
        case Ast::NodeKind::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
            CodeGenExpression(state, pVarAssignment->pAssignment);
            int localIndex = ResolveLocal(state, pVarAssignment->pIdentifier->identifier);
            if (localIndex != -1) {
                PushInstruction(state, pVarAssignment->line, {
                    .opcode     = OpCode::SetLocal,
                    .stackIndex = localIndex - state.pCurrentScope->codeGenStackFrameBase });
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
                PushInstruction(state, pSetField->line, {
                    .opcode          = OpCode::SetFieldPtr,
                    .fieldSize       = (uint32_t)pTargetField->pType->size,
                    .fieldOffset     = (uint32_t)pTargetField->offset });
			} else {
                PushInstruction(state, pSetField->line, {
                    .opcode          = OpCode::SetField,
                    .fieldSize       = (uint32_t)pTargetField->pType->size,
                    .fieldOffset     = (uint32_t)pTargetField->offset });
			}
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
				PushInstruction(state, pGetField->line, {
                    .opcode          = OpCode::PushFieldPtr,
                    .fieldSize       = (uint32_t)pTargetField->pType->size,
                    .fieldOffset     = (uint32_t)pTargetField->offset });
			} else {
				PushInstruction(state, pGetField->line, {
                    .opcode          = OpCode::PushField,
                    .fieldSize       = (uint32_t)pTargetField->pType->size,
                    .fieldOffset     = (uint32_t)pTargetField->offset });
			}
			break;
		}
        case Ast::NodeKind::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            PushInstruction(state, pLiteral->line, {
                    .opcode     = OpCode::PushConstant,
                    .constant   = pLiteral->constantValue }, pLiteral->pType->tag);
            break;
        }
        case Ast::NodeKind::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;

            // Push jump to skip over function code
            int jump = PushInstruction(state, pFunction->pBody->startToken.line, {
                .opcode = OpCode::Jmp,
                .ipOffset = 0 });
                
            // Enter scope
            Scope* pPreviousScope = state.pCurrentScope;
            EnterScope(state, pFunction->pScope);
            
            // Store function start IP in entity
            // Next instruction will be the start of the function
            int initialIPOffset = state.pCurrentlyCompilingProgram->code.count;
            
            // codegen the function
            CodeGenFunction(state, pFunction->pType->name, pFunction);
            
            // Exit scope
            ExitScope(state, pPreviousScope, pFunction->pBody->endToken);
            
            // Patch jump
            PatchJump(state, jump);

            PushInstruction(state, pFunction->line, {
                    .opcode     = OpCode::PushConstant,
                    .constant   = MakeFunctionValue(initialIPOffset) }, TypeInfo::TypeTag::Function);
        
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

                int andJump = PushInstruction(state, pBinary->line, {
                    .opcode = OpCode::JmpIfFalse,
                    .ipOffset = 0 });
                PushInstruction(state, pBinary->line, { .opcode = OpCode::Pop });
                CodeGenExpression(state, pBinary->pRight);
                PatchJump(state, andJump);
                break;
            }

            if (pBinary->op == Operator::Or) {
                CodeGenExpression(state, pBinary->pLeft);
                int andJump = PushInstruction(state, pBinary->line, {
                    .opcode = OpCode::JmpIfTrue,
                    .ipOffset = 0 });
                PushInstruction(state, pBinary->line, { .opcode = OpCode::Pop });
                CodeGenExpression(state, pBinary->pRight);
                PatchJump(state, andJump);
                break;
            }

            CodeGenExpression(state, pBinary->pLeft);
            CodeGenExpression(state, pBinary->pRight);
            switch (pBinary->op) {
                case Operator::Add:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::Add, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::Subtract:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::Subtract, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::Divide:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::Divide, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::Multiply:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::Multiply, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::Greater:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::Greater, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::Less:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::Less, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::GreaterEqual:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::GreaterEqual, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::LessEqual:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::LessEqual, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::Equal:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::Equal, .type = pBinary->pLeft->pType->tag});
                    break;
                case Operator::NotEqual:
                    PushInstruction(state, pBinary->line, { .opcode = OpCode::NotEqual, .type = pBinary->pLeft->pType->tag});
                    break;
                default:
                    break;
            }
			break;
        }
        case Ast::NodeKind::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            CodeGenExpression(state, pUnary->pRight);
            switch (pUnary->op) {
                case Operator::UnaryMinus:
                    PushInstruction(state, pUnary->line, { .opcode = OpCode::Negate, .type = pUnary->pRight->pType->tag});
                    break;
                case Operator::Not:
                    PushInstruction(state, pUnary->line, { .opcode = OpCode::Not, .type = pUnary->pRight->pType->tag});
                    break;
                default:
                    break;
            }
			break;
        }
		case Ast::NodeKind::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;
			CodeGenExpression(state, pCast->pExprToCast);

            int index = PushInstruction(state, pCast->line, { .opcode = OpCode::Cast});
            Instruction* pCastInstruction = &state.pCurrentlyCompilingProgram->code[index];

			if (pCast->pExprToCast->pType == GetI32Type()) {
				pCastInstruction->fromType = TypeInfo::I32;
			} else if (pCast->pExprToCast->pType == GetF32Type()) {
				pCastInstruction->fromType = TypeInfo::F32;
			} else if (pCast->pExprToCast->pType == GetBoolType()) {
				pCastInstruction->fromType = TypeInfo::Bool;
			} else {
				AssertMsg(false, "Don't know how to cast non base types yet");
			}

			if (pCast->pTypeExpr->constantValue.pTypeInfo == GetI32Type()) {
				pCastInstruction->toType = TypeInfo::I32;
			} else if (pCast->pTypeExpr->constantValue.pTypeInfo == GetF32Type()) {
				pCastInstruction->toType = TypeInfo::F32;
			} else if (pCast->pTypeExpr->constantValue.pTypeInfo == GetBoolType()) {
				pCastInstruction->toType = TypeInfo::Bool;
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

            PushInstruction(state, pCall->line, {
                    .opcode     = OpCode::Call,
                    .nArgs   = pCall->args.count });
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
                    
                    // Grab function entity
                    Entity* pEntity = FindEntity(state.pCurrentScope, pDecl->identifier);
                    if (pEntity) {
                        
                        // Push jump to skip over function code
                        int jump = PushInstruction(state, pFunction->pBody->startToken.line, {
                            .opcode = OpCode::Jmp,
                            .ipOffset = 0 });
                            
                        // Enter scope
                        Scope* pPreviousScope = state.pCurrentScope;
                        EnterScope(state, pFunction->pScope);
                        
                        // Store function start IP in entity
                        // Next instruction will be the start of the function
                        int initialIPOffset = state.pCurrentlyCompilingProgram->code.count;
                        
                        // codegen the function
                        CodeGenFunction(state, pEntity->name, pFunction);
                        
                        // Exit scope
                        ExitScope(state, pPreviousScope, pFunction->pBody->endToken);
    
                        // Update function IP constants in entity
                        pEntity->constantValue = MakeFunctionValue(initialIPOffset);
                        
                        // Loop through pending function constants and correct them
                        for (int i=0; i < pEntity->pendingFunctionConstants.count; i++) {
                            uint32_t instructionIndex = pEntity->pendingFunctionConstants[i];
                            Instruction& ins = state.pCurrentlyCompilingProgram->code[instructionIndex];
                            ins.constant = pEntity->constantValue;
                        }
                        pEntity->bFunctionHasBeenGenerated = true;
                        
                        // Patch jump
                        PatchJump(state, jump);
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
                    PushInstruction(state, pDecl->line, {
                        .opcode     = OpCode::PushStruct,
                        .size       = pDecl->pType->size });
				} else { // For all non struct values we just push a new value on the operand stack that is zero
					Value v;
					v.pPtr = 0;
                    PushInstruction(state, pDecl->line, {
                        .opcode     = OpCode::PushConstant,
                        .constant   = v }, pDecl->pType->tag);
				}
            }
            break;
        }
        case Ast::NodeKind::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            CodeGenExpression(state, pPrint->pExpr);
            PushInstruction(state, pPrint->line, {
                        .opcode     = OpCode::Print,
                        .type       = pPrint->pExpr->pType->tag });
			break;
        }
        case Ast::NodeKind::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            if (pReturn->pExpr)
                CodeGenExpression(state, pReturn->pExpr);
            PushInstruction(state, pReturn->line, { .opcode = OpCode::Return });
            break;
        }
        case Ast::NodeKind::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            CodeGenExpression(state, pExprStmt->pExpr);
            PushInstruction(state, pExprStmt->line, { .opcode = OpCode::Pop });
            break;
        }
        case Ast::NodeKind::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            CodeGenExpression(state, pIf->pCondition);

            int ifJump = PushInstruction(state, pIf->line, {
                .opcode = OpCode::JmpIfFalse,
                .ipOffset = 0 });
            PushInstruction(state, pIf->pThenStmt->line, { .opcode = OpCode::Pop });

            CodeGenStatement(state, pIf->pThenStmt);
            if (pIf->pElseStmt) {
                int elseJump = PushInstruction(state, pIf->pElseStmt->line, {
                    .opcode = OpCode::Jmp,
                    .ipOffset = 0 });
                PatchJump(state, ifJump);

                PushInstruction(state, pIf->pElseStmt->line, { .opcode = OpCode::Pop });
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
            // Potentially wrong, but I think this is right
            uint32_t loopStart = state.pCurrentlyCompilingProgram->code.count - 1;
            CodeGenExpression(state, pWhile->pCondition);

            int ifJump = PushInstruction(state, pWhile->line, {
                .opcode = OpCode::JmpIfFalse,
                .ipOffset = 0 });
            PushInstruction(state, pWhile->line, { .opcode = OpCode::Pop });

            CodeGenStatement(state, pWhile->pBody);
            PushInstruction(state, pWhile->pBody->line, {
                .opcode = OpCode::Jmp,
                .ipOffset = loopStart });

            PatchJump(state, ifJump);
            PushInstruction(state, pWhile->pBody->line, { .opcode = OpCode::Pop });
            break;
        }
        case Ast::NodeKind::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            Scope* pPreviousScope = state.pCurrentScope;
            EnterScope(state, pBlock->pScope);
            CodeGenStatements(state, pBlock->declarations);
            ExitScope(state, pPreviousScope, pBlock->endToken);
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

void CodeGenProgram(Compiler& compilerState) {
    if (compilerState.errorState.errors.count == 0) {
        
        // Set up state
        CodeGenState state;
        state.pOutputAllocator = &compilerState.outputMemory;
        state.pCompilerAllocator = &compilerState.compilerMemory;
        state.pErrors = &compilerState.errorState;
        state.pGlobalScope = compilerState.pGlobalScope;
        state.pCurrentScope = state.pGlobalScope;
        state.localsStack.pAlloc = state.pCompilerAllocator;
        state.pCurrentlyCompilingProgram = (Program*)state.pOutputAllocator->Allocate(sizeof(Program));

        // Create a program to output to
        SYS_P_NEW(state.pCurrentlyCompilingProgram) Program();
        state.pCurrentlyCompilingProgram->dbgConstantsTypes.pAlloc = state.pOutputAllocator;
        state.pCurrentlyCompilingProgram->code.pAlloc = state.pOutputAllocator;
        state.pCurrentlyCompilingProgram->dbgLineInfo.pAlloc = state.pOutputAllocator;

        // Set off actual codegen of the main file, will recursively codegen all functions inside it
        state.localsStack.PushBack("<main>");
        CodeGenStatements(state, compilerState.syntaxTree);

        // Put a return instruction at the end of the program
        Token endToken = compilerState.tokens[compilerState.tokens.count - 1];
        PushInstruction(state, endToken.line, {
                    .opcode     = OpCode::PushConstant,
                    .constant   = Value() }, TypeInfo::TypeTag::Void);
        PushInstruction(state, endToken.line, { .opcode = OpCode::Return });

        compilerState.pProgram = state.pCurrentlyCompilingProgram;
    }
}
