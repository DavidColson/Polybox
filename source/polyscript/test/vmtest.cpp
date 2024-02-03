
namespace Test {

typedef uint32_t VMPtr;

namespace OpCode {
enum Enum : uint8_t {
	Const,
	Load,
	Store,
	Drop,
	Copy,
	Add,
	Print
};
}

namespace AddressingMode {
enum Enum : uint8_t {
	None,
	Immediate,
	Absolute,
	Indexed
};
}

// Instructions can be 4 bytes or any multiple of 4 bytes depending on what they carry
// Format is basically an instruction header which includes the opcode, any addressing mode information, and two optional type tags
// Then n number of other arguments, which are 4 bytes each

// 32 bits, maximum!
struct InstructionHeader {
	OpCode::Enum opcode;
	AddressingMode::Enum addrMode;
	TypeInfo::TypeTag type;
	TypeInfo::TypeTag type2; // Used for cast operations when you need a to and a from type
};

struct VirtualMachine {
	uint8_t* pMemory; 
	VMPtr stackBaseAddress; // This is an offset pointer from pMemory, stack works in 4 byte slots
	VMPtr stackAddress; // This is an offset pointer from pMemory, stack works in 4 byte slots
	// Stack is going to be the back 1kb of memory
};

inline void PushInstruction(ResizableArray<uint32_t>& code, InstructionHeader header) {
	code.PushBack(uint32_t(0));
	memcpy(&code.pData[code.count-1], &header, sizeof(InstructionHeader)); 
}

inline void PushParam(ResizableArray<uint32_t>& code, uint32_t param) {
	code.PushBack(param);
}

void Start() {
	// Initialize virtual machine memory
	
	size_t memorySize = 2 * 1024 * 1024; // Two megabytes
	VirtualMachine vm;
	vm.pMemory = (uint8_t*)g_Allocator.Allocate(memorySize);
	defer(g_Allocator.Free(vm.pMemory));
	vm.stackBaseAddress = memorySize - 1024 * 4; // Stack has 1024, 4 byte slots (must be kept to 4 byte alignment)
	vm.stackAddress = vm.stackBaseAddress; 

	// Make some program by shoving manually created instructions into a list
	ResizableArray<uint32_t> code;
	defer(code.Free());
	
	// Next example program must create a struct with some size larger than a stack slot
	// Then set all the members
	// Then read a member and print it 

	// First step will be to push a struct instance into the stack of size 12 bytes, essentially want to round to nearest multiple of 4, then push stack pointer that amount of slots (multiple push 0)
	// Usually the next steps are push the stack reference to the top of the stack, push a constant, then set field on the stack
	// But now?????? wtf do I do????
	// What we really want to do is just Push immediate, then Pop $(StackPtr + StructOffset + MemberOffset)
	// This does somewhat fuck with our codegenning system, where the expression before a set field would be codegenned and leave it's value on the stack, i.e. the struct reference

	// Push a struct pointer onto the stack, then push a member in it
	// --------------------------------------------------------------
	// Push 0x000ffff // Push pointer to struct location in the stack
	// Push 1337 // Push the thing we wanna set in the struct
	// Copy 4 [%-2 + 4] %-1 // Copy the literal value at the top of the stack to the address stored at location 0 on the stack with size 4, two referenced stack items are popped
	// Push 1337 // Leave the stack with the result item there
		
	// How about getting a struct member variable and putting on the stack?
	// --------------------------------------------------------------------
	// Push 0x000ffff // Push pointer to struct location in the stack
	// Push [%0+0x04] // offset addressing again, push onto the stack the value found at the computed address, where %0 is the value currently on top of the stack

	// Example 4 should be a struct within a struct, so we can test out Set/GetFieldPtr
	// I guess I'm gonna work this one out again aren't I
	// What were we even trying to solve? It was basically, what happens if the member value we want to set is itself a struct larger than a slot size
	// SetFieldPtr is kind of a misnomer, it's actually deep memcpying the struct to it's new location	
	// From what we've learned from compiler explorer, the game here is literally to do as many pops as you need for the size of the struct
	// Would be easier to read/faster/simpler to have one more instruction which is memory to memory move, where a size can be specified?

	// So...
	// Push 0x000ffff // Push pointer to the struct we want to set a member of
	// Push 0x000ffaa // Push pointer to the struct we wanna copy
	// Copy 12 [%-2] [%-1] // Copy from pointer at top of stack to pointer 1 below with size 12 (this will pop the last two elements off the stack)
	// Push 0x00ffcc // Push the newly copied value onto the stack to finish







	// WASM has memory addresses for load and store as stack operands too
	// for example

	// Push (immediate 0)
	// Push (immediate 5)
	// Pop 			-- Store the value 5 (top of stack) at offset 0 in memory

	// it appears load and store in WASM have an immediate operand, which is an offset added to the actual address operand (which is already on the stack)

	// WASM load and store are like so:

	// Push constant address onto stack
	// Load (const off)  -- Puts onto stack the value found at "constant address + const off"

	// Push constant address
	// Push value
	// Store (const off) -- Stores at "constant address + const off" the value, popping both off the stack
	
	// Necessitates us having a different instruction for pushing constant values onto the stack, since how do we differentiate between Push Constant Address and Push Constant Offset

	// WASM Has local instructions, do we need those too? It's basically just pushing onto the stack a value at some address relative to the stack
	// We could implement that as a store above where constant address is the stack base + local offset, and the constant offset is 0

	// If we want the WASM route, this is what we'd be left with:
	// Const Value 			-- no stack operands, leaves value on stack
	// Load Offset			-- Previous stack operand used as address, leaves result on stack
	// Store Offset			-- Previous stack operand used as value, next previous used as address, leaves nothing on stack
	// Copy Size			-- Previous stack operand used as destination address, next previous used as src address, leaves nothing on stack
	// Fill Size			-- Previous stack operand used as destination address, leaves nothing on stack

	// Does this copy work for us? If we were moving a struct to the member of another struct it'd go like this:
	// Constant 0x000ffaa // Push pointer to the struct we wanna copy (again known at compile time, stack base + struct location)
	// Constant 0x000ffff // Push pointer to the struct member we wanna set (known at compile time, stack base + struct location + member offset)
	// Copy 12 		  // Copy 12 bytes from -2 to -1
	// Constant 0x00ffff // Push the dest address onto the stack
	// Note you can also calculate the offset at runtime, by doing Constant structOffset, Constant memOffset, Add, Constant destination, then Copy.
	
	// If pushing a value already on the stack directly to a member:
	// Constant 1337	 // Push a value we wanna set as the member
	// Constant 0x000ffff // Push pointer to the struct we wanna set (known at compile time, stack base + struct location)
	// Store 0x04		 // Store value at top of stack at address + offset
	// Constant 0x00ffff // Push the dest address onto the stack
	// Note you can alternatively have store with no offset, and do a constant member offset

	// Benefits of this, no need for 6(?) ish addressing modes for Push and Pull. The _effective_ number of non branching instructions is literally 4. Previously we had just Push/Pop/Copy, but the addressing modes meant we
	// had push immediate, push address, push stack, push stack+offset, same for pull and copy, which is 8+ unique instructions
	
	// Okay I am satisfied that this is MUCH simpler than what I did last night, and should cover all the same functionality. It's flexible in that you can do your mem offset
	// Calculations at runtime, OR at compile time, you can do large copies in one go. Copy and Fill are actually something you can implement just with load and store as does WASM. So we don't technically need them either.


	if (1) {
		// Example Program 5: Can we implement pointers with this instruction set?

	}

	if (0) {
		// Example Program 4: Store two structs on the stack, set one as the member of another, and read some value in it
		
		// Size is 12, so must do 3 loads of zero
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));

		// Set the first member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(1337)); 		// Push the value we want to set in the struct member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000)); 	// Push target struct address
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam(code, uint32_t(0));			// Store the value at effective address struct + offset

		// Set the second member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(321)); 			// Push the value we want to set in the struct member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000)); 	// Push target struct (from traversing the codegen for the target) 
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam(code, uint32_t(4));			// Store the value at the effective address struct + offset

		// Next struct is size is 20, so must do 5 loads of zero
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));

		// Push the two structs onto the stack, and then copy one to the other
		// CODEGEN CHANGE: If the target field is a struct, then you must do this copy instead of a store
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000)); 	// Push the source struct to be copied in
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff00c)); 	// Push the destination (target struct)
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(12)); 			// Push the size of the copy
		PushInstruction(code, {.opcode = OpCode::Copy }); PushParam(code, (uint32_t(4) << 16) | uint32_t(0)); // Copy, params are destOffset, srcOffset

		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff00c));		// From a identifier node, knowing the local is a struct
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(4));			// From a GetField node which knows that it's target field is a struct (the GetFieldPtr codepath) so it can't do a load here, and will do this offset add instead 		
		PushInstruction(code, {.opcode = OpCode::Add });											// Second part of the above GetField node, should leave an address on the stack to the inner field struct
		PushInstruction(code, {.opcode = OpCode::Load }); PushParam(code, uint32_t(4));				// The second, inner GetField node has a targetfield which is a value, so it will do a load as normal

		PushInstruction(code, {.opcode = OpCode::Print });											// Push what's left on the stack (the value 321)
	}


	if (0) {
		// Example Program 3: Store a struct on the stack, set and get members in it

		// Size is 12, so must do 3 loads of zero
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0));

		// Question: When we're doing codegen to get the local of the struct, how do we know to do just the address and not load the actual value?
		// CODEGEN CHANGE: The variable ast node can be used to acquire the entity which will have this information, so probably easiest to store it in locals tracking
		// And then you can just not gen a Load instruction if it's a struct

		// CODEGEN CHANGE: Note for codegen you also need to swap the order in which target and assignment are generated in the assignment generator node
		// Set the first member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(1337)); 		// Push the value we want to set in the struct member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000)); 	// Push target struct member address
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam(code, uint32_t(0));			// Store the value at effective address struct + offset

		// Set the second member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(321)); 			// Push the value we want to set in the struct member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000)); 	// Push target struct (from traversing the codegen for the target) 
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam(code, uint32_t(4));			// Store the value at the effective address struct + offset
		
		// Get the second member and print
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000));		// Push the struct pointer (from codegen it's target field) 
		PushInstruction(code, {.opcode = OpCode::Load }); PushParam(code, uint32_t(4));				// Load the member at given offset

		PushInstruction(code, {.opcode = OpCode::Print });											// Push what's left on the stack (the value 321)
	}

	// A new idea, what if we embrace stack address mode?
	// So we'll have these addressing modes
	// Push 17 		-- immediate decimal
	// Push 0x3c	-- immediate hex
	// Push %0		-- Stack (i.e. push value at location 0 of stack onto top of stack) (this is a bit like register addressing in x86)
	// Push %-1		-- Stack Inverse (i.e. push value at top of stack to top of stack)
	// Push [0xff] 	-- Immediate Address (i.e. push value at this address onto the stack)
	// Push [%1] 	-- Stack address (i.e. push the value found at the address which is at location 1 in the stack)
	// Push [%1+4]	-- Offset Address Similar to absolute, but you can offset with a constant (such as a member variable offset) 

	// TODO FOR DAVE:
	// The copy instruction is good, I think it will greatly reduce the need for needless moves in and out of the stack, 
	// However above I have not accounted for member offsets, so you need to do that, and codify all the various addressing modes we've made above
	// We're just walking perilously close to implementing x86 assembler here

	// There's two parts to this addressing, first the actual value, which might be a stack value to grab, or an immediate, or some addition to do
	// Then we decide if it's a memory address or an immediate value. Will need to be done at runtime, esp for stack values 

	if (0) {
		// Example program 2: This emulates local variable setting and loading

		// Stack starts at 0x001ff000
		// This must be a constant at compile time, users can otherwise set the stack size at compile time.
		
		// var := 5;
		// var = var + 2;
		// print(var);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(5));
		
		// Push address for next load
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000));
		PushInstruction(code, {.opcode = OpCode::Load }); PushParam(code, uint32_t(0));

		PushInstruction(code, {.opcode = OpCode::Const}); PushParam(code, uint32_t(2));

		PushInstruction(code, {.opcode = OpCode::Add });
		
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000));
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam(code, uint32_t(0));
		
		// Not done here, but usually the setting a local will leave the local on the stack, so you'd have to do a const + load here, then the expression statement will put in a "drop"

		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(0x1ff000));
		PushInstruction(code, {.opcode = OpCode::Load }); PushParam(code, uint32_t(0));

		PushInstruction(code, {.opcode = OpCode::Print });
	}

	if (0) {
		// Example program 1; Pushes two constants to the stack adds them and prints them
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(1337));

		PushInstruction(code, {.opcode = OpCode::Const }); PushParam(code, uint32_t(1337));

		PushInstruction(code, {.opcode = OpCode::Add });

		PushInstruction(code, {.opcode = OpCode::Print });
	}
	// Create a little VM loop
	uint32_t* pEndInstruction = code.end();
	uint32_t* pInstruction = code.pData;
	while(pInstruction < pEndInstruction) {
		InstructionHeader* pHeader = (InstructionHeader*)pInstruction;
		switch(pHeader->opcode) {
			case OpCode::Const: {
				// Push immediate value ontop of stack
				uint32_t value = *(++pInstruction);

				// This is the first part of "push" (loads the value into the right slot)
				// Get address of stack slot and cast to appropriate type
				uint32_t* targetStackSlot = (uint32_t*)(vm.pMemory + vm.stackAddress);
				*targetStackSlot = value;

				// Second part, increment stack pointer
				// increment 1 slot
				vm.stackAddress += 4; 
				break;
			}
			case OpCode::Load: {
				// Instruction arg is a memory offset
				uint32_t offset = *(++pInstruction);

				// Pop the source address operand off the stack
				vm.stackAddress -= 4;
				uint32_t* pSourceAddress = (uint32_t*)(vm.pMemory + vm.stackAddress);

				int* pSource = (int*)(vm.pMemory + *pSourceAddress + offset);

				int* pStackTop = (int*)(vm.pMemory + vm.stackAddress);
				*pStackTop = *pSource;
				
				vm.stackAddress += 4;
				break;
			}
			case OpCode::Store: {
				// Instruction arg is a memory offset
				uint32_t offset = *(++pInstruction);

				// Pop the target memory address off the stack
				vm.stackAddress -= 4;
				uint32_t* pDestAddress = (uint32_t*)(vm.pMemory + vm.stackAddress);

				// Pop the value to store off top of stack 
				vm.stackAddress -= 4;
				int* pValue = (int*)(vm.pMemory + vm.stackAddress);

				int* pDest = (int*)(vm.pMemory + *pDestAddress + offset);

				*pDest = *pValue;
				break;
			}
			case OpCode::Copy: {
				uint32_t params = *(++pInstruction);
				uint16_t srcOffset = (uint16_t)params;
				uint16_t desOffset = params >> 16;
				
				// Pop the size off the stack
				vm.stackAddress -= 4;
				uint32_t* pSize = (uint32_t*)(vm.pMemory + vm.stackAddress);

				// Pop the destination address off the stack
				vm.stackAddress -= 4;
				uint32_t* pDestAddress = (uint32_t*)(vm.pMemory + vm.stackAddress);
				int* pDest = (int*)(vm.pMemory + *pDestAddress + desOffset);

				// Pop the source address off the stack
				vm.stackAddress -= 4;
				uint32_t* pSrcAddress = (uint32_t*)(vm.pMemory + vm.stackAddress);
				int* pSrc = (int*)(vm.pMemory + *pSrcAddress + srcOffset);

				memcpy(pDest, pSrc, *pSize);
				break;
			}
			case OpCode::Drop: {
				// Pop item off the top of the stack, discarding it
				vm.stackAddress -= 4;
				break;
			}
			case OpCode::Add: {
				// Take two top items from stack, attempt to add and leave result on stack
				vm.stackAddress -= 4;
				int* targetStackSlot = (int*)(vm.pMemory + vm.stackAddress);
				int v1 = *targetStackSlot;

				vm.stackAddress -= 4;
				int* targetStackSlot2 = (int*)(vm.pMemory + vm.stackAddress);
				int v2 = *targetStackSlot2;

				int result = v1 + v2;

				int* targetStackSlot3 = (int*)(vm.pMemory + vm.stackAddress);
				*targetStackSlot3 = result;
				vm.stackAddress += 4; 
				break;
			}
			case OpCode::Print: {
				// Take top item from stack and print
				vm.stackAddress -= 4;
				int* targetStackSlot = (int*)(vm.pMemory + vm.stackAddress);
				int v = *targetStackSlot;
				Log::Info("%d", v);
				break;
			}
			default:
			break;
		}
		pInstruction++;
	}
	__debugbreak();
}

}
