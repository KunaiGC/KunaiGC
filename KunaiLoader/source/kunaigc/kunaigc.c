/*
 * kunaigc.c
 *
 *  Created on: Jul 11, 2022
 *      Author: mancloud
 */


#include "kunaigc.h"

int kunai_load_payload(u32 addr, size_t size){
	kprintf("Trying loading from internal Memory");
	int res = 1;
	addr <<= 6;

	EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL);
	EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED16MHZ);
	EXI_Imm(EXI_CHANNEL_0, &addr, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	EXI_Imm(EXI_CHANNEL_0, &size, 4, EXI_READ, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	dol_alloc(size);
	if(!dol)
	{
		res = 0;
		goto end;
	}
	kprintf("Receiving file...\n");
	unsigned char* pointer = dol;
	while(size--){
		EXI_Imm(EXI_CHANNEL_0, pointer++, 1, EXI_READ, NULL);
		EXI_Sync(EXI_CHANNEL_0);
	}
	end:
	EXI_Deselect(EXI_CHANNEL_0);
	EXI_Unlock(EXI_CHANNEL_0);
	return res;
}

//wait for "WIP" flag being unset
#define kunai_wait_M() ({					\
		usleep(150000);						\
		kunai_enable_passthrough();			\
		spiflash_wait();					\
		kunai_disable_passthrough();		\
})

static inline void kunai_disable_passthrough(void) {
	EXI_Deselect(EXI_CHANNEL_0);
	EXI_Unlock(EXI_CHANNEL_0);
//	usleep(75000);
}

void kunai_enable_passthrough(void) {
	s32 retVal = 0;
	uint8_t repetitions = 3;
	do {
		u32 addr = 0x80000000; //for passthrough we need to send one '1' and 31 '0' and afterwards whatever we want
		EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL);
		EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED16MHZ);
		EXI_Imm(EXI_CHANNEL_0, &addr, 4, EXI_WRITE, NULL);
		retVal = EXI_Sync(EXI_CHANNEL_0);
	} while(retVal <= 0 && --repetitions);
}



uint16_t kunai_get_deviceid(void) {
	uint16_t deviceID = 0;
	kunai_enable_passthrough();
	deviceID = spiflash_device_id();
	kunai_disable_passthrough();
	return deviceID;
}

uint32_t kunai_read_32bit(uint32_t addr){
	uint32_t retVal = 0;
	kunai_enable_passthrough();
	spiflash_read_start(addr);
	retVal = spiflash_read_uint32();
	kunai_disable_passthrough();
	return retVal;
}

void kunai_write_32bit(uint32_t data, uint32_t addr) {
	kunai_enable_passthrough();
	spiflash_write_enable();
	kunai_disable_passthrough();
	kunai_enable_passthrough();
	spiflash_cmd_addr_start(W25Q80BV_CMD_PAGE_PROG, addr);
	spiflash_write_uint32(data);
	kunai_disable_passthrough();
	kunai_wait_M();

}


//writes W25Q80BV_PAGE_SIZE bytes at given address (rollover at end of page)
//returns 	 0 -  not verified
//			 1 - verify - OK
//			-1 - verify - NOK
int8_t kunai_write_page(uint32_t * data, uint32_t addr, bool verify) {
	uint8_t retVal = 0;
	uint32_t * p_data = data;
	kunai_enable_passthrough();
	spiflash_write_enable();
	kunai_disable_passthrough();
	kunai_enable_passthrough();
	spiflash_cmd_addr_start(W25Q80BV_CMD_PAGE_PROG, addr);
	for(uint8_t i = 0; i < (W25Q80BV_PAGE_SIZE/4); i++) {
		EXI_Imm(EXI_CHANNEL_0, p_data++, 4, EXI_WRITE, NULL);
		EXI_Sync(EXI_CHANNEL_0);
	}
	kunai_disable_passthrough();
	kunai_wait_M();
	//verify
	if(verify) {
		uint32_t buff;
		uint8_t retVal = 1;
		p_data = data;
		kunai_enable_passthrough();
		spiflash_cmd_addr_start(W25Q80BV_CMD_READ_DATA, addr);
		for(uint8_t i = 0; i < (W25Q80BV_PAGE_SIZE/4); i++) {
			EXI_Imm(EXI_CHANNEL_0, &buff, 4, EXI_READ, NULL);
			EXI_Sync(EXI_CHANNEL_0);
			if(*(p_data++) != buff) {
				retVal = -1;
				break;
			}
		}
		kunai_disable_passthrough();
	}
	return retVal;
}

void kunai_disable(void) {
	u32 addr = 0xFFFFFFFF; //disable the KunaiGC by sending only '1' to the Bus
	EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL);
	EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED8MHZ);
	EXI_Imm(EXI_CHANNEL_0, &addr, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	EXI_Deselect(EXI_CHANNEL_0);
	EXI_Unlock(EXI_CHANNEL_0);
}

void kunai_sector_erase(uint32_t addr) {
	kunai_enable_passthrough();
	spiflash_write_enable();
	kunai_disable_passthrough();
	kunai_enable_passthrough();
	spiflash_cmd_addr_start(W25Q80BV_CMD_ERASE_4K, addr);
	kunai_disable_passthrough();
	kunai_wait_M();
}

