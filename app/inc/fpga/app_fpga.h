/******************************************************************************
 * @file     app_fpga.h
 * @brief    This is the header file of app_fpga.c
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12
*****************************************************************************/
#ifndef _APP_FPGA_H_
#define _APP_FPGA_H_

#include "typedef.h"
#include "device.h"

#define CF2ST1_TIMEOUT	(10000)
#define CPLD_CFGFPGA_TIMEOUT  (500)

#define FPGA_CMD_WR          	(0x00)
#define FPGA_CMD_RD          	(0x01)

#define FPGA_REG_VERSION_RO		(0x00)
#define FPGA_REG_TEST_RW			(0x01)
#define FPGA_REG_ASISTATUS_RO	(0x02)
#define FPGA_REG_CHSEL_RW			(0x03)
#define FPGA_REG_TSOUTCLK_RW	(0x04)

#define FPGA_REG_DECODER_TSSRC_RW	(0x07)
#define FPGA_REG_PCRPID_W			(0x401)
#define FPGA_REG_VIDEOPID_W		(0x402)
#define FPGA_REG_AUDIOPID_W		(0x403)

#define FPGA_DEBUG		(1)			/*FPGA模块打印开关*/
#define FPGA_DEBUG_HEADSTR	"Fpga:"

#endif

