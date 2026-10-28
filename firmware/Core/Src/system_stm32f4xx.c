#include "stm32f4xx.h"

#ifndef HSE_VALUE
#define HSE_VALUE 25000000U
#endif

uint32_t SystemCoreClock = 16000000U;
const uint8_t AHBPrescTable[16] = {0,0,0,0,0,0,0,0,1,2,3,4,6,7,8,9};

void SystemInit(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << (10 * 2)) | (3UL << (11 * 2)));
#endif
#if defined(VECT_TAB_SRAM)
    SCB->VTOR = SRAM_BASE;
#else
    SCB->VTOR = FLASH_BASE;
#endif
}

void SystemCoreClockUpdate(void)
{
    uint32_t tmp;
    uint32_t pllvco;
    uint32_t pllp;
    uint32_t pllsource;
    uint32_t pllm;

    tmp = RCC->CFGR & RCC_CFGR_SWS;

    switch (tmp)
    {
    case RCC_CFGR_SWS_HSE:
        SystemCoreClock = HSE_VALUE;
        break;

    case RCC_CFGR_SWS_PLL:
        pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
        pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;

        if (pllsource != 0U)
            pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
        else
            pllvco = (16000000U / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);

        pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16) + 1U) * 2U;
        SystemCoreClock = pllvco / pllp;
        break;

    case RCC_CFGR_SWS_HSI:
    default:
        SystemCoreClock = 16000000U;
        break;
    }

    tmp = (RCC->CFGR & RCC_CFGR_HPRE) >> 4;
    SystemCoreClock >>= AHBPrescTable[tmp];
}
