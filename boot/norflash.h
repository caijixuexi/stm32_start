#ifndef __BL_NORFLASH_H
#define __BL_NORFLASH_H


#include <stdint.h>


void bl_norflash_init(void);
void bl_norflash_erase(uint32_t address, uint32_t size);
void bl_norflash_write(uint32_t address, uint8_t *data, uint32_t size);


#endif /* __BL_NORFLASH_H */
