/******************************************************************************
 * @file     app_i2c.c
 * @brief    hardware I2C interface source file
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12	create
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "M051Series.h"
#include "device.h"
#include "app_i2c.h"

/***********************************变量定义区*******************************************************/
static I2c_t gI2c;
static U08 gDevBuffer[51];
#if TFTP_SUPPORT
static U08 gDevTftpBuffer[519];
#endif

static RegBuffer_t gRegBuffer[] =
{
	{0, 2, 2, &gDevBuffer[0]},	/* 2 Dev(R) */
	{0, 1, 1, &gDevBuffer[2]},	/* 1 Version(R) */
	{0, 1, 1, &gDevBuffer[3]},	/* 1 Status(R) bit0:busy-1  idle-0*/
	{0, 0, 8, &gDevBuffer[4]},	/* 8 random(W)*/
	{0, 8, 8, &gDevBuffer[12]},	/* 8 random(R)*/
	{0, 0, 8, &gDevBuffer[20]},	/* 8 Authentication(W)*/
	{0, 8, 8, &gDevBuffer[20]},	/* 8 Authentication(R)*/
};

#if TFTP_SUPPORT
static RegBuffer_t gRegTftpBuffer[] =
{
	{0, 0, 2, &gDevTftpBuffer[0]},	/* 2 tftp status (W)*/
	{0, 2, 2, &gDevTftpBuffer[0]},	/* 2 tftp status (R)*/
	{0, 0, 1, &gDevTftpBuffer[2]},	/* 1 tftp cmd (W)*/
	{0, 1, 1, &gDevTftpBuffer[2]},	/* 1 tftp cmd (R)*/
	{0, 0, 2, &gDevTftpBuffer[3]},	/* 2 tftp block number (W)*/
	{0, 2, 2, &gDevTftpBuffer[3]},	/* 2 tftp block number (R)*/
	{0, 0, 2, &gDevTftpBuffer[5]},	/* 2 tftp block size, default 512 (W)*/
	{0, 2, 2, &gDevTftpBuffer[5]},	/* 2 tftp block size, default 512 (R)*/
	{0, 0, 512, &gDevTftpBuffer[7]},	/* 512 tftp data (W)*/
};
#endif

static I2CBuffer_t gI2cBuffer[] =
{
	{NULL, &gRegBuffer[0]},	/* 0 Dev(R) */
	{NULL, &gRegBuffer[1]},	/* 1 Version(R) */
	{NULL, &gRegBuffer[2]},	/* 2 Status(R) */
	{&gRegBuffer[3], 	&gRegBuffer[4]},	/* 3 random(W/R) */
	{&gRegBuffer[5], 	&gRegBuffer[6]},	/* 4 Authentication(W/R) */
	{NULL, NULL},		/*5~0x0F reserved*/
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
#if TFTP_SUPPORT
	{&gRegTftpBuffer[0], &gRegTftpBuffer[1]},	/* 0x10 tftp status (WR)*/
	{&gRegTftpBuffer[2], &gRegTftpBuffer[3]},	/* 0x11 tftp cmd (WR)*/
	{&gRegTftpBuffer[4], &gRegTftpBuffer[5]},	/* 0x12 tftp block number (WR)*/
	{&gRegTftpBuffer[6], &gRegTftpBuffer[7]}, /* 0x13 tftp block size, default 512 (WR)*/
	{&gRegTftpBuffer[8], NULL},				/* 0x14 tftp data (W)*/
#else
	{NULL, NULL},	/* 0x10 tftp status (WR)*/
	{NULL, NULL},	/* 0x11 tftp cmd (WR)*/
	{NULL, NULL},	/* 0x12 tftp block number (WR)*/
	{NULL, NULL}, /* 0x13 tftp block size, default 512 (WR)*/
	{NULL, NULL},	
#endif
};

static opRegBuf_t gOpReg;

/***********************************接口定义区*******************************************************/
/*
*brief :序列化系统参数
*/
static VOID SerializeConfigPara(U32 regMask,  VOID *ptr)
{
	U08	reg;
	pSysCfgParam_t param = (pSysCfgParam_t)ptr;
	pRegBuffer_t pBuf = NULL;
	
	for(reg = 0;reg < I2C_REG_SUM;reg++)
	{
		pBuf = gI2cBuffer[reg].sender;
		if((regMask & (1 << reg)))
		{
			switch(reg)
			{
				case I2C_REG_DEV_R:
					pBuf->buf[0] = (U08)(param->dev >> 8);
					pBuf->buf[1] = (U08)param->dev;
					break;
					
				case I2C_REG_VER_R:
					pBuf->buf[0] = param->ver;
					break;
					
				case I2C_REG_STAT_R:
					pBuf->buf[0] = param->status;
					break;
				
				case I2C_REG_RND_WR:
					memcpy(pBuf->buf, param->random, pBuf->size);
					break;
				
				case I2C_REG_AUTH_WR:
					memcpy(pBuf->buf, param->authentication, pBuf->size);
					gI2cBuffer[I2C_REG_STAT_R].sender->buf[0] &= STATUS_IDLE;
					break;

#if TFTP_SUPPORT
				case I2C_REG_TFTP_STATUS_WR:
					pBuf->buf[0] = (U08)(param->tftpStatus>> 8);
					pBuf->buf[1] = (U08)param->tftpStatus;
					break;
					
				case I2C_REG_TFTP_BLOCK_WR:
					gI2cBuffer[I2C_REG_TFTP_DATA_W].receiver->head = 0;
					gI2cBuffer[I2C_REG_TFTP_DATA_W].receiver->tail = 0;
					gI2cBuffer[I2C_REG_TFTP_STATUS_WR].sender->buf[1] |=  (U08)TFTP_STATUS_BUF_EMPTY;
					break;

				case I2C_REG_TFTP_BSIZE_WR:
					pBuf->buf[0] = (U08)(param->tftpBlockSize>> 8);
					pBuf->buf[1] = (U08)param->tftpBlockSize;
					break;
#endif
					
				default:
					break;
			}
		}
	}
}

/*
*brief :反序列化系统参数
*	return  - 写操作掩码
*/
static U32 DeserializeConfigPara(VOID *ptr)
{
	U08 reg = I2C_REG_INVALID;
	U32 regMask = 0;
	pSysCfgParam_t param = (pSysCfgParam_t)ptr;
	pRegBuffer_t pBuf = NULL;

	while(gOpReg.tail!=gOpReg.head)
	{
		reg = gOpReg.buf[gOpReg.tail++];
		pBuf = gI2cBuffer[reg].receiver;
		regMask |= (1 << reg);
		switch(reg)
		{
			case I2C_REG_RND_WR:
				memcpy(param->random, pBuf->buf, pBuf->size);
				break;

			case I2C_REG_AUTH_WR:
				memcpy(param->authentication, pBuf->buf, pBuf->size);
				break;

#if TFTP_SUPPORT
			case I2C_REG_TFTP_CMD_WR:
				param->tftpCmd = pBuf->buf[0];
				break;

			case I2C_REG_TFTP_BLOCK_WR:
				param->tftpBlockNo = (U16)(pBuf->buf[0])<<8 | (U16)(pBuf->buf[1]);
				break;

			case I2C_REG_TFTP_BSIZE_WR:
				param->tftpBlockSize = (U16)(pBuf->buf[0])<<8 | (U16)(pBuf->buf[1]);
				break;
#endif

			default:
				regMask &= ~(1 << reg);
				break;
		}
		gOpReg.tail %= WROP_BUFLEN;
	}
	
	return regMask;
}

/*
* brief: I2C0 中断函数
*/
VOID I2C0_IRQHandler(VOID)
{
	U08 status;
	static U08 i2cReg = I2C_REG_INVALID;
	status = I2C_GET_STATUS(I2C0);
	switch(status)
	{
		/* Slave Transmitter Mode */
		case 0xC0:                        /* DATA has been transmitted and NACK has been returned */
		case 0xC8:                        /* DATA has been transmitted and ACK has been returned */
			break;

		case 0xA8:                        /* SLA+R has been received and ACK has been returned */
		case 0xB0:
		case 0xB8:                        /* DATA has been transmitted and ACK has been returned */
			if(I2C_REG_INVALID != i2cReg && gI2cBuffer[i2cReg].sender!=NULL) {
				if(gI2cBuffer[i2cReg].sender->head!=gI2cBuffer[i2cReg].sender->tail) {
					I2C_SET_DATA(I2C0, gI2cBuffer[i2cReg].sender->buf[gI2cBuffer[i2cReg].sender->head++]);
				} else {
					I2C_SET_DATA(I2C0, 0x00);
				}
			}
			break;

		/* Slave Receiver Mode*/
		case 0x60:	/* SLA+W has been received and ACK has been returned */
		case 0x68:    /* 0x68: Slave Receive Arbitration Lost */
			i2cReg = I2C_REG_INVALID;
			break;
		case 0x80:	/* DATA has been received and ACK has been returned */
			if(I2C_REG_INVALID==i2cReg) {
				i2cReg = I2C_GET_DATA(I2C0);
				if(i2cReg >= I2C_REG_SUM) {
					i2cReg = I2C_REG_INVALID;	//invalid register
					break;
				}
				if(gI2cBuffer[i2cReg].sender!=NULL){		/*重置读缓存头*/
					gI2cBuffer[i2cReg].sender->head = 0;
				}
				if(gI2cBuffer[i2cReg].receiver!=NULL
#if TFTP_SUPPORT
					&& i2cReg != I2C_REG_TFTP_DATA_W
#endif 
				) {	/*重置写缓存头尾*/
					gI2cBuffer[i2cReg].receiver->head = 0;
					gI2cBuffer[i2cReg].receiver->tail = 0;
				}
			}else {
				if(gI2cBuffer[i2cReg].receiver!=NULL && gI2cBuffer[i2cReg].receiver->tail!=gI2cBuffer[i2cReg].receiver->size) {
					gI2cBuffer[i2cReg].receiver->buf[gI2cBuffer[i2cReg].receiver->tail++] = I2C_GET_DATA(I2C0);
				} else {
					U08 invalidData = I2C_GET_DATA(I2C0);
				}
			}
			break;
		case 0x88:	/* DATA has been received and NACK has been returned */
		    break;
			
		case 0xA0:	/* STOP or Repeat START has been received */
			if(i2cReg != I2C_REG_INVALID && gI2cBuffer[i2cReg].receiver!=NULL && gI2cBuffer[i2cReg].receiver->tail != 0) {
				
				if(I2C_REG_RND_WR == i2cReg){
					gI2cBuffer[I2C_REG_STAT_R].sender->buf[0] |= STATUS_BUSY;
				}
				
#if TFTP_SUPPORT				
				if(I2C_REG_TFTP_BLOCK_WR == i2cReg)
				{
					gI2cBuffer[I2C_REG_TFTP_STATUS_WR].sender->buf[1] &= TFTP_STATUS_BUF_NONEMPTY;
				}
#endif

				if((gOpReg.head+1)%WROP_BUFLEN != gOpReg.tail%WROP_BUFLEN){	//buffer满 不受理新请求，每次写前查询busy状态
					gOpReg.buf[gOpReg.head++] = i2cReg;
					gOpReg.head %= WROP_BUFLEN;
				}
			}
	    		break;
		default:
			break;
	}
	I2C_SET_CONTROL_REG(I2C0, I2C_I2CON_SI_AA);
}

/*
* brief: I2C 初始化
*/
VOID InitI2c(VOID)
{	
	/*---------------------------------------------------------------------------------------------------------*/
	/* I2C0: ASI	slave mode																					     */
	/*---------------------------------------------------------------------------------------------------------*/
	/*reset I2C*/
	SYS_ResetModule(I2C0_RST);
	
	/* Open I2C module and set bus clock */
	I2C_Open(I2C0, I2C_CLOCK);

	/* Set I2C Slave Addresses */
	I2C_SetSlaveAddr(I2C0, 0, I2C_ADDR>>1, 1);

	/* Enable I2C interrupt */
	I2C_EnableInt(I2C0);	//默认优先级为0
	
	/* I2C as slave */
	I2C_SET_CONTROL_REG(I2C0, I2C_I2CON_SI_AA);

#if TFTP_SUPPORT
	gI2c.tftpBuffer = gI2cBuffer[I2C_REG_TFTP_DATA_W].receiver;
#endif
	gI2c.serializeConfigPara = SerializeConfigPara;
	gI2c.deserializeConfigPara = DeserializeConfigPara;
	gpDev->i2c = &gI2c;
}

