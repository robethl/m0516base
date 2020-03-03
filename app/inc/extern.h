#ifndef _EXTERN_H_
#define _EXTERN_H_

#include "typedef.h"

//Uart 
extern VOID InitUart(VOID);

//FMC
extern VOID InitFmc(VOID);

//I2C
extern VOID InitI2c(VOID);

//SPI
extern VOID InitSpi(VOID);

//LED
extern VOID InitLed(VOID);

//Time
extern VOID InitTime(VOID);

//algorithm
extern VOID InitAlgorithm(VOID);

//flash
extern VOID InitFlash(VOID);

//tftp
extern VOID InitTftp(VOID);

//fpga
extern VOID InitFPGA(VOID);

#endif

