
namespace Test {

namespace OpCode {
enum Enum : u8 {
	// OpCode 	 | Followed By 						| Stack (right is top of stack)					
	Const,		// 32bit value 						| [] -> [value] 
	Load,		// 16bit offset						| [address] -> [value]
	Store,		// 16bit offset						| [value][address] -> []
	Drop,		// --								| [value] -> []
	Copy,		// 16bit dest off, 16bit src off 	| [srcAddress][destAddress][size] -> []
	Add,		// --								| [value][value] -> [value]
	Print		// --								| [value] -> []
};
}

// Instructions can be 4 bytes or any multiple of 4 bytes depending on what they carry
// Format is basically an instruction header which includes the opcode, any addressing mode information, and two optional type tags
// Then n number of other arguments, which are 4 bytes each

// TODO FOR DAVE: You could actually reduce this to 16 bits you know, header is opcode + type, then params for many things would 32 bit or 16 bit, very little wasted space
// 16 bits, maximum!
struct InstructionHeader {
	OpCode::Enum opcode;
	TypeInfo::TypeTag type;
};

struct VirtualMachine {
	u8* pMemory; 
	u32 stackBaseAddress; // This is an offset pointer from pMemory, stack works in 4 byte slots
	u32 stackAddress; // This is an offset pointer from pMemory, stack works in 4 byte slots
	// Stack is going to be the back 1kb of memory
};

#define GetOperand16bit(ptr) *(++ptr)
#define GetOperand32bit(ptr) ((ptr[1] << 16) | ptr[2]); ptr += 2;

inline void PushStack(VirtualMachine& vm, u32 value) {
	u32* targetStackSlot = (u32*)(vm.pMemory + vm.stackAddress);
	*targetStackSlot = value;
	vm.stackAddress += 4;
}

inline int PopStack(VirtualMachine& vm) {
	vm.stackAddress -= 4;
	int* targetStackSlot = (int*)(vm.pMemory + vm.stackAddress);
	return *targetStackSlot;
}

inline void PushInstruction(ResizableArray<u16>& code, InstructionHeader header) {
	code.PushBack(u16(0));
	memcpy(&code.pData[code.count-1], &header, sizeof(InstructionHeader)); 
}

inline void PushParam16bit(ResizableArray<u16>& code, u16 param) {
	code.PushBack(param);
}

inline void PushParam32bit(ResizableArray<u16>& code, u32 param) {
	code.PushBack(u16(param >> 16));
	code.PushBack(u16(param));
}

void Start() {
	// Initialize virtual machine memory
	
	size memorySize = 2 * 1024 * 1024; // Two megabytes
	VirtualMachine vm;
	vm.pMemory = (u8*)g_Allocator.Allocate(memorySize);
	defer(g_Allocator.Free(vm.pMemory));
	vm.stackBaseAddress = (u32)memorySize - 1024 * 4; // Stack has 1024, 4 byte slots (must be kept to 4 byte alignment)
	vm.stackAddress = vm.stackBaseAddress; 

	// Make some program by shoving manually created instructions into a list
	ResizableArray<u16> code;
	defer(code.Free());
	
	if (1) {
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
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);

		// Set the first member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 1337); 		// Push the value we want to set in the struct member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000); 	// Push target struct address
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam16bit(code, 0);			// Store the value at effective address struct + offset

		// Set the second member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 321); 			// Push the value we want to set in the struct member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000); 	// Push target struct (from traversing the codegen for the target) 
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam16bit(code, 4);			// Store the value at the effective address struct + offset

		// Next struct is size is 20, so must do 5 loads of zero
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);

		// Push the two structs onto the stack, and then copy one to the other
		// CODEGEN CHANGE: If the target field is a struct, then you must do this copy instead of a store
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000); 	// Push the source struct to be copied in
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff00c); 	// Push the destination (target struct)
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 12); 			// Push the size of the copy
		PushInstruction(code, {.opcode = OpCode::Copy }); PushParam16bit(code, 4); PushParam16bit(code, 0); // Copy, params are destOffset, srcOffset

		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff00c);		// From a identifier node, knowing the local is a struct
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 4);			// From a GetField node which knows that it's target field is a struct (the GetFieldPtr codepath) so it can't do a load here, and will do this offset add instead 		
		PushInstruction(code, {.opcode = OpCode::Add });											// Second part of the above GetField node, should leave an address on the stack to the inner field struct
		PushInstruction(code, {.opcode = OpCode::Load }); PushParam16bit(code, 4);				// The second, inner GetField node has a targetfield which is a value, so it will do a load as normal

		PushInstruction(code, {.opcode = OpCode::Print });											// Push what's left on the stack (the value 321)
	}


	if (0) {
		// Example Program 3: Store a struct on the stack, set and get members in it

		// Size is 12, so must do 3 loads of zero
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0);

		// Question: When we're doing codegen to get the local of the struct, how do we know to do just the address and not load the actual value?
		// CODEGEN CHANGE: The variable ast node can be used to acquire the entity which will have this information, so probably easiest to store it in locals tracking
		// And then you can just not gen a Load instruction if it's a struct

		// CODEGEN CHANGE: Note for codegen you also need to swap the order in which target and assignment are generated in the assignment generator node
		// Set the first member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 1337); 		// Push the value we want to set in the struct member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000); 	// Push target struct member address
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam16bit(code, 0);			// Store the value at effective address struct + offset

		// Set the second member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 321); 			// Push the value we want to set in the struct member
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000); 	// Push target struct (from traversing the codegen for the target) 
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam16bit(code, 4);			// Store the value at the effective address struct + offset
		
		// Get the second member and print
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000);		// Push the struct pointer (from codegen it's target field) 
		PushInstruction(code, {.opcode = OpCode::Load }); PushParam16bit(code, 4);				// Load the member at given offset

		PushInstruction(code, {.opcode = OpCode::Print });											// Push what's left on the stack (the value 321)
	}

	if (0) {
		// Example program 2: This emulates local variable setting and loading

		// Stack starts at 0x001ff000
		// This must be a constant at compile time, users can otherwise set the stack size at compile time.
		
		// var := 5;
		// var = var + 2;
		// print(var);
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 5);
		
		// Push address for next load
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000);
		PushInstruction(code, {.opcode = OpCode::Load }); PushParam16bit(code, 0);

		PushInstruction(code, {.opcode = OpCode::Const}); PushParam32bit(code, 2);

		PushInstruction(code, {.opcode = OpCode::Add });
		
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000);
		PushInstruction(code, {.opcode = OpCode::Store }); PushParam16bit(code, 0);
		
		// Not done here, but usually the setting a local will leave the local on the stack, so you'd have to do a const + load here, then the expression statement will put in a "drop"

		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 0x1ff000);
		PushInstruction(code, {.opcode = OpCode::Load }); PushParam16bit(code, 0);

		PushInstruction(code, {.opcode = OpCode::Print });
	}

	if (0) {
		// Example program 1; Pushes two constants to the stack adds them and prints them
		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 1337);

		PushInstruction(code, {.opcode = OpCode::Const }); PushParam32bit(code, 1337);

		PushInstruction(code, {.opcode = OpCode::Add });

		PushInstruction(code, {.opcode = OpCode::Print });
	}

	// Note for Dave: It may be sensible in the VM to completely disregard the type of stack slots
	// Unless you absolutely have to. All these functions can move around ints and nothing else, with no concern for type
	// Only the math operations and prints will care really, nothing else

	// Create a little VM loop
	u16* pEndInstruction = code.end();
	u16* pInstruction = code.pData;
	while(pInstruction < pEndInstruction) {
		InstructionHeader* pHeader = (InstructionHeader*)pInstruction;
		switch(pHeader->opcode) {
			case OpCode::Const: {
				// Push immediate value ontop of stack
				u32 value = GetOperand32bit(pInstruction);
				PushStack(vm, value);
				break;
			}
			case OpCode::Load: {
				u16 offset = GetOperand16bit(pInstruction);
				i32 sourceAddress = PopStack(vm);
				int* pSource = (int*)(vm.pMemory + sourceAddress + offset);
				PushStack(vm, *pSource);
				break;
			}
			case OpCode::Store: {
				// Instruction arg is a memory offset
				u16 offset = GetOperand16bit(pInstruction);

				// Pop the target memory address off the stack
				i32 destAddress = PopStack(vm);
				int* pDest = (int*)(vm.pMemory + destAddress + offset);

				// Pop the value to store off top of stack 
				i32 value = PopStack(vm);
				*pDest = value;
				break;
			}
			case OpCode::Copy: {
				u16 desOffset = GetOperand16bit(pInstruction);
				u16 srcOffset = GetOperand16bit(pInstruction);
				
				// Pop the size off the stack
				i32 size = PopStack(vm);

				// Pop the destination address off the stack
				i32 destAddress = PopStack(vm);
				int* pDest = (int*)(vm.pMemory + destAddress + desOffset);

				// Pop the source address off the stack
				i32 srcAddress = PopStack(vm);
				int* pSrc = (int*)(vm.pMemory + srcAddress + srcOffset);

				memcpy(pDest, pSrc, size);
				break;
			}
			case OpCode::Drop: {
				// Pop item off the top of the stack, discarding it
				vm.stackAddress -= 4;
				break;
			}
			case OpCode::Add: {
				// This function actually cares about types, so you will have to do a reinterpret cast
				// To make sure the values are of the correct type before doing an add

				// Take two top items from stack, attempt to add and leave result on stack
				i32 v1 = PopStack(vm);
				i32 v2 = PopStack(vm);
				PushStack(vm, v1 + v2);
				break;
			}
			case OpCode::Print: {
				// Take top item from stack and print
				i32 v = PopStack(vm);
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
