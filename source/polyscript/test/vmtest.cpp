
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

// Instructions can be 4 bytes or any multiple of 4 bytes depending on what they carry
// Format is basically an instruction header which includes the opcode, any addressing mode information, and two optional type tags
// Then n number of other arguments, which are 4 bytes each

// TODO FOR DAVE: You could actually reduce this to 16 bits you know, header is opcode + type, then params for many things would 32 bit or 16 bit, very little wasted space
// 32 bits, maximum!
struct InstructionHeader {
	OpCode::Enum opcode;
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
	
	if (1) {
		// Example Program 5: Can we implement pointers with this instruction set?
		
	}

	if (0) {
		// Example Program 4: Store two structs on the stack, set one as the member of another, and read some value in it
		
		// TestStruct :: struct {
		// 	intMember: i32;
		// 	intMember2: i32;
		// 	intMember3: i32;
		// };
		//
		// LargeStruct :: struct {
		// 	intMember: i32;
		// 	inner: TestStruct;
		// 	intMember2: i32;
		// };
		//
		// instance : TestStruct;
		// instance.intMember = 1337;
		// instance.intMember2 = 321;
		//
		// largeInstance : LargeStruct;
		// largeInstance.inner = instance;
		//
		// print(largeInstance.inner.intMember2); // 321

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
