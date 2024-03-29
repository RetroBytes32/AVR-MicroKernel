#include <avr/io.h>

#include <kernel/kernel.h>

#include <kernel/fs/fs.h>
#include <kernel/fs/fs.h>

uint32_t fs_device_address = 0x40000;

void fsSetCurrentDevice(uint8_t device_index) {
    
    fs_device_address = 0x40000 + (device_index * 0x10000);
    
    return;
}

uint32_t fsGetCurrentDevice(void) {
    return fs_device_address;
}

uint8_t fsGetDeviceHeaderByte(uint32_t address_offset) {
    uint8_t headerByte = 0;
    
    struct Bus bus;
    bus.read_waitstate  = 4;
    
    bus_read_byte(&bus, fs_device_address + address_offset, &headerByte);
    
    return headerByte;
}

uint32_t fsGetDeviceCapacity(void) {
    
    struct Bus bus;
    bus.read_waitstate  = 4;
    
    uint8_t buffer[SECTOR_SIZE];
    
    // Get header sector
    for (uint8_t i=0; i < SECTOR_SIZE; i++) 
        bus_read_byte(&bus, fsGetCurrentDevice() + i, &buffer[i]);
    
    // Check header byte
    if (buffer[0] != 0x13) {
        return 0;
    }
    
    // Check hardware name
    if ((buffer[1] != 'f') | (buffer[2] != 's')) {
        return 0;
    }
    
    union Pointer sizePointer;
    
    // Get device capacity
    for (uint8_t i=0; i < 4; i++) 
        sizePointer.byte_t[i] = buffer[i + DEVICE_CAPACITY_OFFSET];
    
    return sizePointer.address;
}


uint32_t fsFileCreate(uint8_t* name, uint8_t nameLength, uint32_t fileSize) {
    
    struct Bus bus;
    bus.read_waitstate  = 4;
    
    uint32_t freeSectorCount = 0;
    uint32_t fileTargetAddress = 0;
    
    uint32_t currentDevice = fsGetCurrentDevice();
    
    uint32_t currentCapacity = fsGetDeviceCapacity();
    
    if (currentCapacity == 0) {
        uint8_t deviceNotReady[]= "Device not ready";
        print( &deviceNotReady[0], sizeof(deviceNotReady) );
        printLn();
        return 0;
    }
    
    // Verify the capacity
    // Default to minimum size if size unknown
    if ((currentCapacity != CAPACITY_8K) & 
        (currentCapacity != CAPACITY_16K) & 
        (currentCapacity != CAPACITY_32K)) {
        currentCapacity = CAPACITY_8K;
    }
    
    // Verify max capacity
    if (currentCapacity > CAPACITY_32K) {
        currentCapacity = CAPACITY_32K;
    }
    
    // Calculate sectors required to fit the file
    uint32_t totalSectors=0;
	for (uint32_t i=0; i < fileSize; i += (SECTOR_SIZE - 1)) {
		totalSectors++;
	}
	
	// Always have one sector
	if (totalSectors == 0) 
		totalSectors = 1;
	
    for (uint32_t sector=0; sector < currentCapacity; sector++) {
        
        // Get sector header byte
        uint8_t headerByte = 0;
        bus_read_byte(&bus, currentDevice + (sector * SECTOR_SIZE), &headerByte);
        
        // Find an empty sector
        if (fsGetDeviceHeaderByte( sector * SECTOR_SIZE ) != 0x00) 
            continue;
        
        // Find next sectors for total file size
        for (uint32_t nextSector = sector; nextSector < currentCapacity; nextSector++) {
            
            // Get sector header byte
            uint8_t headerByte = 0;
            bus_read_byte(&bus, currentDevice + (nextSector * SECTOR_SIZE), &headerByte);
            
            if (fsGetDeviceHeaderByte( nextSector * SECTOR_SIZE ) == 0x00) {
                
                // Check target reached
                if (freeSectorCount == totalSectors) {
                    
                    fileTargetAddress = currentDevice + (sector * SECTOR_SIZE);
                    
                    break;
                }
                
                freeSectorCount++;
                
                continue;
            }
            
            freeSectorCount = 0;
            
            break;
        }
        
		// File cannot fit into this empty space, continue seeking free space
		if (totalSectors != freeSectorCount) 
			continue;
		
        // Mark following sectors as taken
        for (uint32_t i = 0; i <= totalSectors; i++) 
            bus_write_byte_eeprom(fileTargetAddress + (i * SECTOR_SIZE), 0xff);
        
        // Mark the end of file sector
        bus_write_byte_eeprom(fileTargetAddress + (totalSectors * SECTOR_SIZE), 0xaa);
        
		// Mark the first sector
        uint8_t fileStartbyte = 0x55; // File start byte is 0x55
		bus_write_byte_eeprom(fileTargetAddress, fileStartbyte);
		
		// Blank the file name
		for (uint8_t i=0; i < 10; i++) 
            bus_write_byte_eeprom( fileTargetAddress + i + OFFSET_FILE_NAME, ' ' );
		
		// Write file name
		for (uint8_t i=0; i < nameLength; i++) 
            bus_write_byte_eeprom( fileTargetAddress + i + OFFSET_FILE_NAME, name[i] );
		
        // Set file size
        union Pointer sizePtr;
        sizePtr.address = fileSize;
        
        for (uint8_t i=0; i < 4; i++) 
            bus_write_byte_eeprom( fileTargetAddress + i + OFFSET_FILE_SIZE, sizePtr.byte_t[i] );
        
        // Write file attributes
        uint8_t attributes[4] = {' ', ' ', 'r', 'w'};
        for (uint8_t i=0; i < 4; i++) 
            bus_write_byte_eeprom( fileTargetAddress + i + OFFSET_FILE_ATTRIBUTES, attributes[i] );
        
        return fileTargetAddress;
    }
    
    return 0;
}

uint32_t fsFileDelete(uint8_t* name, uint8_t nameLength) {
    
    struct Bus bus;
    bus.read_waitstate  = 4;
    
    uint32_t currentDevice = fsGetCurrentDevice();
    
    uint32_t currentCapacity = fsGetDeviceCapacity();
    
    uint8_t clearByte = 0x00;
    
    if (currentCapacity == 0) {
        uint8_t deviceNotReady[]= "Device not ready";
        print( &deviceNotReady[0], sizeof(deviceNotReady) );
        printLn();
        return 0;
    }
    
    // Verify the capacity
    // Default to minimum size if size unknown
    if ((currentCapacity != CAPACITY_8K) & 
        (currentCapacity != CAPACITY_16K) & 
        (currentCapacity != CAPACITY_32K)) {
        currentCapacity = CAPACITY_8K;
    }
    
    // Verify max capacity
    if (currentCapacity > CAPACITY_32K) {
        currentCapacity = CAPACITY_32K;
    }
    
    // Delete following sectors allocated to this file
    for (uint32_t sector=0; sector < currentCapacity; sector++) {
        
        // Find an active file start byte
        if (fsGetDeviceHeaderByte( sector * SECTOR_SIZE ) != 0x55) 
            continue;
        
        uint8_t isFileFound = 0;
        
        // Check file name
        for (uint8_t i=0; i < nameLength; i++) {
            
            uint8_t nameByte = 0;
            
            bus_read_byte(&bus, currentDevice + (sector * SECTOR_SIZE) + OFFSET_FILE_NAME + i, &nameByte);
            
            if (name[i] != nameByte) {
                isFileFound = 0;
                break;
            }
            
            isFileFound = 1;
            
            continue;
        }
        
        // Was the file located
        if (isFileFound == 0) 
            continue;
        
        uint8_t isHeaderDeleted = 0;
        
        // Delete the file sectors
        for (uint32_t nextSector = sector; nextSector < currentCapacity; nextSector++) {
            
            // Get sector header byte
            uint8_t headerByte = 0;
            bus_read_byte(&bus, currentDevice + (nextSector * SECTOR_SIZE), &headerByte);
            
            // Delete file header sector
            if (headerByte == 0x55) {
                
                // Only delete a header once
                if (isHeaderDeleted == 1) 
                    return 1;
                
                bus_write_byte_eeprom(currentDevice + (nextSector * SECTOR_SIZE), clearByte);
                
                isHeaderDeleted = 1;
                continue;
            }
            
            // Delete sector
            if (headerByte == 0xff) {
                bus_write_byte_eeprom(currentDevice + (nextSector * SECTOR_SIZE), clearByte);
                continue;
            }
            
            // Delete end sector
            if (headerByte == 0xaa) {
                
                bus_write_byte_eeprom(currentDevice + (nextSector * SECTOR_SIZE), clearByte);
                
                return 1;
            }
            
            continue;
        }
        
		return 1;
    }
    
    return 0;
}

void fsListDirectory(void) {
    
    struct Bus bus;
    bus.read_waitstate  = 4;
    
    uint8_t buffer[SECTOR_SIZE];
    
    uint32_t currentDevice = fsGetCurrentDevice();
    
    uint32_t currentCapacity = fsGetDeviceCapacity();
    
    if (currentCapacity == 0) {
        
        uint8_t deviceNotReady[]= "Device not ready";
        
        print( &deviceNotReady[0], sizeof(deviceNotReady) );
        
        printLn();
        
        return;
    }
    
    // Default to minimum size if size unknown
    // Verify the capacity
    if ((currentCapacity != CAPACITY_8K) & 
        (currentCapacity != CAPACITY_16K) & 
        (currentCapacity != CAPACITY_32K)) {
        currentCapacity = CAPACITY_8K;
    }
    
    // Verify max capacity
    if (currentCapacity > CAPACITY_32K) {
        currentCapacity = CAPACITY_32K;
    }
    
    for (uint32_t sector=0; sector < currentCapacity; sector++) {
        
        // Get sector header byte
        uint8_t headerByte = 0;
        bus_read_byte(&bus, currentDevice + (sector * SECTOR_SIZE), &headerByte);
        
        // Check active sector
        
        if (fsGetDeviceHeaderByte( sector * SECTOR_SIZE ) != 0x55) 
            continue;
        
        // Get header sector
        for (uint8_t i=0; i < SECTOR_SIZE; i++) 
            bus_read_byte(&bus, currentDevice + (sector * SECTOR_SIZE) + i, &buffer[i]);
        
        // Get file size
        union Pointer fileSize;
        
        for (uint8_t i=0; i < 4; i++) 
            fileSize.byte_t[i] = buffer[ OFFSET_FILE_SIZE + i ];
        
        uint8_t integerString[8];
        uint8_t len = int_to_string(fileSize.address, integerString) + 1;
        
        if (len > 6) 
            len = 6;
        
        // Get file attributes
        uint8_t fileAttributes[4];
        
        for (uint8_t i=0; i < 4; i++) 
            fileAttributes[i] = buffer[ OFFSET_FILE_ATTRIBUTES + i ];
        
        // Check special attribute
        
        // Directory
        if (fileAttributes[0] == 'd') {
            
            print(&buffer[1], FILE_NAME_LENGTH + 1);
            
            printSpace(1);
            
            uint8_t directoryAttribute[] = "<dir>";
            print( &directoryAttribute[0], sizeof(directoryAttribute) );
            
        }
        
        // File
        if (fileAttributes[0] != 'd') {
            
            uint8_t integerOffset = 6 - len;
            
            if (integerOffset > 5) 
                integerOffset = 5;
            
            print( &fileAttributes[1], 4 );
            
            print(&buffer[1], FILE_NAME_LENGTH + 1);
            
            printSpace(integerOffset);
            print( integerString, len);
            
            printSpace(1);
            
        }
        
        printLn();
        
        continue;
    }
    
    return;
}

uint8_t fsFormatDevice(void) {
    
    uint32_t deviceCapacity = CAPACITY_8K;
    
    uint32_t currentDevice = fsGetCurrentDevice();
    
    // Full clean
    for (uint32_t i=0; i < 256; i++) 
        bus_write_byte_eeprom( currentDevice + i, ' ' );
    
    // Mark sectors as available
    for (uint32_t sector = 0; sector < deviceCapacity; sector++) 
        bus_write_byte_eeprom( currentDevice + (sector * SECTOR_SIZE), 0x00 );
    
    // Zero the header sector
    for (uint32_t i=0; i < 32; i++) {
        
        if (i == 0) {
            bus_write_byte_eeprom( currentDevice + i, 0x00 );
        } else {
            bus_write_byte_eeprom( currentDevice + i, ' ' );
        }
        
    }
    
    // Initiate first sector
    bus_write_byte_eeprom( currentDevice + 0, 0x13 );
    bus_write_byte_eeprom( currentDevice + 1, 'f' );
    bus_write_byte_eeprom( currentDevice + 2, 's' );
    
    // Device size
    
    union Pointer deviceSize;
    deviceSize.address = deviceCapacity * SECTOR_SIZE;
    
    for (uint8_t i=0; i < 4; i++) 
        bus_write_byte_eeprom(currentDevice + DEVICE_CAPACITY_OFFSET + i, deviceSize.byte_t[i]);
    
    return 1;
}


uint8_t fsRepairDevice(void) {
    
    struct Bus bus;
    bus.read_waitstate  = 4;
    
    uint8_t buffer[SECTOR_SIZE];
    
    // Get header sector
    uint32_t currentDevice = fsGetCurrentDevice();
    
    for (uint8_t i=0; i < SECTOR_SIZE; i++) 
        bus_read_byte(&bus, currentDevice + i, &buffer[i]);
    
    // Check header byte
    if (buffer[0] != 0x13) {
        
        uint8_t headerByte = 0x13;
        
        // Attempt to fix the header byte
        bus_write_byte_eeprom(currentDevice, headerByte);
        
    }
    
    // Check hardware name
    if ((buffer[1] != 'f') | (buffer[2] != 's')) {
        
        uint8_t deviceName[2] = {'f', 's'};
        
        // Attempt to fix the header byte
        bus_write_byte_eeprom(currentDevice + 1, deviceName[0]);
        bus_write_byte_eeprom(currentDevice + 2, deviceName[1]);
        
        
    }
    
    //union Pointer sizePointer;
    
    // Get device capacity
    //for (uint8_t i=0; i < 4; i++) 
    //    sizePointer.byte_t[i] = buffer[i + DEVICE_CAPACITY_OFFSET];
    
    return 1;
}



