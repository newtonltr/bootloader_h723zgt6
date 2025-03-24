#include "firmware_opt.h"

static uint16_t ymodem_crc(uint8_t * buf, uint16_t len);
static uint8_t frame_check(struct firmware_opt_t *this, uint8_t *data, uint16_t crc, uint32_t len);
static uint8_t frame_recv(struct firmware_opt_t *this, uint8_t *data, uint32_t len);
static uint8_t firmware_write(struct firmware_opt_t *this);

uint8_t iap_protocol_buffer[IAP_PROTOCOL_BUFFER_SIZE];

uint32_t gloabal_time_ms = 0;
uint32_t global_boot_stat = BOOT_STAT_IDLE;

uint8_t firmware_opt_init(struct firmware_opt_t *this)
{
	uint8_t status = 0;

	this->firm_start_addr 	= BOOTLOADER_FIRMWARE_BASE;
	this->firm_current_addr	= this->firm_start_addr;
	this->app_start_addr	= APP_BASE;
	this->index			= 0;
	this->recv 			= frame_recv;
	this->write 		= firmware_write;

	this->buffer = iap_protocol_buffer;
	this->buffer_size = IAP_PROTOCOL_BUFFER_SIZE;

	memset(this->buffer, 0, this->buffer_size);

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
	status = flash_write(this->app_start_addr, (uint8_t *)this->firm_start_addr, bytes);
	if (status != INTERNAL_FLASH_OK) {
		status = FIRMWARE_OPT_FAIL;
		return status;	
	} else {
		status = FIRMWARE_OPT_WRITE_CPLT;
		return status;
	}


	return status;

}

void jump_to_app(void)
{
    typedef void (*pFunction)(void);
    uint32_t jump_address;
    pFunction jump_to_application;
    
    // 禁用所有中断
    __disable_irq();
	for (uint32_t i = 0; i < 8; i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;  // 禁用所有中断
		NVIC->ICPR[i] = 0xFFFFFFFF;  // 清除所有挂起中断
	}
    
    // 关闭所有外设
    HAL_RCC_DeInit();
    
    // 禁用SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    
    // 重新映射向量表
    SCB->VTOR = APP_BASE;
    __DSB();

    // 获取应用程序的复位处理程序地址
    // 应用程序的复位向量位于向量表的第二个条目(偏移量为4)
    jump_address = *(__IO uint32_t*)(APP_BASE + 4);
    jump_to_application = (pFunction)jump_address;
    
    // 初始化应用程序的栈指针(SP)
    // 栈指针位于向量表的第一个条目
    __set_MSP(*(__IO uint32_t*)APP_BASE);
	__DSB();

    // 清除流水线
    __ISB();
    
    // 跳转到应用程序
    jump_to_application();
    
    // 注意：此处代码永远不会执行到，因为已经跳转到应用程序
}
