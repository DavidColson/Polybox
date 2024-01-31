
namespace Test {

typedef uint32_t VMPtr;

namespace OpCode {
enum Enum : uint8_t {
	Push,
	Pop, // Pop can have no addressing mode, in which case the value is discarded!
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

	if (1) {
		// Example program 2: This emulates local variable setting and loading

		// Stack starts at 0x001ff000
		// This must be a constant at compile time, users can otherwise set the stack size at compile time.
		
		// var := 5;
		// var = var + 2;
		// print(var);
		PushInstruction(code, {.opcode = OpCode::Push, .addrMode = AddressingMode::Immediate });
		PushParam(code, uint32_t(5));

		PushInstruction(code, {.opcode = OpCode::Push, .addrMode = AddressingMode::Absolute });
		PushParam(code, uint32_t(0x001ff000 + 0)); // 0 being the local index here 

		PushInstruction(code, {.opcode = OpCode::Push, .addrMode = AddressingMode::Immediate });
		PushParam(code, uint32_t(2));

		PushInstruction(code, {.opcode = OpCode::Add });

		PushInstruction(code, {.opcode = OpCode::Pop, .addrMode = AddressingMode::Absolute });
		PushParam(code, uint32_t(0x001ff000 + 0));

		PushInstruction(code, {.opcode = OpCode::Push, .addrMode = AddressingMode::Absolute });
		PushParam(code, uint32_t(0x001ff000 + 0)); // 0 being the local index here 

		PushInstruction(code, {.opcode = OpCode::Print });
	}

	if (0) {
		// Example program 1; Pushes two constants to the stack adds them and prints them

		// Push 1337 as an immediate value onto the stack
		PushInstruction(code, {.opcode = OpCode::Push, .addrMode = AddressingMode::Immediate });
		PushParam(code, uint32_t(1337));

		PushInstruction(code, {.opcode = OpCode::Push, .addrMode = AddressingMode::Immediate });
		PushParam(code, uint32_t(1337));

		PushInstruction(code, {.opcode = OpCode::Add });

		PushInstruction(code, {.opcode = OpCode::Print });
	}
	// Create a little VM loop
	uint32_t* pEndInstruction = code.end();
	uint32_t* pInstruction = code.pData;
	while(pInstruction < pEndInstruction) {
		InstructionHeader* pHeader = (InstructionHeader*)pInstruction;
		switch(pHeader->opcode) {
			case OpCode::Push: {
				if (pHeader->addrMode == AddressingMode::Immediate) {
					// Take the immediate value from instruction stream, put on stack
					int immediateValue = *(++pInstruction);

					// This is the first part of "push" (loads the value into the right slot)
					// Get address of stack slot and cast to appropriate type
					int* targetStackSlot = (int*)(vm.pMemory + vm.stackAddress);
					*targetStackSlot = immediateValue;

					// Second part, increment stack pointer
					// increment 1 slot
					vm.stackAddress += 4; 
				} else if (pHeader->addrMode == AddressingMode::Absolute) {
					VMPtr targetAddress = *(++pInstruction);
					int* realTargetAddress = (int*)(vm.pMemory + targetAddress);

					int* targetStackSlot = (int*)(vm.pMemory + vm.stackAddress);
					*targetStackSlot = *realTargetAddress;
					
					vm.stackAddress += 4;
				}
				break;
			}
			case OpCode::Pop: {
				if (pHeader->addrMode == AddressingMode::None) {
					// Take value from top of stack and put it at some given address, offset, absolute etc, or not at all, discarding it
					vm.stackAddress -= 4;
				} else if (pHeader->addrMode == AddressingMode::Absolute) {
					vm.stackAddress -= 4;

					int* targetStackSlot = (int*)(vm.pMemory + vm.stackAddress);

					VMPtr targetAddress = *(++pInstruction);
					int* realTargetAddress = (int*)(vm.pMemory + targetAddress);

					// Put value that was in stack slot, at realTargetAddress (where asked)
					*realTargetAddress = *targetStackSlot;
				}
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
