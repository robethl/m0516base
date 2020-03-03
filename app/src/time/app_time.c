/******************************************************************************
 * @file     app_time.c
 * @brief   time api source file
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12	create
*****************************************************************************/
#include "typedef.h"
#include "M051Series.h"
#include "device.h"
#include "app_time.h"

/***********************************变量定义区*******************************************************/
static Time_t gTime;

/***********************************接口定义区*******************************************************/
/*
* brief: us  Delay time. The Max value is 2^24 / CPU Clock(MHz).
*/
static VOID SysDelaynUs(U32 us)
{
	CLK_SysTickDelay(us);
}

static VOID SysDelaynMs(U32 ms)
{
	while(ms--){
		CLK_SysTickDelay(1000);
	}
}

/*
* brief: time 初始化
*/
VOID InitTime(VOID)
{
	gTime.delaynUs = SysDelaynUs;
	gTime.delaynMs = SysDelaynMs;
	gpDev->time= &gTime;
}

