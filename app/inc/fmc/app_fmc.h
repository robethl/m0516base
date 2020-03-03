/******************************************************************************
 * @file     app_fmc.h
 * @brief    This is the header file of app_fmc.c
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/06/20
*****************************************************************************/
#ifndef _APP_FMC_H_
#define _APP_FMC_H_

#define MCU_UID_1ST_WORD_ADDR	(0x00000UL)
#define MCU_UID_2ND_WORD_ADDR	(0x00004UL)
#define MCU_UID_3RD_WORD_ADDR	(0x00008UL)

#define DATAFLASH_BASE_ADDR 		(0x0001F000UL)
#define DATAFLASH_END_ADDR		(0x0001FFFFUL)
#define DATAFLASH_PAGE_LEN		(512)

#pragma pack(push)
#pragma pack(4)

#define SYSPARAM_INIT_FLAG		(0xAAAAAAAA)
typedef struct
{
	U32 flag;	/*初始化标记0xAAAA*/
	/*system param */
}SysParam_t, *pSysParam_t;

#pragma pack(pop)

#define FMC_DEBUG		(1)
#define FMC_DEBUG_HEARSTR	"Fmc:"

#endif

