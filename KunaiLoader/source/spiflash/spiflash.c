/* (c) 2016-07-01 Jens Hauke <jens.hauke@4k2.de> */
#include "spiflash.h"


uint8_t spiflash_is_busy(void) {
	uint8_t res;
	spiflash_write(W25Q80BV_CMD_READ_STAT1);
	res = (spiflash_read_uint8() & W25Q80BV_MASK_STAT_BUSY);
	return res;
}


// Wait until not busy
void spiflash_wait(void) {
	spiflash_write(W25Q80BV_CMD_READ_STAT1);
	// ToDo: Timeout?
	while (spiflash_read_uint8() & W25Q80BV_MASK_STAT_BUSY);
}

void spiflash_cmd_addr_start(uint8_t cmd, uint32_t addr) {
	uint32_t buff = (cmd << 24) | addr;
	EXI_Imm(EXI_CHANNEL_0, &buff, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
}


void spiflash_write_enable(void) {
	spiflash_write(W25Q80BV_CMD_WRITE_ENABLE);
}

void spiflash_write_disable(void) {
	spiflash_write(W25Q80BV_CMD_WRITE_DISABLE);
}


void spiflash_write_start(uint32_t addr) {
	spiflash_write_enable();
	spiflash_cmd_addr_start(W25Q80BV_CMD_PAGE_PROG, addr);
}


void spiflash_read_start(uint32_t addr) {
	spiflash_cmd_addr_start(W25Q80BV_CMD_READ_DATA, addr);
}

void spiflash_read_start_fast(uint32_t addr) {
	spiflash_cmd_addr_start(W25Q80BV_CMD_READ_FAST, addr);
	spiflash_read_uint8();
}

uint8_t spiflash_read_uint8(void) {
	uint8_t val = 0;
	EXI_Imm(EXI_CHANNEL_0, &val, 1, EXI_READ, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	return val;
}


uint16_t spiflash_read_uint16(void) {
	uint16_t val = 0;
	EXI_Imm(EXI_CHANNEL_0, &val, 2, EXI_READ, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	return val;
}


uint32_t spiflash_read_uint32(void) {
	uint32_t val = 0;
	EXI_Imm(EXI_CHANNEL_0, &val, 4, EXI_READ, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	return val;
}


uint16_t spiflash_read_uint16_le(void) {
	return spiflash_read_uint8() | (uint16_t)spiflash_read_uint8() << 8;
}


uint32_t spiflash_read_uint32_le(void) {
	return spiflash_read_uint16_le() | (uint32_t)spiflash_read_uint16_le() << 16;
}


void spiflash_write_uint16(uint16_t val) {
	EXI_Imm(EXI_CHANNEL_0, &val, 2, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
}


void spiflash_write_uint32(uint32_t val) {
	EXI_Imm(EXI_CHANNEL_0, &val, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
}


void spiflash_write_uint16_le(uint16_t val) {
	spiflash_write_uint8(val);
	spiflash_write_uint8(val >> 8);
}


void spiflash_write_uint32_le(uint32_t val) {
	spiflash_write_uint16_le(val);
	spiflash_write_uint16_le(val >> 16);
}


void spiflash_erase4k(uint32_t addr) {
	spiflash_write_enable();
	spiflash_cmd_addr_start(W25Q80BV_CMD_ERASE_4K, addr);
}


void spiflash_erase32k(uint32_t addr) {
	spiflash_write_enable();
	spiflash_cmd_addr_start(W25Q80BV_CMD_ERASE_32K, addr);
}


void spiflash_erase64k(uint32_t addr) {
	spiflash_write_enable();
	spiflash_cmd_addr_start(W25Q80BV_CMD_ERASE_64K, addr);
}


void spiflash_chip_erase(void) {
	spiflash_write_enable();
	spiflash_write(W25Q80BV_CMD_CHIP_ERASE);
}


uint16_t spiflash_device_id(void) {
	uint16_t id;
	spiflash_cmd_addr_start(W25Q80BV_CMD_READ_MAN_DEV_ID, 0x0000);
	id = spiflash_read_uint16();
	return id;
}

uint32_t spiflash_jedec_id(void) {
	uint32_t id;
	uint8_t cmd = W25Q80BV_CMD_READ_JEDEC_ID;
	EXI_Imm(EXI_CHANNEL_0, &cmd, 1, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	id = spiflash_read_uint32() >> 8;
	return id;
}

uint64_t spiflash_unique_id(void) {
	uint64_t id;
	spiflash_cmd_addr_start(W25Q80BV_CMD_READ_UNIQUE_ID, 0x0000);
	spiflash_read();
	id = (uint64_t)spiflash_read_uint32() << 32;
	id |= spiflash_read_uint32();
	return id;
}
