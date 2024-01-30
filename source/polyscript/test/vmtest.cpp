
namespace Test {

typedef uint32_t VMPtr;

namespace OpCode {
enum Enum : uint8_t {
	Load,
	Store,
	Add,
	Print,
	Pop
};
}

namespace AddressingMode {
enum Enum : uint8_t {
	Immediate,
	Absolute,
	StackOffset
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

	// Load 1337 as an immediate value onto the stack
	code.PushBack(uint32_t(0));
	InstructionHeader inst = InstructionHeader{.opcode = OpCode::Load, .addrMode = AddressingMode::Immediate };
	memcpy(&code.pData[code.count-1], &inst, sizeof(InstructionHeader)); 
	code.PushBack(uint32_t(1337));

	// Need some utility for pushing an instruction header, currently have to push a dummy and then memcpy it
	code.PushBack(uint32_t(0));
	InstructionHeader inst2 = InstructionHeader{.opcode = OpCode::Load, .addrMode = AddressingMode::Immediate };
	memcpy(&code.pData[code.count-1], &inst2, sizeof(InstructionHeader)); 
	code.PushBack(uint32_t(1337));

	code.PushBack(uint32_t(0));
	InstructionHeader inst3 = InstructionHeader{.opcode = OpCode::Add };
	memcpy(&code.pData[code.count-1], &inst3, sizeof(InstructionHeader)); 

	code.PushBack(uint32_t(0));
	InstructionHeader inst4 = InstructionHeader{.opcode = OpCode::Print };
	memcpy(&code.pData[code.count-1], &inst4, sizeof(InstructionHeader)); 
	
	// Create a little VM loop
	uint32_t* pEndInstruction = code.end();
	uint32_t* pInstruction = code.pData;
	while(pInstruction < pEndInstruction) {
		InstructionHeader* pHeader = (InstructionHeader*)pInstruction;
		switch(pHeader->opcode) {
			case OpCode::Load: {
				// Take the immediate value from instruction stream, put on stack
				int immediateValue = *(++pInstruction);

				// Get address of stack slot and cast to appropriate type
				int* targetStackSlot = (int*)(vm.pMemory + vm.stackAddress);
				*targetStackSlot = immediateValue;

				// increment 1 slot
				vm.stackAddress += 4; 
				break;
			}
			case OpCode::Store: {
				// Take value from top of stack and put it at some given address, offset, absolute etc
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
			case OpCode::Pop: {
				// remove top stack slot element, moving back stack pointer by 1 slot
				vm.stackAddress -= 4;
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
