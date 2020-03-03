/******************************************************************************
 * @file     app_uart.c
 * @brief    hardware uart interface source file
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12	create
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "M051Series.h"
//#include "extern.h"
#include "device.h"
#include "app_uart.h"

/***********************************变量定义区*******************************************************/
static Uart_t gUart;
static UartBuffer_t gUartBuffer0;	//打印口
static UartBuffer_t gUartBuffer1;	//串口通信协议口

/***********************************接口定义区*******************************************************/
/*
* brief: 串口0中断函数
*/
VOID UART0_IRQHandler(VOID)
{
	U08 bInChar = 0xFF;
	
	if(UART_GET_INT_FLAG(UART0, UART_ISR_RDA_INT_Msk))                              /*检查是否接收中断*/
	{
		while(UART_GET_INT_FLAG(UART0, UART_ISR_RDA_IF_Msk))                        /*检查接收到底数据是否有效*/
		{
			while (UART_GET_RX_EMPTY(UART0));                //等字符
			bInChar = UART_READ(UART0);                      //读取字符
			
			gUartBuffer0.buf[gUartBuffer0.tail++] = bInChar;/*从RXFIFO中读取接收到的数据*/
			gUartBuffer0.tail %= UART_BUFLEN;

			if(gUartBuffer0.tail == gUartBuffer0.head) {	//冲掉旧数据
				gUartBuffer0.head = (gUartBuffer0.head+1)%UART_BUFLEN;				
			}	
		}
	}
}

/*
* brief: 串口1中断函数
*/
VOID UART1_IRQHandler(VOID)
{
	U08 bInChar=0xFF;
	
	if(UART_GET_INT_FLAG(UART1, UART_ISR_RDA_INT_Msk))                              /*检查是否接收中断*/
	{	    
		while(UART_GET_INT_FLAG(UART1, UART_ISR_RDA_IF_Msk))                        /*检查接收到底数据是否有效*/
		{
			while (UART_GET_RX_EMPTY(UART1));
			bInChar = UART_READ(UART1); 
			
			//buffer已满或者监测到结束符不再接收冗余数据
			if(gUartBuffer1.tail == UART_BUFLEN){
				return;
			}
			gUartBuffer1.buf[gUartBuffer1.tail++] = bInChar;/*从RXFIFO中读取接收到的数据*/
		}
	}
}

/*
*brief:启动串口1通讯
*/
static VOID UartSartCommunication(VOID)
{
	/*clear Rx buffer*/
	UART_RX_RESET(UART1);
	gpDev->time->delaynUs(10);
	memset(gUartBuffer1.buf, 0, sizeof(gUartBuffer1.buf));
	gUartBuffer1.head = 0;
	gUartBuffer1.tail = 0;
	
	UART_EnableInt(UART1, UART_IER_RDA_IEN_Msk);
}

/*
*brief:串口1通讯发送数据
*/
static VOID UartSend(const U08 *data, U16 len)
{
	U16 i = 0;
	//发送命令
	for(; i < len; ++i)
	{
		while(UART_IS_TX_FULL(UART1));
		UART_WRITE(UART1, data[i]);
	}

	UART_WAIT_TX_EMPTY(UART1);
}

/*
*brief:停止串口1通讯
*/
static VOID UartStopCommunication(VOID)
{
	UART_DisableInt(UART1, UART_IER_RDA_IEN_Msk);
}

/*
* brief: 串口初始化
*/
VOID InitUart(VOID)
{	
	/*---------------------------------------------------------------------------------------------------------*/
	/* UART0 : 系统调试、程序烧写																	     */
	/* UART1 : 外围控制MCU 通信																					     */	
	/*---------------------------------------------------------------------------------------------------------*/
	/* Reset*/
	SYS_ResetModule(UART0_RST);
	SYS_ResetModule(UART1_RST);
	
	/* Configure UART and set UART Baudrate */
	UART_Open(UART0, UART0_BAUDRATE);
	UART_Open(UART1, UART1_BAUDRATE);

	gUart.startCommunication = UartSartCommunication;
	gUart.stopCommunication = UartStopCommunication;
	gUart.send = UartSend;
	gUart.uartBuffer = &gUartBuffer1;

	gpDev->uart = &gUart;
}

