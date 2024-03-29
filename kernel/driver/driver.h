#ifndef __DRIVER_TABLE_
#define __DRIVER_TABLE_

#include <kernel/bus/bus.h>
#include <kernel/device/device.h>

#include <kernel/cstring.h>

#define PERIPHERAL_ADDRESS_BEGIN  0x40000
#define PERIPHERAL_STRIDE         0x10000

#define NUMBER_OF_PERIPHERALS  5

#define DEVICE_NAME_LENGTH  10

#define DEVICE_TABLE_SIZE  16

#define DEVICE_REGISTRY_SIZE  24

#define nullptr  0x00000000


struct Driver {
    
    struct Device device;
    
    struct Bus interface;
    
    void(*read)(uint32_t address, uint8_t* buffer);
    void(*write)(uint32_t address, uint8_t buffer);
    
};



struct Driver* GetDriverByName(uint8_t* nameString, uint8_t stringSize);

struct Driver* GetDriverByIndex(uint8_t index);

uint8_t RegisterDriver(void* deviceDriverPtr);


uint8_t GetNumberOfDrivers(void);

#endif
