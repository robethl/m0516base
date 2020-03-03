/******************************************************************************
 * @file     app_flash.c
 * @brief     hardware flash interface source file
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12
 *	   v1.1 		  huanglv        16/01/06  refactor
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "M051Series.h"
#include "device.h"
#include "extern.h"
#include "app_flash.h"

#if FLASH_SUPPORT
/***********************************����������*******************************************************/
#if FLASH_DEBUG
#define FLASH_PRINTF 	printf 
#else
#define FLASH_PRINTF( ...)
#endif

static Flash_t gFlash;
static U32 gJEDECId = 0;

/***********************************�ӿڶ�����*******************************************************/
/*
* brief: SPI Ƭѡʹ��
*/
static __INLINE VOID SpiEnableCs(VOID)
{
	gpDev->spi->spiEnableCs(SPI0);
}

/*
* brief: SPI Ƭѡ��ֹ
*/
static __INLINE VOID SpiDisableCs(VOID)
{
	gpDev->spi->spiDisableCs(SPI0);
}

/*
* brief: SPI0 д
*/
static __INLINE VOID SpiWrite(const U08 *pBytes, U16 len)
{
	gpDev->spi->spiWrite(SPI0, pBytes, len);
}

/*
* brief: SPI0 ��
*/
static __INLINE VOID SpiRead(pU08 pBytes, U16 len)
{
	gpDev->spi->spiRead(SPI0, pBytes, len);
}

#if CPLD_USED
/*
* brief: CPLD ������ʱ (us)
*/
static __INLINE VOID CPLDWaitnus(U32 t)
{
	gpDev->time->delaynUs(t);
}

/*
* brief: CPLD LED����
*/
VOID CPLDSetLed(U08 status)
{	
	U08 d[2];
	d[0] = CPLD_CMD_SETLED;	
	d[1] = status;
	SpiEnableCs();
	SpiWrite(d, sizeof(d));
	SpiDisableCs();
	CPLDWaitnus(1);
}

/*
* brief:
*return recv_d[7:4]: 0x50:find CPLD *       
*       recv_d[3]: reserved
*       recv_d[2]: 1: CPLD busy 
*       recv_d[1]: 1: FPGA2 OK
*       recv_d[0]: 1: FPGA2 OK
*				
*/
U16 CPLDGetStatus(VOID)
{		
	U08 cmd = CPLD_CMD_READ;
	U08 status[2];
	SpiEnableCs();
	SpiWrite(&cmd, 1);
	SpiRead(status, 2);
	SpiDisableCs();
	CPLDWaitnus(1);	
	return ((status[0] << 8) | status[1]); 
}

/*
* brief: CPLD ����FPGA
*/
VOID CPLDLoadFpga(VOID)
{	  	
	U08 d[5];
	d[0] = CPLD_CMD_HIGH_CFGFPGA;	
	d[1] = SST_CMD_FASTREAD;
	d[2] = (U08)(FLASH_FPGA_ADDR >> 16);
	d[3] = (U08)(FLASH_FPGA_ADDR >> 8);
	d[4] = (U08)FLASH_FPGA_ADDR;
	SpiEnableCs();
	SpiWrite(d, sizeof(d));
	SpiDisableCs();
	CPLDWaitnus(1);
}

/*
* brief: ����CPLD ����flash
*/
static VOID CPLDCtrlFlash(VOID)
{
	U08 cmd = CPLD_CMD_CTRLFLASH;	
	SpiWrite(&cmd, 1); 
}

/*
* brief: CPLD ����
*/
VOID CPLDReset(VOID)
{
	U08 cmd = CPLD_CMD_RESET;
	SpiEnableCs();
	SpiWrite(&cmd, 1);
	SpiDisableCs();
	CPLDWaitnus(1);
}

/*
* brief: CPLD ��ʼ��
*/
static VOID InitCPLD(VOID) 
{	
	CPLDReset();
}
#endif

/*
* brief: flash������ʱ (us)
*/
static VOID __INLINE SSTWait1us(U32 t) 
{
	gpDev->time->delaynUs(t);
}

/*
* brief: flash������ʱ (ms)
*/
static VOID __INLINE SSTWait1ms(U32 t)
{
	gpDev->time->delaynMs(t);
}

/*
* brief: flashд�������ݣ������ֽ�
*/
static VOID __INLINE SSTWriteCmdData(const pU08 pBytes, U16 len)
{
#if CPLD_USED
	CPLDCtrlFlash();
#endif
	SpiWrite(pBytes, len);
}

/*
* brief: flashд�����֣����ֽ�
*/
static VOID SSTWriteCmd(U08 cmd)
{
	SSTWriteCmdData(&cmd, 1);
}

/*
* brief:flash д״̬�Ĵ���
* 	status register:  
*	BLP[rw]-SEC[r]-BP3[rw]-BP2[rw]-BP1[rw]-BP0[rw]-WEL[r]-BUSY[r]
*
*	BLP: 	1 = BP3��BP2��BP1 ��BP0 Ϊֻ��λ
*			0 = BP3��BP2��BP1 ��BP0 �ɶ�/д
*			
*	WEL �Զ���λ: 	1. �ϵ�; 2. д��ֹ��WRDI��ָ�����;
*						3. д״̬�Ĵ���ָ�����; 4. ҳ���ָ�����;
*						5. ˫����ҳ���ָ�����; 6. ��������ָ�����;
*						7. �����ָ�����; 8. ȫƬ����ָ�����;
*						9. ���SID ָ�����; 10. ����SID ָ�����;
*	WEL ��λ: дʹ������
*
*	SST25VF064C �����״̬�Ĵ����鱣��,��λ��BP3~BP1 : 1111
*	=========================================================
*	||��������	||	״̬�Ĵ���λ	||	�ܱ����Ĵ洢����ַ||
*	||	*			||	  BP3 BP2 BP1 BP0 	||		         64 Mb				||
*	=========================================================
*	||	��					0 0 0 0 							��				||
*	||	ǰ1/128 			0 0 0 1 					7F0000H-7FFFFFH			||
*	||	ǰ1/64 				0 0 1 0 					7E0000H-7FFFFFH			||
*	||	ǰ1/32 				0 0 1 1 					7C0000H-7FFFFFH			||
*	||	ǰ1/16 				0 1 0 0 					780000H-7FFFFFH			||
*	||	ǰ1/8 				0 1 0 1 					700000H-7FFFFFH			||
*	||	ǰ1/4 				0 1 1 0 					600000H-7FFFFFH			||
*	||	ǰ1/2 				0 1 1 1 					400000H-7FFFFFH			||
*	||	���п�			1 * * * 					000000H-7FFFFFH			||
*	==========================================================
*/
static VOID SSTWriteStatus(U08 status)
{
	U08 d[3];
	d[0] = (U08)SST_CMD_WRSR;
	d[1] = status;
	d[2] = 0;
	
	//step1 ʹ��д״̬�Ĵ���
	SpiEnableCs();
	//SSTWriteCmd(SST_CMD_WREN);	//SST25VF064C ��ʹ��ʹ��д����or  ʹ��д״̬�Ĵ�������
	SSTWriteCmd(SST_CMD_EWSR); 
	SpiDisableCs();		//����cs ʹflashִ��ENSR
	SSTWait1us(1);

	//step2 д״̬�Ĵ���
	SpiEnableCs();
	/*
	*brief: flash����Ӧ����
	*/
	if(FLASH_JEDEC_W25Q64FVSIG == gFlash.jedecId){
		SSTWriteCmdData(d, sizeof(d));
	}else{	/*SST25VF064C*/
		SSTWriteCmdData(d, 2);
	}
	SpiDisableCs();
	SSTWait1us(1);
}

/*
* brief:flash��ȡ״̬�Ĵ���
*/
U08 SSTReadStatus(VOID)
{
	U08 status = 0;
	
	SpiEnableCs();
	SSTWriteCmd(SST_CMD_RDSR);
	SpiRead(&status, 1);
	SpiDisableCs();
	SSTWait1us(1);
	return status;
}

/*
* brief:flash æ��
*/
static BOOLEAN SSTWaitFree(VOID)
{
	U16 t = SST_WAITFREE_TIMER;
	BOOLEAN busy = WF_TRUE;
	
	for(; t > 0 ; t--){
		if((SSTReadStatus() & 0x01) == 0){
			busy = WF_FALSE;
			break;
		}
		SSTWait1ms(1);
	}
  return	busy;    
}

/*
* brief: flash�鱣���򿪣���ֹ����д
*/
static VOID __INLINE SSTBPOpen(VOID)
{   
	SSTWriteStatus(SST_SRBP_EN);
}

/*
* brief: flash�鱣���ر�
*/
static VOID __INLINE SSTBPClose(VOID)
{   
	SSTWriteStatus(SST_SRBP_DIS);
}

/*
* brief: flashдʹ��
*/
static VOID SSTWriteEnable(VOID)
{   
	SpiEnableCs();
  	SSTWriteCmd(SST_CMD_WREN);
	SpiDisableCs();	//����cs ʹflashִ��WREN
	SSTWait1us(1);
}

/*
* brief: flash д��ֹ
*/
static VOID SSTWriteDisable(VOID)
{
	SpiEnableCs();
	SSTWriteCmd(SST_CMD_WRDI);
	SpiDisableCs();	//����cs ʹflashִ��WRDI
	SSTWait1us(1);
}

/*
* brief: ��flash��ַ�Ĳ���
*/
static VOID SSTCmdWithAddr(U08 cmd, U32 addr)
{
	U08 buf[5], i = 0;
	buf[i++] = (U08)cmd;     
	buf[i++] = (U08)(addr >> 16);
	buf[i++] = (U08)(addr >> 8);
	buf[i++] = (U08)addr;
	if(cmd == SST_CMD_FASTREAD){
		buf[i++] = 0;  
	}
	SSTWriteCmdData(buf, i);
}

static VOID SSTFastRead(U32 addr)
{
	U08 buf[5], i = 0;
	buf[i++] = (U08)SST_CMD_FASTREAD;     
	buf[i++] = (U08)(addr >> 16);
	buf[i++] = (U08)(addr >> 8);
	buf[i++] = (U08)addr;
	buf[i++] = 0;  
	SSTWriteCmdData(buf, i);
}

/*
* brief: flash оƬ����
*/
static VOID SSTEraseChip(VOID)
{	
	SSTBPClose();
	SSTWriteEnable();
	
	SpiEnableCs();
	SSTWriteCmd(SST_CMD_ERASECHIP);
	SpiDisableCs();	//����cs ʹflashִ��ȫƬ����
	SSTWaitFree();
		
	SSTBPOpen();
}

/*
* brief: flash ����������ADDR�ĵ�12λ��ַ�����壬��13λ��15λ����ָ�����е�ĳ�������� ��16λ�����λ��Чλ����ĳ���顣
*/
static VOID SSTEraseSector(U32 addr, U08 sector)
{		
	addr &= ~(SST_SECTOR_LEN - 1);

	SSTBPClose();
	while(sector--) {
		SSTWriteEnable();
		SpiEnableCs();
		SSTCmdWithAddr(SST_CMD_ERASESECTOR, addr);
		SpiDisableCs();	//����cs ʹflashִ����������
		addr += SST_SECTOR_LEN;
		SSTWaitFree();
	}
	SSTBPOpen();
}	

/*
* brief: flash �������ADDR�ĵ�15λ��ַ�����壬��15λ�����λ�������ĸ��顣��32KByteΪһ���飩
*/
static VOID SSTEraseBlock32K(U32 addr, U08 block)
{	
	addr &= ~(SST_BLOCK32K_LEN - 1);
	
	SSTBPClose();
	while(block--) {	
		SSTWriteEnable();
		SpiEnableCs();
		SSTCmdWithAddr(SST_CMD_ERASEBLOCK32K, addr);
		SpiDisableCs();
		addr += SST_BLOCK32K_LEN;	
		SSTWaitFree();    		
	} 
	SSTBPOpen();
}

/*
* brief: flash �������ADDR�ĵ�16λ��ַ�����壬��16λ�����λ�������ĸ��顣��64KByteΪһ���飩
*/
static VOID SSTEraseBlock64K(U32 addr, U08 block)
{	
	addr &= ~(SST_BLOCK64K_LEN - 1);
	
	SSTBPClose();
	while(block--) {
		SSTWriteEnable();
		SpiEnableCs();
		SSTCmdWithAddr(SST_CMD_ERASEBLOCK64K, addr);
		SpiDisableCs();
		addr += SST_BLOCK64K_LEN;
		SSTWaitFree();    			
	}
	SSTBPOpen();
}

/*
* brief: flashд����ʼ��ַ��ҳ�߽�Ķα��
*/
static VOID SSTWrite8ByteAlignment(U32 addr, const U08 *ptr, U32 size)
{   
	U32 tmp, next_sector_start_addr;
	tmp = addr % SST_SECTOR_LEN;	//addr��sector�е�����λ��
	next_sector_start_addr = (addr + SST_SECTOR_LEN) - tmp; //�����һ��sector����ʼ��ַ
	
	if(size == 0) {	 
		FLASH_PRINTF("%s write error, size is 0!\n", FLASH_DEBUG_HEADSTR); 
		return;
	}

	if(size > next_sector_start_addr - tmp) {	//��ʼ��ַ��Ҷ�߽�ʱ����д������ݲ��ܳ���Ҷ�߽�β�����򳬳������ݻ���д�뵽ҳ��ʼ��ַ.
		FLASH_PRINTF("%s write error, size is too large! try to invoke SSTWrite(*)\n", FLASH_DEBUG_HEADSTR); 
		return;
	}
	
	if(addr % SST_SEGMENT_LEN > 0){	//д��ַ�����Ƕ���ʼ��ַ
		FLASH_PRINTF("%s write error, address is not the segment start address!\n", FLASH_DEBUG_HEADSTR); 
		return;
	}
	
	if(tmp == 0)  {//��ַ��sector����ʼλ��,ɾ����sector
		SSTEraseSector(addr, 1);
	}	

	SSTBPClose();
	while(size > 0) {
		SSTWriteEnable();
		SpiEnableCs();
		SSTCmdWithAddr(SST_CMD_PAGEPROG, addr);
		if(size >= SST_SEGMENT_LEN) {							   	   					
			SpiWrite(ptr, SST_SEGMENT_LEN);
			ptr += SST_SEGMENT_LEN;
			addr += SST_SEGMENT_LEN; 
			size -= SST_SEGMENT_LEN;				
		}else {			 			
			SpiWrite(ptr, size);
			size = 0;
		}
		SpiDisableCs();
		SSTWaitFree(); 
	}	
	SSTBPOpen();
}

/*
* brief: flashд, ��ʼ��ַҳ�߽��ҳ���
*/
static VOID SSTWrite(U32 addr, const U08 *ptr, U32 size)
{   
	U08 sector_num;
	U32 tmp, next_sector_start_addr;
	tmp = addr % SST_SECTOR_LEN;	//addr��sector�е�����λ��
	next_sector_start_addr = (addr + SST_SECTOR_LEN) - tmp; //�����һ��sector����ʼ��ַ
	
	if(size == 0) {	 
		FLASH_PRINTF("%s write error, size is 0!\n", FLASH_DEBUG_HEADSTR); 
		return;
	}
	
	if(addr % SST_PAGE_LEN > 0){	//д��ַ������ҳд�Ŀ�ʼ��ַ
		FLASH_PRINTF("%s write error, address is not the page start address!\n", FLASH_DEBUG_HEADSTR); 
		return;
	}
	
	if(tmp == 0)  {//��ַ��sector����ʼλ��, ��ɾ��size byte������ռ��sector
		sector_num = (size - 1)/SST_SECTOR_LEN + 1;
		SSTEraseSector(addr, sector_num);
	}	
	else if((addr + size) > next_sector_start_addr)  {//��ַ����sector����ʼλ��, size+addr�ִ�����һ��sector����ʼ��ַ, ��ʾ��sectorд, ɾ�������sector, ����ɾ����sector, �������Ա���addr֮ǰд�������
		tmp = size - (SST_SECTOR_LEN - tmp);  //��ȥsize�ڱ�sector��ռ���ֽ���
		sector_num = (tmp - 1)/SST_SECTOR_LEN + 1;	 //���ʣ��size��ռ��sector����
		SSTEraseSector(next_sector_start_addr, sector_num); //������sector, ��ɾ��������sector
	}

	SSTBPClose();
	while(size > 0) {
		SSTWriteEnable();
		SpiEnableCs();
		SSTCmdWithAddr(SST_CMD_PAGEPROG, addr);
		if(size >= SST_PAGE_LEN) {							   	   					
			SpiWrite(ptr, SST_PAGE_LEN);
			ptr += SST_PAGE_LEN;
			addr += SST_PAGE_LEN; 
			size -= SST_PAGE_LEN;				
		}else {			 			
			SpiWrite(ptr, size);
			size = 0;
		}
		SpiDisableCs();
		SSTWaitFree(); 
	}	
	SSTBPOpen();
}

/*
* brief: flash��
*/
static VOID SSTRead(U32 addr, pU08 ptr, U32 size)
{
	if(size == 0){
		return;
	}

	SpiEnableCs();
	SSTCmdWithAddr(SST_CMD_FASTREAD, addr);
	SpiRead(ptr, size);
	SpiDisableCs();
}

/*
*brief: JEDEC��ID
*/
static U32 SSTQueryJEDECInfo(VOID)
{
	U08 buf[3] = {0};
	SpiEnableCs();
	SSTWriteCmd(SST_CMD_RDJEDEC);	
	SpiRead(buf, 3);
	SpiDisableCs();
	return ((U32)buf[0]<<16)|((U32)buf[1]<<8)|((U32)buf[2]);
}

/*
* brief: flashģ���ʼ��
*/
VOID InitFlash(VOID)
{
#if CPLD_USED
	InitCPLD();
#endif
	if(SSTWaitFree()){
		FLASH_PRINTF("%s wait free timeout!\n", FLASH_DEBUG_HEADSTR);
	}

	gFlash.jedecId = SSTQueryJEDECInfo();
	if(FLASH_JEDEC_SST25VF064C == gFlash.jedecId){
		FLASH_PRINTF("%s SST25VF064C\n", FLASH_DEBUG_HEADSTR);
	}else if(FLASH_JEDEC_W25Q64FVSIG == gFlash.jedecId){
		FLASH_PRINTF("%s W25Q64FVSIG\n", FLASH_DEBUG_HEADSTR);
	}else{
		FLASH_PRINTF("%s Unrecongize flash, JEDEC ID[%x06X]\n", FLASH_DEBUG_HEADSTR, gJEDECId);
	}
	
	gFlash.spiEnable = SpiEnableCs;
	gFlash.spiDisable = SpiDisableCs;
	gFlash.spiWrite = SpiWrite;
	gFlash.spiRead = SpiRead;
	gFlash.SSTFastRead = SSTFastRead;
	gFlash.SSTWrite8ByteAlignment = SSTWrite8ByteAlignment;
	gFlash.SSTWrite = SSTWrite;
	gFlash.SSTRead = SSTRead;
#if CPLD_USED
	gFlash.CPLDLoadFpga = CPLDLoadFpga;
	gFlash.CPLDGetStatus = CPLDGetStatus;
	gFlash.CPLDReset = CPLDReset;
#endif

	gpDev->flash = &gFlash;
}
#endif

