#include <drivers/display/LiquidCrystalDisplayController/main.h>

#include <kernel/kernel.h>

struct DisplayDeviceDriver displayDeviceDriver;

struct DisplayDeviceDriver* displayDriver = &displayDeviceDriver;


// Driver function declarations

void __read_display_device(uint32_t address, char* buffer);
void __write_display_device(uint32_t address, char buffer);


void initiateDisplayDriver(void) {
    
    char deviceName[] = "display";
    
    for (unsigned int i=0; i < sizeof(deviceName); i++) {
        displayDriver->device.device_name[i] = deviceName[i];
    }
    
    // Register the driver with the kernel
	RegisterDriver( (void*)displayDriver );
	
	// Set hardware device details
	displayDriver->device.hardware_address = 0x00000;
	
	displayDriver->device.device_id = 0x10;
	
	displayDriver->interface.read_waitstate  = 10;
	displayDriver->interface.write_waitstate = 10;
    
	// Initiate member functions
	
    displayDriver->read  = __read_display_device;
    displayDriver->write = __write_display_device;
    
    return;
}


void __read_display_device(uint32_t address, char* buffer) {
    bus_read_byte( &displayDriver->interface, displayDriver->device.hardware_address + address, buffer );
    return;
}

void __write_display_device(uint32_t address, char buffer) {
    bus_write_byte( &displayDriver->interface, displayDriver->device.hardware_address + address, buffer );
    return;
}
