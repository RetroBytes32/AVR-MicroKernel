#include <kernel/strcase.h>

int is_number(uint8_t* charPtr) {
    if ((*charPtr >= 0x30) & (*charPtr <= 0x39))
        return 1;
    return 0;
}

int is_letter(uint8_t* charPtr) {
    if (is_uppercase(charPtr)) return 1;
    if (is_lowercase(charPtr)) return 1;
    return 0;
}


int is_uppercase(uint8_t* charPtr) {
    if ((*charPtr >= 0x41) & (*charPtr <= 0x5a))
        return 1;
    return 0;
}

int is_lowercase(uint8_t* charPtr) {
    if ((*charPtr >= 0x61) & (*charPtr <= 0x7a))
        return 1;
    return 0;
}

void make_uppercase(uint8_t* charPtr) {
    if (is_lowercase(charPtr) == 1) 
        charPtr -= 0x20;
    return;
}

void make_lowercase(uint8_t* charPtr) {
    if (is_uppercase(charPtr) == 1) 
        charPtr += 0x20;
    return;
}
