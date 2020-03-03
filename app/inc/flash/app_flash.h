/******************************************************************************
 * @file     app_cpld_flash.h
 * @brief    This is the header file of app_cpld_flash.c
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12
*****************************************************************************/
#ifndef 	_APP_CPLD_FLASH_H_
#define  	_APP_CPLD_FLASH_H_

#include "typedef.h"

#define FLASH_SEGMENT_LEN					(8UL)
#define FLASH_PAGE_LEN 							(256UL)
#define FLASH_SECTOR_LEN   					(1024UL * 4)
#define FLASH_BLOCK32K_LEN    				(1024UL * 32)
#define FLASH_BLOCK64K_LEN    				(1024UL * 64)

//CPLD
#define CPLD_CMD_HIGH_CFGFPGA		(0xa2)  //与FLASH的读指令相关, 对应FLASH的high read, 0xa2: fast config one fpga
#define CPLD_CMD_CFGFPGA  				(0xaa)  //与FLASH的读指令相关, 对应FLASH的read       0xaa: config one fpga
#define CPLD_CMD_READ   					(0x47)
#define CPLD_CMD_RESET  					(0xb5)
#define CPLD_CMD_SETGS2971_CH 	(0xc7)
#define CPLD_CMD_CTRLFLASH 			(0xd8)
#define CPLD_CMD_SETLED      				(0xda)

//falsh (sst25vf064c)
#define SST_SEGMENT_LEN					(8UL)	/*自定义8byte段编写，针对非页边界地址为开始位置的flash写采取段编写方式，但开始位置必须为段边界地址，即8byte对齐*/
#define SST_PAGE_LEN 							(256UL)
#define SST_SECTOR_LEN   					(1024UL * 4)
#define SST_BLOCK32K_LEN    				(1024UL * 32)
#define SST_BLOCK64K_LEN    				(1024UL * 64)

/*SST25vf040的命令定义*/
#define SST_CMD_EWSR							(0x50)   
#define SST_CMD_WRSR         				(0x01)
#define SST_CMD_PAGEPROG      			(0x02)
#define SST_CMD_READ        					(0x03)
#define SST_CMD_WRDI         				(0x04)
#define SST_CMD_RDSR       					(0x05)
#define SST_CMD_WREN          				(0x06)
#define SST_CMD_FASTREAD     			(0x0B)
#define SST_CMD_ERASESECTOR 			(0x20)
#define SST_CMD_ERASEBLOCK32K 	(0x52)
#define SST_CMD_ERASEBLOCK64K		(0xD8)
#define SST_CMD_ERASECHIP         		(0x60)
#define SST_CMD_RDJEDEC					(0x9f)

#define SST_WAITFREE_TIMER 			(400) //等待SST040标志次数
#define SST_SRBP_DIS							(0x00)	//状态码，取消写保护
#define SST_SRBP_EN								(0x3c)	//状态码，写保护

#define FLASH_JEDEC_W25Q64FVSIG		(0xEF4017ul)
#define FLASH_JEDEC_SST25VF064C		(0xBF254Bul)

#define FLASH_DEBUG		(1)		/*CPLD flash模块打印开关*/
#define FLASH_DEBUG_HEADSTR	"Flash:"

#endif

