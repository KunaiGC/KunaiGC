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
#include "../lfs/lfs.h"

extern u8 *dol;

extern void dol_alloc(int size);

#define KUNAI_OFFS (256*1024) //first 512KiB are reserver for loader + recovery

void kunai_sector_erase(uint32_t addr);
int kunai_load_payload(u32 addr, size_t size);
void kunai_disable_passthrough(void);
void kunai_enable_passthrough(void);
uint32_t kunai_get_jedecID(void);
uint32_t kunai_read_32bit(uint32_t addr);

int kunai_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int kunai_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int kunai_erase(const struct lfs_config *c, lfs_block_t block);
int kunai_sync(const struct lfs_config *c);

void kunai_write_32bit(uint32_t data, uint32_t addr);
int8_t kunai_write_page(uint32_t * data, uint32_t addr, bool verify);
void kunai_disable(void);
void kunai_reenable(void);

#endif /* KUNAIGC_H_ */
