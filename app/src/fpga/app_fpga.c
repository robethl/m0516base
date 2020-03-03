/******************************************************************************
 * @file     app_fpga.c
 * @brief    hardware FPGA interface source file
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12	create
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "M051Series.h"
#include "device.h"
#include "app_fpga.h"
#include "app_tftp.h"

#if FPGA_SUPPORT
/***********************************变量定义区*******************************************************/
#if FPGA_DEBUG
#define FPGA_PRINTF			printf
#else
#define FPGA_PRINTF(...)
#endif

#define FPGA_CE				P01
#define FPGA_DCLK     		P41
#define FPGA_DATA			P03
#define FPGA_STATUS 		P02
#define FPGA_CONFIG      		P00
#if (0==FPGA_RESET_AUTO)
#define FPGA_RST			P10
#endif

static Fpga_t gFpga;

/***********************************接口定义区*******************************************************/
/*
* brief: SPI 片选使能
*/
static __INLINE VOID SpiEnableCs(VOID)
{
	gpDev->spi->spiEnableCs(SPI1);
}

/*
* brief: SPI 片选禁止
*/
static __INLINE VOID SpiDisableCs(VOID)
{
	gpDev->spi->spiDisableCs(SPI1);
}

/*
* brief: SPI0 写
*/
static __INLINE VOID SpiWrite(const U08 *pBytes, U16 len)
{
	gpDev->spi->spiWrite(SPI1, pBytes, len);
}

/*
* brief: SPI0 读
*/
static __INLINE VOID SpiRead(pU08 pBytes, U16 len)
{
	gpDev->spi->spiRead(SPI1, pBytes, len);
}
/*
* brief: FPGA 写
*/
static VOID FPGAWriteU16(U16 addr, U16 d)
{
	U08 tmp[5];	
	tmp[0] = (U08)FPGA_CMD_WR;
	tmp[1] = (U08)(addr >> 8);
	tmp[2] = (U08)addr;
	tmp[3] = (U08)(d >> 8);
	tmp[4] = (U08)d;
	SpiEnableCs();
	SpiWrite(tmp, sizeof(tmp));
	SpiDisableCs();
}

/*
* brief: FPGA 读
*/
static U16 FPGAReadU16(U16 addr)
{
	U08 tmp[3];
	U16 	d;
	tmp[0] = (U08)FPGA_CMD_RD;
	tmp[1] = (U08)(addr >> 8);
	tmp[2] = (U08)addr;	
	SpiEnableCs();
	SpiWrite(tmp, sizeof(tmp));
	SpiRead(tmp, 2);
	SpiDisableCs();
	d = (tmp[0] << 8) | tmp[1];
	return d;
}

/*
* brief: 复位FPGA
*/
static VOID FPGAReset(VOID)
{
#if (0==FPGA_RESET_AUTO)
	/*reset fpga*/
	FPGA_RST = 1;
	gpDev->time->delaynUs(2000);		/*keep high more than 1ms*/
	FPGA_RST = 0;
#endif	
}

/*
* brief:开始配置FPGA
*/
BOOLEAN FpgaConfigStart(VOID)
{	
	U16 timeOut = 0;
  	FPGA_CONFIG = 1;
	FPGA_STATUS = 1;
	FPGA_CE = 1;
	FPGA_DCLK = 0;	
  	gpDev->time->delaynUs(1);
  	FPGA_CONFIG = 0;
  	gpDev->time->delaynUs(1);
  	if(FPGA_STATUS || FPGA_CE) {
		return WF_FALSE;
  	}
	
  	FPGA_CONFIG = 1;		
 	while(!FPGA_STATUS){
		gpDev->time->delaynUs(10);
		if(CF2ST1_TIMEOUT==(++timeOut))	//100ms
			break;
	}
	
	if(CF2ST1_TIMEOUT == timeOut){
		FPGA_PRINTF("%s config error, wait high status when set config to high time out\n", FPGA_DEBUG_HEADSTR);
		return WF_FALSE;
	}	
	return WF_TRUE;
}

#if FPGA_LOAD
#if (CPLD_USED==0)
/*
* brief:发送配置数据字节
* D板端已将tftp数据转换成LSB在前，所以flash存储的tftp数据
* 为LSB 在前， FPGA加载采用LSB first-MSB last方式
*/
static VOID FpgaWriteCfgByte(VOID)
{  
	U08 i = 0, data = 0;
	gpDev->flash->spiRead(&data, 1);
	for(; i<8; ++i) {
		if(data & 0x80) {
			FPGA_DATA = 1;
		} else{
			FPGA_DATA = 0;
		}
		FPGA_DCLK = 1;
		FPGA_DCLK = 0;
		data <<= 1;
	}
}

/*
* brief: MCU加载配置FPGA
*/
static BOOLEAN MCULoadFpga(VOID)
{
	TftpFile_t file;
	gpDev->flash->SSTRead(FLASH_FINFO_ADDR, (U08*)&file, sizeof(TftpFile_t));

	if(file.flag != TFTP_FLAG && file.errorCode !=TFTP_SUCCESS){
		FPGA_PRINTF("%s rbf file error, tag[0x%08X] EC[%d]\n", FPGA_DEBUG_HEADSTR, file.flag, file.errorCode);
		return WF_FALSE;
	}

	FPGA_PRINTF("%s fpga file length[%d]\n", FPGA_DEBUG_HEADSTR, file.fileLen);
	if(0 == file.fileLen){
		return WF_FALSE;
	}
	
	if(!FpgaConfigStart()) {
		FPGA_PRINTF("%s CONFIG error\n", FPGA_DEBUG_HEADSTR);
		return WF_FALSE;
	}
	
	gpDev->flash->spiEnable();
	gpDev->flash->SSTFastRead(FLASH_FPGA_ADDR);	
	while(file.fileLen > 0){
		FpgaWriteCfgByte();
		if(FPGA_STATUS == 0){
			FPGA_PRINTF("%s STATUS error\n", FPGA_DEBUG_HEADSTR);
			break;
		}
		file.fileLen--;
	}
	gpDev->flash->spiDisable();
	
	if(file.fileLen > 0) {
		return WF_FALSE;
	}
    return WF_TRUE;
}
#else
static BOOLEAN CPLDLoadFpga(VOID)
{
	U16 status, i;	
	//status[7] = 1: busy, status[6] = recr flag, 
	//status[5] == FPGA2_STATUS_ERROR2, status[4] = FPGA2_STATUS_ERROR1, status[3]: FPGA2_OK, 
	//status[2] == FPGA1_STATUS_ERROR2, status[1] = FPGA1_STATUS_ERROR1, status[0]: FPGA1_OK	
	status = gpDev->flash->CPLDGetStatus();
	if((status & 0xff00) != 0x5a00)
	{
		FPGA_PRINTF("%s CPLD ERROR[0x%04X]\n", FPGA_DEBUG_HEADSTR, status);
		return WF_FALSE;
	}	
	FPGA_PRINTF("%s CPLD OK\n", FPGA_DEBUG_HEADSTR);	

	gpDev->flash->CPLDLoadFpga();
	for(i = 0; i < CPLD_CFGFPGA_TIMEOUT; i++) {
		gpDev->time->delaynMs(10);	//10ms
		status = gpDev->flash->CPLDGetStatus();								
		if(!(status & 0x0080)) break;	//cpld config fpga OK
	}			
	gpDev->time->delaynMs(1000);  //wait start complete 

	if(i >= CPLD_CFGFPGA_TIMEOUT) {
		gpDev->flash->CPLDReset();
		FPGA_PRINTF("%s Config timeout\n", FPGA_DEBUG_HEADSTR);
	} else {
		FPGA_PRINTF("%s CPLD status[0x%04X], FPGA config time[%dms]\n", FPGA_DEBUG_HEADSTR, status, (U16)(i * 10)); 
	}

	if((status & 0x0004)) {
		FPGA_PRINTF("%s FPGA[1] error[2]\n", FPGA_DEBUG_HEADSTR);
	}else if((status & 0x0002)){ 
		FPGA_PRINTF("%s FPGA[1] error[1]\n", FPGA_DEBUG_HEADSTR);
	}else if((status & 0x0001)){
		FPGA_PRINTF("%s FPGA[1] OK\n", FPGA_DEBUG_HEADSTR);
		return WF_TRUE;
	}else {
		FPGA_PRINTF("%s FPGA[1] error[unkown]\n", FPGA_DEBUG_HEADSTR);
	}
	return WF_FALSE;
}
#endif
#endif

/*
* brief: FPGA 加载
*/
static VOID FPGALoad(VOID)
{
#if FPGA_LOAD
	FPGA_PRINTF("%s start loading FPGA!\n", FPGA_DEBUG_HEADSTR);
	assert_param( gpDev->flash!=NULL);	
#if CPLD_USED
	CPLDLoadFpga();
#else
	if(!MCULoadFpga())
	{
		FPGA_PRINTF("%s config failed\n", FPGA_DEBUG_HEADSTR);
		return;
	}
	FPGA_PRINTF("%s config successfully\n", FPGA_DEBUG_HEADSTR);
#endif
#endif
}

/*
* brief: FPGA 初始化
*/
VOID InitFPGA(VOID)
{
	gFpga.write = FPGAWriteU16;
	gFpga.read = FPGAReadU16;
	gFpga.load = FPGALoad;
	gFpga.reset = FPGAReset;
	
	gpDev->fpga = &gFpga;
}
#endif

