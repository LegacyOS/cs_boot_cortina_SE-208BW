#ifndef _HAL_INTR_H
#define _HAL_INTR_H

#include <board_config.h>
#include <sl2312.h>

#define HAL_ISR_FIQ_OFFSET					32

#define HAL_INTERRUPT_NONE					-1

#define HAL_ISR_IRQ_MIN						0
#ifndef MIDWAY
#define HAL_ISR_IRQ_MAX						(HAL_INTERRUPT_SERIRQ15)
#else
#define HAL_ISR_IRQ_MAX						(HAL_IRQ_SERIRQ)
#endif

#define HAL_ISR_FIQ_MIN						0
#define HAL_ISR_FIQ_MAX						0 

#define HAL_ISR_MIN							HAL_ISR_IRQ_MIN 
#define HAL_ISR_MAX							HAL_ISR_IRQ_MAX

#define HAL_ISR_IRQ_COUNT					(HAL_ISR_IRQ_MAX-HAL_ISR_IRQ_MIN+1)
#define HAL_ISR_FIQ_COUNT					0
#define HAL_ISR_COUNT						(HAL_ISR_IRQ_COUNT + HAL_ISR_FIQ_COUNT)

//----------------------------------------------------------------------------
// Reset.

// Writing a bad value to the watchdog reload register causes a reset.
#define HAL_PLATFORM_RESET()                       \
    HAL_WRITE_UINT32(SL2312_WAQTCHDOG_BASE, 0)

#define HAL_PLATFORM_RESET_ENTRY SL2312_ROM_BASE

#endif // _HAL_INTR_H
