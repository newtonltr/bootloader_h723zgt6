#ifndef __FIRMWARE_OPT_H
#define __FIRMWARE_OPT_H

#include "main.h"
#include "internal_flash.h"

// 使用内部flash，如果以后添加外部flash，只需要修改这部分代码
#define sector_erase Internal_Flash_EraseSector
#define flash_write Internal_Flash_Write
#define flash_read Internal_Flash_Read

struct firmware_trans_protocol_t {
	uint8_t head;
};


struct firmware_opt_t {
    uint32_t start_addr;
    uint32_t end_addr;
	uint32_t current_addr;
	uint16_t crc_local;

	struct firmware_trans_protocol_t *frame;

	uint8_t (*check)(struct firmware_opt_t *this, uint8_t *data, uint16_t crc, uint32_t len);			// 校验每帧数据
	uint8_t (*recv)(struct firmware_opt_t *this, uint8_t *data, uint32_t len);			// 接收每帧数据并存入firmware区域
	uint8_t (*write)(struct firmware_opt_t *this);		// 将完整的bin文件从firmware区域写入app区域
};

uint8_t firmware_opt_init(struct firmware_opt_t *this);

#endif
