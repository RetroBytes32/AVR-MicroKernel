//
// Application command function table

#ifndef ____COMMAND_MODULE_TABLE__
#define ____COMMAND_MODULE_TABLE__

#define _COMMAND_TABLE_SIZE__        20  // Total number of elements
#define _COMMAND_TABLE_NAME_SIZE__   10  // Max name length

#ifdef  __CORE_MAIN_


// Install a module into the function table
uint8_t loadModule(void(*function_ptr)(), const char name[], uint8_t name_length);
// Initiate the module function table
void __module_init_(void);

struct CommandFunctionTable {
	
	char functionNameIndex[_COMMAND_TABLE_SIZE__][_COMMAND_TABLE_NAME_SIZE__];
	void (*command_function_table[_COMMAND_TABLE_SIZE__])(void);
	
}static moduleTable;


uint8_t loadModule(void(*function_ptr)(), const char name[], uint8_t name_length) {
	
	if (name_length > _COMMAND_TABLE_NAME_SIZE__) return 0;
	
	uint8_t index;
	for (index=0; index < _COMMAND_TABLE_SIZE__; index++)
	if (moduleTable.functionNameIndex[index][0] == 0x20) break;
	
	for (uint8_t i=0; i < name_length-1; i++)
	moduleTable.functionNameIndex[index][i] = name[i];
	
	moduleTable.command_function_table[index] = function_ptr;
	
	return 1;
}

void __module_init_(void) {
	for (uint8_t i=0; i < _COMMAND_TABLE_SIZE__; i++) {
		for (uint8_t a=0; a < _COMMAND_TABLE_NAME_SIZE__; a++)
			moduleTable.functionNameIndex[i][a] = 0x20;
	}
}

#endif

#endif


