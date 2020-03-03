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

/***********************************����������*******************************************************/
static Uart_t gUart;
static UartBuffer_t gUartBuffer0;	//��ӡ��
static UartBuffer_t gUartBuffer1;	//����ͨ��Э���

/***********************************�ӿڶ�����*******************************************************/
/*
* brief: ����0�жϺ���
*/
VOID UART0_IRQHandler(VOID)
{
	U08 bInChar = 0xFF;
	
	if(UART_GET_INT_FLAG(UART0, UART_ISR_RDA_INT_Msk))                              /*����Ƿ�����ж�*/
	{
		while(UART_GET_INT_FLAG(UART0, UART_ISR_RDA_IF_Msk))                        /*�����յ��������Ƿ���Ч*/
		{
			while (UART_GET_RX_EMPTY(UART0));                //���ַ�
			bInChar = UART_READ(UART0);                      //��ȡ�ַ�
			
			gUartBuffer0.buf[gUartBuffer0.tail++] = bInChar;/*��RXFIFO�ж�ȡ���յ�������*/
			gUartBuffer0.tail %= UART_BUFLEN;

			if(gUartBuffer0.tail == gUartBuffer0.head) {	//���������
				gUartBuffer0.head = (gUartBuffer0.head+1)%UART_BUFLEN;				
			}	
		}
	}
}

/*
* brief: ����1�жϺ���
*/
VOID UART1_IRQHandler(VOID)
{
	U08 bInChar=0xFF;
	
	if(UART_GET_INT_FLAG(UART1, UART_ISR_RDA_INT_Msk))                              /*����Ƿ�����ж�*/
	{	    
		while(UART_GET_INT_FLAG(UART1, UART_ISR_RDA_IF_Msk))                        /*�����յ��������Ƿ���Ч*/
		{
			while (UART_GET_RX_EMPTY(UART1));
			bInChar = UART_READ(UART1); 
			
			//buffer�������߼�⵽���������ٽ�����������
			if(gUartBuffer1.tail == UART_BUFLEN){
				return;
			}
			gUartBuffer1.buf[gUartBuffer1.tail++] = bInChar;/*��RXFIFO�ж�ȡ���յ�������*/
		}
	}
}

/*
*brief:��������1ͨѶ
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
*brief:����1ͨѶ��������
*/
static VOID UartSend(const U08 *data, U16 len)
{
	U16 i = 0;
	//��������
	for(; i < len; ++i)
	{
		while(UART_IS_TX_FULL(UART1));
		UART_WRITE(UART1, data[i]);
	}

	UART_WAIT_TX_EMPTY(UART1);
}

/*
*brief:ֹͣ����1ͨѶ
*/
static VOID UartStopCommunication(VOID)
{
	UART_DisableInt(UART1, UART_IER_RDA_IEN_Msk);
}

/*
* brief: ���ڳ�ʼ��
*/
VOID InitUart(VOID)
{	
	/*---------------------------------------------------------------------------------------------------------*/
	/* UART0 : ϵͳ���ԡ�������д																	     */
	/* UART1 : ��Χ����MCU ͨ��																					     */	
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

