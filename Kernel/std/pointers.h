// Pointer definitions

#define nullptr 0x00000000

#define _32_BIT_POINTERS__
//#define _64_BIT_POINTERS__


// Address pointer wrapper
union WrappedPointer {
	
#ifdef _32_BIT_POINTERS__
	char     byte[4];
	uint8_t  byte_t[8];
	uint16_t word[2];
	uint32_t dword;
	uint32_t address;
#endif
#ifdef _64_BIT_POINTERS__
	char     byte[8];
	uint8_t  byte_t[8];
	uint16_t word[4];
	uint32_t dword[2];
	uint64_t address;
#endif
	
	WrappedPointer() {address=0;}
	
#ifdef _32_BIT_POINTERS__
	WrappedPointer(uint32_t newAddress) {address=newAddress;}
#endif
#ifdef _64_BIT_POINTERS__
	WrappedPointer(uint64_t newAddress) {address=newAddress;}
#endif
	
};

