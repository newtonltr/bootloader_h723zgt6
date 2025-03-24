#ifndef __FIRMWARE_OPT_H
#define __FIRMWARE_OPT_H

#include "main.h"
#include "internal_flash.h"

#define PROTOCOL_ACK_HEAD 0x1212

// 使用内部flash，如果以后添加外部flash，只需要修改这部分代码
#define sector_erase Internal_Flash_EraseSector
#define flash_write Internal_Flash_Write
#define flash_read Internal_Flash_Read

#define BOOT_STAT_IDLE		0
#define BOOT_STAT_SOCKET	1
#define BOOT_STAT_SERIAL	2

enum f_opt_status{
	FIRMWARE_OPT_SUCCESS = 0,
	FIRMWARE_OPT_RECV_CPLT,
	FIRMWARE_OPT_WRITE_CPLT,
	FIRMWARE_OPT_FAIL,
};

struct firmware_trans_protocol_t {
	uint32_t index;			// 本帧序号
	uint32_t total_frame;	// 总帧数
	uint32_t total_byte;	// 总字节数
	uint32_t len;			// 本帧数据段有效长度
	uint8_t data[1024];		// 本帧数据段，最多1024字节
	uint16_t crc;
};

#define IAP_PROTOCOL_BUFFER_SIZE sizeof(struct firmware_trans_protocol_t)
extern uint8_t iap_protocol_buffer[IAP_PROTOCOL_BUFFER_SIZE];
extern uint32_t gloabal_time_ms;
extern uint32_t global_boot_stat;

struct firmware_opt_t {
    uint32_t firm_start_addr;
	uint32_t firm_current_addr;
	uint32_t app_start_addr;

	uint32_t index;	// 帧序号
	uint8_t *buffer;
	uint32_t buffer_size;

	uint8_t (*recv)(struct firmware_opt_t *this, uint8_t *data, uint32_t len);			// 接收每帧数据并存入firmware区域
	uint8_t (*write)(struct firmware_opt_t *this);		// 将完整的bin文件从firmware区域写入app区域
};

uint8_t firmware_opt_init(struct firmware_opt_t *this);
void jump_to_app(void);


#endif
