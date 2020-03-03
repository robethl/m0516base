/******************************************************************************
 * @file     app_fmc.c
 * @brief   the interface of fmc emmbed in m0516 source file
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12
*****************************************************************************/
#include <stdio.h>
#include "typedef.h"
#include "M051Series.h"
#include "device.h"
#include "app_fmc.h"

/***********************************变量定义区*******************************************************/
#if FMC_DEBUG
#define FMC_PRINTF 	printf 
#else
#define FMC_PRINTF( ...)
#endif

static Fmc_t gFmc;

/***********************************接口定义区*******************************************************/
static U32 FMCGetUniqueId(U32 addr)
{
	U32 uId = 0;
	SYS_UnlockReg();
	FMC_ENABLE_ISP();
	
	uId = FMC_ReadUID(addr);

	FMC_DISABLE_ISP();
	SYS_LockReg();

	return uId;
}

static BOOLEAN FMCDataErasePage(U32 addr)
{
	if(addr<DATAFLASH_BASE_ADDR || addr>=DATAFLASH_END_ADDR){
		FMC_PRINTF("%s FMCDataErasePage, address isn't in data flash region\n", FMC_DEBUG_HEARSTR);
		return FALSE;
	}

	if(addr%DATAFLASH_PAGE_LEN){
		FMC_PRINTF("%s FMCDataErasePage, address isn't page alignment\n", FMC_DEBUG_HEARSTR);
		return FALSE;
	}
	
	SYS_UnlockReg();
	FMC_ENABLE_ISP();
	FMC_Erase(addr);
   	FMC_DISABLE_ISP();
	SYS_LockReg();

	return TRUE;
}

static VOID FMCDataWrite(U32 addr, pU32 pBuf, U32 len)
{
	if(!FMCDataErasePage(addr)){
		return;
	}
		
	if(addr<DATAFLASH_BASE_ADDR || addr>=DATAFLASH_END_ADDR){
		FMC_PRINTF("%s FMCDataWrite, address isn't in data flash region\n", FMC_DEBUG_HEARSTR);
		return;
	}
	
	if(addr%4){
		FMC_PRINTF("%s FMCDataWrite, M0516 only support word program, address isn't word alignment\n", FMC_DEBUG_HEARSTR);
		return;
	}
	
   	SYS_UnlockReg();
	FMC_ENABLE_ISP();
	
	while(len--)
	{
		FMC_Write(addr, *pBuf);
		addr += 4;
		pBuf ++;
	}
	
   	FMC_DISABLE_ISP();
	SYS_LockReg();
}

static VOID FMCDataRead(U32 addr, pU32 pBuf, U32 len)
{
	if(addr<DATAFLASH_BASE_ADDR || addr>=DATAFLASH_END_ADDR){
		FMC_PRINTF("%s FMCDataRead, address isn't in data flash region\n", FMC_DEBUG_HEARSTR);
		return;
	}
	
	if(addr%4){
		FMC_PRINTF("%s FMCDataRead, M0516 only support word program, address isn't word alignment\n", FMC_DEBUG_HEARSTR);
		return;
	}
	
   	SYS_UnlockReg();
	FMC_ENABLE_ISP();
	
	while(len--)
	{
		*pBuf = FMC_Read(addr);
		addr += 4;
		pBuf ++;
	}
	
   	FMC_DISABLE_ISP();
	SYS_LockReg();
}

static VOID SystemParamSave(VOID *para)
{
	FMCDataWrite(DATAFLASH_BASE_ADDR, (U32*)para, sizeof(SysParam_t)/sizeof(U32));
}

static VOID SystemParamLoad(VOID *para)
{
	FMCDataRead(DATAFLASH_BASE_ADDR, (U32*)para, sizeof(SysParam_t)/sizeof(U32));
}

/*
* brief: FMC 初始化
*/
VOID InitFmc(VOID)
{	
	gFmc.getUniqueId = FMCGetUniqueId;
	gFmc.systemParamSave = SystemParamSave;
	gFmc.systemParamLoad = SystemParamLoad;
	gpDev->fmc= &gFmc;
}

