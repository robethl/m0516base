/******************************************************************************
 * @file     device.h
 * @brief    This is the header file of device definition
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		16/01/06 	refactor for M0516 base template
 *	   v1.1		  huanglv		16/01/18 	FLASH_SUPPORT �궨��Ϊ0ʱ��TFTP_SUPPORT����Ϊ0
*****************************************************************************/
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "typedef.h"

#define FLASH_SUPPORT	(1)
#if FLASH_SUPPORT
/*֧��flash����ʱ��TFTP���ܿ�ѡ*/
#define TFTP_SUPPORT	(1)
#else
/*��֧��flash����ʱ����ֹTFTP*/
#define TFTP_SUPPORT	(0)
#endif
#define FPGA_SUPPORT	(1)
#define FPGA_RESET_AUTO	(1)	// 1:�Ը�λ; 0-�ⲿ��λ
#define FPGA_LOAD	(0)
#define CPLD_USED	(0)	// 1: CPLD����flash ��д��fpga����; 0: mcuֱ�ӿ���
#define FLASH_BASE_ADDR  		(0UL)
#define FLASH_FINFO_ADDR		(FLASH_BASE_ADDR )
#define FLASH_TFTP_ADDR		(FLASH_FINFO_ADDR + 1024UL * 4)
#define FLASH_FPGA_ADDR		(FLASH_TFTP_ADDR)

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	U16 dev;
	U08 ver;
	U08 status;		/*bit0: 1-busy; 0-idle*/
	U08 random[8];
	U08 authentication[8];
#if TFTP_SUPPORT
	U16 tftpStatus;	/*bit0: 1-fpga valid, 0-unsupport fpga; bit1: 1-tftp buffer empty. 0-not empty*/
	U08 tftpCmd;
	U16 tftpBlockNo;
	U16 tftpBlockSize;
#endif
}SysCfgParam_t, *pSysCfgParam_t;

typedef struct
{
	/*Uart0:ϵͳ���Դ�ӡ; Uart1:ͨ�Ŷ˿�*/
	VOID *uartBuffer;
	VOID (*startCommunication)(VOID);
	VOID (*stopCommunication)(VOID);
	VOID (*send)(const U08* data, U16 len);
}Uart_t, *pUart_t;

typedef struct
{
	U32 (*getUniqueId)(U32 addr);
	VOID (*systemParamSave)(VOID *para);
	VOID (*systemParamLoad)(VOID *para);
}Fmc_t, *pFmc_t;

typedef struct
{
#if TFTP_SUPPORT
	VOID *tftpBuffer;
#endif
	VOID (*serializeConfigPara)(U32 regMask,  VOID *ptr);
	U32 (*deserializeConfigPara)(VOID *ptr);
}I2c_t, *pI2c_t;

typedef struct
{
	/*���spi*/
	VOID (*swSpiByteInterval)(U08 spi);
	VOID (*swSpiEnableCs)(U08 spi);
	VOID (*swSpiDisableCs)(U08 spi);
	VOID (*swSpiWrite)(U08 spi, const U08* pBytes, U16 len);
	VOID (*swSpiRead)(U08 spi, pU08 pBytes, U16 len);

	/*Ӳ��spi�ӿ�*/
	VOID (*spiEnableCs)(VOID *spi);
	VOID (*spiDisableCs)(VOID *spi);
	VOID (*spiWrite)(VOID *spi, const U08* pBytes, U16 len);
	VOID (*spiRead)(VOID *spi, pU08 pBytes, U16 len);
}Spi_t, *pSpi_t;

typedef struct
{
	VOID (*setWorkLed)(U08 s);
	VOID (*setAuthLed)(U08 s);
	VOID (*setDeviceInitLed)(U08 s);
	VOID (*workLedFlicker)(VOID);
}Led_t, *pLed_t;

typedef struct
{
	VOID (*delaynUs)(U32 us);
	VOID (*delaynMs)(U32 ms);
}Time_t, *pTime_t;

typedef struct
{
	U16 (*hex)(U08 x);
	U08 (*hexBackWard)(U16 x);
	U16 (*calculateCRC16)(const U08* ptr, U32 len);
	VOID (*displayHex)(pU08 pData, U16 len);
}Algorithm_t, *pAlgorithm_t;

typedef struct
{
	U32 jedecId;
	VOID (*spiEnable)(VOID);
	VOID (*spiDisable)(VOID);
	VOID (*spiWrite)(const U08 *ptr, U16 len);
	VOID (*spiRead)(pU08 ptr, U16 len);
	VOID (*SSTFastRead)(U32 addr);
	VOID (*SSTWrite8ByteAlignment)(U32 addr, const U08 *ptr, U32 size);
	VOID (*SSTWrite)(U32 addr, const U08 *ptr, U32 size);
	VOID (*SSTRead)(U32 addr, pU08 ptr, U32 size);
#if CPLD_USED
	VOID (*CPLDLoadFpga)(VOID);
	U16 (*CPLDGetStatus)(VOID);
	VOID (*CPLDReset)(VOID);
#endif	
}Flash_t, *pFlash_t;

typedef struct
{
	BOOLEAN idle; 	/*WF_FALSE - tftp�ļ�����, WF_TRUE - tftp����*/
	VOID (*start)(VOID);
	VOID (*end)(VOID);
	VOID (*save)(U16 block);
}Tftp_t, *pTftp_t;

typedef struct
{
	VOID (*write)(U16 addr, U16 data);
	U16 (*read)(U16 addr);
	VOID (*load)(VOID);
	VOID (*reset)(VOID);	//���غ��Ƿ�reset ��fpga���ط�ʽ����
}Fpga_t, *pFpga_t;

typedef struct
{
	pUart_t uart;
	pFmc_t fmc;
	pI2c_t i2c;
	pSpi_t spi;
	pLed_t led;
	pTime_t time;
	pAlgorithm_t algorithm;

#if FLASH_SUPPORT
	pFlash_t flash;
#endif
#if TFTP_SUPPORT
	pTftp_t tftp;
#endif
#if FPGA_SUPPORT
	pFpga_t fpga;
#endif
}M0516_Device_t, *pM0516_Device_t;

#define TFTP_FLAG	(0x12345678)
typedef struct
{
	U32 flag;
	U32 fileLen;
	U08 errorCode;		/*0-no error, other-error*/
}TftpFile_t;

#pragma pack(pop)

extern pM0516_Device_t gpDev;	//�豸�ṹָ�룬main�ļ�����

typedef VOID (*InitFun)(VOID);

#endif

