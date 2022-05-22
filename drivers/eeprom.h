// Built-in EEPROM storage driver

#ifndef _EEPROM_DRIVER__
#define _EEPROM_DRIVER__

void EEPROMDeviceDriverEntryPoint(uint8_t, uint8_t&, uint8_t&, uint8_t&, uint8_t&);

struct EEPROMDriverLoader {
	EEPROMDriverLoader() {loadLibrary(_EEPROM_STORAGE__, sizeof(_EEPROM_STORAGE__), (DriverEntryPoint)EEPROMDeviceDriverEntryPoint);}
};EEPROMDriverLoader eepromDriverLoader;


struct EEPROMStorage {
	
	WrappedPointer pointer;
	
	void initiate(void) {
		
	}
	
	void shutdown(void) {
		
	}
	
	
	void write(uint32_t address, char byte) {
		// Wait for completion of previous write
		while(EECR & (1<<EEPE));
		
		// Set up address and Data Registers
		EEAR = address;
		EEDR = byte;
		
		// Write logical one to EEMPE
		EECR |= (1<<EEMPE);
		// Start EEPROM write by setting EEPE
		EECR |= (1<<EEPE);
	}
	
	void read(uint32_t address, char& byte) {
		// Wait for completion of previous write
		while(EECR & (1<<EEPE));
		
		// Set up address register
		EEAR = address;
		// Start EEPROM read by writing EERE
		EECR |= (1<<EERE);
		
		// Return data from Data Register
		byte = EEDR;
	}
	
} static eepromStorage;





void EEPROMDeviceDriverEntryPoint(uint8_t functionCall, uint8_t& paramA, uint8_t& paramB, uint8_t& paramC, uint8_t& paramD) {
	
	switch(functionCall) {
		
		case _DRIVER_INITIATE__: {eepromStorage.initiate(); break;}
		case _DRIVER_SHUTDOWN__: {eepromStorage.shutdown(); break;}
		
		case 0x00:  eepromStorage.pointer.byte_t[0] = paramA; eepromStorage.pointer.byte_t[1] = paramB; eepromStorage.pointer.byte_t[2] = paramC; eepromStorage.pointer.byte_t[3] = paramD; break;
		
		case 0x01:  eepromStorage.write(eepromStorage.pointer.address, paramA); break;
		case 0x02:  eepromStorage.read(eepromStorage.pointer.address, (char&)paramA); break;
		
		// Auto increment Read / Write
		case 0x03:  eepromStorage.write(eepromStorage.pointer.address, paramA);       eepromStorage.pointer.address++; break;
		case 0x04:  eepromStorage.read(eepromStorage.pointer.address, (char&)paramA); eepromStorage.pointer.address++; break;
		
		default: break;
	}
	
	return;
}




#endif
