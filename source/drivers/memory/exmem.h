//
// Extended memory driver

#ifndef __HIGH_MEMORY__
#define __HIGH_MEMORY__

#include <avr/interrupt.h>

char msg_bytes_free[]       = "bytes free";
char error_stack_error[]    = "Stack error!";
char error_stack_fault[]    = "Stack fault!";
char error_out_of_memory[]  = "Out of memory!";
char error_seg_fault[]      = "Segmentation-fault";

struct exMem {
	
	Bus device_bus;
	
	uint32_t _STACK_BEGIN__;
	uint32_t _STACK_END__;
	
	uint32_t currentAddress;
	uint32_t returnAddress;
	
	exMem() {
		
		_STACK_BEGIN__ = _KERNEL_STACK_BEGIN__;
		_STACK_END__   = 0;
		
		currentAddress = 0;
		returnAddress  = 0;
		
		device_bus.waitstate_read  = 2;
		device_bus.waitstate_write = 0;
		
	}
	
	void write(uint32_t address, char byte) {bus_write_byte(device_bus, address, byte); return;}
	void read(uint32_t address, char& byte) {bus_read_byte(device_bus, address, byte); return;}
	
	uint32_t stack_push(uint32_t size) {
		
		return _KERNEL_STACK_BEGIN__;
		
		WrappedPointer numberOfAllocations;
		for (uint8_t i=0; i < 4; i++) read(_KERNEL_STACK_COUNTER__ + i, numberOfAllocations.byte[i]);
		
		if ((numberOfAllocations.address + size + _STACK_BEGIN__) > _STACK_END__) {
			write(_KERNEL_FLAGS__, _KERNEL_STATE_OUT_OF_MEMORY__);
			return 0;
		}
		
		uint32_t new_pointer = _STACK_BEGIN__ + (numberOfAllocations.address);
		
		numberOfAllocations.address += size;
		
		for (uint8_t i=0; i<4; i++) write(_KERNEL_STACK_COUNTER__ + i, numberOfAllocations.byte[i]);
		
		return new_pointer;
	}
	
	void stack_pop(uint32_t size) {
		
		return;
		
		WrappedPointer numberOfAllocations;
		for (uint8_t i=0; i<4; i++) read(_KERNEL_STACK_COUNTER__ + i, numberOfAllocations.byte[i]);
		
		numberOfAllocations.address -= size;
		
		for (uint8_t i=0; i<4; i++) write(_KERNEL_STACK_COUNTER__ + i, numberOfAllocations.byte[i]);
		
		return;
	}
	
	void mem_zero(uint32_t address_pointer, uint32_t size) {
		for (uint32_t i=0; i<=size; i++) {write(address_pointer+i, 0x00);}
	}
	
	uint32_t stack_size(void) {
		
		WrappedPointer numberOfAllocations;
		for (uint8_t i=0; i<4; i++) read(_KERNEL_STACK_COUNTER__ + i, numberOfAllocations.byte[i]);
		
		return numberOfAllocations.address;
	}
	
}static exMem;




// Driver entry point
void ExtendedMemoryDeviceDriverEntryPoint(uint8_t functionCall, uint8_t& paramA, uint8_t& paramB, uint8_t& paramC, uint8_t& paramD) {
	
	if (functionCall == 0x00) {exMem.returnAddress = exMem.stack_push(exMem.currentAddress); return;}
	if (functionCall == 0x01) {exMem.stack_pop(exMem.currentAddress); return;}
	if (functionCall == 0x02) {exMem.mem_zero(exMem.currentAddress, paramA); exMem.currentAddress += paramA; return;}
	if (functionCall == 0x03) {exMem.returnAddress = exMem.stack_size(); return;}
	if (functionCall == 0x04) {exMem.returnAddress = exMem._STACK_END__; return;}
	if (functionCall == 0x05) {exMem._STACK_END__ = exMem.currentAddress; return;}
	
	// Set the address pointer
	if (functionCall == 0x0a) {
		WrappedPointer pointer;
		pointer.byte_t[0] = paramA;
		pointer.byte_t[1] = paramB;
		pointer.byte_t[2] = paramC;
		pointer.byte_t[3] = paramD;
		exMem.currentAddress = pointer.address;
		return;
	}
	
	// Get the return pointer
	if (functionCall == 0x0c) {
		WrappedPointer pointer;
		pointer.address = exMem.returnAddress;
		paramA = pointer.byte_t[0];
		paramB = pointer.byte_t[1];
		paramC = pointer.byte_t[2];
		paramD = pointer.byte_t[3];
		return;
	}
	
	return;
}





#endif
