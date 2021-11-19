//
// AVR Micro Kernel
#include "strings.h"

#define _KERNEL_BEGIN__       0x00000
#define _KERNEL_FLAGS__       0x00020

#define _KERNEL_STATE_NORMAL__          0x00
#define _KERNEL_STATE_OUT_OF_MEMORY__   0xff

#define _MESSAGE_QUEUE_LENGTH__         32

void memory_read(uint32_t address, char& buffer);
void memory_write(uint32_t address, char byte);

void device_read(uint32_t address, char& byte);
void device_write(uint32_t address, char byte);

#include "string_const.h"

#include "stack_allocator.h"
#include "device_index.h"

#include "drivers/display_driver.h"
#include "drivers/file_system.h"
#include "drivers/keyboard.h"

#include "console.h"


// Default message callback procedure
uint8_t defaultCallbackProcedure(uint8_t message);

struct Kernel {
	
	uint8_t keyboardState;
	uint8_t lastChar;
	
	uint8_t messageQueue[_MESSAGE_QUEUE_LENGTH__];
	uint8_t queueCounter;
	
	uint8_t (*callbackPointer)(uint8_t);
	
	// Post a message to the kernel
	void postMessage(uint8_t message) {
		
		// Shift up the queue
		for (uint8_t i=0; i < _MESSAGE_QUEUE_LENGTH__; i++) 
			messageQueue[i+1] = messageQueue[i];
		
		// Add queue
		messageQueue[0] = message;
		queueCounter++;
		
		return;
	}
	
	// Check the number of messages currently in the queue
	uint8_t peekMessage(void) {return queueCounter;}
	
	Kernel() {
		
		keyboardState=0;
		lastChar=0;
		
		// Initiate message queue
		for (uint8_t i=0; i < _MESSAGE_QUEUE_LENGTH__; i++) 
			messageQueue[i] = 0xff;
		
		queueCounter=0;
		
		// Set default message callback procedure
		callbackPointer = &defaultCallbackProcedure;
		
	}
	
	// Initiate kernel memory
	void initiate(void) {
		
		mem_zero(_ALLOCATOR_COUNTER_ADDRESS__, 4); // Number of external memory allocations
		mem_zero(_KERNEL_FLAGS__, 8);              // Kernel state flags
		
		// Setup keyboard
		lastChar = keyboard.read();
		
	}
	
	//
	// Starts the kernel loop
	void run(void) {
		
		uint16_t kernelTimeOut   = 50;
		uint16_t keyboardTimeOut = 800;
		
		uint16_t kernelCounter   = 0;
		uint16_t keyboardCounter = 0;
		
		console.setPrompt();
		
		bool isActive=1;
		while(isActive) {
			
			kernelCounter++;
			if (kernelCounter < kernelTimeOut) {continue;} kernelCounter=0;
			
			keyboardCounter++;
			if (keyboardCounter > keyboardTimeOut) {keyboardCounter=0; checkKeyboardState();}
			
			processMessageQueue();
			
		}
		
		return;
	}
	
	// Process a message from the message queue
	void processMessageQueue(void) {
		
		if (queueCounter == 0) return;
		
		// Grab the first message
		uint8_t message = messageQueue[0];
		
		// Shift down the queue
		for (uint8_t i=0; i < _MESSAGE_QUEUE_LENGTH__; i++)
		messageQueue[i] = messageQueue[i+1];
		
		// Decrement the queue
		queueCounter--;
		
		// Call the callback procedure function with the message
		(callbackPointer)( message );
		
	}
	
	// Check keyboard input
	void checkKeyboardState(void) {
		
		uint8_t scanCodeAccepted = 0;
		uint8_t currentChar=0x00;
		
		uint8_t readKeyCode = keyboard.read();
		
		// Decode the scan code
		if (console.keyboard_string_length < 19) {
			// Check current code
			currentChar = readKeyCode;
			if (currentChar > 0x1f) scanCodeAccepted=1;
		} else {
			// Allow backspace and enter past max length
			if (readKeyCode < 0x03) currentChar = readKeyCode;
		}
		
		// Prevent wild key repeats
		if (lastChar == currentChar) {keyboardState=1; return;} lastChar = currentChar;
		
		// Backspace
		if (currentChar == 0x01) {
			
			if (console.keyboard_string_length > 0) {keyboardState=1;
				
				// Remove last char from display
				displayDriver.writeChar(0x20, console.cursorLine, console.keyboard_string_length);
				displayDriver.cursorSetPosition(console.cursorLine, console.keyboard_string_length);
				// From keyboard string
				console.keyboard_string_length--;
				console.keyboard_string[console.keyboard_string_length] = 0x20;
			}
			
		}
		
		// Enter
		if (currentChar == 0x02) {keyboardState = 1; console.executeKeyboardString(); return;}
		
		// ASCII key character accepted
		if (scanCodeAccepted == 1) {keyboardState=1;
			
			console.keyboard_string[console.keyboard_string_length] = currentChar;
			console.keyboard_string_length++;
			// Update cursor
			displayDriver.cursorSetPosition(console.cursorLine, console.keyboard_string_length+1);
			// Display keyboard string
			displayDriver.writeString(console.keyboard_string, console.keyboard_string_length+1, console.cursorLine, console.cursorPos);
			displayDriver.writeChar(console.promptCharacter, console.cursorLine, 0);
		}
	}
	
};
Kernel kernel;

uint8_t defaultCallbackProcedure(uint8_t message) {
	
	switch (message) {
		
		case 0x00: {
			char String[] = "Message callback";
			console.addString(String, sizeof(String));
			return 1;
		}
		
		default:
			break;
	}
	
	return 0;
}




//
// Low level memory IO
void memory_read(uint32_t address, char& buffer) {
	
	// Address the device
	_BUS_LOWER_DIR__ = 0xff;
	
	_CONTROL_OUT__ = 0b00111111;
	_BUS_LOWER_OUT__ = (address & 0xff);
	_BUS_MIDDLE_OUT__ = (address >> 8);
	_BUS_UPPER_OUT__ = (address >> 16);
	_CONTROL_OUT__ = 0b00110100;
	
	// Set data direction
	_BUS_LOWER_DIR__ = 0x00; // Data bus input
	_BUS_LOWER_OUT__ = 0xff; // Pull-up resistors
	
	_CONTROL_OUT__ = 0b00100100;
	
	// Wait states
#ifdef _MEMORY_READ_WAIT_STATE_1__
	nop;
#endif
#ifdef _MEMORY_READ_WAIT_STATE_2__
	nop; nop;
#endif
#ifdef _MEMORY_READ_WAIT_STATE_3__
	nop; nop; nop;
#endif
	
	// Read the data byte
	buffer = _BUS_LOWER_IN__;
	
	_CONTROL_OUT__ = 0b00111111;
	_BUS_UPPER_OUT__ = 0x0f;
	
	return;
}

void memory_write(uint32_t address, char byte) {
	
	// Address the device
	_BUS_LOWER_DIR__ = 0xff;
	
	_CONTROL_OUT__ = 0b00111111;
	//_CONTROL_OUT__ |= (1 << _CONTROL_ADDRESS_LATCH__);
	_BUS_LOWER_OUT__ = (address & 0xff);
	_BUS_MIDDLE_OUT__ = (address >> 8);
	_BUS_UPPER_OUT__ = (address >> 16);
	//_CONTROL_OUT__ &= ~(1 << _CONTROL_ADDRESS_LATCH__);
	_CONTROL_OUT__ = 0b00110101;
	
	// Cast the data byte
	_BUS_LOWER_OUT__ = byte;
	
	_CONTROL_OUT__ = 0b00010101;
	
	// Wait states
#ifdef _MEMORY_WRITE_WAIT_STATE_1__
	nop;
#endif
#ifdef _MEMORY_WRITE_WAIT_STATE_2__
	nop; nop;
#endif
#ifdef _MEMORY_WRITE_WAIT_STATE_3__
	nop; nop; nop;
#endif
	
	_CONTROL_OUT__ = 0b00111111;
	_BUS_UPPER_OUT__ = 0x0f;
	
	return;
}

void device_read(uint32_t address, char& byte) {
	
	// Address the device
	_BUS_LOWER_DIR__ = 0xff;
	
	_CONTROL_OUT__ = 0b00111111;
	_BUS_LOWER_OUT__ = (address & 0xff);
	_BUS_MIDDLE_OUT__ = (address >> 8);
	_BUS_UPPER_OUT__ = (address >> 16);
	_CONTROL_OUT__ = 0b00110100;
	
	// Set data direction
	_BUS_LOWER_DIR__ = 0x00; // Data bus input
	_BUS_LOWER_OUT__ = 0xff; // Pull-up resistors
	
	// Read operation
	_CONTROL_OUT__ = 0b00100100;
	
	// Wait states
#ifdef _DEVICE_READ_WAIT_STATE_1__
	for (uint8_t i=0; i<10; i++) {nop;nop;nop;nop;}
#endif
#ifdef _DEVICE_READ_WAIT_STATE_2__
	for (uint8_t i=0; i<50; i++) {nop;nop;nop;nop;}
#endif
#ifdef _DEVICE_READ_WAIT_STATE_3__
	for (uint8_t i=0; i<100; i++) {nop;nop;nop;nop;}
#endif
	
	byte = _BUS_LOWER_IN__;
	
	_CONTROL_OUT__ = 0b00111111;
	_BUS_UPPER_OUT__ = 0x0f;
	
	return;
}

void device_write(uint32_t address, char byte) {
	
	// Address the device
	_BUS_LOWER_DIR__ = 0xff;
	
	_CONTROL_OUT__ = 0b00111111;
	_BUS_LOWER_OUT__ = (address & 0xff);
	_BUS_MIDDLE_OUT__ = (address >> 8);
	_BUS_UPPER_OUT__ = (address >> 16);
	_CONTROL_OUT__ = 0b00110101;
	
	_BUS_LOWER_OUT__ = byte;
	
	// Write operation
	_CONTROL_OUT__ = 0b00010101;
	
	// Wait states
#ifdef _DEVICE_WRITE_WAIT_STATE_1__
	for (uint8_t i=0; i<10; i++) {nop;nop;nop;nop;}
#endif
#ifdef _DEVICE_WRITE_WAIT_STATE_2__
	for (uint8_t i=0; i<50; i++) {nop;nop;nop;nop;}
#endif
#ifdef _DEVICE_WRITE_WAIT_STATE_3__
	for (uint8_t i=0; i<100; i++) {nop;nop;nop;nop;}
#endif
	
	_CONTROL_OUT__ = 0b00111111;
	_BUS_UPPER_OUT__ = 0x0f;
	
	return;
}


// Allocate and display total available system memory
uint32_t allocate_system_memory(void) {
	
	displayDriver.cursorSetPosition(1, 0);
	console.cursorLine = 1;
	console.cursorPos  = 0;
	
	uint32_t updateTimer=0;
	
	displayDriver.writeString(string_memory_allocator, sizeof(string_memory_allocator));
	
	string memoryString;
	
	#ifdef _TEST_RAM_EXTENSIVELY__
	char readByteTest=0x00;
	#endif
	
	char readByte=0x00;
	uint32_t total_system_memory;
	
	// Copy the kernel memory before wiping the ram
	uint8_t kernelBuffer[0x1f];
	uint8_t kernelBufferSize = 0x1f;
	for (uint8_t address=0; address < kernelBufferSize; address++) {
		memory_read(address, readByte);
		kernelBuffer[address] = readByte;
	}
	
	for (total_system_memory=0x00000; total_system_memory < 0x40000; ) {
		
		// Run some memory cycles
		memory_write(total_system_memory, 0x55);
		memory_read(total_system_memory, readByte);
		
		// Check if the byte(s) stuck
#ifdef _TEST_RAM_EXTENSIVELY__
		// Run some more memory cycles
		memory_write(total_system_memory, 0xaa);
		memory_read(total_system_memory, readByteTest);
		if ((readByte == 0x55) && (readByteTest == 0xaa)) {
#else
		if (readByte == 0x55) {
#endif
		
#ifdef _FAST_BOOT__
		total_system_memory += 0x8000;
#else
		total_system_memory++;
#endif
	} else {
	
	if (total_system_memory == 0x00000) {
		
		// Restore the kernel memory(if its installed)
		for (uint8_t address=0; address < kernelBufferSize; address++) memory_write(address, kernelBuffer[address]);
		
		displayDriver.writeString(error_exmem_not_installed, sizeof(error_exmem_not_installed));
		displayDriver.cursorSetPosition(2, 0);
		while(1) nop;
	}
	break;
	}
	
	// Restore the kernel memory
	if ((total_system_memory < 0x0001f) || (total_system_memory == 0x8000)) {
		for (uint8_t address=0; address < kernelBufferSize; address++) memory_write(address, kernelBuffer[address]);
	}
	
	
	// Update display
	if (updateTimer > 1024) {updateTimer=0;
		
		string memoryString;
		intToString(total_system_memory, memoryString.str);
		displayDriver.writeString(memoryString.str, 7);
		
		} else {updateTimer++;}
		
	}
	
	// Calculate free memory
	uint32_t memoryFree = (total_system_memory - 0x00100);
	
	// Display available memory
	intToString(memoryFree, memoryString.str);
	displayDriver.writeString(memoryString.str, 7);
	
	return total_system_memory;
}


