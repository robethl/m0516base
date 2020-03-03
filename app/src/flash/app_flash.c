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
/***********************************变量定义区*******************************************************/
#if FLASH_DEBUG
#define FLASH_PRINTF 	printf 
#else
#define FLASH_PRINTF( ...)
#endif

static Flash_t gFlash;
static U32 gJEDECId = 0;

/***********************************接口定义区*******************************************************/
/*
* brief: SPI 片选使能
*/
static __INLINE VOID SpiEnableCs(VOID)
{
	gpDev->spi->spiEnableCs(SPI0);
}

/*
* brief: SPI 片选禁止
*/
static __INLINE VOID SpiDisableCs(VOID)
{
	gpDev->spi->spiDisableCs(SPI0);
}

/*
* brief: SPI0 写
*/
static __INLINE VOID SpiWrite(const U08 *pBytes, U16 len)
{
	gpDev->spi->spiWrite(SPI0, pBytes, len);
}

/*
* brief: SPI0 读
*/
static __INLINE VOID SpiRead(pU08 pBytes, U16 len)
{
	gpDev->spi->spiRead(SPI0, pBytes, len);
}

#if CPLD_USED
/*
* brief: CPLD 操作延时 (us)
*/
static __INLINE VOID CPLDWaitnus(U32 t)
{
	gpDev->time->delaynUs(t);
}

/*
* brief: CPLD LED设置
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
* brief: CPLD 加载FPGA
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
* brief: 经由CPLD 控制flash
*/
static VOID CPLDCtrlFlash(VOID)
{
	U08 cmd = CPLD_CMD_CTRLFLASH;	
	SpiWrite(&cmd, 1); 
}

/*
* brief: CPLD 重置
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
* brief: CPLD 初始化
*/
static VOID InitCPLD(VOID) 
{	
	CPLDReset();
}
#endif

/*
* brief: flash操作延时 (us)
*/
static VOID __INLINE SSTWait1us(U32 t) 
{
	gpDev->time->delaynUs(t);
}

/*
* brief: flash操作延时 (ms)
*/
static VOID __INLINE SSTWait1ms(U32 t)
{
	gpDev->time->delaynMs(t);
}

/*
* brief: flash写命令数据，不定字节
*/
static VOID __INLINE SSTWriteCmdData(const pU08 pBytes, U16 len)
{
#if CPLD_USED
	CPLDCtrlFlash();
#endif
	SpiWrite(pBytes, len);
}

/*
* brief: flash写命令字，单字节
*/
static VOID SSTWriteCmd(U08 cmd)
{
	SSTWriteCmdData(&cmd, 1);
}

/*
* brief:flash 写状态寄存器
* 	status register:  
*	BLP[rw]-SEC[r]-BP3[rw]-BP2[rw]-BP1[rw]-BP0[rw]-WEL[r]-BUSY[r]
*
*	BLP: 	1 = BP3、BP2、BP1 和BP0 为只读位
*			0 = BP3、BP2、BP1 和BP0 可读/写
*			
*	WEL 自动复位: 	1. 上电; 2. 写禁止（WRDI）指令完成;
*						3. 写状态寄存器指令完成; 4. 页编程指令完成;
*						5. 双输入页编程指令完成; 6. 扇区擦除指令完成;
*						7. 块擦除指令完成; 8. 全片擦除指令完成;
*						9. 编程SID 指令完成; 10. 锁定SID 指令完成;
*	WEL 置位: 写使能命令
*
*	SST25VF064C 的软件状态寄存器块保护,复位后BP3~BP1 : 1111
*	=========================================================
*	||保护级别	||	状态寄存器位	||	受保护的存储器地址||
*	||	*			||	  BP3 BP2 BP1 BP0 	||		         64 Mb				||
*	=========================================================
*	||	无					0 0 0 0 							无				||
*	||	前1/128 			0 0 0 1 					7F0000H-7FFFFFH			||
*	||	前1/64 				0 0 1 0 					7E0000H-7FFFFFH			||
*	||	前1/32 				0 0 1 1 					7C0000H-7FFFFFH			||
*	||	前1/16 				0 1 0 0 					780000H-7FFFFFH			||
*	||	前1/8 				0 1 0 1 					700000H-7FFFFFH			||
*	||	前1/4 				0 1 1 0 					600000H-7FFFFFH			||
*	||	前1/2 				0 1 1 1 					400000H-7FFFFFH			||
*	||	所有块			1 * * * 					000000H-7FFFFFH			||
*	==========================================================
*/
static VOID SSTWriteStatus(U08 status)
{
	U08 d[3];
	d[0] = (U08)SST_CMD_WRSR;
	d[1] = status;
	d[2] = 0;
	
	//step1 使能写状态寄存器
	SpiEnableCs();
	//SSTWriteCmd(SST_CMD_WREN);	//SST25VF064C 可使用使能写操作or  使能写状态寄存器操作
	SSTWriteCmd(SST_CMD_EWSR); 
	SpiDisableCs();		//拉高cs 使flash执行ENSR
	SSTWait1us(1);

	//step2 写状态寄存器
	SpiEnableCs();
	/*
	*brief: flash自适应处理
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
* brief:flash读取状态寄存器
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
* brief:flash 忙等
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
* brief: flash块保护打开，防止意外写
*/
static VOID __INLINE SSTBPOpen(VOID)
{   
	SSTWriteStatus(SST_SRBP_EN);
}

/*
* brief: flash块保护关闭
*/
static VOID __INLINE SSTBPClose(VOID)
{   
	SSTWriteStatus(SST_SRBP_DIS);
}

/*
* brief: flash写使能
*/
static VOID SSTWriteEnable(VOID)
{   
	SpiEnableCs();
  	SSTWriteCmd(SST_CMD_WREN);
	SpiDisableCs();	//拉高cs 使flash执行WREN
	SSTWait1us(1);
}

/*
* brief: flash 写禁止
*/
static VOID SSTWriteDisable(VOID)
{
	SpiEnableCs();
	SSTWriteCmd(SST_CMD_WRDI);
	SpiDisableCs();	//拉高cs 使flash执行WRDI
	SSTWait1us(1);
}

/*
* brief: 带flash地址的操作
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
* brief: flash 芯片擦除
*/
static VOID SSTEraseChip(VOID)
{	
	SSTBPClose();
	SSTWriteEnable();
	
	SpiEnableCs();
	SSTWriteCmd(SST_CMD_ERASECHIP);
	SpiDisableCs();	//拉高cs 使flash执行全片擦除
	SSTWaitFree();
		
	SSTBPOpen();
}

/*
* brief: flash 扇区擦除。ADDR的低12位地址无意义，第13位到15位决定指定块中的某个扇区， 第16位到最高位有效位决定某个块。
*/
static VOID SSTEraseSector(U32 addr, U08 sector)
{		
	addr &= ~(SST_SECTOR_LEN - 1);

	SSTBPClose();
	while(sector--) {
		SSTWriteEnable();
		SpiEnableCs();
		SSTCmdWithAddr(SST_CMD_ERASESECTOR, addr);
		SpiDisableCs();	//拉高cs 使flash执行扇区擦除
		addr += SST_SECTOR_LEN;
		SSTWaitFree();
	}
	SSTBPOpen();
}	

/*
* brief: flash 块擦除。ADDR的低15位地址无意义，第15位到最高位决定在哪个块。（32KByte为一个块）
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
* brief: flash 块擦除。ADDR的低16位地址无意义，第16位到最高位决定在哪个块。（64KByte为一个块）
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
* brief: flash写，起始地址非页边界的段编程
*/
static VOID SSTWrite8ByteAlignment(U32 addr, const U08 *ptr, U32 size)
{   
	U32 tmp, next_sector_start_addr;
	tmp = addr % SST_SECTOR_LEN;	//addr在sector中的所处位置
	next_sector_start_addr = (addr + SST_SECTOR_LEN) - tmp; //求出下一个sector的起始地址
	
	if(size == 0) {	 
		FLASH_PRINTF("%s write error, size is 0!\n", FLASH_DEBUG_HEADSTR); 
		return;
	}

	if(size > next_sector_start_addr - tmp) {	//起始地址非叶边界时，待写入的数据不能超出叶边界尾，否则超出的数据回绕写入到页起始地址.
		FLASH_PRINTF("%s write error, size is too large! try to invoke SSTWrite(*)\n", FLASH_DEBUG_HEADSTR); 
		return;
	}
	
	if(addr % SST_SEGMENT_LEN > 0){	//写地址必需是段起始地址
		FLASH_PRINTF("%s write error, address is not the segment start address!\n", FLASH_DEBUG_HEADSTR); 
		return;
	}
	
	if(tmp == 0)  {//地址在sector的起始位置,删除本sector
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
* brief: flash写, 起始地址页边界的页编程
*/
static VOID SSTWrite(U32 addr, const U08 *ptr, U32 size)
{   
	U08 sector_num;
	U32 tmp, next_sector_start_addr;
	tmp = addr % SST_SECTOR_LEN;	//addr在sector中的所处位置
	next_sector_start_addr = (addr + SST_SECTOR_LEN) - tmp; //求出下一个sector的起始地址
	
	if(size == 0) {	 
		FLASH_PRINTF("%s write error, size is 0!\n", FLASH_DEBUG_HEADSTR); 
		return;
	}
	
	if(addr % SST_PAGE_LEN > 0){	//写地址必需是页写的开始地址
		FLASH_PRINTF("%s write error, address is not the page start address!\n", FLASH_DEBUG_HEADSTR); 
		return;
	}
	
	if(tmp == 0)  {//地址在sector的起始位置, 先删除size byte数据所占的sector
		sector_num = (size - 1)/SST_SECTOR_LEN + 1;
		SSTEraseSector(addr, sector_num);
	}	
	else if((addr + size) > next_sector_start_addr)  {//地址不在sector的起始位置, size+addr又大于下一个sector的起始地址, 表示跨sector写, 删除跨过的sector, 但不删除本sector, 这样可以保留addr之前写入的数据
		tmp = size - (SST_SECTOR_LEN - tmp);  //减去size在本sector所占的字节数
		sector_num = (tmp - 1)/SST_SECTOR_LEN + 1;	 //求出剩下size所占的sector数量
		SSTEraseSector(next_sector_start_addr, sector_num); //跳过本sector, 并删除后续的sector
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
* brief: flash读
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
*brief: JEDEC读ID
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
* brief: flash模块初始化
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

