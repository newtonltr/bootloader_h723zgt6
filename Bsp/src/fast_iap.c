#include "fast_iap.h"
#include "internal_flash.h"
#include <string.h>  // 添加string.h用于memcpy函数

struct fast_iap_t fast_iap;
uint8_t fi_buffer[fi_buffer_size] = {0};
static uint16_t ymodem_crc(uint8_t * buf, uint16_t len);

uint8_t fi_recv(struct fast_iap_t *this, uint8_t *data, uint32_t len)
{
	static uint32_t received_len = 0;  // 已接收的数据长度
	struct fast_iap_protocol_t *frame;
	uint16_t calc_crc;
	
	// 检查参数有效性
	if (this == NULL || data == NULL || len == 0) {
		return FI_ERROR;
	}
	
	// 检查缓冲区是否足够
	if (received_len + len > this->buffer_size) {
		received_len = 0;  // 重置接收长度
		return FI_ERROR;
	}
	
	// 复制新数据到缓冲区
	memcpy(this->buffer + received_len, data, len);
	received_len += len;
	
	// 检查是否收到完整帧
	if (received_len >= sizeof(struct fast_iap_protocol_t)) {
		frame = (struct fast_iap_protocol_t *)this->buffer;
		
		// 验证CRC
		calc_crc = ymodem_crc(frame->data, frame->len);
		if (calc_crc != frame->crc) {
			received_len = 0;
			return FI_ERROR;
		}
		
		// 检查帧序号
		if (frame->index != this->index + 1) {
			received_len = 0;
			return FI_ERROR;
		}
		
		// 写入flash
		if (this->flash_write(this->firm_current_addr, frame->data, frame->len) != INTERNAL_FLASH_OK) {
			received_len = 0;
			return FI_ERROR;
		}
		
		// 更新状态
		this->firm_current_addr += frame->len;
		this->index = frame->index;
		received_len = 0;
		
		// 检查是否接收完成
		if (frame->index == frame->total_frame) {
			return FI_RECV_CPLT;
		}
		
		return FI_SUCCESS;
	}
	
	return FI_RECVING;
}

uint8_t fi_write(struct fast_iap_t *this)
{
	uint8_t status = FI_ERROR;
	uint32_t firmware_size;
	uint32_t read_addr, write_addr;
	uint32_t remaining_size, current_size;
	uint32_t flash_result;
	uint8_t temp_buffer[512];  // 临时缓冲区，用于存储读取的数据
	
	// 检查参数有效性
	if (this == NULL) {
		return FI_ERROR;
	}
	
	// 计算固件实际大小
	firmware_size = this->firm_current_addr - this->firm_start_addr;
	
	// 检查固件大小是否在合理范围内
	if (firmware_size == 0 || firmware_size > APP_SIZE) {
		return FI_ERROR;
	}
	
	// 擦除app区域的扇区
	flash_result = this->flash_erase(APP_SECTOR_START, APP_SECTOR_COUNT);
	if (flash_result != INTERNAL_FLASH_OK) {
		return FI_ERROR;
	}
	
	// 设置状态为正在写入
	status = FI_WRITING;
	
	// 初始化读写地址
	read_addr = this->firm_start_addr;
	write_addr = this->app_start_addr;
	remaining_size = firmware_size;
	
	// 循环读取firmware区域数据并写入app区域
	while (remaining_size > 0) {
		// 确定当前批次要处理的大小
		current_size = (remaining_size > sizeof(temp_buffer)) ? sizeof(temp_buffer) : remaining_size;
		
		// 从firmware区域读取数据
		flash_result = this->flash_read(read_addr, temp_buffer, current_size);
		if (flash_result != INTERNAL_FLASH_OK) {
			return FI_ERROR;
		}
		
		// 将数据写入app区域
		flash_result = this->flash_write(write_addr, temp_buffer, current_size);
		if (flash_result != INTERNAL_FLASH_OK) {
			return FI_ERROR;
		}
		
		// 更新地址和剩余大小
		read_addr += current_size;
		write_addr += current_size;
		remaining_size -= current_size;
	}
	
	// 可选：验证写入的数据
	read_addr = this->firm_start_addr;
	write_addr = this->app_start_addr;
	remaining_size = firmware_size;
	
	while (remaining_size > 0) {
		uint8_t src_data[512], dst_data[512];
		current_size = (remaining_size > sizeof(src_data)) ? sizeof(src_data) : remaining_size;
		
		// 读取源数据和目标数据
		this->flash_read(read_addr, src_data, current_size);
		this->flash_read(write_addr, dst_data, current_size);
		
		// 比较数据
		if (memcmp(src_data, dst_data, current_size) != 0) {
			return FI_ERROR;
		}
		
		// 更新地址和剩余大小
		read_addr += current_size;
		write_addr += current_size;
		remaining_size -= current_size;
	}
	
	// 设置状态为写入完成
	status = FI_WRITE_CPLT;
	
	return status;
}

uint8_t fi_init(struct fast_iap_t *this)
{
	// 设置固件存放区域起始地址 (firmware: 0x08040000-0x0809ffff)
	this->firm_start_addr = 0x08040000;
	this->firm_current_addr = this->firm_start_addr;
	// 设置app起始地址 (app: 0x080a0000-0x08100000)
	this->app_start_addr = 0x080A0000;
	
	// 初始化帧序号
	this->index = 0;
	
	// 分配数据接收缓冲区，大小为struct fast_iap_protocol_t
	this->buffer_size = fi_buffer_size;
	this->buffer = fi_buffer;

	
	// 设置flash操作接口函数
	this->flash_erase = Internal_Flash_EraseSector;
	this->flash_write = Internal_Flash_Write;
	this->flash_read = Internal_Flash_Read;
	
	// 初始时未设置固件接收和写入函数，交由具体应用实现
	this->recv = fi_recv;
	this->write = fi_write;
	
	return 0; // 初始化成功
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
