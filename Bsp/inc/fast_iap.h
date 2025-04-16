#ifndef _FAST_IAP_H
#define _FAST_IAP_H

#include "main.h"


struct fast_iap_protocol_t {
	uint32_t index;			// 本帧序号
	uint32_t total_frame;	// 总帧数
	uint32_t total_byte;	// 总字节数
	uint32_t len;			// 本帧数据段有效长度
	uint8_t data[512];		// 本帧数据段，最多512字节
	uint16_t crc;
};

#define fi_buffer_size sizeof(struct fast_iap_protocol_t)

struct fast_iap_t  {
    uint32_t firm_start_addr; 	// 固件存放区域起始地址
	uint32_t firm_current_addr;	// 当前已写入的固件地址
	uint32_t app_start_addr;	// app起始地址

	uint32_t index;		// 帧序号
	uint8_t *buffer;	// 接收帧数据的缓冲区
	uint32_t buffer_size;

	// flash option port
	uint32_t (*flash_erase)(uint32_t start_addr, uint32_t sector_num);
	uint32_t (*flash_write)(uint32_t start_addr, uint8_t *data, uint32_t len);
	uint32_t (*flash_read)(uint32_t start_addr, uint8_t *data, uint32_t len);

	// firmware option port
	uint8_t (*recv)(struct fast_iap_t *this, uint8_t *data, uint32_t len);			// 接收每帧数据并存入firmware区域
	uint8_t (*write)(struct fast_iap_t *this);		// 将完整的bin文件从firmware区域写入app区域
};

enum BOOT_STATUS {
	BOOT_STAT_IDLE = 0,
	BOOT_STAT_SOCKET,
	BOOT_STAT_SERIAL,
};

enum FI_STATUS {
	FI_SUCCESS = 0,
	FI_RECVING,
	FI_RECV_CPLT,
	FI_WRITING,
	FI_WRITE_CPLT,
	FI_ERROR,
};

extern struct fast_iap_t fast_iap;

uint8_t fi_init(struct fast_iap_t *this);

#endif 