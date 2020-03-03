/******************************************************************************
 * @file     app_tftp.c
 * @brief     tftp interface source file
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		16/03/12
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "M051Series.h"
#include "device.h"
#include "extern.h"
#include "app_tftp.h"
#include "app_i2c.h"

#if TFTP_SUPPORT
/***********************************变量定义区*******************************************************/
#if TFTP_DEBUG
#define TFTP_PRINTF		printf
#else
#define TFTP_PRINTF(...)
#endif

static TftpFile_t gTftpFile;
static Tftp_t gTftp;

/***********************************接口定义区*******************************************************/
/*
* brief: 启动tftp传输
*/
static VOID StartTftp(VOID)
{
	gTftpFile.flag = TFTP_FLAG;
	gTftpFile.fileLen = 0;
	gTftpFile.errorCode = TFTP_TRANSMITTING;

	assert_param( gpDev->flash!=NULL);	
	gpDev->flash ->SSTWrite(FLASH_FINFO_ADDR, (const U08*)&gTftpFile, sizeof(TftpFile_t));
	TFTP_PRINTF("%s transmition start!\n", TFTP_DEBUG_HEARSTR);
}

/*
* brief: tftp传输结束
*/
static VOID EndTftp(VOID)
{
	gTftpFile.flag = TFTP_FLAG;
	if(gTftpFile.errorCode != TFTP_DATA_INCOMPLETE){
		gTftpFile.errorCode = TFTP_SUCCESS;
	}

	assert_param( gpDev->flash!=NULL);	
	gpDev->flash ->SSTWrite(FLASH_FINFO_ADDR, (const U08*)&gTftpFile, sizeof(TftpFile_t));
	TFTP_PRINTF("%s transmition end, LEN[%d] EC[%d]\n", TFTP_DEBUG_HEARSTR, gTftpFile.fileLen, gTftpFile.errorCode);
}

/*
* brief:保存tftp数据
*/
static VOID SaveTftp(U16 blockNo)
{
	RegBuffer_t* tftpBuffer = (RegBuffer_t*)gpDev->i2c->tftpBuffer;
	
	//校准tftp长度 , 忽略最后一个bank 错误
	if(gTftpFile.fileLen%TFTP_BLOCK_SIZE){
		gTftpFile.errorCode = TFTP_DATA_INCOMPLETE;
		gTftpFile.fileLen = blockNo << 9;
		TFTP_PRINTF("%s tftp data is incomplete\n", TFTP_DEBUG_HEARSTR);
	}

	assert_param( gpDev->flash!=NULL);	
	gpDev->flash->SSTWrite(FLASH_TFTP_ADDR + gTftpFile.fileLen, tftpBuffer->buf, tftpBuffer->tail);
	gTftpFile.fileLen += tftpBuffer->tail;
}

/*
*brief: tftp 初始化，基于flash初始化, I2C初始化
*/
VOID InitTftp(VOID)
{
	gTftp.idle = WF_TRUE;
	gTftp.start = StartTftp;
	gTftp.end = EndTftp;
	gTftp.save = SaveTftp;
	gpDev->tftp = &gTftp;
}
#endif

