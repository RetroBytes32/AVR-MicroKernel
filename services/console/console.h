//
// Command console system service

void keyboard_event_handler(void);               // Event handler entry point
void eventKeyboardEnter(void);                   // Enter function
void eventKeyboardBackspace(void);               // Backspace function
void eventKeyboardAcceptChar(uint8_t new_char);  // Accept a character onto the keyboard string of typed characters
void eventKeyboardShiftPressed(void);            // Shift pressed state
void eventKeyboardShiftReleased(void);           // Shift released state

struct CommandConsoleServiceLauncher {
	
	Device keyboard_device;
	
	uint8_t scanCodeLow;
	uint8_t scanCodeHigh;
	
	CommandConsoleServiceLauncher() {
		
		// Link to the keyboard device driver
		keyboard_device = (Device)get_func_address(_KEYBOARD_INPUT__, sizeof(_KEYBOARD_INPUT__));
		
		// Prevent a key from being accepted on system startup
		call_extern(keyboard_device, 0x02, scanCodeLow, scanCodeHigh);
		
		call_extern(keyboard_device, 0x00, console.lastChar);
		
	}
}static commandConsole;


void keyboard_event_handler(void) {
	
	uint8_t currentChar         = 0x00;
	uint8_t scanCodeAccepted    = 0x00;
	uint8_t readKeyCode         = 0x00;
	
	uint8_t scanCodeLow         = 0x00;
	uint8_t scanCodeHigh        = 0x00;
	
	// Check for a scan code change
	call_extern(commandConsole.keyboard_device, 0x02, scanCodeLow, scanCodeHigh);
	if ((commandConsole.scanCodeLow == scanCodeLow) & (commandConsole.scanCodeHigh == scanCodeHigh))
		return;
	
	commandConsole.scanCodeLow  = scanCodeLow;
	commandConsole.scanCodeHigh = scanCodeHigh;
	
	// Read and convert scan code to an ASCII character
	call_extern(commandConsole.keyboard_device, 0x00, readKeyCode);
	
	// Check MAX string length
	if (console.keyboard_string_length < _MAX_KEYBOARD_STRING_LENGTH__) {
		currentChar = readKeyCode;
		if (readKeyCode > 0x1f) {scanCodeAccepted=1;}
	} else {
		// Allow special keys past max string length
		if (readKeyCode < 0x20) currentChar = readKeyCode;
	}
	
	if (console.lastChar == currentChar) return;
	console.lastChar = currentChar;
	
	// Check special keys
	switch (currentChar) {
		
		case 0x01: {eventKeyboardBackspace(); break;}
		case 0x02: {eventKeyboardEnter(); break;}
		case 0x11: {eventKeyboardShiftPressed(); break;}
		case 0x12: {eventKeyboardShiftReleased(); break;}
		
		default: break;
	}
	
	// Check key accepted
	if (scanCodeAccepted == 1) {
		
		// Check shift state
		if (console.shiftState == 1) {
			if ((currentChar >= 'a') & (currentChar <= 'z'))
			currentChar -= 0x20;
		}
		
		eventKeyboardAcceptChar(currentChar);
		
	}
	
	return;
}

void eventKeyboardEnter(void) {
	
	console.last_string_length = console.keyboard_string_length+1;
	console.keyboard_string_length = 0;
	
	if (console.cursorLine == 3) {console.shiftUp();}
	if (console.cursorLine == 2) console.cursorLine++;
	if (console.cursorLine == 1) console.cursorLine++;
	if (console.cursorLine == 0) {if (console.promptState == 0) {console.promptState++;} else {console.cursorLine++;}}
	
	// Check string length
	if (console.last_string_length > 0) {console.cursorPos=0;
		
		// Function look up
		for (uint8_t i=0; i<DEVICE_TABLE_SIZE; i++) {
			
			if (device_table.type[i] != DEVICE_TYPE_MODULE) continue;
			
			char functionName[DEVICE_NAME_LENGTH_MAX];
			for (uint8_t a=0; a<DEVICE_NAME_LENGTH_MAX; a++) functionName[a] = 0x20;
			
			// Extract name from function index
			uint8_t name_length;
			for (name_length=0; name_length < DEVICE_NAME_LENGTH_MAX; name_length++) {
				
				functionName[name_length] = device_table.name[i][name_length];
				if (device_table.name[i][name_length] == 0x20) break;
				
			}
			
			// Check for the function name in keyboard string
			if (strcmp(functionName, console.keyboard_string, name_length+1) == 0) continue;
			if ((console.keyboard_string[ name_length ]) != 0x20) continue;
			
			// Execute the command
			Device console_function;
			console_function = (void(*)(uint8_t, uint8_t&, uint8_t&, uint8_t&, uint8_t&))device_table.table[i];
			console_function(0x00, nullchar, nullchar, nullchar, nullchar);
			
			console.clearKeyboardString();
			console.printPrompt();
			
			return;
		}
		
		// Function not found. Check if the filename exists
		if (fs.file_open(console.keyboard_string) == 0) {
			
			console.print("Bad cmd or filename", sizeof("Bad cmd or filename"));
			console.printLn();
			
		} else {
			
			// File found
			
		}
		
	}
	
	console.clearKeyboardString();
	console.printPrompt();
	return;
}

void eventKeyboardBackspace(void) {
	if (console.keyboard_string_length > 0) {
		
		if (console.cursorPos == 0) {
			
			if (console.cursorLine > 0) {
				console.cursorPos = 20;
				console.cursorLine--;
			}
			
		}
		
		// Overwrite the character on the display ...
		console.cursorPos--;
		console.printSpace();
		console.cursorPos--;
		console.setCursorPosition(console.cursorLine, console.cursorPos);
		
		// and the keyboard string
		console.keyboard_string_length--;
		console.keyboard_string[console.keyboard_string_length] = 0x20;
	}
	return;
}

void eventKeyboardAcceptChar(uint8_t new_char) {
	
	console.keyboard_string[console.keyboard_string_length] = new_char;
	console.keyboard_string_length++;
	
	// Add the ASCII char
	console.printChar((char&)new_char);
	console.setCursorPosition(console.cursorLine, console.cursorPos);
	
	// Check to increment the display line
	if (console.cursorPos == 20) {
		
		if (console.cursorLine == 3) {
			console.shiftUp();
		} else {
			console.cursorLine++;
		}
		
		console.cursorPos=0;
		
		console.setCursorPosition(console.cursorLine, console.cursorPos);
	}
	
	return;
}

void eventKeyboardShiftPressed(void) {
	console.shiftState = 1;
	return;
}

void eventKeyboardShiftReleased(void) {
	console.shiftState = 0;
	return;
}


