//
// Device list command module

void command_memory(void);

struct ModuleMemoryCommand {
	
	ModuleMemoryCommand() {
		
		const char command_name[] = "mem";
		kernel.function.install(&command_memory, command_name, sizeof(command_name));
		
		return;
	}
	
};
static ModuleMemoryCommand moduleMemoryCommand;

void command_memory(void) {
	
	// Get amount of available system memory
	uint32_t ammountOfMemory = _STACK_END__ - (_KERNEL_END__ + stack_size());
	
	string memoryAmmount(7);
	intToString(ammountOfMemory, memoryAmmount);
	
	// Strings
	console.print(memoryAmmount, sizeof(memoryAmmount));
	console.print(string_memory_allocator_bytes, sizeof(string_memory_allocator_bytes));
	console.print(string_memory_allocator_free, sizeof(string_memory_allocator_free));
	console.newLine();
	
	return;
}

