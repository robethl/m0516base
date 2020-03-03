/******************************************************************************
 * @file     app_i2c.h
 * @brief    This is the header file of app_i2c.c
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		15/11/25
*****************************************************************************/
#ifndef _APP_I2C_H_
#define _APP_I2C_H_

#include "typedef.h"
#include "device.h"

#define I2C_ADDR	(0xB2)
#define I2C_CLOCK 	(100)	//Hz

typedef enum
{
	I2C_REG_DEV_R = 0x00,
	I2C_REG_VER_R  = 0x01,
	I2C_REG_STAT_R = 0x02,	/*bit0: 1-busy, 0-idle; bit 4: 1-param setting, 0-idle; bit5:1-initialized, 0-initializing*/
	I2C_REG_RND_WR = 0x03,
	I2C_REG_AUTH_WR = 0x04,

#if TFTP_SUPPORT
	I2C_REG_TFTP_STATUS_WR = 0x10,
	I2C_REG_TFTP_CMD_WR = 0x11,
	I2C_REG_TFTP_BLOCK_WR = 0x12,
	I2C_REG_TFTP_BSIZE_WR = 0x13,
	I2C_REG_TFTP_DATA_W = 0x14,
#endif

	I2C_REG_SUM,
	I2C_REG_INVALID,
}I2C_REG_e;

#define REG_DEV_MASK			(1 << I2C_REG_DEV_R)
#define REG_VER_MASK			(1 << I2C_REG_VER_R)
#define REG_STAT_MASK			(1 << I2C_REG_STAT_R)
#define REG_RND_MASK			(1 << I2C_REG_RND_WR)
#define REG_AUTH_MASK			(1 << I2C_REG_AUTH_WR)
#if TFTP_SUPPORT
#define REG_TFTP_STATUS_MASK	(1 << I2C_REG_TFTP_STATUS_WR)
#define REG_TFTP_CMD_MASK		(1 << I2C_REG_TFTP_CMD_WR)
#define REG_TFTP_BLOCK_MASK	(1 << I2C_REG_TFTP_BLOCK_WR)
#define REG_TFTP_BSIZE_MASK	(1 << I2C_REG_TFTP_BSIZE_WR)
#define REG_TFTP_DATA_MASK	(1 << I2C_REG_TFTP_DATA_W)
#endif
#if TFTP_SUPPORT
#define REG_INIT_MASK				(REG_DEV_MASK | REG_VER_MASK | REG_STAT_MASK | REG_TFTP_STATUS_MASK | REG_TFTP_BSIZE_MASK)
#else
#define REG_INIT_MASK				(REG_DEV_MASK | REG_VER_MASK | REG_STAT_MASK )
#endif

#define STATUS_BUSY						(1 << 0)
#define STATUS_IDLE						(~STATUS_BUSY)
#define STATUS_PARAM_SETTING		(1 << 4)
#define STATUS_PARAM_SETIDLE		(~STATUS_PARAM_SETTING)
#define STATUS_DEVICE_INITING		(1 << 5)
#define STATUS_DEVICE_INITED		(~STATUS_DEVICE_INITING)

#if TFTP_SUPPORT
#define TFTP_STATUS_BUF_EMPTY		(1 << 1)
#define TFTP_STATUS_BUF_NONEMPTY	(~TFTP_STATUS_BUF_EMPTY)
#define TFTP_STATUS_FPGA_VALID	(1 << 0)	
#define TFTP_STATUS_FPGA_INVALID	(~TFTP_STATUS_FPGA_VALID)
#define TFTP_CMD_NEWREQ			(0)
#define TFTP_CMD_END				(1)
#define TFTP_BLOCK_DEFAULT_SIZE	(512)
#endif

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	U16 head;
	U16 tail;
	U16 size;
	pU08 buf;
}RegBuffer_t, *pRegBuffer_t;

typedef struct
{
	pRegBuffer_t receiver;		/*W*/
	pRegBuffer_t sender;		/*R*/
}I2CBuffer_t, *pI2CBuffer_t;

#define WROP_BUFLEN	64	//20 tftp升级时，main轮训来不及处理I2C写数据，必须预留足够大的缓存
typedef struct
{
	U08 head;
	U08 tail;
	U08 buf[WROP_BUFLEN];
}opRegBuf_t;

#pragma pack(pop)

#endif

