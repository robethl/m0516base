#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#include <stdint.h> 

#define	VOID	void

typedef unsigned char		BOOLEAN;
typedef unsigned char		U08, *pU08;
typedef signed char		S08,  *pS08;
typedef unsigned short		U16, *pU16; 
typedef signed short		S16, *pS16;
typedef unsigned int 		U32, *pU32;
typedef signed int			S32, *pS32;

static const BOOLEAN WF_TRUE = 1;
static const BOOLEAN WF_FALSE = 0;

#if   defined ( __CC_ARM )
  #define __ASM            __asm                                      /*!< asm keyword for ARM Compiler          */
  #define __INLINE         __inline                                   /*!< inline keyword for ARM Compiler       */
  #define __STATIC_INLINE  static __inline

#elif defined ( __ICCARM__ )
  #define __ASM           __asm                                       /*!< asm keyword for IAR Compiler          */
  #define __INLINE        inline                                      /*!< inline keyword for IAR Compiler. Only available in High optimization mode! */
  #define __STATIC_INLINE  static inline

#elif defined ( __GNUC__ )
  #define __ASM            __asm                                      /*!< asm keyword for GNU Compiler          */
  #define __INLINE         inline                                     /*!< inline keyword for GNU Compiler       */
  #define __STATIC_INLINE  static inline

#elif defined ( __TASKING__ )
  #define __ASM            __asm                                      /*!< asm keyword for TASKING Compiler      */
  #define __INLINE         inline                                     /*!< inline keyword for TASKING Compiler   */
  #define __STATIC_INLINE  static inline

#endif

#endif
