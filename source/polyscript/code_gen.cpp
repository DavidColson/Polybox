// Copyright 2020-2022 David Colson. All rights reserved.

#include "code_gen.h"

#include "parser.h"
#include "virtual_machine.h"
#include "type_checker.h"
#include "compiler.h"

#include <maths.h>
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
    ResizableArray<String> localStorage;
    
    ErrorState* pErrors;
    Program* pCurrentlyCompilingProgram;

	i32 targetCPUMemory;
	i32 stackSize;
	i32 stackBaseAddress;
};
}

inline i32 PushInstruction(CodeGenState& state, u32 line, InstructionHeader header) {
    state.pCurrentlyCompilingProgram->code.PushBack(u16(0));
	size end = state.pCurrentlyCompilingProgram->code.count-1;
	memcpy(&state.pCurrentlyCompilingProgram->code.pData[end], &header, sizeof(InstructionHeader)); 

    state.pCurrentlyCompilingProgram->dbgLineInfo.PushBack(line);

    return (i32)state.pCurrentlyCompilingProgram->code.count - 1;
}

inline void PushOperand16bit(CodeGenState& state, u16 param) {
	state.pCurrentlyCompilingProgram->code.PushBack(param);
	size prevLine = *state.pCurrentlyCompilingProgram->dbgLineInfo.end();
    state.pCurrentlyCompilingProgram->dbgLineInfo.PushBack(prevLine);
}

inline void PushOperand32bit(CodeGenState& state, u32 param) {
	state.pCurrentlyCompilingProgram->code.PushBack(u16(param));
	state.pCurrentlyCompilingProgram->code.PushBack(u16(param >> 16));
	size prevLine = *state.pCurrentlyCompilingProgram->dbgLineInfo.end();
    state.pCurrentlyCompilingProgram->dbgLineInfo.PushBack(prevLine);
    state.pCurrentlyCompilingProgram->dbgLineInfo.PushBack(prevLine);
}

void PushFunctionDbgInfo(CodeGenState& state, size functionEntryIndex, String name, TypeInfo* pType) {
	state.pCurrentlyCompilingProgram->dbgFunctionInfo.Add(functionEntryIndex, {name, pType});
}

// ***********************************************************************

void EnterScope(CodeGenState& state, Scope* pNewScope) {
    state.pCurrentScope = pNewScope;
    if (pNewScope->kind == ScopeKind::Function) {
        state.pCurrentScope->localStorageFunctionBase = (i32)state.localStorage.count;
    } else {
        // If we're not a function scope, then presumably some scope above us is a function scope
        Scope* pParentFunctionScope = pNewScope;
        while (pParentFunctionScope != nullptr && pParentFunctionScope->kind != ScopeKind::Global && pParentFunctionScope->kind != ScopeKind::Function) {
            pParentFunctionScope = pParentFunctionScope->pParent;
        }
        state.pCurrentScope->localStorageFunctionBase = pParentFunctionScope->localStorageFunctionBase;
    }
}

// ***********************************************************************

void ExitScope(CodeGenState& state, Scope* pOldScope, Token popToken) {
	// All local storage is reserved on function entry, so we only rollback the local storage when exiting a function
	if (state.pCurrentScope->kind == ScopeKind::Function) {
		while (state.localStorage.count > state.pCurrentScope->localStorageFunctionBase) {
			state.localStorage.PopBack();
		}
	}
    state.pCurrentScope = pOldScope;
}

// ***********************************************************************

Program* CurrentProgram(CodeGenState& state) {
    return state.pCurrentlyCompilingProgram;
}

// ***********************************************************************

void AddLocal(CodeGenState& state, String name, size sizeInBytes) {
	size slotsUsed = (size)ceil((f32)sizeInBytes / 4.0);
	for (size i = 0; i < slotsUsed; i++) {
    	state.localStorage.PushBack(name);
	}
}

// ***********************************************************************

i32 ResolveLocal(CodeGenState& state, String name) {
	// This does a reverse search so as not to incorrectly find locals in higher scopes, it should find in the lowest scope possible
	bool found = false;
	for (size i = state.pCurrentScope->localStorageFunctionBase; i < state.localStorage.count; i++) {
		String& local = state.localStorage[i];
		if (name == local) {
			return ((i32)i - state.pCurrentScope->localStorageFunctionBase) * 4;
		}
	}
    return -1;
}

// ***********************************************************************

void PatchJump(CodeGenState& state, i32 jumpInstructionIndex) {
    state.pCurrentlyCompilingProgram->code[jumpInstructionIndex + 1] = i16(state.pCurrentlyCompilingProgram->code.count - 1); 
}

// ***********************************************************************

void ReserveLocalsForScope(CodeGenState& state, Scope* pScope) {
	for (size i = 0; i < pScope->entities.tableSize; i++) { 
		HashNode<String, Entity*>& node = pScope->entities.pTable[i];
		if (node.hash != UNUSED_HASH) {
			Entity* pEntity = node.value;
			if (pEntity->kind == EntityKind::Variable)
				AddLocal(state, pEntity->name, pEntity->pType->size);
		}
	}
}

// ***********************************************************************

void ReserveLocalsForScopeRecursive(CodeGenState& state, Scope* pScope) {
	ReserveLocalsForScope(state, pScope);

	// Reserve locals for lower level imperative scopes (just blocks for now)
	for (size i = 0; i < pScope->children.count; i++) {
		Scope* pChildScope = pScope->children[i];
		if (pChildScope->kind == ScopeKind::Block) 
			ReserveLocalsForScope(state, pChildScope);
	}
}

// ***********************************************************************

void CodeGenStatements(CodeGenState& state, ResizableArray<Ast::Statement*>& statements);

void CodeGenFunction(CodeGenState& state, String identifier, Ast::Function* pFunction) {
    // Push storage for this function
	AddLocal(state, identifier, 4);

	// Iterate function type scope (storage space for params)
	ReserveLocalsForScope(state, pFunction->pScope);

	// The function, and the params were already pushed to the stack before the function is called
	// So we only need to reserve space the locals and temporaries inside the function scope
	i32 storageSizeStart = (i32)state.localStorage.count;

	// Iterate function body scope (storage space for locals in the function)
	ReserveLocalsForScopeRecursive(state, pFunction->pBody->pScope);
	
	// Need to emit some opcode to move the stack pointer above all the local space we've reserved.
	// So that no further function can use the same space.
	i32 storageSlotsUsed = (i32)state.localStorage.count - storageSizeStart;
	if (storageSlotsUsed > 0) {
		PushInstruction(state, pFunction->pBody->startToken.line, { .opcode = OpCode::Const, .type = TypeInfo::TypeTag::I32 });
		PushOperand32bit(state, storageSlotsUsed);
		PushInstruction(state, pFunction->pBody->startToken.line, { .opcode = OpCode::StackChange });
	}

	// Codegen the statements in the body
	Scope* pPreviousScope = state.pCurrentScope;
	EnterScope(state, pFunction->pBody->pScope);
	CodeGenStatements(state, pFunction->pBody->declarations);
	ExitScope(state, pPreviousScope, pFunction->pBody->endToken);

    // Put a return instruction at the end of the function
    PushInstruction(state, pFunction->pBody->endToken.line, { .opcode = OpCode::Const, .type = TypeInfo::TypeTag::Void });
	PushOperand32bit(state, 0);
    PushInstruction(state, pFunction->pBody->endToken.line, { .opcode = OpCode::Return });
}

// TODO: When casting down values to 16 bit for parameters, you should check for overflow in the compiler and give an error

// ***********************************************************************

void CodeGenExpression(CodeGenState& state, Ast::Expression* pExpr) {

    // Constant folding
    // If this expression is a constant, there's no need to generate any bytecode, just emit the constant literal
    // Functions need their bodies codegen'd so we let them through
    // Identifiers are already in the constant table, so we let them through and they'll be looked up and reused
    if (pExpr->isConstant && (pExpr->nodeKind != Ast::NodeKind::Function && pExpr->nodeKind != Ast::NodeKind::Identifier)) {
        PushInstruction(state, pExpr->line, { .opcode = OpCode::Const, .type = pExpr->pType->tag });
		PushOperand32bit(state, pExpr->constantValue.i32Value);
        return;
    }

    switch (pExpr->nodeKind) {
        case Ast::NodeKind::Identifier: {
            Ast::Identifier* pVariable = (Ast::Identifier*)pExpr;
			// Load from the constant table (it's already been put there)
			Entity* pEntity = FindEntity(state.pCurrentScope, pVariable->identifier);

            if (pVariable->isConstant) {
                if (pEntity->kind == EntityKind::Function && pEntity->bFunctionHasBeenGenerated == false) {
                    PushInstruction(state, pVariable->line, { .opcode = OpCode::Const, .type = pEntity->pType->tag });
					PushOperand32bit(state, 0);
                    pEntity->pendingFunctionConstants.PushBack(state.pCurrentlyCompilingProgram->code.count-2);
                } else {
                    PushInstruction(state, pVariable->line, { .opcode = OpCode::Const, .type = pEntity->pType->tag} );
                    PushOperand32bit(state, pEntity->constantValue.i32Value);
                }
            } else {
				i32 localIndex = ResolveLocal(state, pVariable->identifier);
				if (localIndex != -1) {
					if (pEntity->pType->tag == TypeInfo::TypeTag::Struct) {
						PushInstruction(state, pVariable->line, { .opcode = OpCode::LocalAddr });
						PushOperand16bit(state, localIndex);
					} else {
						PushInstruction(state, pVariable->line, { .opcode = OpCode::LocalAddr });
						PushOperand16bit(state, localIndex);
						PushInstruction(state, pVariable->line, { .opcode = OpCode::Load });
						PushOperand16bit(state, 0);
					}
				}
            }
            break;
        }
        case Ast::NodeKind::VariableAssignment: {
            Ast::VariableAssignment* pVarAssignment = (Ast::VariableAssignment*)pExpr;
			// Codegen the value to be set
            CodeGenExpression(state, pVarAssignment->pAssignment);
            i32 localIndex = ResolveLocal(state, pVarAssignment->pIdentifier->identifier);
            if (localIndex != -1) {
				// Codegen the address of the identifier (as opposed to the value as you would do above)
				PushInstruction(state, pVarAssignment->line, { .opcode = OpCode::LocalAddr });
				PushOperand16bit(state, localIndex);

				// Do a store
				PushInstruction(state, pVarAssignment->line, { .opcode = OpCode::Store });
				PushOperand16bit(state, 0);
            }
            break;
        }
		case Ast::NodeKind::SetField: {
			Ast::SetField* pSetField = (Ast::SetField*)pExpr;
			
			// Codegen value to be set (same as above)
			CodeGenExpression(state, pSetField->pAssignment);

			// Codegen the target which is an identifier node, but it just happens to generate an address cause it's guaranteed
			// to be a struct
			CodeGenExpression(state, pSetField->pTarget);

			TypeInfoStruct* pTargetType = (TypeInfoStruct*)pSetField->pTarget->pType;
			TypeInfoStruct::Member* pTargetField = nullptr;
			for (size i = 0; i < pTargetType->members.count; i++) {
				TypeInfoStruct::Member& mem = pTargetType->members[i];
				if (mem.identifier == pSetField->fieldName) {
					pTargetField = &pTargetType->members[i];
					break;
				}
			}

			if (pTargetField->pType->tag == TypeInfo::TypeTag::Struct) {
				// In this case you will be doing a copy instruction
				// First push the copy size (the struct to copy in and the target struct addresses are already on the stack)
				PushInstruction(state, pSetField->line, { .opcode = OpCode::Const, .type = TypeInfo::TypeTag::I32 });
				PushOperand32bit(state, (u32)pTargetField->pType->size);
				PushInstruction(state, pSetField->line, {.opcode = OpCode::Copy }); 
				PushOperand16bit(state, (u16)pTargetField->offset);
				PushOperand16bit(state, 0); 
			} else {
				// In this case you will do a store
				// From the identifier node above, what will be left is the address of the struct underneath the value to store
				// So you just need to do store, where the offset is the member offset
				PushInstruction(state, pSetField->line, { .opcode = OpCode::Store });
				PushOperand16bit(state, (u16)pTargetField->offset);
			}
			break;
		}
		case Ast::NodeKind::GetField: {
			Ast::GetField* pGetField = (Ast::GetField*)pExpr;
			
			CodeGenExpression(state, pGetField->pTarget);

			TypeInfoStruct* pTargetType = (TypeInfoStruct*)pGetField->pTarget->pType;
			TypeInfoStruct::Member* pTargetField = nullptr;
			for (size i = 0; i < pTargetType->members.count; i++) {
				TypeInfoStruct::Member& mem = pTargetType->members[i];
				if (mem.identifier == pGetField->fieldName) {
					pTargetField = &pTargetType->members[i];
					break;
				}
			}

			if (pTargetField->pType->tag == TypeInfo::TypeTag::Struct) {
				// Similar to the identifier local load. You need to leave on the stack the address of the targetField
				// The address of the baseStruct is already on the stack, so you need to load the offset as a const
				// Then do an add, leaving the new address on the stack
				PushInstruction(state, pGetField->line, { .opcode = OpCode::Const, .type = TypeInfo::TypeTag::I32 });
				PushOperand32bit(state, (u32)pTargetField->offset);
				PushInstruction(state, pGetField->line, {.opcode = OpCode::Add, .type = TypeInfo::TypeTag::I32 }); 
			} else {
				// In this case the targetfield is just any old plain value we'd want copied into the stack
				// SO here you can do an actual load with offset parameter
				PushInstruction(state, pGetField->line, {.opcode = OpCode::Load }); 
				PushOperand16bit(state, (u16)pTargetField->offset);
			}
			break;
		}
        case Ast::NodeKind::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
			PushInstruction(state, pLiteral->line, { .opcode = OpCode::Const, .type = pLiteral->pType->tag });
			PushOperand32bit(state, pLiteral->constantValue.i32Value);
            break;
        }
        case Ast::NodeKind::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;

            // Push jump to skip over function code
			i32 jump = PushInstruction(state, pFunction->pBody->startToken.line, { .opcode = OpCode::Jmp });
			PushOperand16bit(state, 0);
                
            // Enter scope
            Scope* pPreviousScope = state.pCurrentScope;
            EnterScope(state, pFunction->pScope);
            
            // Store function start IP in entity
            // Next instruction will be the start of the function
            i32 initialIPOffset = (i32)state.pCurrentlyCompilingProgram->code.count;
            
            // codegen the function
            CodeGenFunction(state, pFunction->pType->name, pFunction);
            
            // Exit scope
            ExitScope(state, pPreviousScope, pFunction->pBody->endToken);
            
            // Patch jump
            PatchJump(state, jump);

			// leave function pointer on the stack
			PushInstruction(state, pFunction->line, { .opcode = OpCode::Const, .type = TypeInfo::TypeTag::Function });
			PushOperand32bit(state, initialIPOffset);
			PushFunctionDbgInfo(state, initialIPOffset, pFunction->pType->name, pFunction->pType);
        
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

				i32 andJump = PushInstruction(state, pBinary->line, { .opcode = OpCode::JmpIfFalse });
				PushOperand16bit(state, 0);
                PushInstruction(state, pBinary->line, { .opcode = OpCode::Drop });
                CodeGenExpression(state, pBinary->pRight);
                PatchJump(state, andJump);
                break;
            }

            if (pBinary->op == Operator::Or) {
                CodeGenExpression(state, pBinary->pLeft);
				i32 orJump = PushInstruction(state, pBinary->line, { .opcode = OpCode::JmpIfTrue });
				PushOperand16bit(state, 0);
                PushInstruction(state, pBinary->line, { .opcode = OpCode::Drop });
                CodeGenExpression(state, pBinary->pRight);
                PatchJump(state, orJump);
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

			TypeInfo::TypeTag toType;
			if (FindTypeByValue(pCast->pTypeExpr->constantValue) == GetI32Type()) {
				toType = TypeInfo::I32;
			} else if (FindTypeByValue(pCast->pTypeExpr->constantValue) == GetF32Type()) {
				toType = TypeInfo::F32;
			} else if (FindTypeByValue(pCast->pTypeExpr->constantValue) == GetBoolType()) {
				toType = TypeInfo::Bool;
			} else {
				AssertMsg(false, "Don't know how to cast non base types yet");
			}
			PushInstruction(state, pCast->line, { .opcode = OpCode::Cast, .type = toType });

			TypeInfo::TypeTag fromType;
			if (pCast->pExprToCast->pType == GetI32Type()) {
				fromType = TypeInfo::I32;
			} else if (pCast->pExprToCast->pType == GetF32Type()) {
				fromType = TypeInfo::F32;
			} else if (pCast->pExprToCast->pType == GetBoolType()) {
				fromType = TypeInfo::Bool;
			} else {
				AssertMsg(false, "Don't know how to cast non base types yet");
			}
			PushOperand16bit(state, (i16)fromType);
			break;
		}
        case Ast::NodeKind::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;
            CodeGenExpression(state, pCall->pCallee);

            for (Ast::Expression* pExpr : pCall->args) {
                CodeGenExpression(state, pExpr);
            }

			PushInstruction(state, pCall->line, { .opcode = OpCode::Call });
			PushOperand16bit(state, (i16)pCall->args.count);
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
						i32 jump = PushInstruction(state, pFunction->pBody->startToken.line, { .opcode = OpCode::Jmp });
						PushOperand16bit(state, 0);
                            
                        // Enter scope
                        Scope* pPreviousScope = state.pCurrentScope;
                        EnterScope(state, pFunction->pScope);
                        
                        // Store function start IP in entity
                        // Next instruction will be the start of the function
                        i32 initialIPOffset = (i32)state.pCurrentlyCompilingProgram->code.count;
                        
                        // codegen the function
                        CodeGenFunction(state, pEntity->name, pFunction);
                        
                        // Exit scope
                        ExitScope(state, pPreviousScope, pFunction->pBody->endToken);
    
                        // Update function IP constants in entity
                        pEntity->constantValue = MakeFunctionValue(initialIPOffset);
                        
                        // Loop through pending function constants and correct them
                        for (int i=0; i < pEntity->pendingFunctionConstants.count; i++) {
                            size fnPointerIndex = pEntity->pendingFunctionConstants[i];
                            i32* ins = (i32*)&state.pCurrentlyCompilingProgram->code[fnPointerIndex];
							*ins = pEntity->constantValue.functionPointer;
                        }
						PushFunctionDbgInfo(state, initialIPOffset, pEntity->name, pEntity->pType);
                        pEntity->bFunctionHasBeenGenerated = true;
                        
                        // Patch jump
                        PatchJump(state, jump);
                    }
                }
                break;
            }

			i32 localIndex = ResolveLocal(state, pDecl->identifier);
            if (pDecl->pInitializerExpr) {
                CodeGenExpression(state, pDecl->pInitializerExpr);
				PushInstruction(state, pDecl->line, { .opcode = OpCode::LocalAddr });
				PushOperand16bit(state, localIndex);
				PushInstruction(state, pDecl->line, { .opcode = OpCode::Store });
				PushOperand16bit(state, 0);
			}
            else {
				if (pDecl->pType->tag == TypeInfo::Struct) {
					// Structs need to store 0s to their local locations to initialize themselves
					i32 numSlots = (i32)ceil((f32)pDecl->pType->size / 4.f);
					for (i32 i = 0; i < numSlots; i++) {
						PushInstruction(state, pDecl->line, {.opcode = OpCode::Const, .type = pDecl->pType->tag}); PushOperand32bit(state, 0);
						PushInstruction(state, pDecl->line, { .opcode = OpCode::LocalAddr });
						PushOperand16bit(state, localIndex + i);
						PushInstruction(state, pDecl->line, { .opcode = OpCode::Store });
						PushOperand16bit(state, 0);
					}
				} else { // For all non struct values we initialize to 0
					PushInstruction(state, pDecl->line, {.opcode = OpCode::Const, .type = pDecl->pType->tag}); PushOperand32bit(state, 0);
					PushInstruction(state, pDecl->line, { .opcode = OpCode::LocalAddr });
					PushOperand16bit(state, localIndex);
					PushInstruction(state, pDecl->line, { .opcode = OpCode::Store });
					PushOperand16bit(state, 0);
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
            PushInstruction(state, pExprStmt->line, { .opcode = OpCode::Drop });
            break;
        }
        case Ast::NodeKind::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            CodeGenExpression(state, pIf->pCondition);

			i32 ifJump = PushInstruction(state, pIf->line, { .opcode = OpCode::JmpIfFalse });
			PushOperand16bit(state, 0);
            PushInstruction(state, pIf->pThenStmt->line, { .opcode = OpCode::Drop });

            CodeGenStatement(state, pIf->pThenStmt);
            if (pIf->pElseStmt) {
				i32 elseJump = PushInstruction(state, pIf->pElseStmt->line, { .opcode = OpCode::Jmp });
				PushOperand16bit(state, 0);
                PatchJump(state, ifJump);

                PushInstruction(state, pIf->pElseStmt->line, { .opcode = OpCode::Drop });
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
            u16 loopStart = (u16)state.pCurrentlyCompilingProgram->code.count - 1;
            CodeGenExpression(state, pWhile->pCondition);

			i32 ifJump = PushInstruction(state, pWhile->line, { .opcode = OpCode::JmpIfFalse });
			PushOperand16bit(state, 0);
            PushInstruction(state, pWhile->line, { .opcode = OpCode::Drop });

            CodeGenStatement(state, pWhile->pBody);
			PushInstruction(state, pWhile->pBody->line, { .opcode = OpCode::Jmp });
			PushOperand16bit(state, loopStart);

            PatchJump(state, ifJump);
            PushInstruction(state, pWhile->pBody->line, { .opcode = OpCode::Drop });
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
    for (size i = 0; i < statements.count; i++) {
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
        state.localStorage.pAlloc = state.pCompilerAllocator;
        state.pCurrentlyCompilingProgram = (Program*)state.pOutputAllocator->Allocate(sizeof(Program));

		// Setup cpu memory
		state.targetCPUMemory = 2 * 1024 * 1024; // 2 megabytes
		state.stackSize = 1024*4; // 1024 stack slots, 4 bytes each
		state.stackBaseAddress = state.targetCPUMemory - state.stackSize;

        // Create a program to output to
        SYS_P_NEW(state.pCurrentlyCompilingProgram) Program();
        state.pCurrentlyCompilingProgram->dbgConstantsTypes.pAlloc = state.pOutputAllocator;
        state.pCurrentlyCompilingProgram->dbgFunctionInfo.pAlloc = state.pOutputAllocator;
        state.pCurrentlyCompilingProgram->code.pAlloc = state.pOutputAllocator;
        state.pCurrentlyCompilingProgram->dbgLineInfo.pAlloc = state.pOutputAllocator;

		// Reserve storage for all the global variables (this is ostensibly the same as functions reserving local storage, 
		// But handled a bit differently because the main file is not technically a function
		AddLocal(state, "<main>", 4);
		i32 storageSizeStart = (i32)state.localStorage.count;
		ReserveLocalsForScopeRecursive(state, state.pGlobalScope);
		i32 storageSlotsUsed = (i32)state.localStorage.count - storageSizeStart;
		if (storageSlotsUsed > 0) {
			PushInstruction(state, 0, { .opcode = OpCode::Const, .type = TypeInfo::TypeTag::I32 });
			PushOperand32bit(state, storageSlotsUsed);
			PushInstruction(state, 0, { .opcode = OpCode::StackChange });
		}

		// Set off actual codegen of the main file, will recursively codegen all functions inside it
        CodeGenStatements(state, compilerState.syntaxTree);

        // Put a return instruction at the end of the program
        Token endToken = compilerState.tokens[compilerState.tokens.count - 1];
		PushInstruction(state, endToken.line, {.opcode = OpCode::Const, .type = TypeInfo::TypeTag::Void}); PushOperand32bit(state, 0);
        PushInstruction(state, endToken.line, { .opcode = OpCode::Return });

        compilerState.pProgram = state.pCurrentlyCompilingProgram;
    }
}
