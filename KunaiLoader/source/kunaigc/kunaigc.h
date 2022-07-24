/*
 * kunaigc.h
 *
 *  Created on: Jul 11, 2022
 *      Author: mancloud
 */

#ifndef KUNAIGC_H_
#define KUNAIGC_H_

#include <gccore.h>
#include <unistd.h>


#include "../spiflash/spiflash.h"

extern u8 *dol;

extern void dol_alloc(int size);

void kunai_sector_erase(uint32_t addr);
int kunai_load_payload(u32 addr, size_t size);
static inline void kunai_disable_passthrough(void);
void kunai_enable_passthrough(void);
uint16_t kunai_get_deviceid(void);
uint32_t kunai_read_32bit(uint32_t addr);
void kunai_write_32bit(uint32_t data, uint32_t addr);
int8_t kunai_write_page(uint32_t * data, uint32_t addr, bool verify);
void kunai_disable(void);

#endif /* KUNAIGC_H_ */
