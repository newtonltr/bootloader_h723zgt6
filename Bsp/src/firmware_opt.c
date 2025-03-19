#include "firmware_opt.h"

static uint8_t frame_check(struct firmware_opt_t *this, uint8_t *data, uint16_t crc, uint32_t len)
{

}

static uint8_t frame_recv(struct firmware_opt_t *this, uint8_t *data, uint32_t len)
{

}

static uint8_t firmware_write(struct firmware_opt_t *this)
{

}

uint8_t firmware_opt_init(struct firmware_opt_t *this)
{
	uint8_t status = 0;

	this->start_addr 	= BOOTLOADER_FIRMWARE_BASE;
	this->end_addr 		= BOOTLOADER_FIRMWARE_END;
	this->current_addr	= this->start_addr;
	this->crc_local		= 0;

	this->check = frame_check;
	this->recv 	= frame_recv;
	this->write = firmware_write;

	status = sector_erase(FLASH_OPT_START_SECTOR, FLASH_OPT_SECTOR_COUNT);

	return status;
}

