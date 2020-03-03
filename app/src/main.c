/******************************************************************************
 * @file     main.c
 * @brief   M0516 deal with i2c command, convert cmd to uart or spi protocol
 * @history
 *	=version=	=author=		=date= 
 *	v1.00a		huanglv		16/01/06  create base code for M0516
 *	v1.01a		huanglv		16/01/15  修复初始化bug; 增加模拟spi;
*	v1.02a		huanglv		16/01/18  修改现行框架下tftp传送时需求的I2C写寄存器缓存大小;
*****************************************************************************/
#include <stdio.h>
#include "typedef.h"
#include "M051Series.h"
#include "extern.h"
#include "device.h"
#include "app_fmc.h"
#include "app_i2c.h"
#include "app_led.h"

/***********************************变量定义区*******************************************************/
#define MAIN_DEBUG		(1)
#if MAIN_DEBUG
#define MAIN_LOG			printf
#else
#define MAIN_LOG( ...)
#endif

#define PLLCON_SETTING	CLK_PLLCON_50MHz_HXT
#define PLL_CLOCK           	__HSI

#define PROJECT_NAME		"M0516 baseline"
#define EMAIL				"hl@wanfacatv.com"
#define VERSION_INFO_HW	('a') 
#define VERSION_INFO_SW_P	(1L) 	//主版本号(0~3)
#define VERSION_INFO_SW_S	(3L)		//从版本号(0~63)
#define VERSION_INFO_SW		((VERSION_INFO_SW_P << 6) | VERSION_INFO_SW_S)
					
#define KHz 	(1000L)
#define MHz 	(1000L*KHz)

#define MONITOR_TSOIP	(1)	//监控TSoIP状态
#define DEVICE_ID		(0x0000)

static SysCfgParam_t gCfgParam;
static M0516_Device_t gDevice;
pM0516_Device_t gpDev = &gDevice;

static VOID SystemInfo(VOID)
{
	MAIN_LOG("*********************************************** \n");
	MAIN_LOG("Project	: %s\n", PROJECT_NAME);
	MAIN_LOG("Version	: %u.%02u%c  \n", (VERSION_INFO_SW >> 6) & 0x03, VERSION_INFO_SW & 0x3f, VERSION_INFO_HW);
	MAIN_LOG("Date	: %s %s\n", __DATE__, __TIME__);
	MAIN_LOG("Mail	: %s\n", EMAIL);
	MAIN_LOG("*********************************************** \n");
	MAIN_LOG("CPU@clock:  %d.%dMHz\n", (PLL_CLOCK) / MHz, (PLL_CLOCK) % MHz);
	MAIN_LOG("CPU@UID:  %08X-%08X-%08X\n", gDevice.fmc->getUniqueId(MCU_UID_1ST_WORD_ADDR), gDevice.fmc->getUniqueId(MCU_UID_2ND_WORD_ADDR), gDevice.fmc->getUniqueId(MCU_UID_3RD_WORD_ADDR));
}

static VOID InitStart(VOID)
{
	SystemInfo();

	MAIN_LOG("Main: initialization...\n");
	gDevice.led->setWorkLed(LED_OFF);
	gDevice.led->setAuthLed(LED_OFF);
	gDevice.led->setDeviceInitLed(LED_ON);	//初始化
	
	gCfgParam.dev = DEVICE_ID;
	gCfgParam.ver = VERSION_INFO_SW;
	gCfgParam.status = (STATUS_IDLE & STATUS_PARAM_SETIDLE) |STATUS_DEVICE_INITING;
#if TFTP_SUPPORT
	gCfgParam.tftpStatus = TFTP_STATUS_BUF_EMPTY | TFTP_STATUS_FPGA_VALID;
	gCfgParam.tftpBlockSize = TFTP_BLOCK_DEFAULT_SIZE;
#endif
	gDevice.i2c->serializeConfigPara(REG_INIT_MASK, &gCfgParam); 
}

static VOID InitEnd(VOID)
{
#if FPGA_LOAD
	//加载FPGA
	gDevice.fpga->load();
	gDevice.fpga->reset();
#endif
	
	/*initialization done*/
	gCfgParam.status &= STATUS_DEVICE_INITED;
	gDevice.i2c->serializeConfigPara(REG_STAT_MASK, &gCfgParam); 
	gDevice.led->setWorkLed(LED_ON);
	gDevice.led->setAuthLed(LED_ON);
	gDevice.led->setDeviceInitLed(LED_OFF);
	MAIN_LOG("Main: initialization has done !\n");
}
static InitFun gInitTable[] =
{
	//first
	InitUart,
	InitFmc,
	InitI2c,
	InitSpi,
	InitLed,
	InitTime,
	InitAlgorithm,
	
	InitStart,
	//second 
#if FLASH_SUPPORT
	InitFlash, //依赖于time
#endif	
#if TFTP_SUPPORT
	InitTftp,
#endif
#if FPGA_SUPPORT
	InitFPGA,
#endif
	InitEnd,
};

/***********************************接口定义区*******************************************************/
static VOID InitChip(VOID)
{
	 SYS_UnlockReg();
	/*---------------------------------------------------------------------------------------------------------*/
	/* Init System Clock                                                                                       */
	/*---------------------------------------------------------------------------------------------------------*/
#if 0 
	/* Enable Internal RC 22.1184MHz clock */
	CLK->PWRCON |= CLK_PWRCON_OSC22M_EN_Msk;

	/* Waiting for Internal RC clock ready */
	while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_OSC22M_STB_Msk));

	/* Switch HCLK clock source to Internal RC */
	CLK->CLKSEL0 &= ~CLK_CLKSEL0_HCLK_S_Msk;
	CLK->CLKSEL0 |= CLK_CLKSEL0_HCLK_S_HIRC;
#endif
	/* Enable external XTAL 12MHz clock */
	CLK->PWRCON |= CLK_PWRCON_XTL12M_EN_Msk;

	/* Set core clock as PLL_CLOCK from PLL */
	CLK->PLLCON = PLLCON_SETTING;

	/* Waiting for external XTAL clock ready */
	CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk|CLK_CLKSTATUS_PLL_STB_Msk);

	CLK->CLKSEL0 &= ~CLK_CLKSEL0_HCLK_S_Msk;
	CLK->CLKSEL0 |= CLK_CLKSEL0_HCLK_S_PLL;

	/* Update System Core Clock */
	/* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CycylesPerUs automatically. */
	//SystemCoreClockUpdate();
	PllClock        = PLL_CLOCK;            // PLL
	SystemCoreClock = PLL_CLOCK / 1;        // HCLK
	CyclesPerUs     = PLL_CLOCK / 1000000;  // For SYS_SysTickDelay()

	/* Enable UART0\1 I2C SPI0\1 module clock */
	CLK->APBCLK |= CLK_APBCLK_UART0_EN_Msk | CLK_APBCLK_UART1_EN_Msk | CLK_APBCLK_I2C0_EN_Msk |CLK_APBCLK_I2C1_EN_Msk | CLK_APBCLK_SPI0_EN_Msk | CLK_APBCLK_SPI1_EN_Msk;

	/* Select UART module clock source */
	CLK->CLKSEL1 &= ~ CLK_CLKSEL1_UART_S_Msk;
	CLK->CLKSEL1 |= CLK_CLKSEL1_UART_S_PLL;
	
	/*---------------------------------------------------------------------------------------------------------*/
	/* Init I/O Multi-function                                                                                 */
	/*---------------------------------------------------------------------------------------------------------*/
	/*UART. uart0 for debug; uart1 for fpga*/
	/* Set P3 multi-function pins for UART0 RXD and TXD */
	SYS->P3_MFP &= ~(SYS_MFP_P30_Msk | SYS_MFP_P31_Msk);
	SYS->P3_MFP |= SYS_MFP_P30_RXD0 | SYS_MFP_P31_TXD0;
	/* Set P1 multi-function pins for UART1 RXD and TXD */
	SYS->P1_MFP &= ~(SYS_MFP_P12_Msk | SYS_MFP_P13_Msk);
	SYS->P1_MFP |= SYS_MFP_P12_RXD1 | SYS_MFP_P13_TXD1;

	/*I2C. i2c for communicating with D board*/
	/* Configure the SDA0 & SCL0 of I2C0 pins */
	SYS->P3_MFP &= ~(SYS_MFP_P34_Msk | SYS_MFP_P35_Msk);
	SYS->P3_MFP |= SYS_MFP_P34_SDA0 | SYS_MFP_P35_SCL0;
//	GPIO_SetMode(P3, BIT4|BIT5, GPIO_PMD_OPEN_DRAIN);		//开漏模式
		
	/*SPI. spi0 for CPLD; spi1 for fpga*/
	/* Set P1 multi-function pins for SPI0*/
	SYS->P1_MFP &= ~(SYS_MFP_P14_Msk | SYS_MFP_P15_Msk | SYS_MFP_P16_Msk | SYS_MFP_P17_Msk);
	SYS->P1_MFP |= SYS_MFP_P14_SPISS0 | SYS_MFP_P15_MOSI_0 |SYS_MFP_P16_MISO_0 | SYS_MFP_P17_SPICLK0 ;
	/* Set P0 multi-function pins for SPI1*/
	SYS->P0_MFP &= ~(SYS_MFP_P04_Msk | SYS_MFP_P05_Msk | SYS_MFP_P06_Msk | SYS_MFP_P07_Msk);
	SYS->P0_MFP |= SYS_MFP_P04_SPISS1| SYS_MFP_P05_MOSI_1 |SYS_MFP_P06_MISO_1 | SYS_MFP_P07_SPICLK1 ;

	/*config gpio*/
	SYS_LockReg();
}

static VOID InitEnvironment (VOID)
{
	S32 i  = 0, len = sizeof(gInitTable)/sizeof(InitFun);
	for(; i < len; ++i){
		gInitTable[i]();
	}
	//device param struct initialize if necessary
}

S32 main(void)
{
	U32 regMask = 0, i = 0;
	/* Init System, IP clock and multi-function I/O */
	InitChip();
	InitEnvironment();
		
	while(WF_TRUE){
		regMask = gDevice.i2c->deserializeConfigPara(&gCfgParam);

#if TFTP_SUPPORT
		if(regMask & REG_TFTP_CMD_MASK)
		{
			if(TFTP_CMD_NEWREQ == gCfgParam.tftpCmd )
			{
				gDevice.tftp->start();
				gDevice.tftp->idle = WF_FALSE;
			}
			else if(TFTP_CMD_END == gCfgParam.tftpCmd )
			{
				gDevice.tftp->end();
				gDevice.tftp->idle = WF_TRUE;
				/*加载FPGA*/
				gDevice.fpga->load();
				gDevice.fpga->reset();
			}
		}
		if(regMask & REG_TFTP_BLOCK_MASK)
		{
			gDevice.tftp->save(gCfgParam.tftpBlockNo);
			gDevice.i2c->serializeConfigPara(REG_TFTP_BLOCK_MASK, &gCfgParam);
		}
#endif

		if( (i++ & 0xfffff) == 0 ) {	
			gDevice.led->workLedFlicker();

			//other polling operation
#if TFTP_SUPPORT
			if(gDevice.tftp->idle)
#endif
			{
				//to add polling code
#if MONITOR_TSOIP				
#endif	
			}
		}
	}
	
	return 0;
}

