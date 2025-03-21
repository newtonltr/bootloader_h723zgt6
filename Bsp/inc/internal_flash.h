/**
  ******************************************************************************
  * @file    internal_flash.h
  * @brief   Flash操作相关函数头文件
  ******************************************************************************
  */

#ifndef __INTERNAL_FLASH_H
#define __INTERNAL_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* 包含头文件 */
#include "main.h"
#include "stm32_hal_legacy.h"

/* 错误代码定义 */
#define INTERNAL_FLASH_OK           0x00U  /* 操作成功 */
#define INTERNAL_FLASH_ERROR        0x01U  /* 操作失败 */
#define INTERNAL_FLASH_BUSY         0x02U  /* Flash忙 */
#define INTERNAL_FLASH_TIMEOUT      0x03U  /* 操作超时 */
#define INTERNAL_FLASH_INVALID_ADDR 0x04U  /* 无效地址 */
#define INTERNAL_FLASH_ALIGN_ERROR  0x05U  /* 地址未对齐 */

/* Flash超时值定义 */
#define FLASH_TIMEOUT_VALUE    50000U /* Flash操作超时时间 */

/* Sector 定义 */
/* Flash扇区定义 - 每个扇区128KB */
#define FLASH_SECTOR0_BASE    0x08000000  /* 扇区0起始地址: 0x0800 0000 - 0x0801 FFFF */
#define FLASH_SECTOR1_BASE    0x08020000  /* 扇区1起始地址: 0x0802 0000 - 0x0803 FFFF */
#define FLASH_SECTOR2_BASE    0x08040000  /* 扇区2起始地址: 0x0804 0000 - 0x0805 FFFF */
#define FLASH_SECTOR3_BASE    0x08060000  /* 扇区3起始地址: 0x0806 0000 - 0x0807 FFFF */
#define FLASH_SECTOR4_BASE    0x08080000  /* 扇区4起始地址: 0x0808 0000 - 0x0809 FFFF */
#define FLASH_SECTOR5_BASE    0x080A0000  /* 扇区5起始地址: 0x080A 0000 - 0x080B FFFF */
#define FLASH_SECTOR6_BASE    0x080C0000  /* 扇区6起始地址: 0x080C 0000 - 0x080D FFFF */
#define FLASH_SECTOR7_BASE    0x080E0000  /* 扇区7起始地址: 0x080E 0000 - 0x080F FFFF */


/* 扇区枚举定义 - 避免与HAL库冲突，使用不同的命名 */
typedef enum {
	INTERNAL_FLASH_SECTOR_0 = 0,  /* 扇区0 */
	INTERNAL_FLASH_SECTOR_1,      /* 扇区1 */
	INTERNAL_FLASH_SECTOR_2,      /* 扇区2 */
	INTERNAL_FLASH_SECTOR_3,      /* 扇区3 */
	INTERNAL_FLASH_SECTOR_4,      /* 扇区4 */
	INTERNAL_FLASH_SECTOR_5,      /* 扇区5 */
	INTERNAL_FLASH_SECTOR_6,      /* 扇区6 */
	INTERNAL_FLASH_SECTOR_7,      /* 扇区7 */
	INTERNAL_FLASH_SECTOR_MAX
} FLASH_SectorTypeDef;

/* ADDRESS 定义 */
/* Bootloader区域细分 */
#define BOOTLOADER_CODE_BASE      FLASH_SECTOR0_BASE           /* Bootloader代码区起始地址 */
#define BOOTLOADER_CODE_END       (FLASH_SECTOR1_BASE+FLASH_SECTOR_SIZE-1)  /* Bootloader代码区结束地址 */
#define BOOTLOADER_CODE_SIZE      (BOOTLOADER_CODE_END-BOOTLOADER_CODE_BASE+1)  /* Bootloader代码区大小: 256KB */

#define BOOTLOADER_FIRMWARE_BASE  FLASH_SECTOR2_BASE           /* Bootloader固件区起始地址 */
#define BOOTLOADER_FIRMWARE_END   (FLASH_SECTOR4_BASE+FLASH_SECTOR_SIZE-1)  /* Bootloader固件区结束地址 */
#define BOOTLOADER_FIRMWARE_SIZE  (BOOTLOADER_FIRMWARE_END-BOOTLOADER_FIRMWARE_BASE+1)  /* Bootloader固件区大小: 384KB */

/* 整体区域定义 */
#define BOOTLOADER_BASE           FLASH_SECTOR0_BASE           /* Bootloader起始地址 */
#define BOOTLOADER_END            (FLASH_SECTOR4_BASE+FLASH_SECTOR_SIZE-1)  /* Bootloader结束地址 */
#define BOOTLOADER_SIZE           (BOOTLOADER_END-BOOTLOADER_BASE+1)  /* Bootloader总大小: 640KB */

#define APP_BASE                  FLASH_SECTOR5_BASE           /* 应用程序起始地址 */
#define APP_END                   (FLASH_SECTOR7_BASE+FLASH_SECTOR_SIZE-1)  /* 应用程序结束地址 */
#define APP_SIZE                  (APP_END-APP_BASE+1)         /* 应用程序大小: 384KB */

/* sector 定义 */
#define BOOTLOADER_CODE_SECTOR_START  INTERNAL_FLASH_SECTOR_0  /* Bootloader代码区起始扇区 */
#define BOOTLOADER_CODE_SECTOR_END    INTERNAL_FLASH_SECTOR_1  /* Bootloader代码区结束扇区 */
#define BOOTLOADER_CODE_SECTOR_COUNT  (BOOTLOADER_CODE_SECTOR_END-BOOTLOADER_CODE_SECTOR_START+1)  /* Bootloader代码区扇区数量 */

#define BOOTLOADER_FIRMWARE_SECTOR_START  INTERNAL_FLASH_SECTOR_2  /* Bootloader固件区起始扇区 */
#define BOOTLOADER_FIRMWARE_SECTOR_END    INTERNAL_FLASH_SECTOR_4  /* Bootloader固件区结束扇区 */
#define BOOTLOADER_FIRMWARE_SECTOR_COUNT  (BOOTLOADER_FIRMWARE_SECTOR_END-BOOTLOADER_FIRMWARE_SECTOR_START+1)  /* Bootloader固件区扇区数量 */

#define APP_SECTOR_START           INTERNAL_FLASH_SECTOR_5  /* 应用程序起始扇区 */
#define APP_SECTOR_END             INTERNAL_FLASH_SECTOR_7  /* 应用程序结束扇区 */
#define APP_SECTOR_COUNT           (APP_SECTOR_END-APP_SECTOR_START+1)  /* 应用程序扇区数量 */

/**
  * @brief  按扇区擦除Flash
  * @param  StartSector: 起始扇区编号 (0-7)
  * @param  SectorCount: 要擦除的连续扇区数量
  * @retval 操作状态
  */
uint32_t Internal_Flash_EraseSector(uint32_t StartSector, uint32_t SectorCount);

/**
  * @brief  写入数据到Flash
  * @param  Address: 写入的起始地址 (必须4字节对齐)
  * @param  Data: 要写入的数据指针 (8位)
  * @param  Length: 要写入的字节数
  * @retval 操作状态
  */
uint32_t Internal_Flash_Write(uint32_t Address, uint8_t *Data, uint32_t Length);

/**
  * @brief  从Flash读取数据
  * @param  Address: 读取的起始地址
  * @param  Buffer: 存储读取数据的缓冲区指针 (8位)
  * @param  Length: 要读取的字节数
  * @retval 操作状态
  */
uint32_t Internal_Flash_Read(uint32_t Address, uint8_t *Buffer, uint32_t Length);

#ifdef __cplusplus
}
#endif

#endif /* __INTERNAL_FLASH_H */