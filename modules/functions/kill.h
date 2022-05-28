//
// Kill command

void command_kill(void);

struct ModuleLoaderKill {
	ModuleLoaderKill() {
		load_module("kill", 5, command_kill);
	}
}static loadModuleKill;

void command_kill(void) {
	
	DriverEntryPoint consoleDriver;
	if (get_func_address(_COMMAND_CONSOLE__, sizeof(_COMMAND_CONSOLE__), consoleDriver) == 0) return;
	
	char keyboard_string[_PROCESS_NAME_SIZE__];
	char task_name[_PROCESS_NAME_SIZE__];
	for (uint8_t a=0; a < _PROCESS_NAME_SIZE__; a++) {keyboard_string[a] = 0x20; task_name[a] = 0x20;}
	
	// Get keyboard string
	for (uint8_t a=0; a < _PROCESS_NAME_SIZE__; a++) keyboard_string[a] = console.keyboard_string[a + 5];
	
	// Check running tasks/services
	for (uint8_t i=0; i<_PROCESS_LIST_SIZE__; i++) {
		
		if (proc_info.processName[i][0] == 0x20) continue;
		
		// Get task name
		for (uint8_t a=0; a < console.last_string_length-1; a++) task_name[a] = proc_info.processName[i][a];
		
		for(uint8_t i=0; i < console.last_string_length-1; i++)
			call_extern(consoleDriver, 0x00, (uint8_t&)task_name[i]);
		
		
		
		/*
		// Check function parameter
		if (string_compare(keyboard_string, task_name, console.last_string_length - 5) == 1) {
			
			for (uint8_t c=0; c < _PROCESS_NAME_SIZE__; c++) proc_info.processName[i][c] = 0x20;
			
			proc_info.processType[i] = 0x00;
			proc_info.processPriority[i] = 0x00;
			proc_info.processCounters[i] = 0x00;
			proc_info.process_pointer_table[i] = 0;
			
			//for(uint8_t i=0; i < sizeof(msg_task_stopped)-1; i++) 
			//	callExtern(consoleDriver, 0x00, (uint8_t&)msg_task_stopped[i]);
			
			callExtern(consoleDriver, 0x01); // new line
			return;
			
		}
		*/
	}
	
	//for(uint8_t i=0; i < sizeof(msg_task_not_found)-1; i++)
		//callExtern(consoleDriver, 0x00, (uint8_t&)msg_task_not_found[i]);
	
	call_extern(consoleDriver, 0x01); // new line
	return;
}

