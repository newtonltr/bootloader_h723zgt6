/**
  ******************************************************************************
  * @file    internal_flash.c
  * @brief   Flash操作相关函数实现
  ******************************************************************************
  */

/* 包含头文件 */
#include "internal_flash.h"

/* 私有函数声明 */
static uint32_t Internal_Flash_WaitForLastOperation(uint32_t Timeout);
static uint32_t Internal_Flash_Unlock(void);
static uint32_t Internal_Flash_Lock(void);

/**
  * @brief  按扇区擦除Flash
  * @param  StartSector: 起始扇区编号 (0-7)
  * @param  SectorCount: 要擦除的连续扇区数量
  * @retval 操作状态
  */
 uint32_t Internal_Flash_EraseSector(uint32_t StartSector, uint32_t SectorCount)
 {
	 uint32_t status = INTERNAL_FLASH_OK;
	 uint32_t currentSector;
	 uint32_t endSector;
	 
	 /* 检查参数 */
	 if (StartSector >= INTERNAL_FLASH_SECTOR_MAX)
	 {
		 return INTERNAL_FLASH_ERROR;
	 }
	 
	 /* 计算结束扇区 */
	 endSector = StartSector + SectorCount - 1;
	 
	 /* 检查结束扇区是否有效 */
	 if (endSector >= INTERNAL_FLASH_SECTOR_MAX)
	 {
		 return INTERNAL_FLASH_ERROR;
	 }
	 
	 /* 解锁Flash */
	 status = Internal_Flash_Unlock();
	 if (status != INTERNAL_FLASH_OK)
	 {
		 return status;
	 }
	 
	 /* 等待上一次操作完成 */
	 status = Internal_Flash_WaitForLastOperation(HAL_FLASH_TIMEOUT_VALUE);
	 if (status != INTERNAL_FLASH_OK)
	 {
		 /* 锁定Flash */
		 Internal_Flash_Lock();
		 return status;
	 }
	 
	 /* 逐个擦除扇区 */
	 for (currentSector = StartSector; currentSector <= endSector; currentSector++)
	 {
		 /* 擦除当前扇区 */
		 FLASH_Erase_Sector(currentSector, FLASH_BANK_1, FLASH_VOLTAGE_RANGE_3);
		 
		 /* 等待擦除操作完成 */
		 status = Internal_Flash_WaitForLastOperation(HAL_FLASH_TIMEOUT_VALUE);
		 if (status != INTERNAL_FLASH_OK)
		 {
			 break;
		 }
	 }
	 
	 /* 锁定Flash */
	 Internal_Flash_Lock();
	 
	 return status;
}

/**
  * @brief  写入数据到Flash
  * @param  Address: 写入的起始地址 (必须4字节对齐)
  * @param  Data: 要写入的数据指针 (8位)
  * @param  Length: 要写入的字节数
  * @retval 操作状态
  */
uint32_t Internal_Flash_Write(uint32_t Address, uint8_t *Data, uint32_t Length)
{
	uint32_t status = INTERNAL_FLASH_OK;
	uint32_t index = 0;
	uint32_t row_index = 0;
	uint32_t *dest_addr = (uint32_t *)Address;
	uint32_t flash_word[8]; // Flash字为8个32位字
	uint32_t endAddress = Address + Length; // 计算结束地址
	uint32_t wordCount = (Length + 3) / 4; // 计算32位字的数量（向上取整）
	uint32_t temp;
	uint8_t *bytePtr;
	
	/* 检查地址是否4字节对齐 */
	if ((Address & 0x3) != 0)
	{
		return INTERNAL_FLASH_ALIGN_ERROR;
	}
	
	/* 解锁Flash */
	status = Internal_Flash_Unlock();
	if (status != INTERNAL_FLASH_OK)
	{
		return status;
	}
	
	/* 等待上一次操作完成 */
	status = Internal_Flash_WaitForLastOperation(HAL_FLASH_TIMEOUT_VALUE);
	if (status != INTERNAL_FLASH_OK)
	{
		/* 锁定Flash */
		Internal_Flash_Lock();
		return status;
	}
	
	/* STM32H7的Flash编程以256位(32字节)为单位，即8个32位字 */
	while (index < Length)
	{
		/* 准备一个Flash字(8个32位字) */
		row_index = 0;
		while ((row_index < 8) && (index + 3 < Length))
		{
			/* 从字节数组中构建32位字 */
			temp = ((uint32_t)Data[index]) | 
				   ((uint32_t)Data[index + 1] << 8) | 
				   ((uint32_t)Data[index + 2] << 16) | 
				   ((uint32_t)Data[index + 3] << 24);
			
			flash_word[row_index] = temp;
			row_index++;
			index += 4;
		}
		
		/* 处理剩余不足4字节的数据 */
		if (index < Length && row_index < 8)
		{
			temp = 0xFFFFFFFF;
			bytePtr = (uint8_t *)&temp;
			
			for (uint32_t i = 0; i < 4 && index < Length; i++, index++)
			{
				bytePtr[i] = Data[index];
			}
			
			flash_word[row_index] = temp;
			row_index++;
		}
		
		/* 如果不足8个字，用0xFF填充 */
		while (row_index < 8)
		{
			flash_word[row_index] = 0xFFFFFFFF;
			row_index++;
		}
		
		/* 编程一个Flash字 */
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, (uint32_t)dest_addr, (uint32_t)flash_word) != HAL_OK)
		{
			status = INTERNAL_FLASH_ERROR;
			break;
		}
		
		/* 等待编程操作完成 */
		status = Internal_Flash_WaitForLastOperation(HAL_FLASH_TIMEOUT_VALUE);
		if (status != INTERNAL_FLASH_OK)
		{
			break;
		}
		
		/* 更新目标地址 */
		dest_addr += 8;
	}
	
	/* 锁定Flash */
	Internal_Flash_Lock();
	
	return status;
}

/**
  * @brief  从Flash读取数据
  * @param  Address: 读取的起始地址
  * @param  Buffer: 存储读取数据的缓冲区指针 (8位)
  * @param  Length: 要读取的字节数
  * @retval 操作状态
  */
uint32_t Internal_Flash_Read(uint32_t Address, uint8_t *Buffer, uint32_t Length)
{
	uint32_t i;
	uint8_t *src_addr = (uint8_t *)Address;
	uint32_t endAddress = Address + Length; // 计算结束地址
		
	/* 直接读取数据 */
	for (i = 0; i < Length; i++)
	{
		Buffer[i] = src_addr[i];
	}
	
	return INTERNAL_FLASH_OK;
}

/**
  * @brief  等待Flash操作完成
  * @param  Timeout: 超时时间
  * @retval 操作状态
  */
static uint32_t Internal_Flash_WaitForLastOperation(uint32_t Timeout)
{
	HAL_StatusTypeDef hal_status;
	
	/* 等待Flash操作完成或超时 */
	hal_status = FLASH_WaitForLastOperation(Timeout, FLASH_BANK_1);
	
	if (hal_status != HAL_OK)
	{
		return INTERNAL_FLASH_TIMEOUT;
	}
	
	return INTERNAL_FLASH_OK;
}

/**
  * @brief  解锁Flash
  * @param  无
  * @retval 操作状态
  */
static uint32_t Internal_Flash_Unlock(void)
{
	HAL_StatusTypeDef status = HAL_OK;
	
	status = HAL_FLASH_Unlock();
	
	if (status != HAL_OK)
	{
		return INTERNAL_FLASH_ERROR;
	}
	
	return INTERNAL_FLASH_OK;
}

/**
  * @brief  锁定Flash
  * @param  无
  * @retval 操作状态
  */
static uint32_t Internal_Flash_Lock(void)
{
	HAL_StatusTypeDef status = HAL_OK;
	
	status = HAL_FLASH_Lock();
	
	if (status != HAL_OK)
	{
		return INTERNAL_FLASH_ERROR;
	}
	
	return INTERNAL_FLASH_OK;
}

