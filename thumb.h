#ifndef THUMB_H
#define THUMB_H

#include <stdint.h>


// Function prototypes
void thumb_init_table();
void thumb_decode_and_execute(uint16_t instruction);

#endif // THUMB_H
