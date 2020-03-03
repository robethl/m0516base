#include "typedef.h"
#include "M051Series.h"
#include "device.h"
#include "app_led.h"

/***********************************变量定义区*******************************************************/
/*
* brief:
*	D1 - LED1 - work - P23
*	D2 - LED2 - auth - P24
*	D3 - LED3 - reserved - P40
*/
#define LED1					P40
#define LED2					P24
#define LED3					P23

static Led_t gLed;

/***********************************接口定义区*******************************************************/
static VOID SetWorkLed(U08 s)
{
#ifdef LED1
	if(LED_ON == (LED_STATUS_e)s) {
		LED1 = 0;
	} else {
		LED1 = 1;
	}
#endif
}

static VOID SetAuthLed(U08 s)
{
#ifdef LED2
	if(LED_ON == (LED_STATUS_e)s) {
		LED2 = 0;
	} else {
		LED2 = 1;
	}
#endif	
}

static VOID SetDeviceInitLed(U08 s)
{
#ifdef LED3
	if(LED_ON == (LED_STATUS_e)s) {
		LED3 = 0;
	} else {
		LED3 = 1;
	}
#endif	
}

static VOID WorkLedFlicker(VOID)
{
#ifdef LED1
	if(LED1){
		LED1 = 0;
	}else{
		LED1 = 1;
	}
#endif	
}

VOID InitLed(VOID)
{
	gLed.setWorkLed = SetWorkLed;
	gLed.setAuthLed = SetAuthLed;
	gLed.setDeviceInitLed = SetDeviceInitLed;
	gLed.workLedFlicker = WorkLedFlicker;

	gpDev->led = &gLed;
}

