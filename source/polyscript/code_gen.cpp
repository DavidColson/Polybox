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
    Function* pCurrentlyCompilingFunction;
};
}

// ***********************************************************************

Function* CurrentFunction(CodeGenState& state) {
    return state.pCurrentlyCompilingFunction;
}

// ***********************************************************************

int PushInstruction(CodeGenState& state, uint32_t line, Instruction instruction) {
    CurrentFunction(state)->code2.PushBack(instruction);
    CurrentFunction(state)->dbgLineInfo.PushBack(line);
    return CurrentFunction(state)->code2.count - 1;
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

// TODO: Yeet
uint8_t CreateConstant(CodeGenState& state, Value constant, TypeInfo* pType) {
    CurrentProgram(state)->constantTable.PushBack(constant);
    CurrentProgram(state)->dbgConstantsTypes.PushBack(pType);
    // TODO: When you refactor instructions, allow space for bigger constant indices
    return (uint8_t)CurrentProgram(state)->constantTable.count - 1;
}

// ***********************************************************************

void PatchJump(CodeGenState& state, int jumpInstructionIndex) {
    // Not sure if this is right, should we skip over the instruction at the end of the list? 
    // I think it will increment on next loop, so this should be fine. The next instruction executed will be the one after this index
    CurrentFunction(state)->code2[jumpInstructionIndex].ipOffset = CurrentFunction(state)->code2.count - 1; 
}

// ***********************************************************************

void CodeGenStatements(CodeGenState& state, ResizableArray<Ast::Statement*>& statements);

void CodeGenFunction(CodeGenState& state, String identifier, Ast::Function* pFunction) {
    state.pCurrentlyCompilingFunction->name = CopyString(identifier, state.pOutputAllocator);

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
                .constant   = Value() });
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
                    .constant   = pExpr->constantValue });
        return;
    }

    switch (pExpr->nodeKind) {
        case Ast::NodeKind::Identifier: {
            Ast::Identifier* pVariable = (Ast::Identifier*)pExpr;
            if (pVariable->isConstant) {
                // Load from the constant table (it's already been put there)
                Entity* pEntity = FindEntity(state.pCurrentScope, pVariable->identifier);

                PushInstruction(state, pVariable->line, {
                    .opcode     = OpCode::PushConstant,
                    .constant   = CurrentProgram(state)->constantTable[pEntity->codeGenConstIndex] });
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
                    .constant   = pLiteral->constantValue });
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
            ExitScope(state, pPreviousScope, pFunction->pBody->endToken);
            state.pCurrentlyCompilingFunction = pParentFunction;

            PushInstruction(state, pFunction->line, {
                    .opcode     = OpCode::PushConstant,
                    .constant   = MakeValue(pNewFunction) });
        
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
            Instruction* pCastInstruction = &CurrentFunction(state)->code2[index];

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
                    Entity* pEntity = FindEntity(state.pCurrentScope, pDecl->identifier);

                    if (pEntity) {
                        Scope* pPreviousScope = state.pCurrentScope;
                        Function* pParentFunction = CurrentFunction(state);
                        state.pCurrentlyCompilingFunction = pEntity->constantValue.pFunction;
                        EnterScope(state, pFunction->pScope);
                        CodeGenFunction(state, pEntity->name, pFunction);
                        ExitScope(state, pPreviousScope, pFunction->pBody->endToken);
                        state.pCurrentlyCompilingFunction = pParentFunction;
                    }
                }
                break;
            }

            state.localsStack.PushBack(pDecl->identifier); // Is this incorrect? Has the function node already done this?

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
                        .constant   = v });
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
            uint32_t loopStart = CurrentFunction(state)->code.count - 1;
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

void GenerateConstantTable(CodeGenState& state, Scope* pScope) {
    for (size_t i = 0; i < pScope->entities.tableSize; i++) { 
            HashNode<String, Entity*>& node = pScope->entities.pTable[i];
            if (node.hash != UNUSED_HASH) {
                Entity* pEntity = node.value;
                if (pEntity->kind == EntityKind::Constant) {
                    if (pEntity->pDeclaration->pInitializerExpr && pEntity->pDeclaration->pInitializerExpr->nodeKind == Ast::NodeKind::Function) {
                        Function* pFunctionLiteral = (Function*)state.pOutputAllocator->Allocate(sizeof(Function));
                        SYS_P_NEW(pFunctionLiteral) Function();
                        pFunctionLiteral->code.pAlloc = state.pOutputAllocator;
                        pFunctionLiteral->dbgLineInfo.pAlloc = state.pOutputAllocator;

                        pEntity->constantValue.pFunction = pFunctionLiteral;
                        pEntity->codeGenConstIndex = CreateConstant(state, MakeValue(pFunctionLiteral), pEntity->pType);
                    } else {
                        uint8_t constIndex = CreateConstant(state, pEntity->constantValue, pEntity->pType);
                        pEntity->codeGenConstIndex = constIndex;
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
        state.localsStack.pAlloc = state.pCompilerAllocator;
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
        state.localsStack.PushBack("<main>");
        CodeGenStatements(state, compilerState.syntaxTree);

        // Put a return instruction at the end of the program
        Token endToken = compilerState.tokens[compilerState.tokens.count - 1];
        uint8_t constIndex = CreateConstant(state, Value(), GetVoidType());
        PushInstruction(state, endToken.line, {
                    .opcode     = OpCode::PushConstant,
                    .constant   = Value() });
        PushInstruction(state, endToken.line, { .opcode = OpCode::Return });

        compilerState.pProgram = state.pCurrentlyCompilingProgram;
    }
}