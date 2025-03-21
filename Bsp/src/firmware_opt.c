#include "firmware_opt.h"

static uint16_t ymodem_crc(uint8_t * buf, uint16_t len);
static uint8_t frame_check(struct firmware_opt_t *this, uint8_t *data, uint16_t crc, uint32_t len);
static uint8_t frame_recv(struct firmware_opt_t *this, uint8_t *data, uint32_t len);
static uint8_t firmware_write(struct firmware_opt_t *this);

uint8_t iap_protocol_buffer[IAP_PROTOCOL_BUFFER_SIZE];

uint8_t firmware_opt_init(struct firmware_opt_t *this)
{
	uint8_t status = 0;

	this->firm_start_addr 	= BOOTLOADER_FIRMWARE_BASE;
	this->firm_current_addr	= this->firm_start_addr;
	this->app_start_addr	= APP_BASE;
	this->index			= 0;
	this->recv 			= frame_recv;
	this->write 		= firmware_write;

	status = sector_erase(BOOTLOADER_FIRMWARE_SECTOR_START, BOOTLOADER_FIRMWARE_SECTOR_COUNT);

	return status;
}

//CRC16 
static uint16_t ymodem_crc(uint8_t * buf, uint16_t len)
{
    uint16_t chsum;
    uint16_t stat;
    uint16_t i;
    uint8_t * in_ptr;
   
    //指向要计算CRC的缓冲区开头
    in_ptr = buf;
    chsum = 0;
    for (stat = len ; stat > 0; stat--) //len是所要计算的长度
    {
        chsum = chsum^(uint16_t)(*in_ptr++) << 8;
        for (i=8; i!=0; i--) {
            if (chsum & 0x8000){
                chsum = chsum << 1 ^ 0x1021;
            } else {
                chsum = chsum << 1;
            }
        }
    }
    return chsum;
}

static uint8_t frame_check(struct firmware_opt_t *this, uint8_t *data, uint16_t crc, uint32_t len)
{
	uint8_t status = 0;
	uint16_t crc_local = ymodem_crc(data, len);

	if (crc_local == crc) {
		status = FIRMWARE_OPT_SUCCESS;
	} else {
		status = FIRMWARE_OPT_FAIL;
	}

	return status;
}

static uint8_t frame_recv(struct firmware_opt_t *this, uint8_t *data, uint32_t len)
{
	uint8_t status = 0;
	struct firmware_trans_protocol_t *f = (struct firmware_trans_protocol_t *)data;

	status = frame_check(this, data, f->crc, f->len);
	if (status == FIRMWARE_OPT_FAIL) {
		return status;
	}
	if (f->index != this->index) {
		status = FIRMWARE_OPT_FAIL;
		return status;
	}
	status = flash_write(this->firm_current_addr, f->data, f->len);
	if (status == INTERNAL_FLASH_ERROR) {
		status = FIRMWARE_OPT_FAIL;
		return status;	
	}
	this->index++;
	this->firm_current_addr += f->len;

	// 判断是否接收完成
	if (this->index == f->total_frame && this->firm_current_addr == this->firm_start_addr + f->total_byte) {
		status = FIRMWARE_OPT_RECV_CPLT;
	}

	return status;
}

static uint8_t firmware_write(struct firmware_opt_t *this)
{
	uint8_t status = 0;
	uint32_t bytes = 0;

	bytes = this->firm_current_addr - this->firm_start_addr;
	status = sector_erase(APP_SECTOR_START, APP_SECTOR_COUNT);
	if (status == FIRMWARE_OPT_FAIL) {
		return status;
	}
	status = flash_write(APP_BASE, (uint8_t *)this->app_start_addr, bytes);
	if (status != INTERNAL_FLASH_OK) {
		status = FIRMWARE_OPT_FAIL;
		return status;	
	} else {
		status = FIRMWARE_OPT_WRITE_CPLT;
		return status;
	}


	return status;

}


